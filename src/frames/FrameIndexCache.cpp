#include "frames/FrameIndexCache.hpp"

#include <QCryptographicHash>
#include <QDataStream>
#include <QDir>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>

#include <cmath>

namespace frameviewer {
namespace {

constexpr quint32 CacheMagic = 0x46564958; // FVIX

QString canonicalOrAbsolute(const QFileInfo& information)
{
    const QString canonical = information.canonicalFilePath();
    return canonical.isEmpty() ? information.absoluteFilePath() : canonical;
}

} // namespace

QString FrameIndexCache::cacheKey(const QString& mediaPath, int streamIndex) const
{
    const QFileInfo information(mediaPath);
    const QString identity =
        QStringLiteral("%1|%2|%3|%4|%5")
            .arg(canonicalOrAbsolute(information))
            .arg(information.size())
            .arg(information.lastModified().toMSecsSinceEpoch())
            .arg(streamIndex)
            .arg(SchemaVersion);
    return QString::fromLatin1(
        QCryptographicHash::hash(identity.toUtf8(), QCryptographicHash::Sha256).toHex());
}

QString FrameIndexCache::cachePath(const QString& mediaPath, int streamIndex) const
{
    QDir directory(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    directory.mkpath(QStringLiteral("frame-indexes"));
    return directory.filePath(QStringLiteral("frame-indexes/%1.fvix")
                                  .arg(cacheKey(mediaPath, streamIndex)));
}

bool FrameIndexCache::load(const QString& mediaPath,
                           FrameIndex& destination,
                           int streamIndex) const
{
    QFile file(cachePath(mediaPath, streamIndex));
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QDataStream stream(&file);
    stream.setVersion(QDataStream::Qt_6_4);

    quint32 magic = 0;
    quint32 schema = 0;
    QString path;
    qint64 fileSize = 0;
    qint64 modified = 0;
    qint32 storedStream = -1;
    qint64 count = 0;
    stream >> magic >> schema >> path >> fileSize >> modified >> storedStream >> count;

    const QFileInfo information(mediaPath);
    if (magic != CacheMagic || schema != SchemaVersion
        || path != canonicalOrAbsolute(information) || fileSize != information.size()
        || modified != information.lastModified().toMSecsSinceEpoch()
        || storedStream != streamIndex || count <= 0 || count > 500'000'000) {
        return false;
    }

    FrameIndex loaded;
    loaded.entries().reserve(static_cast<qsizetype>(count));
    for (qint64 i = 0; i < count; ++i) {
        FrameEntry entry;
        bool hasDuration = false;
        double duration = 0.0;
        stream >> entry.ordinal >> entry.timestamp >> hasDuration >> duration >> entry.keyframe
            >> entry.exactTimestamp;
        if (hasDuration) {
            entry.duration = duration;
        }
        loaded.append(entry);
    }

    if (stream.status() != QDataStream::Ok || !loaded.isValid()) {
        return false;
    }
    destination = std::move(loaded);
    return true;
}

bool FrameIndexCache::save(const QString& mediaPath,
                           const FrameIndex& index,
                           int streamIndex) const
{
    if (!index.isValid()) {
        return false;
    }

    QSaveFile file(cachePath(mediaPath, streamIndex));
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    const QFileInfo information(mediaPath);
    QDataStream stream(&file);
    stream.setVersion(QDataStream::Qt_6_4);
    stream << CacheMagic << SchemaVersion << canonicalOrAbsolute(information)
           << information.size() << information.lastModified().toMSecsSinceEpoch()
           << static_cast<qint32>(streamIndex) << static_cast<qint64>(index.count());
    for (const auto& entry : index.entries()) {
        stream << entry.ordinal << entry.timestamp << entry.duration.has_value()
               << entry.duration.value_or(0.0) << entry.keyframe << entry.exactTimestamp;
    }
    return stream.status() == QDataStream::Ok && file.commit();
}

} // namespace frameviewer
