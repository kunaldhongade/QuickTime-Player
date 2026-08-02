#include "frames/FrameRangeExporter.hpp"

#include "platform/ExecutableLocator.hpp"

#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QRegularExpression>

#include <algorithm>

Q_LOGGING_CATEGORY(frameExportLog, "frameviewer.frames.exporter")

namespace frameviewer {

FrameRangeExporter::FrameRangeExporter(QObject* parent)
    : QObject(parent)
{
}

FrameRangeExporter::~FrameRangeExporter()
{
    cancel();
}

bool FrameRangeExporter::isRunning() const
{
    return m_process != nullptr;
}

QString FrameRangeExporter::outputDirectory() const
{
    return m_outputDirectory;
}

void FrameRangeExporter::start(const QString& mediaPath, qint64 firstFrame, qint64 lastFrame)
{
    if (isRunning()) {
        emit failed(tr("A frame export is already running."));
        return;
    }
    if (firstFrame < 1 || lastFrame < firstFrame) {
        emit failed(tr("Choose a valid start and end frame before exporting."));
        return;
    }

    const QString ffmpeg = locateExecutable(QStringLiteral("ffmpeg"));
    if (ffmpeg.isEmpty()) {
        emit failed(tr("FFmpeg was not found. Install FFmpeg or add it to PATH."));
        return;
    }

    m_outputDirectory = createOutputDirectory(mediaPath);
    if (m_outputDirectory.isEmpty()) {
        emit failed(tr("The output folder could not be created beside the video."));
        return;
    }

    m_totalFrames = lastFrame - firstFrame + 1;
    m_completedFrames = 0;
    m_pendingProgress.clear();
    m_standardError.clear();
    m_cancelling = false;
    m_outputPattern = QDir(m_outputDirectory)
                          .filePath(safeStem(mediaPath)
                                    + QStringLiteral("-frame-%06d.png"));

    m_process = new QProcess(this);
    connect(m_process,
            &QProcess::readyReadStandardOutput,
            this,
            &FrameRangeExporter::consumeProgress);
    connect(m_process, &QProcess::readyReadStandardError, this, [this] {
        if (!m_process) {
            return;
        }
        m_standardError += QString::fromUtf8(m_process->readAllStandardError());
        if (m_standardError.size() > 16'384) {
            m_standardError.remove(0, m_standardError.size() - 16'384);
        }
    });
    connect(m_process,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            &FrameRangeExporter::processFinished);
    connect(m_process,
            &QProcess::errorOccurred,
            this,
            &FrameRangeExporter::processError);

    const QString selection =
        QStringLiteral("select=between(n\\,%1\\,%2)").arg(firstFrame - 1).arg(lastFrame - 1);
    const QStringList arguments{
        QStringLiteral("-hide_banner"),
        QStringLiteral("-v"),
        QStringLiteral("error"),
        QStringLiteral("-nostdin"),
        QStringLiteral("-i"),
        QFileInfo(mediaPath).absoluteFilePath(),
        QStringLiteral("-map"),
        QStringLiteral("0:V:0"),
        QStringLiteral("-vf"),
        selection,
        QStringLiteral("-an"),
        QStringLiteral("-fps_mode"),
        QStringLiteral("passthrough"),
        QStringLiteral("-start_number"),
        QString::number(firstFrame),
        QStringLiteral("-frames:v"),
        QString::number(m_totalFrames),
        QStringLiteral("-progress"),
        QStringLiteral("pipe:1"),
        QStringLiteral("-nostats"),
        m_outputPattern,
    };

    qCInfo(frameExportLog) << "Exporting frames" << firstFrame << "through" << lastFrame
                           << "to" << m_outputDirectory;
    emit started(m_totalFrames, m_outputDirectory);
    emit progressChanged(0, m_totalFrames);
    m_process->start(ffmpeg, arguments, QIODevice::ReadOnly);
}

void FrameRangeExporter::cancel()
{
    if (!m_process) {
        return;
    }

    m_cancelling = true;
    disconnect(m_process, nullptr, this, nullptr);
    if (m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        if (!m_process->waitForFinished(500)) {
            m_process->kill();
            m_process->waitForFinished(1000);
        }
    }
    m_process->deleteLater();
    m_process = nullptr;
    emit cancelled();
}

void FrameRangeExporter::consumeProgress()
{
    if (!m_process) {
        return;
    }
    m_pendingProgress += m_process->readAllStandardOutput();
    qsizetype lineStart = 0;
    qsizetype newline = -1;
    while ((newline = m_pendingProgress.indexOf('\n', lineStart)) >= 0) {
        const QByteArray line =
            m_pendingProgress.mid(lineStart, newline - lineStart).trimmed();
        lineStart = newline + 1;
        if (!line.startsWith("frame=")) {
            continue;
        }
        bool valid = false;
        const qint64 frame = line.mid(6).trimmed().toLongLong(&valid);
        if (valid && frame > m_completedFrames) {
            m_completedFrames = std::min(frame, m_totalFrames);
            emit progressChanged(m_completedFrames, m_totalFrames);
        }
    }
    if (lineStart > 0) {
        m_pendingProgress.remove(0, lineStart);
    }
}

void FrameRangeExporter::processFinished(int exitCode, QProcess::ExitStatus status)
{
    if (!m_process) {
        return;
    }
    consumeProgress();

    QProcess* completedProcess = m_process;
    m_process = nullptr;
    completedProcess->deleteLater();

    if (m_cancelling) {
        return;
    }
    if (status != QProcess::NormalExit || exitCode != 0) {
        const QString details = m_standardError.trimmed();
        emit failed(details.isEmpty() ? tr("FFmpeg could not export the selected frames.")
                                      : details);
        return;
    }

    m_completedFrames = m_totalFrames;
    emit progressChanged(m_completedFrames, m_totalFrames);
    qCInfo(frameExportLog) << "Frame export complete:" << m_totalFrames << "frames";
    emit finished(m_outputDirectory, m_totalFrames);
}

void FrameRangeExporter::processError(QProcess::ProcessError error)
{
    if (m_cancelling || error == QProcess::Crashed) {
        return;
    }
    if (error != QProcess::FailedToStart) {
        return;
    }
    QProcess* failedProcess = m_process;
    m_process = nullptr;
    if (failedProcess) {
        failedProcess->deleteLater();
    }
    emit failed(tr("FFmpeg could not be started."));
}

QString FrameRangeExporter::createOutputDirectory(const QString& mediaPath)
{
    const QFileInfo media(mediaPath);
    QDir parent(media.absolutePath());
    const QString base = media.completeBaseName().isEmpty()
        ? QStringLiteral("frames")
        : media.completeBaseName();

    QString candidate = parent.filePath(base);
    int suffix = 2;
    while (QFileInfo::exists(candidate)
           && (!QFileInfo(candidate).isDir()
               || !QDir(candidate).entryList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty())) {
        candidate = parent.filePath(QStringLiteral("%1-%2").arg(base).arg(suffix++));
    }
    return parent.mkpath(QFileInfo(candidate).fileName()) ? candidate : QString{};
}

QString FrameRangeExporter::safeStem(const QString& mediaPath)
{
    QString stem = QFileInfo(mediaPath).completeBaseName();
    stem.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9._-]+")),
                 QStringLiteral("_"));
    return stem.isEmpty() ? QStringLiteral("video") : stem;
}

} // namespace frameviewer
