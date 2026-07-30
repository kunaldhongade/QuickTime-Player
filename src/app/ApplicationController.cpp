#include "app/ApplicationController.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QKeySequence>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTimer>

#include <algorithm>

namespace frameviewer {

ApplicationController::ApplicationController(QObject* parent)
    : QObject(parent)
    , m_engine(this)
    , m_playback(&m_engine, this)
    , m_indexer(this)
    , m_exporter(this)
{
    QGuiApplication::instance()->installEventFilter(this);

    connect(&m_engine, &MpvEngine::fileStarted, this, [this] {
        setState(PlayerState::Opening);
    });
    connect(&m_engine, &MpvEngine::fileLoaded, this, [this] {
        m_engine.setPaused(true);
        m_indexer.setMediaDuration(m_engine.duration());
        updatePlaybackState();
        emit playerChanged();
    });
    connect(&m_engine, &MpvEngine::errorOccurred, this, [this](const QString& message) {
        setError(message, true);
    });
    connect(&m_engine, &MpvEngine::pausedChanged, this, [this] {
        updatePlaybackState();
        emit playerChanged();
    });
    connect(&m_engine, &MpvEngine::timePositionChanged, this, &ApplicationController::playerChanged);
    connect(&m_engine, &MpvEngine::durationChanged, this, [this] {
        m_indexer.setMediaDuration(m_engine.duration());
        emit playerChanged();
    });
    connect(&m_engine, &MpvEngine::volumeChanged, this, &ApplicationController::playerChanged);
    connect(&m_engine, &MpvEngine::mutedChanged, this, &ApplicationController::playerChanged);
    connect(&m_engine, &MpvEngine::seekingChanged, this, &ApplicationController::playerChanged);
    connect(&m_engine, &MpvEngine::hasMediaChanged, this, &ApplicationController::playerChanged);
    connect(&m_engine, &MpvEngine::filenameChanged, this, [this] {
        if (!m_engine.filename().isEmpty() && m_engine.filename() != m_filename) {
            m_filename = m_engine.filename();
            emit filenameChanged();
        }
    });
    connect(&m_engine, &MpvEngine::playbackEnded, this, [this](int) {
        if (m_engine.endOfFile()) {
            setState(PlayerState::Ended);
            emit playerChanged();
        }
    });

    connect(&m_indexer,
            &FrameIndexer::progressChanged,
            this,
            [this](quint64 generation, double progress) {
                if (generation != m_indexGeneration) {
                    return;
                }
                m_indexingProgress = std::clamp(progress, 0.0, 1.0);
                emit indexingChanged();
            });
    connect(&m_indexer,
            &FrameIndexer::finished,
            this,
            [this](quint64 generation, const FrameIndex& index, bool) {
                if (generation != m_indexGeneration) {
                    return;
                }
                m_indexing = false;
                m_indexingProgress = 1.0;
                m_playback.setFrameIndex(index);
                emit indexingChanged();
                emit frameChanged();
                emit exportChanged();
                updatePlaybackState();
            });
    connect(&m_indexer,
            &FrameIndexer::failed,
            this,
            [this](quint64 generation, const QString& message) {
                if (generation != m_indexGeneration) {
                    return;
                }
                m_indexing = false;
                m_indexingProgress = 0.0;
                emit indexingChanged();
                updatePlaybackState();
                emit toastRequested(tr("Frame indexing failed: %1").arg(message));
            });

    connect(&m_playback, &PlaybackController::currentFrameChanged, this, &ApplicationController::frameChanged);
    connect(&m_playback, &PlaybackController::totalFramesChanged, this, &ApplicationController::frameChanged);
    connect(&m_playback,
            &PlaybackController::stepAvailabilityChanged,
            this,
            &ApplicationController::frameChanged);
    connect(&m_playback,
            &PlaybackController::exactFrameIndexAvailableChanged,
            this,
            &ApplicationController::frameChanged);
    connect(&m_playback, &PlaybackController::seekStarted, this, [this] {
        m_controllerSeeking = true;
        setState(PlayerState::Seeking);
        emit playerChanged();
    });
    connect(&m_playback, &PlaybackController::seekSettled, this, [this] {
        m_controllerSeeking = false;
        updatePlaybackState();
        emit playerChanged();
    });
    connect(&m_playback, &PlaybackController::seekFailed, this, [this](const QString& message) {
        emit toastRequested(message);
    });

    connect(&m_exporter,
            &FrameRangeExporter::started,
            this,
            [this](qint64, const QString&) {
                m_exportingFrames = true;
                m_exportProgress = 0.0;
                emit exportChanged();
            });
    connect(&m_exporter,
            &FrameRangeExporter::progressChanged,
            this,
            [this](qint64 completed, qint64 total) {
                m_exportProgress =
                    total > 0 ? std::clamp(static_cast<double>(completed) / total, 0.0, 1.0)
                              : 0.0;
                emit exportChanged();
            });
    connect(&m_exporter,
            &FrameRangeExporter::finished,
            this,
            [this](const QString& directory, qint64 exportedFrames) {
                m_exportingFrames = false;
                m_exportProgress = 1.0;
                emit exportChanged();
                emit toastRequested(
                    tr("Exported %1 frames to %2")
                        .arg(exportedFrames)
                        .arg(QDir::toNativeSeparators(directory)));
            });
    connect(&m_exporter, &FrameRangeExporter::failed, this, [this](const QString& message) {
        m_exportingFrames = false;
        m_exportProgress = 0.0;
        emit exportChanged();
        emit toastRequested(tr("Frame export failed: %1").arg(message));
    });

    if (!m_engine.available()) {
        setError(m_engine.initializationError(), true);
    }
}

ApplicationController::~ApplicationController()
{
    QGuiApplication::instance()->removeEventFilter(this);
    m_indexer.cancel();
}

MpvEngine* ApplicationController::engine()
{
    return &m_engine;
}

PlayerState::Value ApplicationController::state() const
{
    return m_state;
}

bool ApplicationController::hasMedia() const
{
    return m_engine.hasMedia();
}

bool ApplicationController::isPlaying() const
{
    return hasMedia() && !m_engine.paused();
}

bool ApplicationController::isPaused() const
{
    return !isPlaying();
}

bool ApplicationController::isSeeking() const
{
    return m_engine.seeking() || m_controllerSeeking;
}

bool ApplicationController::isIndexing() const
{
    return m_indexing;
}

double ApplicationController::indexingProgress() const
{
    return m_indexing ? m_indexingProgress : 0.0;
}

qint64 ApplicationController::currentFrame() const
{
    return m_playback.currentFrame();
}

qint64 ApplicationController::totalFrames() const
{
    return m_playback.totalFrames();
}

double ApplicationController::currentTime() const
{
    return m_engine.timePosition();
}

double ApplicationController::duration() const
{
    return m_engine.duration();
}

double ApplicationController::volume() const
{
    return m_engine.volume();
}

bool ApplicationController::muted() const
{
    return m_engine.muted();
}

QString ApplicationController::filename() const
{
    return m_filename;
}

QString ApplicationController::errorMessage() const
{
    return m_errorMessage;
}

bool ApplicationController::canStepBackward() const
{
    return m_playback.canStepBackward();
}

bool ApplicationController::canStepForward() const
{
    return m_playback.canStepForward();
}

bool ApplicationController::fullscreen() const
{
    return m_fullscreen;
}

bool ApplicationController::exactFrameIndexAvailable() const
{
    return m_playback.exactFrameIndexAvailable();
}

bool ApplicationController::canOpenPreviousVideo() const
{
    return m_directoryNavigator.canOpenPrevious();
}

bool ApplicationController::canOpenNextVideo() const
{
    return m_directoryNavigator.canOpenNext();
}

qint64 ApplicationController::rangeStartFrame() const
{
    return m_rangeStartFrame;
}

qint64 ApplicationController::rangeEndFrame() const
{
    return m_rangeEndFrame;
}

qint64 ApplicationController::selectedFrameCount() const
{
    if (m_rangeStartFrame <= 0 || m_rangeEndFrame < m_rangeStartFrame) {
        return 0;
    }
    return m_rangeEndFrame - m_rangeStartFrame + 1;
}

bool ApplicationController::canExportFrameRange() const
{
    return exactFrameIndexAvailable() && selectedFrameCount() > 0 && !m_exportingFrames;
}

bool ApplicationController::isExportingFrames() const
{
    return m_exportingFrames;
}

double ApplicationController::exportProgress() const
{
    return m_exportProgress;
}

void ApplicationController::setFullscreen(bool fullscreen)
{
    if (m_fullscreen == fullscreen) {
        return;
    }
    m_fullscreen = fullscreen;
    emit fullscreenChanged();
}

bool ApplicationController::eventFilter(QObject* watched, QEvent* event)
{
    Q_UNUSED(watched)
    if (event->type() != QEvent::KeyPress || QGuiApplication::modalWindow()) {
        return false;
    }

    auto* keyEvent = static_cast<QKeyEvent*>(event);
    const int key = keyEvent->key();
    if ((key == Qt::Key_Left || key == Qt::Key_Right) && keyEvent->isAutoRepeat()) {
        keyEvent->accept();
        return true;
    }

    if (keyEvent->matches(QKeySequence::Open)) {
        emit openDialogRequested();
    } else if (key == Qt::Key_Space) {
        togglePlayback();
    } else if (key == Qt::Key_Left
               && keyEvent->modifiers().testFlag(Qt::ControlModifier)) {
        openPreviousVideo();
    } else if (key == Qt::Key_Right
               && keyEvent->modifiers().testFlag(Qt::ControlModifier)) {
        openNextVideo();
    } else if (key == Qt::Key_Left) {
        stepFrames(keyEvent->modifiers().testFlag(Qt::ShiftModifier) ? -10 : -1);
    } else if (key == Qt::Key_Right) {
        stepFrames(keyEvent->modifiers().testFlag(Qt::ShiftModifier) ? 10 : 1);
    } else if (key == Qt::Key_Home) {
        seekToFirstFrame();
    } else if (key == Qt::Key_End) {
        seekToLastFrame();
    } else if (key == Qt::Key_F) {
        emit fullscreenRequested(!m_fullscreen);
    } else if (key == Qt::Key_Escape) {
        if (!m_fullscreen) {
            return false;
        }
        emit fullscreenRequested(false);
    } else if (key == Qt::Key_M) {
        toggleMute();
    } else if (key == Qt::Key_Up) {
        changeVolume(5.0);
    } else if (key == Qt::Key_Down) {
        changeVolume(-5.0);
    } else if (key == Qt::Key_S && keyEvent->modifiers() == Qt::NoModifier) {
        saveCurrentFrame();
    } else {
        return false;
    }

    reportUserActivity();
    keyEvent->accept();
    return true;
}

void ApplicationController::openFile(const QUrl& url)
{
    const QString path = url.isLocalFile() ? url.toLocalFile() : url.toString();
    const QFileInfo information(path);
    if (!information.exists() || !information.isFile()) {
        setError(tr("Could not open “%1”: the file does not exist.").arg(information.fileName()),
                 false);
        return;
    }

    m_indexer.cancel();
    m_playback.clear();
    m_indexing = true;
    m_indexingProgress = 0.0;
    m_errorMessage.clear();
    m_currentPath = information.absoluteFilePath();
    m_filename = information.fileName();
    m_directoryNavigator.setCurrentFile(m_currentPath);
    resetFrameRange();
    emit indexingChanged();
    emit errorChanged();
    emit filenameChanged();
    emit folderNavigationChanged();
    emit frameChanged();
    emit exportChanged();
    setState(PlayerState::Opening);
    m_indexer.start(m_currentPath);
    m_indexGeneration = m_indexer.generation();
    m_engine.loadFile(m_currentPath);
    reportUserActivity();
}

void ApplicationController::togglePlayback()
{
    if (!hasMedia()) {
        emit openDialogRequested();
        return;
    }
    m_engine.togglePause();
    reportUserActivity();
}

void ApplicationController::stepFrames(int count)
{
    if (!exactFrameIndexAvailable()) {
        if (m_indexing) {
            emit toastRequested(tr("Frame controls will be ready when indexing finishes."));
        }
        return;
    }
    m_playback.stepBy(count);
    reportUserActivity();
}

void ApplicationController::seekToFrame(qint64 zeroBasedFrame)
{
    m_playback.seekToFrame(zeroBasedFrame);
    reportUserActivity();
}

void ApplicationController::seekToFirstFrame()
{
    m_playback.seekToFirstFrame();
    reportUserActivity();
}

void ApplicationController::seekToLastFrame()
{
    m_playback.seekToLastFrame();
    reportUserActivity();
}

void ApplicationController::toggleMute()
{
    if (hasMedia()) {
        m_engine.setMuted(!m_engine.muted());
    }
}

void ApplicationController::changeVolume(double delta)
{
    if (hasMedia()) {
        m_engine.setVolume(m_engine.volume() + delta);
    }
}

void ApplicationController::saveCurrentFrame()
{
    if (!hasMedia()) {
        return;
    }
    const QString path = nextScreenshotPath();
    m_engine.takeScreenshot(path);
    QTimer::singleShot(800, this, [this, path] {
        if (QFileInfo::exists(path)) {
            emit toastRequested(tr("Saved frame to %1").arg(QDir::toNativeSeparators(path)));
        } else {
            emit toastRequested(tr("The frame could not be saved."));
        }
    });
}

void ApplicationController::openPreviousVideo()
{
    const QString path = m_directoryNavigator.previousPath();
    if (!path.isEmpty()) {
        openFile(QUrl::fromLocalFile(path));
    }
}

void ApplicationController::openNextVideo()
{
    const QString path = m_directoryNavigator.nextPath();
    if (!path.isEmpty()) {
        openFile(QUrl::fromLocalFile(path));
    }
}

void ApplicationController::markRangeStart()
{
    if (!exactFrameIndexAvailable() || currentFrame() <= 0) {
        emit toastRequested(tr("Wait for frame indexing to finish before setting a marker."));
        return;
    }
    m_rangeStartFrame = currentFrame();
    if (m_rangeEndFrame > 0 && m_rangeEndFrame < m_rangeStartFrame) {
        m_rangeEndFrame = 0;
    }
    emit frameRangeChanged();
    emit exportChanged();
    emit toastRequested(tr("Start frame set to %1.").arg(m_rangeStartFrame));
}

void ApplicationController::markRangeEnd()
{
    if (!exactFrameIndexAvailable() || currentFrame() <= 0) {
        emit toastRequested(tr("Wait for frame indexing to finish before setting a marker."));
        return;
    }
    if (m_rangeStartFrame <= 0) {
        emit toastRequested(tr("Set the start frame first."));
        return;
    }
    if (currentFrame() < m_rangeStartFrame) {
        emit toastRequested(tr("The end frame must be at or after frame %1.")
                                .arg(m_rangeStartFrame));
        return;
    }
    m_rangeEndFrame = currentFrame();
    emit frameRangeChanged();
    emit exportChanged();
    emit toastRequested(tr("End frame set to %1.").arg(m_rangeEndFrame));
}

void ApplicationController::clearFrameRange()
{
    if (m_rangeStartFrame == 0 && m_rangeEndFrame == 0) {
        return;
    }
    resetFrameRange();
    emit toastRequested(tr("Frame selection cleared."));
}

void ApplicationController::exportFrameRange()
{
    if (!canExportFrameRange()) {
        emit toastRequested(tr("Set both a start and end frame before exporting."));
        return;
    }
    m_exporter.start(m_currentPath, m_rangeStartFrame, m_rangeEndFrame);
}

void ApplicationController::clearError()
{
    if (m_errorMessage.isEmpty()) {
        return;
    }
    m_errorMessage.clear();
    emit errorChanged();
    updatePlaybackState();
}

void ApplicationController::reportUserActivity()
{
    emit userActivity();
}

void ApplicationController::setState(PlayerState::Value state)
{
    if (m_state == state) {
        return;
    }
    m_state = state;
    emit stateChanged();
}

void ApplicationController::setError(const QString& message, bool fatal)
{
    m_errorMessage = message;
    emit errorChanged();
    if (fatal) {
        m_indexer.cancel();
        if (m_indexing) {
            m_indexing = false;
            m_indexingProgress = 0.0;
            emit indexingChanged();
        }
        setState(PlayerState::Error);
    }
}

void ApplicationController::updatePlaybackState()
{
    if (!hasMedia()) {
        if (!m_errorMessage.isEmpty()) {
            setState(PlayerState::Error);
        } else {
            setState(m_currentPath.isEmpty() ? PlayerState::Empty : PlayerState::Opening);
        }
    } else if (m_controllerSeeking || m_engine.seeking()) {
        setState(PlayerState::Seeking);
    } else if (m_indexing) {
        setState(PlayerState::Indexing);
    } else if (m_engine.endOfFile()) {
        setState(PlayerState::Ended);
    } else if (m_engine.paused()) {
        setState(PlayerState::ReadyPaused);
    } else {
        setState(PlayerState::Playing);
    }
}

void ApplicationController::resetFrameRange()
{
    m_rangeStartFrame = 0;
    m_rangeEndFrame = 0;
    emit frameRangeChanged();
    emit exportChanged();
}

QString ApplicationController::nextScreenshotPath() const
{
    QDir directory(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
    directory.mkpath(QStringLiteral("FrameViewer"));
    directory.cd(QStringLiteral("FrameViewer"));

    QString base = QFileInfo(m_currentPath).completeBaseName();
    base.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9._-]+")), QStringLiteral("_"));
    const qint64 frame = std::max<qint64>(1, currentFrame());
    const QString stem = QStringLiteral("%1-frame-%2").arg(base).arg(frame, 6, 10, QLatin1Char('0'));
    QString candidate = directory.filePath(stem + QStringLiteral(".png"));
    int suffix = 2;
    while (QFileInfo::exists(candidate)) {
        candidate = directory.filePath(
            QStringLiteral("%1-%2.png").arg(stem).arg(suffix++));
    }
    return candidate;
}

} // namespace frameviewer
