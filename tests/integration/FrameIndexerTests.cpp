#include "frames/FrameIndexer.hpp"
#include "frames/FrameRangeExporter.hpp"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

using namespace frameviewer;

class FrameIndexerTests final : public QObject {
    Q_OBJECT

private slots:
    void indexesExactlyFortyEightFrames();
    void staleGenerationCannotWin();
    void exportsInclusiveFrameRange();

private:
    static bool createFixture(const QString& path, int frames);
};

void FrameIndexerTests::indexesExactlyFortyEightFrames()
{
    if (QStandardPaths::findExecutable(QStringLiteral("ffmpeg")).isEmpty()
        || QStandardPaths::findExecutable(QStringLiteral("ffprobe")).isEmpty()) {
        QSKIP("FFmpeg and FFprobe are required for this integration test.");
    }
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("48-frames.mp4"));
    QVERIFY(createFixture(path, 48));

    FrameIndexer indexer;
    QSignalSpy finished(&indexer, &FrameIndexer::finished);
    QSignalSpy failed(&indexer, &FrameIndexer::failed);
    QSignalSpy progress(&indexer, &FrameIndexer::progressChanged);
    QSignalSpy frameCount(&indexer, &FrameIndexer::frameCountAvailable);
    indexer.start(path);
    indexer.setMediaDuration(2.0);
    if (frameCount.isEmpty()) {
        QVERIFY(frameCount.wait(1000));
    }
    QCOMPARE(frameCount.first().at(1).toLongLong(), 48);
    QVERIFY(frameCount.first().at(2).toBool());
    QVERIFY(finished.wait(20000));
    QCOMPARE(failed.count(), 0);
    QVERIFY(!progress.isEmpty());
    QCOMPARE(progress.last().at(1).toDouble(), 1.0);
    const QList<QVariant> result = finished.takeFirst();
    const FrameIndex index = qvariant_cast<FrameIndex>(result.at(1));
    QCOMPARE(index.count(), 48);
    QVERIFY(index.isValid());
    QVERIFY(index.at(0).keyframe);
}

void FrameIndexerTests::staleGenerationCannotWin()
{
    if (QStandardPaths::findExecutable(QStringLiteral("ffmpeg")).isEmpty()
        || QStandardPaths::findExecutable(QStringLiteral("ffprobe")).isEmpty()) {
        QSKIP("FFmpeg and FFprobe are required for this integration test.");
    }
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString first = directory.filePath(QStringLiteral("first.mp4"));
    const QString second = directory.filePath(QStringLiteral("second.mp4"));
    QVERIFY(createFixture(first, 60));
    QVERIFY(createFixture(second, 12));

    FrameIndexer indexer;
    QSignalSpy finished(&indexer, &FrameIndexer::finished);
    indexer.start(first);
    const quint64 firstGeneration = indexer.generation();
    indexer.start(second);
    const quint64 secondGeneration = indexer.generation();
    QVERIFY(secondGeneration > firstGeneration);
    QVERIFY(finished.wait(20000));
    const QList<QVariant> result = finished.takeLast();
    QCOMPARE(result.at(0).toULongLong(), secondGeneration);
    QCOMPARE(qvariant_cast<FrameIndex>(result.at(1)).count(), 12);
}

void FrameIndexerTests::exportsInclusiveFrameRange()
{
    if (QStandardPaths::findExecutable(QStringLiteral("ffmpeg")).isEmpty()) {
        QSKIP("FFmpeg is required for this integration test.");
    }
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("range.mp4"));
    QVERIFY(createFixture(path, 12));

    FrameRangeExporter exporter;
    QSignalSpy finished(&exporter, &FrameRangeExporter::finished);
    QSignalSpy failed(&exporter, &FrameRangeExporter::failed);
    exporter.start(path, 3, 6);
    QVERIFY(finished.wait(20000));
    QCOMPARE(failed.count(), 0);

    const QString outputDirectory = finished.takeFirst().at(0).toString();
    QCOMPARE(QFileInfo(outputDirectory).fileName(), QStringLiteral("range"));
    const QStringList frames =
        QDir(outputDirectory)
            .entryList({QStringLiteral("range-frame-*.png")}, QDir::Files, QDir::Name);
    QCOMPARE(frames.count(), 4);
    QCOMPARE(frames.first(), QStringLiteral("range-frame-000003.png"));
    QCOMPARE(frames.last(), QStringLiteral("range-frame-000006.png"));
}

bool FrameIndexerTests::createFixture(const QString& path, int frames)
{
    QProcess ffmpeg;
    ffmpeg.start(
        QStandardPaths::findExecutable(QStringLiteral("ffmpeg")),
        {QStringLiteral("-v"),
         QStringLiteral("error"),
         QStringLiteral("-f"),
         QStringLiteral("lavfi"),
         QStringLiteral("-i"),
         QStringLiteral("testsrc=size=160x90:rate=24"),
         QStringLiteral("-frames:v"),
         QString::number(frames),
         QStringLiteral("-c:v"),
         QStringLiteral("mpeg4"),
         QStringLiteral("-q:v"),
         QStringLiteral("5"),
         QStringLiteral("-an"),
         QStringLiteral("-y"),
         path});
    return ffmpeg.waitForFinished(20000) && ffmpeg.exitStatus() == QProcess::NormalExit
        && ffmpeg.exitCode() == 0;
}

QTEST_GUILESS_MAIN(FrameIndexerTests)
#include "FrameIndexerTests.moc"
