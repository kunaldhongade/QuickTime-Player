import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import FrameViewer.Native

ApplicationWindow {
    id: window

    width: 1100
    height: 700
    minimumWidth: 640
    minimumHeight: 400
    visible: true
    color: Theme.canvas
    title: appController.filename.length > 0 ? appController.filename : qsTr("FrameViewer")

    FileDialog {
        id: openDialog
        title: qsTr("Open Video")
        fileMode: FileDialog.OpenFile
        nameFilters: [
            appController.videoFileFilter,
            qsTr("All files (*)")
        ]
        onAccepted: appController.openFile(selectedFile)
    }

    Connections {
        target: appController

        function onOpenDialogRequested() {
            openDialog.open()
        }

        function onFullscreenRequested(fullscreen) {
            if (fullscreen)
                window.showFullScreen()
            else
                window.showNormal()
            appController.fullscreen = fullscreen
        }

        function onToastRequested(message) {
            toast.show(message)
        }
    }

    onVisibilityChanged: appController.fullscreen = window.visibility === Window.FullScreen

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: Theme.canvasTop }
            GradientStop { position: 1; color: Theme.canvasBottom }
        }
    }

    VideoViewport {
        id: viewport
        anchors.fill: parent
        engine: appController.engine
        visible: appController.hasMedia || appController.state === 1
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.NoButton
        hoverEnabled: true
        onPositionChanged: {
            appController.reportUserActivity()
            playbackOverlay.reveal()
        }
    }

    DropArea {
        anchors.fill: parent
        keys: ["text/uri-list"]
        onEntered: function(drag) {
            if (drag.hasUrls)
                drag.acceptProposedAction()
        }
        onDropped: function(drop) {
            if (drop.hasUrls && drop.urls.length > 0) {
                appController.openFile(drop.urls[0])
                drop.acceptProposedAction()
            }
        }
    }

    EmptyState {
        anchors.centerIn: parent
        visible: !appController.hasMedia
                 && appController.state !== 1
                 && appController.errorMessage.length === 0
        onOpenRequested: openDialog.open()
    }

    ErrorView {
        anchors.centerIn: parent
        width: Math.min(520, parent.width - 64)
        visible: !appController.hasMedia && appController.errorMessage.length > 0
        filename: appController.filename
        message: appController.errorMessage
        onOpenRequested: openDialog.open()
    }

    Column {
        anchors.centerIn: parent
        spacing: 12
        visible: appController.state === 1

        BusyIndicator {
            anchors.horizontalCenter: parent.horizontalCenter
            running: parent.visible
            palette.light: "white"
            palette.mid: "#909090"
        }

        Label {
            text: qsTr("Opening %1…").arg(appController.filename)
            color: appController.hasMedia ? Theme.mediaText : Theme.textPrimary
            font.pixelSize: 15
        }
    }

    PlaybackOverlay {
        id: playbackOverlay
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: window.visibility === Window.FullScreen ? 36 : 28
        width: Math.min(700, parent.width - 32)
        controller: appController
        visible: appController.hasMedia && appController.controlsVisible
    }

    Toast {
        id: toast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: playbackOverlay.visible ? playbackOverlay.top : parent.bottom
        anchors.bottomMargin: 18
    }

    MacSetup {
        controller: appController
    }
}
