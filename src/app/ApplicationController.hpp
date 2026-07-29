#pragma once

#include "frames/FrameIndexer.hpp"
#include "models/PlayerState.hpp"
#include "playback/MpvEngine.hpp"
#include "playback/PlaybackController.hpp"

#include <QEvent>
#include <QObject>
#include <QUrl>

namespace frameviewer {

class ApplicationController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(frameviewer::MpvEngine* engine READ engine CONSTANT)
    Q_PROPERTY(frameviewer::PlayerState::Value state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool hasMedia READ hasMedia NOTIFY playerChanged)
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY playerChanged)
    Q_PROPERTY(bool isPaused READ isPaused NOTIFY playerChanged)
    Q_PROPERTY(bool isSeeking READ isSeeking NOTIFY playerChanged)
    Q_PROPERTY(bool isIndexing READ isIndexing NOTIFY indexingChanged)
    Q_PROPERTY(double indexingProgress READ indexingProgress NOTIFY indexingChanged)
    Q_PROPERTY(qint64 currentFrame READ currentFrame NOTIFY frameChanged)
    Q_PROPERTY(qint64 totalFrames READ totalFrames NOTIFY frameChanged)
    Q_PROPERTY(double currentTime READ currentTime NOTIFY playerChanged)
    Q_PROPERTY(double duration READ duration NOTIFY playerChanged)
    Q_PROPERTY(double volume READ volume NOTIFY playerChanged)
    Q_PROPERTY(bool muted READ muted NOTIFY playerChanged)
    Q_PROPERTY(QString filename READ filename NOTIFY filenameChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)
    Q_PROPERTY(bool canStepBackward READ canStepBackward NOTIFY frameChanged)
    Q_PROPERTY(bool canStepForward READ canStepForward NOTIFY frameChanged)
    Q_PROPERTY(bool fullscreen READ fullscreen WRITE setFullscreen NOTIFY fullscreenChanged)
    Q_PROPERTY(bool exactFrameIndexAvailable READ exactFrameIndexAvailable NOTIFY frameChanged)

public:
    explicit ApplicationController(QObject* parent = nullptr);
    ~ApplicationController() override;

    [[nodiscard]] MpvEngine* engine();
    [[nodiscard]] PlayerState::Value state() const;
    [[nodiscard]] bool hasMedia() const;
    [[nodiscard]] bool isPlaying() const;
    [[nodiscard]] bool isPaused() const;
    [[nodiscard]] bool isSeeking() const;
    [[nodiscard]] bool isIndexing() const;
    [[nodiscard]] double indexingProgress() const;
    [[nodiscard]] qint64 currentFrame() const;
    [[nodiscard]] qint64 totalFrames() const;
    [[nodiscard]] double currentTime() const;
    [[nodiscard]] double duration() const;
    [[nodiscard]] double volume() const;
    [[nodiscard]] bool muted() const;
    [[nodiscard]] QString filename() const;
    [[nodiscard]] QString errorMessage() const;
    [[nodiscard]] bool canStepBackward() const;
    [[nodiscard]] bool canStepForward() const;
    [[nodiscard]] bool fullscreen() const;
    [[nodiscard]] bool exactFrameIndexAvailable() const;

    void setFullscreen(bool fullscreen);
    bool eventFilter(QObject* watched, QEvent* event) override;

    Q_INVOKABLE void openFile(const QUrl& url);
    Q_INVOKABLE void togglePlayback();
    Q_INVOKABLE void stepFrames(int count);
    Q_INVOKABLE void seekToFrame(qint64 zeroBasedFrame);
    Q_INVOKABLE void seekToFirstFrame();
    Q_INVOKABLE void seekToLastFrame();
    Q_INVOKABLE void toggleMute();
    Q_INVOKABLE void changeVolume(double delta);
    Q_INVOKABLE void saveCurrentFrame();
    Q_INVOKABLE void clearError();
    Q_INVOKABLE void reportUserActivity();

signals:
    void stateChanged();
    void playerChanged();
    void indexingChanged();
    void frameChanged();
    void filenameChanged();
    void errorChanged();
    void fullscreenChanged();
    void openDialogRequested();
    void fullscreenRequested(bool fullscreen);
    void userActivity();
    void toastRequested(const QString& message);

private:
    void setState(PlayerState::Value state);
    void setError(const QString& message, bool fatal);
    void updatePlaybackState();
    [[nodiscard]] QString nextScreenshotPath() const;

    MpvEngine m_engine;
    PlaybackController m_playback;
    FrameIndexer m_indexer;
    PlayerState::Value m_state = PlayerState::Empty;
    QString m_currentPath;
    QString m_filename;
    QString m_errorMessage;
    quint64 m_indexGeneration = 0;
    bool m_indexing = false;
    bool m_controllerSeeking = false;
    bool m_fullscreen = false;
};

} // namespace frameviewer
