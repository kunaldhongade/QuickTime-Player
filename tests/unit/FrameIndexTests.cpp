#include "files/MediaDirectoryNavigator.hpp"
#include "frames/FrameIndex.hpp"
#include "frames/FrameIndexCache.hpp"
#include "frames/FramePositionResolver.hpp"
#include "input/ArrowKeyGesture.hpp"

#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

using namespace frameviewer;

class FrameIndexTests final : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void oneBasedDisplayAndBoundaries();
    void resolvesVariableFrameTimestamps();
    void duplicateTimestampsResolveStably();
    void nonZeroAndNegativeStarts();
    void oneFrameVideo();
    void cacheRoundTripAndInvalidation();
    void navigatesSupportedVideosInNaturalOrder();
    void arrowTapRequestsOneStep();
    void arrowHoldStartsAndStopsShuttle();
    void arrowAutoRepeatIsIgnored();
    void cancellingActiveArrowHoldStopsShuttle();

private:
    static FrameIndex makeIndex(std::initializer_list<double> timestamps);
};

void FrameIndexTests::initTestCase()
{
    QCoreApplication::setOrganizationName(QStringLiteral("FrameViewerTests"));
    QCoreApplication::setApplicationName(QStringLiteral("FrameViewerTests"));
    QStandardPaths::setTestModeEnabled(true);
}

void FrameIndexTests::oneBasedDisplayAndBoundaries()
{
    const FrameIndex index = makeIndex({0.0, 0.04, 0.08});
    QCOMPARE(index.displayFrame(0), 1);
    QCOMPARE(index.displayFrame(2), 3);
    QCOMPARE(FramePositionResolver::clampTarget(index, -4), 0);
    QCOMPARE(FramePositionResolver::clampTarget(index, 900), 2);
}

void FrameIndexTests::resolvesVariableFrameTimestamps()
{
    const FrameIndex index = makeIndex({0.0, 0.041, 0.110, 0.145, 0.300});
    QCOMPARE(FramePositionResolver::resolve(index, 0.0), 0);
    QCOMPARE(FramePositionResolver::resolve(index, 0.109), 1);
    QCOMPARE(FramePositionResolver::resolve(index, 0.110), 2);
    QCOMPARE(FramePositionResolver::resolve(index, 0.299), 3);
    QCOMPARE(FramePositionResolver::resolve(index, 10.0), 4);
    QCOMPARE(FramePositionResolver::resolve(index, 0.0, true), 4);
}

void FrameIndexTests::duplicateTimestampsResolveStably()
{
    const FrameIndex index = makeIndex({0.0, 0.04, 0.04, 0.08});
    QCOMPARE(FramePositionResolver::resolve(index, 0.04), 2);
}

void FrameIndexTests::nonZeroAndNegativeStarts()
{
    const FrameIndex positive = makeIndex({5.0, 5.04, 5.08});
    QCOMPARE(FramePositionResolver::resolve(positive, 4.0), 0);
    QCOMPARE(FramePositionResolver::resolve(positive, 5.05), 1);

    const FrameIndex negative = makeIndex({-0.08, -0.04, 0.0});
    QCOMPARE(FramePositionResolver::resolve(negative, -1.0), 0);
    QCOMPARE(FramePositionResolver::resolve(negative, -0.02), 1);
}

void FrameIndexTests::oneFrameVideo()
{
    const FrameIndex index = makeIndex({0.0});
    QCOMPARE(index.count(), 1);
    QCOMPARE(index.displayFrame(0), 1);
    QCOMPARE(FramePositionResolver::resolve(index, 99.0), 0);
}

void FrameIndexTests::cacheRoundTripAndInvalidation()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString mediaPath = directory.filePath(QStringLiteral("sample.bin"));
    QFile media(mediaPath);
    QVERIFY(media.open(QIODevice::WriteOnly));
    QCOMPARE(media.write("media-a"), 7);
    media.close();

    const FrameIndex original = makeIndex({0.0, 0.04, 0.08});
    FrameIndexCache cache;
    QVERIFY(cache.save(mediaPath, original));

    FrameIndex loaded;
    QVERIFY(cache.load(mediaPath, loaded));
    QCOMPARE(loaded.count(), original.count());
    QVERIFY(loaded.entries() == original.entries());

    QVERIFY(media.open(QIODevice::Append));
    QCOMPARE(media.write("changed"), 7);
    media.close();
    FrameIndex stale;
    QVERIFY(!cache.load(mediaPath, stale));
}

