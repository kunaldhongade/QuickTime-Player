#pragma once

#include "frames/FrameIndex.hpp"
#include "frames/FrameIndexCache.hpp"

#include <QObject>
#include <QProcess>

namespace frameviewer {

class FrameIndexer final : public QObject {
    Q_OBJECT

public:
    explicit FrameIndexer(QObject* parent = nullptr);
    ~FrameIndexer() override;

    [[nodiscard]] bool isRunning() const;
    [[nodiscard]] quint64 generation() const;

public slots:
    void start(const QString& mediaPath);
    void cancel();

signals:
    void started(quint64 generation);
    void finished(quint64 generation, const frameviewer::FrameIndex& index, bool cacheHit);
    void failed(quint64 generation, const QString& message);
    void cancelled(quint64 generation);

private slots:
    void consumeOutput();
    void processFinished(int exitCode, QProcess::ExitStatus status);
    void processError(QProcess::ProcessError error);

private:
    void consumeLine(const QByteArray& line);
    void completeFromCache(const FrameIndex& index, quint64 generation);
    [[nodiscard]] static std::optional<double> parseNumber(const QByteArray& value);

    FrameIndexCache m_cache;
    QProcess* m_process = nullptr;
    QByteArray m_pendingOutput;
    FrameIndex m_workingIndex;
    QString m_mediaPath;
    QString m_standardError;
    quint64 m_generation = 0;
    bool m_cancelling = false;
};

} // namespace frameviewer
