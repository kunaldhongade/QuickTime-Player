#pragma once

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QString>

#include <atomic>
#include <mutex>

struct mpv_handle;
struct mpv_render_context;
struct mpv_opengl_init_params;

namespace frameviewer {

class MpvEngine final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool available READ available CONSTANT)
    Q_PROPERTY(bool hasMedia READ hasMedia NOTIFY hasMediaChanged)
    Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(double timePosition READ timePosition NOTIFY timePositionChanged)
    Q_PROPERTY(double duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(double volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(bool seeking READ seeking NOTIFY seekingChanged)
    Q_PROPERTY(bool endOfFile READ endOfFile NOTIFY endOfFileChanged)
    Q_PROPERTY(QString filename READ filename NOTIFY filenameChanged)

public:
    explicit MpvEngine(QObject* parent = nullptr);
    ~MpvEngine() override;

    [[nodiscard]] bool available() const;
    [[nodiscard]] bool hasMedia() const;
    [[nodiscard]] bool paused() const;
    [[nodiscard]] double timePosition() const;
    [[nodiscard]] double duration() const;
    [[nodiscard]] double volume() const;
    [[nodiscard]] bool muted() const;
    [[nodiscard]] bool seeking() const;
    [[nodiscard]] bool endOfFile() const;
    [[nodiscard]] QString filename() const;
    [[nodiscard]] QString initializationError() const;

    void setPaused(bool paused);
    void setVolume(double volume);
    void setMuted(bool muted);
    void setPlaybackSpeed(double speed);
    [[nodiscard]] bool setPlaybackDirection(bool backward);

    Q_INVOKABLE void loadFile(const QString& path);
    Q_INVOKABLE void togglePause();
    void frameStep();
    void frameBackStep();
    void seekExact(double timestamp);
    void takeScreenshot(const QString& path);

    // These functions are called only with Qt's scene-graph OpenGL context current.
    bool createRenderContext(mpv_opengl_init_params* initialization);
    void releaseRenderContext();
    void renderFrame(int framebuffer, int width, int height);

signals:
    void fileStarted();
    void fileLoaded();
    void playbackRestarted();
    void playbackEnded(int reason);
    void commandFinished(quint64 request, int error);
    void renderUpdateRequested();
    void errorOccurred(const QString& message);
    void logMessage(const QString& prefix, const QString& level, const QString& text);

    void hasMediaChanged();
    void pausedChanged();
    void timePositionChanged();
    void durationChanged();
    void volumeChanged();
    void mutedChanged();
    void seekingChanged();
    void endOfFileChanged();
    void filenameChanged();

private slots:
    void drainEvents();

private:
    static void wakeupCallback(void* context);
    static void renderUpdateCallback(void* context);
    void observeProperties();
    quint64 issueCommand(const QList<QByteArray>& arguments);
    void setBooleanProperty(const char* name, bool value);
    void setDoubleProperty(const char* name, double value);
    [[nodiscard]] bool setStringProperty(const char* name, const char* value);
    void updateBooleanProperty(const char* name, bool value);
    void updateDoubleProperty(const char* name, double value);
    void updateStringProperty(const char* name, const QString& value);
    void shutdown();

    mpv_handle* m_mpv = nullptr;
    mpv_render_context* m_renderContext = nullptr;
    mutable std::mutex m_renderMutex;
    std::atomic_bool m_shuttingDown{false};
    std::atomic_bool m_renderReady{false};
    std::atomic<quint64> m_nextRequest{1};

    bool m_available = false;
    bool m_hasMedia = false;
    bool m_paused = true;
    bool m_seeking = false;
    bool m_endOfFile = false;
    double m_timePosition = 0.0;
    double m_duration = 0.0;
    double m_volume = 100.0;
    bool m_muted = false;
    QString m_filename;
    QString m_initializationError;
    QString m_pendingLoadPath;
};

} // namespace frameviewer
