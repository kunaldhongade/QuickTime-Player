#include "frames/FrameIndexer.hpp"

#include <QFileInfo>
#include <QLoggingCategory>
#include <QStandardPaths>
#include <QTimer>

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

    emit started(m_generation);

    FrameIndex cached;
    if (m_cache.load(m_mediaPath, cached)) {
        const quint64 requestedGeneration = m_generation;
        QTimer::singleShot(0, this, [this, cached, requestedGeneration] {
            completeFromCache(cached, requestedGeneration);
        });
        return;
    }

    const QString ffprobe = QStandardPaths::findExecutable(QStringLiteral("ffprobe"));
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
        QStringLiteral("-select_streams"),
        QStringLiteral("V:0"),
        QStringLiteral("-show_frames"),
        QStringLiteral("-show_entries"),
        QStringLiteral(
            "frame=best_effort_timestamp_time,pts_time,pkt_duration_time,key_frame"),
        QStringLiteral("-of"),
        QStringLiteral("compact=p=0:nk=0"),
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

void FrameIndexer::consumeOutput()
{
    if (!m_process) {
        return;
    }
    m_pendingOutput += m_process->readAllStandardOutput();
    qsizetype newline = -1;
    while ((newline = m_pendingOutput.indexOf('\n')) >= 0) {
        const QByteArray line = m_pendingOutput.left(newline).trimmed();
        m_pendingOutput.remove(0, newline + 1);
        if (!line.isEmpty()) {
            consumeLine(line);
        }
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
    std::optional<double> duration;
    bool keyframe = false;

    for (const QByteArray& field : line.split('|')) {
        const qsizetype equals = field.indexOf('=');
        if (equals < 0) {
            continue;
        }
        const QByteArray key = field.left(equals);
        const QByteArray value = field.mid(equals + 1);
        if (key == "best_effort_timestamp_time") {
            bestEffort = parseNumber(value);
        } else if (key == "pts_time") {
            presentation = parseNumber(value);
        } else if (key == "pkt_duration_time") {
            duration = parseNumber(value);
        } else if (key == "key_frame") {
            keyframe = value == "1";
        }
    }

    FrameEntry entry;
    entry.keyframe = keyframe;
    entry.duration = duration;
    entry.exactTimestamp = bestEffort.has_value() || presentation.has_value();
    if (bestEffort) {
        entry.timestamp = *bestEffort;
    } else if (presentation) {
        entry.timestamp = *presentation;
    } else if (!m_workingIndex.isEmpty()) {
        const FrameEntry& previous = m_workingIndex.at(m_workingIndex.count() - 1);
        entry.timestamp = previous.timestamp + previous.duration.value_or(0.000001);
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
}

void FrameIndexer::completeFromCache(const FrameIndex& index, quint64 generation)
{
    if (generation != m_generation || m_process) {
        return;
    }
    qCInfo(frameIndexLog) << "Frame index cache hit:" << index.count() << "frames";
    emit finished(generation, index, true);
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
