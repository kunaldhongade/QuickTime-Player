#include "playback/PlaybackController.hpp"

#include "frames/FramePositionResolver.hpp"
#include "playback/MpvEngine.hpp"

#include <QLoggingCategory>

#include <algorithm>
#include <cmath>

Q_LOGGING_CATEGORY(playbackControllerLog, "frameviewer.playback.controller")

namespace frameviewer {

PlaybackController::PlaybackController(MpvEngine* engine, QObject* parent)
    : QObject(parent)
    , m_engine(engine)
{
    Q_ASSERT(m_engine);
    m_confirmationTimer.setSingleShot(true);
    m_confirmationTimer.setInterval(450);
    connect(&m_confirmationTimer,
            &QTimer::timeout,
            this,
            &PlaybackController::confirmationTimedOut);
    m_requestTimer.setSingleShot(true);
    m_requestTimer.setInterval(60);
    connect(&m_requestTimer,
            &QTimer::timeout,
            this,
            &PlaybackController::processRequestedTarget);
    connect(m_engine,
            &MpvEngine::videoFrameRendered,
            this,
            &PlaybackController::renderedFrameChanged);
}

qint64 PlaybackController::currentFrame() const
{
    return m_currentFrame < 0 ? 0 : m_currentFrame + 1;
}

qint64 PlaybackController::totalFrames() const
{
    return m_index.count();
}

qsizetype PlaybackController::currentZeroBasedFrame() const
{
    return m_currentFrame;
}

bool PlaybackController::exactFrameIndexAvailable() const
{
    return !m_index.isEmpty();
}

bool PlaybackController::canStepBackward() const
{
    return !m_index.isEmpty() && m_currentFrame > 0;
}

bool PlaybackController::canStepForward() const
{
    return !m_index.isEmpty() && m_currentFrame >= 0 && m_currentFrame < m_index.count() - 1;
}

const FrameIndex& PlaybackController::frameIndex() const
{
    return m_index;
}

void PlaybackController::setFrameIndex(FrameIndex index)
{
    const bool previouslyAvailable = exactFrameIndexAvailable();
    m_index = std::move(index);
    m_busy = false;
    m_confirmationTimer.stop();
    m_requestTimer.stop();
    m_expectedFrame = -1;
    m_requestedFrame = -1;
    m_nativeTraversalRequested = false;
    m_currentFrame = m_lastRenderedPosition.has_value()
                         ? resolvedPosition(*m_lastRenderedPosition)
                         : resolvedEnginePosition();
    emit totalFramesChanged();
    emit currentFrameChanged();
    emit stepAvailabilityChanged();
    if (previouslyAvailable != exactFrameIndexAvailable()) {
        emit exactFrameIndexAvailableChanged();
    }
}

void PlaybackController::clear()
{
    const bool wasAvailable = exactFrameIndexAvailable();
    m_index.clear();
    m_confirmationTimer.stop();
    m_requestTimer.stop();
    m_busy = false;
    m_lastRenderedPosition.reset();
    m_currentFrame = -1;
    m_expectedFrame = -1;
    m_requestedFrame = -1;
    m_nativeTraversalRequested = false;
    emit currentFrameChanged();
    emit totalFramesChanged();
    emit stepAvailabilityChanged();
    if (wasAvailable) {
        emit exactFrameIndexAvailableChanged();
    }
}

void PlaybackController::stepBy(int frameCount)
{
    if (frameCount == 0 || m_index.isEmpty()) {
        return;
    }

    const qsizetype baseFrame =
        m_requestedFrame >= 0 ? m_requestedFrame
                              : (m_busy ? m_expectedFrame : m_currentFrame);
    const qsizetype target =
        FramePositionResolver::clampTarget(m_index, baseFrame + frameCount);
    if (target == baseFrame) {
        return;
    }
    m_requestedFrame = target;
    m_nativeTraversalRequested = std::abs(frameCount) == 1;
    // Wait briefly for additional taps so rapid key travel is issued as one
    // decoder operation instead of a queue of stale intermediate commands.
    m_requestTimer.start();
}

void PlaybackController::seekToFrame(qint64 zeroBasedFrame)
{
    if (m_index.isEmpty()) {
        return;
    }
    const qsizetype target = FramePositionResolver::clampTarget(m_index, zeroBasedFrame);
    m_requestedFrame = target;
    m_nativeTraversalRequested = false;
    m_requestTimer.stop();
    if (target == m_currentFrame && !m_busy) {
        m_requestedFrame = -1;
        return;
    }
    processRequestedTarget();
}

void PlaybackController::seekToFirstFrame()
{
    seekToFrame(0);
}

void PlaybackController::seekToLastFrame()
{
    seekToFrame(m_index.count() - 1);
}

void PlaybackController::renderedFrameChanged(double timePosition)
{
    m_lastRenderedPosition = timePosition;
    if (m_index.isEmpty()) {
        return;
    }
    const qsizetype renderedFrame = resolvedPosition(timePosition);
    if (m_busy) {
        if (renderedFrame == m_expectedFrame) {
            settleAt(renderedFrame);
        } else {
            // A multi-frame forward step deliberately renders every frame on
            // its way to the target. Keep the counter synchronized with those
            // intermediate pixels without treating them as a failed seek.
            publishCurrent(renderedFrame);
        }
        return;
    }
    publishCurrent(renderedFrame);
    processRequestedTarget();
}

void PlaybackController::confirmationTimedOut()
{
    if (!m_busy) {
        return;
    }
    const qsizetype renderedFrame =
        m_lastRenderedPosition.has_value()
            ? resolvedPosition(*m_lastRenderedPosition)
            : resolvedEnginePosition();
    if (renderedFrame == m_expectedFrame) {
        settleAt(m_expectedFrame);
        return;
    }
    if (!m_correctiveSeekIssued) {
        confirmOrCorrect();
        return;
    }

    qCWarning(playbackControllerLog)
        << "Frame seek failed to settle on indexed frame" << m_expectedFrame;
    const qsizetype failedFrame = m_expectedFrame;
    m_busy = false;
    m_expectedFrame = -1;
    if (m_requestedFrame == failedFrame) {
        m_requestedFrame = -1;
        m_nativeTraversalRequested = false;
    }
    emit seekFailed(tr("The playback engine could not settle on the requested frame."));
    emit seekSettled();
    publishCurrent(renderedFrame);
    if (m_requestedFrame >= 0 && m_requestedFrame != m_currentFrame) {
        m_requestTimer.start();
    }
}

void PlaybackController::processRequestedTarget()
{
    if (m_busy || m_requestTimer.isActive() || m_requestedFrame < 0 || m_index.isEmpty()
        || !m_lastRenderedPosition.has_value()) {
        return;
    }
    const qsizetype target = m_requestedFrame;
    if (target == m_currentFrame) {
        m_requestedFrame = -1;
        m_nativeTraversalRequested = false;
        return;
    }
    const qsizetype operationDelta = target - m_currentFrame;
    // libmpv's reverse frame-step uses an estimated seek and can skip source
    // frames in variable-cadence video. Indexed exact seeks are deterministic
    // in that direction; native multi-frame playback remains smooth forward.
    const bool nativeStep = m_nativeTraversalRequested && operationDelta > 0;
    beginTarget(target, nativeStep, operationDelta);
}

void PlaybackController::beginTarget(qsizetype target, bool useNativeStep, qsizetype delta)
{
    if (target < 0 || target >= m_index.count()) {
        return;
    }
    m_busy = true;
    m_expectedFrame = target;
    m_correctiveSeekIssued = !useNativeStep;
    m_engine->setPaused(true);
    emit seekStarted();

    if (useNativeStep && delta > 0) {
        m_engine->frameStep(delta);
    } else if (useNativeStep && delta < 0) {
        m_engine->frameBackStep();
    } else {
        m_engine->seekExact(engineTimestampFor(target));
    }
    if (useNativeStep && delta > 0) {
        const double travelSeconds =
            std::max(0.0, engineTimestampFor(target) - engineTimestampFor(m_currentFrame));
        const int timeout =
            std::clamp(static_cast<int>(std::lround(travelSeconds * 1000.0)) + 1500,
                       1500,
                       15000);
        m_confirmationTimer.start(timeout);
    } else {
        m_confirmationTimer.start(useNativeStep ? 1500 : 4000);
    }
}

void PlaybackController::confirmOrCorrect()
{
    const qsizetype actual =
        m_lastRenderedPosition.has_value()
            ? resolvedPosition(*m_lastRenderedPosition)
            : resolvedEnginePosition();
    if (actual == m_expectedFrame) {
        settleAt(actual);
        return;
    }
    if (m_correctiveSeekIssued) {
        m_confirmationTimer.start(4000);
        return;
    }

    qCInfo(playbackControllerLog) << "Correcting frame step from" << actual << "to"
                                  << m_expectedFrame;
    m_correctiveSeekIssued = true;
    m_engine->seekExact(engineTimestampFor(m_expectedFrame));
    m_confirmationTimer.start(4000);
}

void PlaybackController::settleAt(qsizetype frame)
{
    m_confirmationTimer.stop();
    // Some libmpv builds transiently clear pause while executing frame-step. Reassert it only
    // after the indexed target is confirmed so a frame inspection command can never leak into
    // normal playback.
    m_engine->setPaused(true);
    publishCurrent(frame);
    m_busy = false;
    m_expectedFrame = -1;
    m_correctiveSeekIssued = false;
    if (m_requestedFrame == frame) {
        m_requestedFrame = -1;
        m_nativeTraversalRequested = false;
    }
    emit seekSettled();
    if (m_requestedFrame >= 0) {
        // Give a rapid key burst a brief window to accumulate. This avoids issuing
        // back-to-back native frame-step commands while libmpv is still restoring
        // its paused state, then processes the newest target in one operation.
        m_requestTimer.start();
    }
}

qsizetype PlaybackController::resolvedEnginePosition() const
{
    if (m_index.isEmpty()) {
        return -1;
    }
    const double streamTime = m_engine->timePosition() + m_index.at(0).timestamp;
    return FramePositionResolver::resolve(m_index, streamTime, m_engine->endOfFile());
}

qsizetype PlaybackController::resolvedPosition(double timePosition) const
{
    if (m_index.isEmpty()) {
        return -1;
    }
    const double streamTime = timePosition + m_index.at(0).timestamp;
    const qsizetype resolved =
        FramePositionResolver::resolve(m_index, streamTime, m_engine->endOfFile());

    // Some demuxers report time-pos a few hundred microseconds before the PTS
    // requested by an exact seek. Apply that tolerance only when the adjacent
    // indexed frame is already expected or displayed; a global offset would
    // incorrectly skip the first frame of files with a non-zero start time.
    constexpr double renderedTimestampTolerance = 0.0005;
    const qsizetype anticipated = m_busy ? m_expectedFrame : m_currentFrame;
    if (anticipated == resolved + 1 && anticipated < m_index.count()) {
        const double distance = m_index.timestampForFrame(anticipated) - streamTime;
        if (distance >= 0.0 && distance <= renderedTimestampTolerance) {
            return anticipated;
        }
    }
    return resolved;
}

double PlaybackController::engineTimestampFor(qsizetype frame) const
{
    if (m_index.isEmpty()) {
        return 0.0;
    }
    return std::max(0.0, m_index.timestampForFrame(frame) - m_index.at(0).timestamp);
}

void PlaybackController::publishCurrent(qsizetype frame)
{
    if (frame < 0 || frame == m_currentFrame) {
        return;
    }
    m_currentFrame = frame;
    emit currentFrameChanged();
    emit stepAvailabilityChanged();
}

} // namespace frameviewer
