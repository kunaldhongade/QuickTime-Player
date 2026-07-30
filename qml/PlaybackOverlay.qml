import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    required property var controller
    property bool temporarilyVisible: true
    property bool pointerInside: panelHover.hovered

    implicitHeight: 226
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
        running: root.controller.isPlaying
        onTriggered: {
            if (root.controller.isPlaying && !root.pointerInside)
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
            anchors.topMargin: 12
            anchors.bottomMargin: 13
            spacing: 7

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                PanelButton {
                    text: qsTr("Previous Video")
                    enabled: root.controller.canOpenPreviousVideo
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Previous video in this folder (Ctrl+Left)")
                    onClicked: root.controller.openPreviousVideo()
                }

                Label {
                    Layout.fillWidth: true
                    text: root.controller.filename
                    color: "#d8d8da"
                    font.pixelSize: 12
                    font.weight: Font.Medium
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideMiddle
                }

                PanelButton {
                    text: qsTr("Next Video")
                    enabled: root.controller.canOpenNextVideo
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Next video in this folder (Ctrl+Right)")
                    onClicked: root.controller.openNextVideo()
                }
            }

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

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                PanelButton {
                    text: qsTr("Set Start")
                    enabled: root.controller.exactFrameIndexAvailable
                    onClicked: root.controller.markRangeStart()
                }

                Label {
                    Layout.fillWidth: true
                    text: root.controller.rangeStartFrame <= 0
                          ? qsTr("No frame range selected")
                          : root.controller.rangeEndFrame <= 0
                            ? qsTr("%1 F → set end").arg(root.controller.rangeStartFrame)
                            : qsTr("%1 F → %2 F · %3 frames")
                                .arg(root.controller.rangeStartFrame)
                                .arg(root.controller.rangeEndFrame)
                                .arg(root.controller.selectedFrameCount)
                    color: root.controller.rangeStartFrame > 0 ? "#dcecff" : "#aeb0b3"
                    font.pixelSize: 11
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                }

                PanelButton {
                    text: qsTr("Set End")
                    enabled: root.controller.exactFrameIndexAvailable
                             && root.controller.rangeStartFrame > 0
                    onClicked: root.controller.markRangeEnd()
                }

                PanelButton {
                    text: qsTr("Clear")
                    visible: root.controller.rangeStartFrame > 0
                    onClicked: root.controller.clearFrameRange()
                }

                PanelButton {
                    text: root.controller.isExportingFrames
                          ? qsTr("Export %1%")
                                .arg(Math.round(root.controller.exportProgress * 100))
                          : qsTr("Export PNGs")
                    enabled: root.controller.canExportFrameRange
                    onClicked: root.controller.exportFrameRange()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                visible: root.controller.isIndexing || root.controller.isExportingFrames
                spacing: 10

                Label {
                    text: root.controller.isExportingFrames
                          ? qsTr("Saving selected frames…")
                          : root.controller.indexingProgress > 0
                            ? qsTr("Indexing frames… %1%")
                                .arg(Math.round(root.controller.indexingProgress * 100))
                            : qsTr("Indexing frames…")
                    color: "#bfc0c2"
                    font.pixelSize: 11
                }

                ProgressBar {
                    Layout.fillWidth: true
                    from: 0
                    to: 1
                    value: root.controller.isExportingFrames
                           ? root.controller.exportProgress
                           : root.controller.indexingProgress
                    indeterminate: value <= 0
                }
            }
        }
    }
}
