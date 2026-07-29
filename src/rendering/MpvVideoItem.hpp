#pragma once

#include <QPointer>
#include <QQuickFramebufferObject>

namespace frameviewer {

class MpvEngine;

class MpvVideoItem : public QQuickFramebufferObject {
    Q_OBJECT
    Q_PROPERTY(frameviewer::MpvEngine* engine READ engine WRITE setEngine NOTIFY engineChanged)

public:
    explicit MpvVideoItem(QQuickItem* parent = nullptr);

    [[nodiscard]] Renderer* createRenderer() const override;
    [[nodiscard]] MpvEngine* engine() const;
    void setEngine(MpvEngine* engine);

signals:
    void engineChanged();

private:
    QPointer<MpvEngine> m_engine;
    QMetaObject::Connection m_renderConnection;
};

} // namespace frameviewer
