#include "frames/FrameIndex.hpp"
#include "frames/FrameIndexCache.hpp"
#include "frames/FramePositionResolver.hpp"

#include <QFile>
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
