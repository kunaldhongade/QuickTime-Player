#include "frames/FramePositionResolver.hpp"

#include <algorithm>
#include <cmath>

namespace frameviewer {

qsizetype FramePositionResolver::resolve(const FrameIndex& index,
                                         double playbackTime,
                                         bool endOfFile)
{
    if (index.isEmpty()) {
        return -1;
    }
    if (endOfFile || !std::isfinite(playbackTime)) {
        return index.count() - 1;
    }

    const auto& entries = index.entries();
    const auto iterator = std::upper_bound(
        entries.cbegin(), entries.cend(), playbackTime + 0.000001,
        [](double time, const FrameEntry& entry) { return time < entry.timestamp; });
    if (iterator == entries.cbegin()) {
        return 0;
    }
    return std::clamp<qsizetype>(std::distance(entries.cbegin(), iterator) - 1,
                                  0,
                                  entries.count() - 1);
}

qsizetype FramePositionResolver::clampTarget(const FrameIndex& index, qint64 target)
{
    if (index.isEmpty()) {
        return -1;
    }
    return std::clamp<qint64>(target, 0, index.count() - 1);
}

} // namespace frameviewer
