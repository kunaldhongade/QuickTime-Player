#include "playback/MpvEngine.hpp"

#include <QFileInfo>
#include <QLoggingCategory>
#include <QMetaObject>

#include <mpv/client.h>
#include <mpv/render.h>
#include <mpv/render_gl.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

Q_LOGGING_CATEGORY(mpvLog, "frameviewer.playback.mpv")

namespace frameviewer {
namespace {

void setOption(mpv_handle* handle, const char* name, const char* value)
{
    const int result = mpv_set_option_string(handle, name, value);
    if (result < 0) {
        qCWarning(mpvLog) << "Could not set mpv option" << name << mpv_error_string(result);
    }
}

} // namespace

MpvEngine::MpvEngine(QObject* parent)
    : QObject(parent)
{
    m_mpv = mpv_create();
    if (!m_mpv) {
        m_initializationError = tr("libmpv could not be created.");
        return;
    }

    const std::array options{
        std::pair{"vo", "libmpv"},
        std::pair{"terminal", "no"},
        std::pair{"input-default-bindings", "no"},
        std::pair{"input-vo-keyboard", "no"},
        std::pair{"osd-level", "0"},
        std::pair{"idle", "yes"},
        std::pair{"keep-open", "yes"},
        std::pair{"pause", "yes"},
        std::pair{"hwdec", "auto-safe"},
        std::pair{"interpolation", "no"},
        std::pair{"deinterlace", "no"},
        std::pair{"hr-seek", "yes"},
        std::pair{"hr-seek-framedrop", "no"},
    };
    for (const auto& [name, value] : options) {
        setOption(m_mpv, name, value);
    }

    const int result = mpv_initialize(m_mpv);
    if (result < 0) {
        m_initializationError =
            tr("libmpv initialization failed: %1").arg(QString::fromUtf8(mpv_error_string(result)));
        mpv_terminate_destroy(m_mpv);
        m_mpv = nullptr;
        return;
    }

    m_available = true;
    observeProperties();
    mpv_request_log_messages(m_mpv, "warn");
    mpv_set_wakeup_callback(m_mpv, &MpvEngine::wakeupCallback, this);
    qCInfo(mpvLog) << "libmpv client API" << mpv_client_api_version();
}

MpvEngine::~MpvEngine()
{
    shutdown();
}

bool MpvEngine::available() const
{
    return m_available;
}

bool MpvEngine::hasMedia() const
{
    return m_hasMedia;
}

bool MpvEngine::paused() const
{
    return m_paused;
}

double MpvEngine::timePosition() const
{
    return m_timePosition;
}

double MpvEngine::duration() const
{
    return m_duration;
}

double MpvEngine::volume() const
{
    return m_volume;
}

bool MpvEngine::muted() const
{
    return m_muted;
}

bool MpvEngine::seeking() const
{
    return m_seeking;
}

bool MpvEngine::endOfFile() const
{
    return m_endOfFile;
}

QString MpvEngine::filename() const
{
    return m_filename;
}

QString MpvEngine::initializationError() const
{
    return m_initializationError;
}

void MpvEngine::setPaused(bool paused)
{
    if (!m_mpv) {
        return;
    }
    setBooleanProperty("pause", paused);
}

void MpvEngine::setVolume(double volume)
{
    if (!m_mpv) {
        return;
    }
    setDoubleProperty("volume", std::clamp(volume, 0.0, 100.0));
}

void MpvEngine::setMuted(bool muted)
{
    if (!m_mpv || muted == m_muted) {
        return;
    }
    setBooleanProperty("mute", muted);
}

void MpvEngine::loadFile(const QString& path)
{
    if (!m_mpv || path.isEmpty()) {
        return;
    }
    qCInfo(mpvLog) << "Opening media" << QFileInfo(path).fileName();
    m_hasMedia = false;
    emit hasMediaChanged();
    m_endOfFile = false;
    emit endOfFileChanged();
    if (!m_renderReady.load()) {
        m_pendingLoadPath = path;
        qCInfo(mpvLog) << "Waiting for the Qt OpenGL surface before loading media";
        return;
    }
    m_pendingLoadPath.clear();
    issueCommand({QByteArrayLiteral("loadfile"), path.toUtf8(), QByteArrayLiteral("replace")});
}

void MpvEngine::togglePause()
{
    setPaused(!m_paused);
}

void MpvEngine::frameStep()
{
    issueCommand({QByteArrayLiteral("frame-step")});
}

void MpvEngine::frameBackStep()
{
    issueCommand({QByteArrayLiteral("frame-back-step")});
}

void MpvEngine::seekExact(double timestamp)
{
    issueCommand({QByteArrayLiteral("seek"),
                  QByteArray::number(timestamp, 'f', 9),
                  QByteArrayLiteral("absolute+exact")});
}

void MpvEngine::takeScreenshot(const QString& path)
{
    issueCommand(
        {QByteArrayLiteral("screenshot-to"), path.toUtf8(), QByteArrayLiteral("video")});
}

bool MpvEngine::createRenderContext(mpv_opengl_init_params* initialization)
{
    std::scoped_lock lock(m_renderMutex);
    if (!m_mpv || m_renderContext || m_shuttingDown.load()) {
        return m_renderContext != nullptr;
    }

    mpv_render_param parameters[]{
        {MPV_RENDER_PARAM_API_TYPE, const_cast<char*>(MPV_RENDER_API_TYPE_OPENGL)},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, initialization},
        {MPV_RENDER_PARAM_INVALID, nullptr},
    };
    const int result = mpv_render_context_create(&m_renderContext, m_mpv, parameters);
    if (result < 0) {
        qCCritical(mpvLog) << "Could not create mpv OpenGL render context:"
                           << mpv_error_string(result);
        return false;
    }
    mpv_render_context_set_update_callback(
        m_renderContext, &MpvEngine::renderUpdateCallback, this);
    m_renderReady.store(true);
    qCInfo(mpvLog) << "Created libmpv OpenGL render context";
    QMetaObject::invokeMethod(
        this,
        [this] {
            if (m_pendingLoadPath.isEmpty() || m_shuttingDown.load()) {
                return;
            }
            const QString path = std::exchange(m_pendingLoadPath, QString{});
            issueCommand(
                {QByteArrayLiteral("loadfile"), path.toUtf8(), QByteArrayLiteral("replace")});
        },
        Qt::QueuedConnection);
    return true;
}

