#include "frames/FrameIndex.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace frameviewer {

bool FrameIndex::isEmpty() const
{
    return m_entries.isEmpty();
}

qsizetype FrameIndex::count() const
{
    return m_entries.count();
}

const FrameEntry& FrameIndex::at(qsizetype index) const
{
    return m_entries.at(index);
}

const QVector<FrameEntry>& FrameIndex::entries() const
{
    return m_entries;
}

QVector<FrameEntry>& FrameIndex::entries()
{
    return m_entries;
}

double FrameIndex::timestampForFrame(qsizetype zeroBasedFrame) const
{
    if (m_entries.isEmpty()) {
        return 0.0;
    }
    return m_entries.at(std::clamp<qsizetype>(zeroBasedFrame, 0, m_entries.count() - 1)).timestamp;
}

qint64 FrameIndex::displayFrame(qsizetype zeroBasedFrame) const
{
    if (m_entries.isEmpty()) {
        return 0;
    }
    return std::clamp<qsizetype>(zeroBasedFrame + 1, 1, m_entries.count());
}

bool FrameIndex::isValid() const
{
    if (m_entries.isEmpty()) {
        return false;
    }

    double previous = -std::numeric_limits<double>::infinity();
    for (qsizetype i = 0; i < m_entries.count(); ++i) {
        const auto& entry = m_entries.at(i);
        if (entry.ordinal != i || !std::isfinite(entry.timestamp) || entry.timestamp < previous) {
            return false;
        }
        previous = entry.timestamp;
    }
    return true;
}

void FrameIndex::clear()
{
    m_entries.clear();
}

void FrameIndex::append(FrameEntry entry)
{
    entry.ordinal = m_entries.count();
    m_entries.append(std::move(entry));
}

} // namespace frameviewer
