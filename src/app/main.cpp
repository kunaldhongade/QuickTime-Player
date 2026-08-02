#include "app/ApplicationController.hpp"
#include "platform/MacPlatformIntegration.hpp"
#include "rendering/MpvVideoItem.hpp"

#include <QGuiApplication>
#include <QIcon>
#include <QImage>
#include <QKeyEvent>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QTimer>

#include <clocale>

int main(int argc, char* argv[])
{
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QGuiApplication application(argc, argv);
    // Qt adopts the user's system locale on Unix, but libmpv requires the process-wide
    // numeric C locale when mpv_create() is called. Restore it after Qt initializes and
    // before ApplicationController constructs MpvEngine.
    std::setlocale(LC_NUMERIC, "C");
    QCoreApplication::setOrganizationName(QStringLiteral("FrameViewer"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("frameviewer.local"));
    QCoreApplication::setApplicationName(QStringLiteral("FrameViewer"));
    QCoreApplication::setApplicationVersion(QStringLiteral(FRAMEVIEWER_VERSION));
    application.setWindowIcon(
        QIcon(QStringLiteral(":/qt/qml/FrameViewer/assets/icons/icon.png")));
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    qmlRegisterType<frameviewer::MpvVideoItem>(
        "FrameViewer.Native", 1, 0, "MpvVideoItem");

    frameviewer::ApplicationController controller;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("appController"), &controller);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &application,
        [] { QCoreApplication::exit(EXIT_FAILURE); },
        Qt::QueuedConnection);
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/FrameViewer/qml/Main.qml")));
    if (engine.rootObjects().isEmpty()) {
        return EXIT_FAILURE;
    }

    auto* rootWindow = qobject_cast<QQuickWindow*>(engine.rootObjects().constFirst());
    frameviewer::macos::registerApplicationBundle();
    frameviewer::macos::configureWindow(rootWindow);

    if (application.arguments().count() > 1) {
        controller.openFile(QUrl::fromLocalFile(application.arguments().at(1)));
    }

    // A deterministic, opt-in capture hook lets CI inspect the native QML/OpenGL composition
    // without screen-recording permission. It is inert in normal application launches.
    const QString capturePath =
        qEnvironmentVariable("FRAMEVIEWER_CAPTURE_PATH");
    if (!capturePath.isEmpty()) {
        auto* window = rootWindow;
        if (qEnvironmentVariableIsSet("FRAMEVIEWER_TEST_RIGHT_KEY")) {
            QTimer::singleShot(1500, window, [window] {
                QKeyEvent press(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
                QCoreApplication::sendEvent(window, &press);
                QKeyEvent autoRepeat(QEvent::KeyPress,
                                     Qt::Key_Right,
                                     Qt::NoModifier,
                                     QString{},
                                     true,
                                     2);
                QCoreApplication::sendEvent(window, &autoRepeat);
            });
            QTimer::singleShot(1600, window, [window] {
                QKeyEvent release(QEvent::KeyRelease, Qt::Key_Right, Qt::NoModifier);
                QCoreApplication::sendEvent(window, &release);
            });
        }
        if (qEnvironmentVariableIsSet("FRAMEVIEWER_TEST_RIGHT_BURST")) {
            for (int step = 0; step < 12; ++step) {
                QTimer::singleShot(1200 + step * 25, window, [window] {
                    QKeyEvent press(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
                    QCoreApplication::sendEvent(window, &press);
                    QKeyEvent release(QEvent::KeyRelease, Qt::Key_Right, Qt::NoModifier);
                    QCoreApplication::sendEvent(window, &release);
                });
            }
        }
        if (qEnvironmentVariableIsSet("FRAMEVIEWER_TEST_LEFT_BURST")) {
            QTimer::singleShot(1000, window, [&controller] {
                controller.seekToFrame(35);
            });
            for (int step = 0; step < 12; ++step) {
                QTimer::singleShot(1800 + step * 25, window, [window] {
                    QKeyEvent press(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier);
                    QCoreApplication::sendEvent(window, &press);
                    QKeyEvent release(QEvent::KeyRelease, Qt::Key_Left, Qt::NoModifier);
                    QCoreApplication::sendEvent(window, &release);
                });
            }
        }
        if (qEnvironmentVariableIsSet("FRAMEVIEWER_TEST_HOLD_RIGHT_KEY")) {
            QTimer::singleShot(1200, window, [window] {
                QKeyEvent press(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
                QCoreApplication::sendEvent(window, &press);
            });
            QTimer::singleShot(2600, window, [window] {
                QKeyEvent release(QEvent::KeyRelease, Qt::Key_Right, Qt::NoModifier);
                QCoreApplication::sendEvent(window, &release);
            });
        }
        if (qEnvironmentVariableIsSet("FRAMEVIEWER_TEST_HOLD_LEFT_KEY")) {
            QTimer::singleShot(1000, window, [&controller] {
                controller.seekToFrame(35);
            });
            QTimer::singleShot(1800, window, [window] {
                QKeyEvent press(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier);
                QCoreApplication::sendEvent(window, &press);
            });
            QTimer::singleShot(3200, window, [window] {
                QKeyEvent release(QEvent::KeyRelease, Qt::Key_Left, Qt::NoModifier);
                QCoreApplication::sendEvent(window, &release);
            });
        }
        if (qEnvironmentVariableIsSet("FRAMEVIEWER_TEST_HIDE_CONTROLS")) {
            QTimer::singleShot(900, window, [window] {
                QKeyEvent press(QEvent::KeyPress, Qt::Key_H, Qt::NoModifier);
                QCoreApplication::sendEvent(window, &press);
            });
        }
        if (qEnvironmentVariableIsSet("FRAMEVIEWER_TEST_END_BOUNDARY")) {
            QTimer::singleShot(1500, window, [&controller] {
                controller.seekToLastFrame();
            });
            QTimer::singleShot(2500, window, [&controller] {
                controller.stepFrames(1);
            });
        }
        const int captureDelay =
            qEnvironmentVariableIntValue("FRAMEVIEWER_CAPTURE_DELAY_MS");
        QTimer::singleShot(captureDelay > 0 ? captureDelay : 2500,
                           window,
                           [window, capturePath, &controller] {
                               if (window) {
                                   window->grabWindow().save(capturePath);
                               }
                               qInfo() << "Captured frame" << controller.currentFrame() << "of"
                                       << controller.totalFrames() << "controls visible"
                                       << controller.controlsVisible();
                               if (qEnvironmentVariableIsSet("FRAMEVIEWER_CAPTURE_AND_EXIT")) {
                                   QCoreApplication::quit();
                               }
                           });
    }
    return application.exec();
}
