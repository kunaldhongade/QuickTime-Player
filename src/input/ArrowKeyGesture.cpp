#include "input/ArrowKeyGesture.hpp"

#include <algorithm>
#include <cstdlib>

namespace frameviewer {

ArrowKeyGesture::ArrowKeyGesture(QObject* parent)
    : QObject(parent)
{
    m_holdTimer.setSingleShot(true);
    m_holdTimer.setInterval(320);
    connect(&m_holdTimer, &QTimer::timeout, this, [this] {
        if (m_direction == 0) {
            return;
        }
        m_holdActive = true;
        emit holdStarted(m_direction);
    });
}

void ArrowKeyGesture::press(int direction, bool autoRepeat)
{
    if (autoRepeat || std::abs(direction) != 1) {
        return;
    }
    if (m_direction == direction) {
        return;
    }
    if (m_direction != 0) {
        cancel();
    }

    m_direction = direction;
    m_holdActive = false;
    m_holdTimer.start();
}

void ArrowKeyGesture::release(int direction, bool autoRepeat)
{
    if (autoRepeat || direction != m_direction || m_direction == 0) {
        return;
    }

    const int releasedDirection = m_direction;
    m_holdTimer.stop();
    m_direction = 0;
    if (m_holdActive) {
        m_holdActive = false;
        emit holdFinished(releasedDirection);
    } else {
        emit tapRequested(releasedDirection);
    }
}

void ArrowKeyGesture::cancel()
{
    if (m_direction == 0) {
        return;
    }

    const int cancelledDirection = m_direction;
    m_holdTimer.stop();
    m_direction = 0;
    if (m_holdActive) {
        m_holdActive = false;
        emit holdFinished(cancelledDirection);
    }
}

void ArrowKeyGesture::setHoldThreshold(int milliseconds)
{
    m_holdTimer.setInterval(std::max(1, milliseconds));
}

bool ArrowKeyGesture::isTracking() const
{
    return m_direction != 0;
}

bool ArrowKeyGesture::holdActive() const
{
    return m_holdActive;
}

} // namespace frameviewer
