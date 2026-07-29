#include "rendering/MpvVideoItem.hpp"

#include "playback/MpvEngine.hpp"

#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QQuickOpenGLUtils>

#include <mpv/render_gl.h>

#include <memory>

namespace frameviewer {
namespace {

void* resolveOpenGlProcedure(void*, const char* name)
{
    QOpenGLContext* context = QOpenGLContext::currentContext();
    if (!context) {
        return nullptr;
    }
    return reinterpret_cast<void*>(context->getProcAddress(QByteArray(name)));
}

class MpvVideoRenderer final : public QQuickFramebufferObject::Renderer {
public:
    ~MpvVideoRenderer() override
    {
        if (m_engine && m_contextCreated) {
            m_engine->releaseRenderContext();
        }
    }

    void synchronize(QQuickFramebufferObject* item) override
    {
        auto* videoItem = qobject_cast<MpvVideoItem*>(item);
        MpvEngine* nextEngine = videoItem ? videoItem->engine() : nullptr;
        if (nextEngine == m_engine) {
            return;
        }
        if (m_engine && m_contextCreated) {
            m_engine->releaseRenderContext();
        }
        m_engine = nextEngine;
        m_contextCreated = false;
    }

    QOpenGLFramebufferObject* createFramebufferObject(const QSize& size) override
    {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        return new QOpenGLFramebufferObject(size, format);
    }

    void render() override
    {
        QOpenGLFramebufferObject* target = framebufferObject();
        if (!target) {
            return;
        }

        if (m_engine && !m_contextCreated) {
            mpv_opengl_init_params initialization{
                &resolveOpenGlProcedure,
                nullptr,
            };
            m_contextCreated = m_engine->createRenderContext(&initialization);
        }

        if (m_engine && m_contextCreated) {
            m_engine->renderFrame(target->handle(), target->width(), target->height());
        } else if (QOpenGLContext::currentContext()) {
            QOpenGLFunctions* functions = QOpenGLContext::currentContext()->functions();
            functions->glBindFramebuffer(GL_FRAMEBUFFER, target->handle());
            functions->glViewport(0, 0, target->width(), target->height());
            functions->glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
            functions->glClear(GL_COLOR_BUFFER_BIT);
        }
        QQuickOpenGLUtils::resetOpenGLState();
    }

private:
    MpvEngine* m_engine = nullptr;
    bool m_contextCreated = false;
};

} // namespace

MpvVideoItem::MpvVideoItem(QQuickItem* parent)
    : QQuickFramebufferObject(parent)
{
    setMirrorVertically(false);
}

QQuickFramebufferObject::Renderer* MpvVideoItem::createRenderer() const
{
    return new MpvVideoRenderer();
}

MpvEngine* MpvVideoItem::engine() const
{
    return m_engine;
}

void MpvVideoItem::setEngine(MpvEngine* engine)
{
    if (m_engine == engine) {
        return;
    }
    if (m_renderConnection) {
        disconnect(m_renderConnection);
    }
    m_engine = engine;
    if (m_engine) {
        m_renderConnection = connect(m_engine,
                                     &MpvEngine::renderUpdateRequested,
                                     this,
                                     &MpvVideoItem::update,
                                     Qt::QueuedConnection);
    }
    emit engineChanged();
    update();
}

} // namespace frameviewer
