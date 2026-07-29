#pragma once

#include "frames/FrameIndex.hpp"

namespace frameviewer {

class FramePositionResolver {
public:
    [[nodiscard]] static qsizetype resolve(const FrameIndex& index,
                                            double playbackTime,
                                            bool endOfFile = false);
    [[nodiscard]] static qsizetype clampTarget(const FrameIndex& index, qint64 target);
};

} // namespace frameviewer
