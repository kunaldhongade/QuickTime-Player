import QtQuick
import QtQuick.Controls
import QtQml

Slider {
    id: root

    required property var controller
    property int pendingFrame: Math.round(value)

    from: 0
    to: controller.exactFrameIndexAvailable
        ? Math.max(0, controller.totalFrames - 1)
        : Math.max(0.001, controller.duration)
    stepSize: controller.exactFrameIndexAvailable ? 1 : 0
    enabled: controller.hasMedia && to > from
    implicitHeight: 24

    Binding {
        target: root
        property: "value"
        when: !root.pressed
        value: root.controller.exactFrameIndexAvailable
               ? Math.max(0, root.controller.currentFrame - 1)
               : root.controller.currentTime
        restoreMode: Binding.RestoreNone
    }

    onMoved: {
        if (controller.exactFrameIndexAvailable)
            previewThrottle.restart()
    }

    onPressedChanged: {
        if (!pressed && controller.exactFrameIndexAvailable) {
            previewThrottle.stop()
            controller.seekToFrame(Math.round(value))
        }
    }

    Timer {
        id: previewThrottle
        interval: 90
        repeat: false
        onTriggered: root.controller.seekToFrame(Math.round(root.value))
    }

    ToolTip.visible: pressed && controller.exactFrameIndexAvailable
    ToolTip.text: qsTr("%1 F").arg(Math.round(value) + 1)

    background: Rectangle {
        x: root.leftPadding
        y: root.topPadding + root.availableHeight / 2 - height / 2
        width: root.availableWidth
        height: 5
        radius: 2.5
        color: root.enabled ? "#54ffffff" : "#28ffffff"

        Rectangle {
            width: root.visualPosition * parent.width
            height: parent.height
            radius: parent.radius
            color: root.enabled ? "#f5f5f5" : "#6ff5f5f5"
        }

        Rectangle {
            readonly property real firstPosition: root.controller.totalFrames > 1
                ? (root.controller.rangeStartFrame - 1)
                    / (root.controller.totalFrames - 1)
                : 0
            readonly property real lastPosition: root.controller.totalFrames > 1
                ? (root.controller.rangeEndFrame - 1)
                    / (root.controller.totalFrames - 1)
                : 1

            visible: root.controller.rangeStartFrame > 0
                     && root.controller.rangeEndFrame >= root.controller.rangeStartFrame
            x: Math.max(0, firstPosition * parent.width)
            width: Math.max(5, (lastPosition - firstPosition) * parent.width)
            height: parent.height
            radius: parent.radius
            color: "#5fa8ff"
        }
    }

    handle: Rectangle {
        x: root.leftPadding + root.visualPosition * (root.availableWidth - width)
        y: root.topPadding + root.availableHeight / 2 - height / 2
        width: root.pressed ? 18 : 16
        height: width
        radius: width / 2
        color: root.enabled ? "white" : "#9a9a9a"
        border.width: 1
        border.color: "#26000000"

        Behavior on width {
            NumberAnimation { duration: 110 }
        }
    }
}