void MpvEngine::releaseRenderContext()
{
    std::scoped_lock lock(m_renderMutex);
    if (!m_renderContext) {
        return;
    }
    mpv_render_context_set_update_callback(m_renderContext, nullptr, nullptr);
    mpv_render_context_free(m_renderContext);
    m_renderContext = nullptr;
    m_renderReady.store(false);
}

void MpvEngine::renderFrame(int framebuffer, int width, int height)
{
    std::scoped_lock lock(m_renderMutex);
    if (!m_renderContext || m_shuttingDown.load()) {
        return;
    }

    mpv_opengl_fbo framebufferDescription{framebuffer, width, height, 0};
    // Qt supplies an offscreen FBO here, not OpenGL's vertically inverted default
    // framebuffer. Asking libmpv to flip this target produces an upside-down texture.
    mpv_render_param parameters[]{
        {MPV_RENDER_PARAM_OPENGL_FBO, &framebufferDescription},
        {MPV_RENDER_PARAM_INVALID, nullptr},
    };
    mpv_render_context_update(m_renderContext);
    mpv_render_context_render(m_renderContext, parameters);
}

void MpvEngine::drainEvents()
{
    if (!m_mpv || m_shuttingDown.load()) {
        return;
    }

    while (true) {
        mpv_event* event = mpv_wait_event(m_mpv, 0.0);
        if (!event || event->event_id == MPV_EVENT_NONE) {
            break;
        }

        switch (event->event_id) {
        case MPV_EVENT_START_FILE:
            qCInfo(mpvLog) << "Playback start-file";
            emit fileStarted();
            break;
        case MPV_EVENT_FILE_LOADED:
            qCInfo(mpvLog) << "Playback file-loaded";
            if (!m_hasMedia) {
                m_hasMedia = true;
                emit hasMediaChanged();
            }
            emit fileLoaded();
            break;
        case MPV_EVENT_PLAYBACK_RESTART:
            emit playbackRestarted();
            break;
        case MPV_EVENT_END_FILE: {
            const auto* end = static_cast<mpv_event_end_file*>(event->data);
            emit playbackEnded(end ? end->reason : MPV_END_FILE_REASON_ERROR);
            if (end && end->error < 0) {
                emit errorOccurred(
                    tr("Playback failed: %1").arg(QString::fromUtf8(mpv_error_string(end->error))));
            }
            break;
        }
        case MPV_EVENT_PROPERTY_CHANGE: {
            const auto* property = static_cast<mpv_event_property*>(event->data);
            if (!property || !property->data) {
                break;
            }
            if (property->format == MPV_FORMAT_FLAG) {
                updateBooleanProperty(property->name,
                                      *static_cast<int*>(property->data) != 0);
            } else if (property->format == MPV_FORMAT_DOUBLE) {
                updateDoubleProperty(property->name, *static_cast<double*>(property->data));
            } else if (property->format == MPV_FORMAT_STRING) {
                const char* const value = *static_cast<char**>(property->data);
                updateStringProperty(property->name, QString::fromUtf8(value ? value : ""));
            }
            break;
        }
        case MPV_EVENT_COMMAND_REPLY:
            if (event->error < 0) {
                qCWarning(mpvLog) << "Async command failed:" << mpv_error_string(event->error);
            }
            emit commandFinished(event->reply_userdata, event->error);
            break;
        case MPV_EVENT_LOG_MESSAGE: {
            const auto* message = static_cast<mpv_event_log_message*>(event->data);
            if (message) {
                qCWarning(mpvLog).noquote()
                    << QStringLiteral("[%1] %2")
                           .arg(QString::fromUtf8(message->prefix),
                                QString::fromUtf8(message->text).trimmed());
                emit logMessage(QString::fromUtf8(message->prefix),
                                QString::fromUtf8(message->level),
                                QString::fromUtf8(message->text).trimmed());
            }
            break;
        }
        case MPV_EVENT_SHUTDOWN:
            m_available = false;
            break;
        default:
            break;
        }
    }
}

