import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    required property var controller
    property bool temporarilyVisible: true
    property bool pointerInside: panelHover.hovered

    implicitHeight: 140
    opacity: controller.isPaused || temporarilyVisible || pointerInside ? 1 : 0
    enabled: opacity > 0.01

    function reveal() {
        temporarilyVisible = true
        hideTimer.restart()
    }

    Behavior on opacity {
        NumberAnimation {
            duration: 180
            easing.type: Easing.OutCubic
        }
    }

    Timer {
        id: hideTimer
        interval: 2000
        repeat: false
        running: controller.isPlaying
        onTriggered: {
            if (controller.isPlaying && !root.pointerInside)
                root.temporarilyVisible = false
        }
    }

    Connections {
        target: root.controller

        function onUserActivity() {
            root.reveal()
        }

        function onStateChanged() {
            root.reveal()
        }
    }

    Rectangle {
        id: shadow
        anchors.fill: panel
        anchors.margins: -5
        radius: panel.radius + 5
        color: "#4d000000"
        opacity: 0.7
    }

    Rectangle {
        id: panel
        anchors.fill: parent
        radius: 19
        color: "#d91c1d1f"
        border.width: 1
        border.color: "#33ffffff"

        HoverHandler {
            id: panelHover
            onHoveredChanged: {
                if (hovered)
                    root.reveal()
                else if (root.controller.isPlaying)
                    hideTimer.restart()
            }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: 22
            anchors.rightMargin: 22
            anchors.topMargin: 15
            anchors.bottomMargin: 16
            spacing: 8

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 8

                PlayerButton {
                    iconSource: Qt.resolvedUrl("../assets/icons/backward.svg")
                    accessibleName: qsTr("Previous frame")
                    enabled: root.controller.canStepBackward
                    onClicked: root.controller.stepFrames(-1)
                }

                PlayerButton {
                    prominent: true
                    iconSource: root.controller.isPlaying
                                ? Qt.resolvedUrl("../assets/icons/pause.svg")
                                : Qt.resolvedUrl("../assets/icons/play.svg")
                    accessibleName: root.controller.isPlaying ? qsTr("Pause") : qsTr("Play")
                    enabled: root.controller.hasMedia
                    onClicked: root.controller.togglePlayback()
                }

                PlayerButton {
                    iconSource: Qt.resolvedUrl("../assets/icons/forward.svg")
                    accessibleName: qsTr("Next frame")
                    enabled: root.controller.canStepForward
                    onClicked: root.controller.stepFrames(1)
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Label {
                    Layout.preferredWidth: 58
                    text: root.controller.exactFrameIndexAvailable
                          ? qsTr("%1 F").arg(root.controller.currentFrame)
                          : qsTr("— F")
                    color: "#f2f2f2"
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    horizontalAlignment: Text.AlignLeft
                }

                FrameSeekBar {
                    Layout.fillWidth: true
                    controller: root.controller
                }

                Label {
                    Layout.preferredWidth: 66
                    text: root.controller.exactFrameIndexAvailable
                          ? qsTr("%1 F").arg(root.controller.totalFrames)
                          : qsTr("— F")
                    color: "#f2f2f2"
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    horizontalAlignment: Text.AlignRight
                }
            }

            Label {
                Layout.alignment: Qt.AlignHCenter
                visible: root.controller.isIndexing
                text: qsTr("Indexing frames…")
                color: "#bfc0c2"
                font.pixelSize: 11
            }
        }
    }
}
