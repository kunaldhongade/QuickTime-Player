#include "frames/FrameIndexer.hpp"

#include "platform/ExecutableLocator.hpp"

#include <QFileInfo>
#include <QLoggingCategory>
#include <QTimer>

#include <algorithm>
#include <cmath>

Q_LOGGING_CATEGORY(frameIndexLog, "frameviewer.frames.indexer")

namespace frameviewer {

FrameIndexer::FrameIndexer(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<FrameIndex>();
}

FrameIndexer::~FrameIndexer()
{
    cancel();
}

bool FrameIndexer::isRunning() const
{
    return m_process != nullptr;
}

quint64 FrameIndexer::generation() const
{
    return m_generation;
}

void FrameIndexer::start(const QString& mediaPath)
{
    cancel();
    ++m_generation;
    m_mediaPath = QFileInfo(mediaPath).absoluteFilePath();
    m_workingIndex.clear();
    m_pendingOutput.clear();
    m_standardError.clear();
    m_cancelling = false;
    m_mediaDuration = 0.0;
    m_lastProgress = -1.0;
    m_firstTimestamp.reset();
    m_progressTimer.start();

    emit started(m_generation);

    FrameIndex cached;
    if (m_cache.load(m_mediaPath, cached)) {
        const quint64 requestedGeneration = m_generation;
        QTimer::singleShot(0, this, [this, cached, requestedGeneration] {
            completeFromCache(cached, requestedGeneration);
        });
        return;
    }

    const QString ffprobe = locateExecutable(QStringLiteral("ffprobe"));
    if (ffprobe.isEmpty()) {
        emit failed(m_generation,
                    tr("FFprobe was not found. Install FFmpeg or add ffprobe to PATH."));
        return;
    }

    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &FrameIndexer::consumeOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, [this] {
        if (m_process) {
            m_standardError += QString::fromUtf8(m_process->readAllStandardError());
        }
    });
    connect(m_process,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            &FrameIndexer::processFinished);
    connect(m_process, &QProcess::errorOccurred, this, &FrameIndexer::processError);

    const QStringList arguments{
        QStringLiteral("-v"),
        QStringLiteral("error"),
        QStringLiteral("-threads"),
        QStringLiteral("0"),
        QStringLiteral("-select_streams"),
        QStringLiteral("V:0"),
        QStringLiteral("-show_frames"),
        QStringLiteral("-show_entries"),
        QStringLiteral("frame=key_frame,best_effort_timestamp_time,pts_time"),
        QStringLiteral("-of"),
        QStringLiteral("csv=p=0"),
        m_mediaPath,
    };
    qCInfo(frameIndexLog) << "Starting exact frame index for" << QFileInfo(m_mediaPath).fileName();
    m_process->start(ffprobe, arguments, QIODevice::ReadOnly);
}

void FrameIndexer::cancel()
{
    if (!m_process) {
        return;
    }

    m_cancelling = true;
    const quint64 cancelledGeneration = m_generation;
    disconnect(m_process, nullptr, this, nullptr);
    if (m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        if (!m_process->waitForFinished(300)) {
            m_process->kill();
            m_process->waitForFinished(1000);
        }
    }
    m_process->deleteLater();
    m_process = nullptr;
    m_workingIndex.clear();
    m_pendingOutput.clear();
    emit cancelled(cancelledGeneration);
}

void FrameIndexer::setMediaDuration(double seconds)
{
    if (!std::isfinite(seconds) || seconds <= 0.0) {
        return;
    }
    m_mediaDuration = seconds;
    if (!m_workingIndex.isEmpty()) {
        reportProgress(m_workingIndex.at(m_workingIndex.count() - 1).timestamp);
    }
}

void FrameIndexer::consumeOutput()
{
    if (!m_process) {
        return;
    }
    m_pendingOutput += m_process->readAllStandardOutput();
    qsizetype lineStart = 0;
    qsizetype newline = -1;
    while ((newline = m_pendingOutput.indexOf('\n', lineStart)) >= 0) {
        const QByteArray line = m_pendingOutput.mid(lineStart, newline - lineStart).trimmed();
        lineStart = newline + 1;
        if (!line.isEmpty()) {
            consumeLine(line);
        }
    }
    if (lineStart > 0) {
        m_pendingOutput.remove(0, lineStart);
    }
}