void MpvEngine::wakeupCallback(void* context)
{
    auto* engine = static_cast<MpvEngine*>(context);
    if (!engine || engine->m_shuttingDown.load()) {
        return;
    }
    QMetaObject::invokeMethod(engine, &MpvEngine::drainEvents, Qt::QueuedConnection);
}

void MpvEngine::renderUpdateCallback(void* context)
{
    auto* engine = static_cast<MpvEngine*>(context);
    if (!engine || engine->m_shuttingDown.load()) {
        return;
    }
    QMetaObject::invokeMethod(
        engine,
        [engine] {
            if (!engine->m_shuttingDown.load()) {
                emit engine->renderUpdateRequested();
            }
        },
        Qt::QueuedConnection);
}

void MpvEngine::observeProperties()
{
    struct Observation {
        const char* name;
        mpv_format format;
    };
    const std::array properties{
        Observation{"pause", MPV_FORMAT_FLAG},
        Observation{"time-pos", MPV_FORMAT_DOUBLE},
        Observation{"duration", MPV_FORMAT_DOUBLE},
        Observation{"volume", MPV_FORMAT_DOUBLE},
        Observation{"mute", MPV_FORMAT_FLAG},
        Observation{"seeking", MPV_FORMAT_FLAG},
        Observation{"eof-reached", MPV_FORMAT_FLAG},
        Observation{"filename", MPV_FORMAT_STRING},
    };
    quint64 identifier = 1;
    for (const auto& property : properties) {
        mpv_observe_property(m_mpv, identifier++, property.name, property.format);
    }
}

quint64 MpvEngine::issueCommand(const QList<QByteArray>& arguments)
{
    if (!m_mpv || arguments.isEmpty() || m_shuttingDown.load()) {
        return 0;
    }
    QList<const char*> command;
    command.reserve(arguments.count() + 1);
    for (const auto& argument : arguments) {
        command.append(argument.constData());
    }
    command.append(nullptr);
    const quint64 request = m_nextRequest.fetch_add(1);
    const int result = mpv_command_async(m_mpv, request, command.data());
    if (result < 0) {
        emit errorOccurred(
            tr("Playback command failed: %1").arg(QString::fromUtf8(mpv_error_string(result))));
        return 0;
    }
    return request;
}

void MpvEngine::setBooleanProperty(const char* name, bool value)
{
    int flag = value ? 1 : 0;
    const int result = mpv_set_property(m_mpv, name, MPV_FORMAT_FLAG, &flag);
    if (result < 0) {
        emit errorOccurred(
            tr("Could not change playback state: %1")
                .arg(QString::fromUtf8(mpv_error_string(result))));
    }
}

void MpvEngine::setDoubleProperty(const char* name, double value)
{
    const int result = mpv_set_property(m_mpv, name, MPV_FORMAT_DOUBLE, &value);
    if (result < 0) {
        emit errorOccurred(
            tr("Could not change playback setting: %1")
                .arg(QString::fromUtf8(mpv_error_string(result))));
    }
}

void MpvEngine::updateBooleanProperty(const char* name, bool value)
{
    const QByteArray property(name);
    if (property == "pause" && value != m_paused) {
        m_paused = value;
        emit pausedChanged();
    } else if (property == "mute" && value != m_muted) {
        m_muted = value;
        emit mutedChanged();
    } else if (property == "seeking" && value != m_seeking) {
        m_seeking = value;
        emit seekingChanged();
    } else if (property == "eof-reached" && value != m_endOfFile) {
        m_endOfFile = value;
        emit endOfFileChanged();
    }
}

void MpvEngine::updateDoubleProperty(const char* name, double value)
{
    const QByteArray property(name);
    if (property == "time-pos" && !qFuzzyCompare(value + 1.0, m_timePosition + 1.0)) {
        m_timePosition = value;
        emit timePositionChanged();
    } else if (property == "duration" && !qFuzzyCompare(value + 1.0, m_duration + 1.0)) {
        m_duration = value;
        emit durationChanged();
    } else if (property == "volume" && !qFuzzyCompare(value + 1.0, m_volume + 1.0)) {
        m_volume = value;
        emit volumeChanged();
    }
}

void MpvEngine::updateStringProperty(const char* name, const QString& value)
{
    if (QByteArray(name) == "filename" && value != m_filename) {
        m_filename = value;
        emit filenameChanged();
    }
}

void MpvEngine::shutdown()
{
    if (m_shuttingDown.exchange(true)) {
        return;
    }
    if (!m_mpv) {
        return;
    }
    mpv_set_wakeup_callback(m_mpv, nullptr, nullptr);
    releaseRenderContext();
    mpv_terminate_destroy(m_mpv);
    m_mpv = nullptr;
    m_available = false;
}

} // namespace frameviewer
