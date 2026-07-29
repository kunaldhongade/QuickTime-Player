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
    connect(m_engine,
            &MpvEngine::timePositionChanged,
            this,
            &PlaybackController::positionChanged);
    connect(m_engine,
            &MpvEngine::playbackRestarted,
            this,
            &PlaybackController::playbackRestarted);
    connect(m_engine, &MpvEngine::endOfFileChanged, this, &PlaybackController::positionChanged);
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
    m_pendingSteps.clear();
    m_busy = false;
    m_confirmationTimer.stop();
    m_expectedFrame = -1;
    m_currentFrame = resolvedEnginePosition();
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
    m_pendingSteps.clear();
    m_confirmationTimer.stop();
    m_busy = false;
    m_currentFrame = -1;
    m_expectedFrame = -1;
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
    m_pendingSteps.enqueue(frameCount);
    processQueue();
}

void PlaybackController::seekToFrame(qint64 zeroBasedFrame)
{
    if (m_index.isEmpty()) {
        return;
    }
    m_pendingSteps.clear();
    const qsizetype target = FramePositionResolver::clampTarget(m_index, zeroBasedFrame);
    if (target == m_currentFrame && !m_busy) {
        return;
    }
    beginTarget(target, false, 0);
}

void PlaybackController::seekToFirstFrame()
{
    seekToFrame(0);
}

void PlaybackController::seekToLastFrame()
{
    seekToFrame(m_index.count() - 1);
}

void PlaybackController::positionChanged()
{
    if (m_index.isEmpty()) {
        return;
    }
    if (m_busy) {
        if (resolvedEnginePosition() == m_expectedFrame) {
            settleAt(m_expectedFrame);
        }
        return;
    }
    publishCurrent(resolvedEnginePosition());
}

void PlaybackController::playbackRestarted()
{
    if (m_busy) {
        confirmOrCorrect();
    } else {
        positionChanged();
    }
}

void PlaybackController::confirmationTimedOut()
{
    if (!m_busy) {
        return;
    }
    if (resolvedEnginePosition() == m_expectedFrame) {
        settleAt(m_expectedFrame);
        return;
    }
    if (!m_correctiveSeekIssued) {
        confirmOrCorrect();
        return;
    }

    qCWarning(playbackControllerLog)
        << "Frame seek failed to settle on indexed frame" << m_expectedFrame;
    m_busy = false;
    m_expectedFrame = -1;
    emit seekFailed(tr("The playback engine could not settle on the requested frame."));
    emit seekSettled();
    publishCurrent(resolvedEnginePosition());
    processQueue();
}

void PlaybackController::processQueue()
{
    if (m_busy || m_pendingSteps.isEmpty() || m_index.isEmpty()) {
        return;
    }
    const int delta = m_pendingSteps.dequeue();
    const qsizetype target =
        FramePositionResolver::clampTarget(m_index, m_currentFrame + delta);
    if (target == m_currentFrame) {
        processQueue();
        return;
    }
    const bool nativeStep = std::abs(delta) == 1;
    beginTarget(target, nativeStep, delta < 0 ? -1 : 1);
}

void PlaybackController::beginTarget(qsizetype target, bool useNativeStep, int direction)
{
    if (target < 0 || target >= m_index.count()) {
        return;
    }
    if (m_busy) {
        m_confirmationTimer.stop();
    }

    m_busy = true;
    m_expectedFrame = target;
    m_correctiveSeekIssued = !useNativeStep;
    m_engine->setPaused(true);
    emit seekStarted();

    if (useNativeStep && direction > 0) {
        m_engine->frameStep();
    } else if (useNativeStep && direction < 0) {
        m_engine->frameBackStep();
    } else {
        m_engine->seekExact(engineTimestampFor(target));
    }
    m_confirmationTimer.start(useNativeStep ? 450 : 750);
}

void PlaybackController::confirmOrCorrect()
{
    const qsizetype actual = resolvedEnginePosition();
    if (actual == m_expectedFrame) {
        settleAt(actual);
        return;
    }
    if (m_correctiveSeekIssued) {
        m_confirmationTimer.start(500);
        return;
    }

    qCInfo(playbackControllerLog) << "Correcting frame step from" << actual << "to"
                                  << m_expectedFrame;
    m_correctiveSeekIssued = true;
    m_engine->seekExact(engineTimestampFor(m_expectedFrame));
    m_confirmationTimer.start(750);
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
    emit seekSettled();
    processQueue();
}

qsizetype PlaybackController::resolvedEnginePosition() const
{
    if (m_index.isEmpty()) {
        return -1;
    }
    const double streamTime = m_engine->timePosition() + m_index.at(0).timestamp;
    return FramePositionResolver::resolve(m_index, streamTime, m_engine->endOfFile());
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