void FrameIndexer::processFinished(int exitCode, QProcess::ExitStatus status)
{
    if (!m_process) {
        return;
    }
    consumeOutput();
    if (!m_pendingOutput.trimmed().isEmpty()) {
        consumeLine(m_pendingOutput.trimmed());
    }

    QProcess* completedProcess = m_process;
    m_process = nullptr;
    completedProcess->deleteLater();

    if (m_cancelling) {
        return;
    }
    if (status != QProcess::NormalExit || exitCode != 0) {
        const QString details = m_standardError.trimmed();
        emit failed(m_generation,
                    details.isEmpty() ? tr("FFprobe could not read this video.") : details);
        return;
    }
    if (!m_workingIndex.isValid()) {
        emit failed(m_generation, tr("No decodable video frames were found."));
        return;
    }

    if (!m_cache.save(m_mediaPath, m_workingIndex)) {
        qCWarning(frameIndexLog) << "Could not save the frame index cache";
    }
    emit progressChanged(m_generation, 1.0);
    qCInfo(frameIndexLog) << "Frame index complete:" << m_workingIndex.count() << "frames";
    emit finished(m_generation, m_workingIndex, false);
}

void FrameIndexer::processError(QProcess::ProcessError error)
{
    if (m_cancelling || error == QProcess::Crashed) {
        return;
    }
    if (error == QProcess::FailedToStart) {
        QProcess* failedProcess = m_process;
        m_process = nullptr;
        if (failedProcess) {
            failedProcess->deleteLater();
        }
        emit failed(m_generation, tr("FFprobe could not be started."));
    }
}

void FrameIndexer::consumeLine(const QByteArray& line)
{
    std::optional<double> bestEffort;
    std::optional<double> presentation;
    const QList<QByteArray> fields = line.split(',');
    const bool keyframe = !fields.isEmpty() && fields.at(0) == "1";
    if (fields.count() > 1) {
        presentation = parseNumber(fields.at(1));
    }
    if (fields.count() > 2) {
        bestEffort = parseNumber(fields.at(2));
    }

    FrameEntry entry;
    entry.keyframe = keyframe;
    entry.exactTimestamp = bestEffort.has_value() || presentation.has_value();
    if (bestEffort) {
        entry.timestamp = *bestEffort;
    } else if (presentation) {
        entry.timestamp = *presentation;
    } else if (!m_workingIndex.isEmpty()) {
        const FrameEntry& previous = m_workingIndex.at(m_workingIndex.count() - 1);
        double fallbackDuration = previous.duration.value_or(0.000001);
        if (m_workingIndex.count() > 1) {
            fallbackDuration = std::max(
                0.000001,
                previous.timestamp
                    - m_workingIndex.at(m_workingIndex.count() - 2).timestamp);
        }
        entry.timestamp = previous.timestamp + fallbackDuration;
    }

    if (!m_workingIndex.isEmpty()) {
        const double previous = m_workingIndex.at(m_workingIndex.count() - 1).timestamp;
        if (entry.timestamp < previous) {
            qCWarning(frameIndexLog) << "Clamping non-monotonic frame timestamp"
                                     << entry.timestamp << "after" << previous;
            entry.timestamp = previous;
            entry.exactTimestamp = false;
        }
    }
    m_workingIndex.append(entry);
    reportProgress(entry.timestamp);
}

void FrameIndexer::completeFromCache(const FrameIndex& index, quint64 generation)
{
    if (generation != m_generation || m_process) {
        return;
    }
    emit progressChanged(generation, 1.0);
    qCInfo(frameIndexLog) << "Frame index cache hit:" << index.count() << "frames";
    emit finished(generation, index, true);
}

void FrameIndexer::reportProgress(double timestamp)
{
    if (!std::isfinite(timestamp)) {
        return;
    }
    if (!m_firstTimestamp) {
        m_firstTimestamp = timestamp;
    }
    if (m_mediaDuration <= 0.0) {
        return;
    }
    const double elapsed = std::max(0.0, timestamp - *m_firstTimestamp);
    const double progress = std::clamp(elapsed / m_mediaDuration, 0.0, 0.99);
    if (m_lastProgress >= 0.0 && progress - m_lastProgress < 0.01
        && m_progressTimer.elapsed() < 250) {
        return;
    }
    m_lastProgress = progress;
    m_progressTimer.restart();
    emit progressChanged(m_generation, progress);
}

std::optional<double> FrameIndexer::parseNumber(const QByteArray& value)
{
    if (value.isEmpty() || value == "N/A") {
        return std::nullopt;
    }
    bool valid = false;
    const double number = value.toDouble(&valid);
    if (!valid || !std::isfinite(number)) {
        return std::nullopt;
    }
    return number;
}

} // namespace frameviewer
