#pragma once

#include "frames/FrameIndex.hpp"

#include <QString>

namespace frameviewer {

class FrameIndexCache {
public:
    static constexpr quint32 SchemaVersion = 1;

    [[nodiscard]] QString cacheKey(const QString& mediaPath, int streamIndex = 0) const;
    [[nodiscard]] QString cachePath(const QString& mediaPath, int streamIndex = 0) const;
    [[nodiscard]] bool load(const QString& mediaPath,
                            FrameIndex& destination,
                            int streamIndex = 0) const;
    [[nodiscard]] bool save(const QString& mediaPath,
                            const FrameIndex& index,
                            int streamIndex = 0) const;
};

} // namespace frameviewer
