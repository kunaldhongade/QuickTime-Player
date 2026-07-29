#pragma once

#include "frames/FrameIndex.hpp"

#include <QObject>
#include <QQueue>
#include <QTimer>

namespace frameviewer {

class MpvEngine;

class PlaybackController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(qint64 currentFrame READ currentFrame NOTIFY currentFrameChanged)
    Q_PROPERTY(qint64 totalFrames READ totalFrames NOTIFY totalFramesChanged)
    Q_PROPERTY(bool exactFrameIndexAvailable READ exactFrameIndexAvailable
                   NOTIFY exactFrameIndexAvailableChanged)
    Q_PROPERTY(bool canStepBackward READ canStepBackward NOTIFY stepAvailabilityChanged)
    Q_PROPERTY(bool canStepForward READ canStepForward NOTIFY stepAvailabilityChanged)

public:
    explicit PlaybackController(MpvEngine* engine, QObject* parent = nullptr);

    [[nodiscard]] qint64 currentFrame() const;
    [[nodiscard]] qint64 totalFrames() const;
    [[nodiscard]] qsizetype currentZeroBasedFrame() const;
    [[nodiscard]] bool exactFrameIndexAvailable() const;
    [[nodiscard]] bool canStepBackward() const;
    [[nodiscard]] bool canStepForward() const;
    [[nodiscard]] const FrameIndex& frameIndex() const;

    void setFrameIndex(FrameIndex index);
    void clear();

    Q_INVOKABLE void stepBy(int frameCount);
    Q_INVOKABLE void seekToFrame(qint64 zeroBasedFrame);
    Q_INVOKABLE void seekToFirstFrame();
    Q_INVOKABLE void seekToLastFrame();

signals:
    void currentFrameChanged();
    void totalFramesChanged();
    void exactFrameIndexAvailableChanged();
    void stepAvailabilityChanged();
    void seekStarted();
    void seekSettled();
    void seekFailed(const QString& message);

private slots:
    void positionChanged();
    void playbackRestarted();
    void confirmationTimedOut();

private:
    void processQueue();
    void beginTarget(qsizetype target, bool useNativeStep, int direction);
    void confirmOrCorrect();
    void settleAt(qsizetype frame);
    [[nodiscard]] qsizetype resolvedEnginePosition() const;
    [[nodiscard]] double engineTimestampFor(qsizetype frame) const;
    void publishCurrent(qsizetype frame);

    MpvEngine* m_engine = nullptr;
    FrameIndex m_index;
    QQueue<int> m_pendingSteps;
    QTimer m_confirmationTimer;
    qsizetype m_currentFrame = -1;
    qsizetype m_expectedFrame = -1;
    bool m_busy = false;
    bool m_correctiveSeekIssued = false;
};

} // namespace frameviewer
