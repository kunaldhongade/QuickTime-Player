#pragma once

#include <QObject>

namespace frameviewer {

class PlayerState final {
    Q_GADGET

public:
    enum Value {
        Empty,
        Opening,
        Indexing,
        ReadyPaused,
        Playing,
        Seeking,
        Ended,
        Error
    };
    Q_ENUM(Value)
};

} // namespace frameviewer
