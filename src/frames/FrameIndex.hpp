#pragma once

#include <QMetaType>
#include <QString>
#include <QVector>

#include <optional>

namespace frameviewer {

struct FrameEntry {
    qint64 ordinal = 0;
    double timestamp = 0.0;
    std::optional<double> duration;
    bool keyframe = false;
    bool exactTimestamp = true;

    bool operator==(const FrameEntry&) const = default;
};

class FrameIndex {
public:
    [[nodiscard]] bool isEmpty() const;
    [[nodiscard]] qsizetype count() const;
    [[nodiscard]] const FrameEntry& at(qsizetype index) const;
    [[nodiscard]] const QVector<FrameEntry>& entries() const;
    [[nodiscard]] QVector<FrameEntry>& entries();
    [[nodiscard]] double timestampForFrame(qsizetype zeroBasedFrame) const;
    [[nodiscard]] qint64 displayFrame(qsizetype zeroBasedFrame) const;
    [[nodiscard]] bool isValid() const;

    void clear();
    void append(FrameEntry entry);

private:
    QVector<FrameEntry> m_entries;
};

} // namespace frameviewer

Q_DECLARE_METATYPE(frameviewer::FrameEntry)
Q_DECLARE_METATYPE(frameviewer::FrameIndex)
