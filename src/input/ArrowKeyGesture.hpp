#pragma once

#include <QObject>
#include <QTimer>

namespace frameviewer {

class ArrowKeyGesture final : public QObject {
    Q_OBJECT

public:
    explicit ArrowKeyGesture(QObject* parent = nullptr);

    void press(int direction, bool autoRepeat);
    void release(int direction, bool autoRepeat);
    void cancel();
    void setHoldThreshold(int milliseconds);

    [[nodiscard]] bool isTracking() const;
    [[nodiscard]] bool holdActive() const;

signals:
    void tapRequested(int direction);
    void holdStarted(int direction);
    void holdFinished(int direction);

private:
    QTimer m_holdTimer;
    int m_direction = 0;
    bool m_holdActive = false;
};

} // namespace frameviewer