void FrameIndexTests::navigatesSupportedVideosInNaturalOrder()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QStringList names{
        QStringLiteral("camera.mxf"),
        QStringLiteral("clip1.mp4"),
        QStringLiteral("clip2.MOV"),
        QStringLiteral("clip10.mkv"),
        QStringLiteral("legacy.WMV"),
        QStringLiteral("stream.hevc"),
        QStringLiteral("notes.txt"),
    };
    for (const QString& name : names) {
        QFile file(directory.filePath(name));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("fixture"), 7);
    }

    MediaDirectoryNavigator navigator;
    navigator.setCurrentFile(directory.filePath(QStringLiteral("clip2.MOV")));
    QCOMPARE(navigator.files().count(), 6);
    QCOMPARE(QFileInfo(navigator.previousPath()).fileName(), QStringLiteral("clip1.mp4"));
    QCOMPARE(QFileInfo(navigator.nextPath()).fileName(), QStringLiteral("clip10.mkv"));
    QVERIFY(navigator.canOpenPrevious());
    QVERIFY(navigator.canOpenNext());
    QVERIFY(!MediaDirectoryNavigator::isSupportedVideoFile(
        directory.filePath(QStringLiteral("notes.txt"))));
    QVERIFY(MediaDirectoryNavigator::isSupportedVideoFile(
        directory.filePath(QStringLiteral("camera.mxf"))));
    QVERIFY(MediaDirectoryNavigator::isSupportedVideoFile(
        directory.filePath(QStringLiteral("legacy.WMV"))));
    QVERIFY(MediaDirectoryNavigator::isSupportedVideoFile(
        directory.filePath(QStringLiteral("stream.hevc"))));
    QVERIFY(MediaDirectoryNavigator::supportedFilePatterns().contains(
        QStringLiteral("*.webm")));
    QVERIFY(MediaDirectoryNavigator::supportedFilePatterns().contains(
        QStringLiteral("*.mxf")));
}

void FrameIndexTests::arrowTapRequestsOneStep()
{
    ArrowKeyGesture gesture;
    QSignalSpy tapSpy(&gesture, &ArrowKeyGesture::tapRequested);
    QSignalSpy holdSpy(&gesture, &ArrowKeyGesture::holdStarted);

    gesture.press(1, false);
    QVERIFY(gesture.isTracking());
    gesture.release(1, false);

    QCOMPARE(tapSpy.count(), 1);
    QCOMPARE(tapSpy.constFirst().constFirst().toInt(), 1);
    QCOMPARE(holdSpy.count(), 0);
    QVERIFY(!gesture.isTracking());
}

void FrameIndexTests::arrowHoldStartsAndStopsShuttle()
{
    ArrowKeyGesture gesture;
    gesture.setHoldThreshold(20);
    QSignalSpy tapSpy(&gesture, &ArrowKeyGesture::tapRequested);
    QSignalSpy startSpy(&gesture, &ArrowKeyGesture::holdStarted);
    QSignalSpy finishSpy(&gesture, &ArrowKeyGesture::holdFinished);

    gesture.press(-1, false);
    QTRY_COMPARE_WITH_TIMEOUT(startSpy.count(), 1, 250);
    QVERIFY(gesture.holdActive());
    QCOMPARE(startSpy.constFirst().constFirst().toInt(), -1);

    gesture.release(-1, false);
    QCOMPARE(finishSpy.count(), 1);
    QCOMPARE(finishSpy.constFirst().constFirst().toInt(), -1);
    QCOMPARE(tapSpy.count(), 0);
    QVERIFY(!gesture.isTracking());
    QVERIFY(!gesture.holdActive());
}

void FrameIndexTests::arrowAutoRepeatIsIgnored()
{
    ArrowKeyGesture gesture;
    QSignalSpy tapSpy(&gesture, &ArrowKeyGesture::tapRequested);
    QSignalSpy startSpy(&gesture, &ArrowKeyGesture::holdStarted);
    QSignalSpy finishSpy(&gesture, &ArrowKeyGesture::holdFinished);

    gesture.press(1, true);
    gesture.release(1, true);

    QCOMPARE(tapSpy.count(), 0);
    QCOMPARE(startSpy.count(), 0);
    QCOMPARE(finishSpy.count(), 0);
    QVERIFY(!gesture.isTracking());
}

void FrameIndexTests::cancellingActiveArrowHoldStopsShuttle()
{
    ArrowKeyGesture gesture;
    gesture.setHoldThreshold(20);
    QSignalSpy startSpy(&gesture, &ArrowKeyGesture::holdStarted);
    QSignalSpy finishSpy(&gesture, &ArrowKeyGesture::holdFinished);

    gesture.press(1, false);
    QTRY_COMPARE_WITH_TIMEOUT(startSpy.count(), 1, 250);
    gesture.cancel();

    QCOMPARE(finishSpy.count(), 1);
    QVERIFY(!gesture.isTracking());
    QVERIFY(!gesture.holdActive());
}

FrameIndex FrameIndexTests::makeIndex(std::initializer_list<double> timestamps)
{
    FrameIndex index;
    for (double timestamp : timestamps) {
        FrameEntry entry;
        entry.timestamp = timestamp;
        index.append(entry);
    }
    return index;
}

QTEST_GUILESS_MAIN(FrameIndexTests)
#include "FrameIndexTests.moc"
