import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    property string message: ""

    function show(text) {
        message = text
        opacity = 1
        hideTimer.restart()
    }

    implicitWidth: Math.min(implicitLabelWidth + 32, 560)
    implicitHeight: label.implicitHeight + 18
    property real implicitLabelWidth: label.implicitWidth
    radius: 10
    color: "#e8232325"
    border.width: 1
    border.color: "#2dffffff"
    opacity: 0
    visible: opacity > 0

    Behavior on opacity {
        NumberAnimation { duration: 160 }
    }

    Timer {
        id: hideTimer
        interval: 2600
        onTriggered: root.opacity = 0
    }

    Label {
        id: label
        anchors.centerIn: parent
        width: Math.min(implicitWidth, 528)
        text: root.message
        color: "white"
        font.pixelSize: 13
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
    }
}
