#pragma once

#include <QObject>
#include <QProcess>

namespace frameviewer {

class FrameRangeExporter final : public QObject {
    Q_OBJECT

public:
    explicit FrameRangeExporter(QObject* parent = nullptr);
    ~FrameRangeExporter() override;

    [[nodiscard]] bool isRunning() const;
    [[nodiscard]] QString outputDirectory() const;

public slots:
    void start(const QString& mediaPath, qint64 firstFrame, qint64 lastFrame);
    void cancel();

signals:
    void started(qint64 totalFrames, const QString& outputDirectory);
    void progressChanged(qint64 completedFrames, qint64 totalFrames);
    void finished(const QString& outputDirectory, qint64 exportedFrames);
    void failed(const QString& message);
    void cancelled();

private slots:
    void consumeProgress();
    void processFinished(int exitCode, QProcess::ExitStatus status);
    void processError(QProcess::ProcessError error);

private:
    [[nodiscard]] static QString createOutputDirectory(const QString& mediaPath);
    [[nodiscard]] static QString safeStem(const QString& mediaPath);

    QProcess* m_process = nullptr;
    QByteArray m_pendingProgress;
    QString m_standardError;
    QString m_outputDirectory;
    QString m_outputPattern;
    qint64 m_totalFrames = 0;
    qint64 m_completedFrames = 0;
    bool m_cancelling = false;
};

} // namespace frameviewer
