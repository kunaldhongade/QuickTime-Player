import QtQuick
import QtQuick.Controls

GlassPanel {
    id: root

    property string message: ""

    function show(text) {
        message = text
        opacity = 1
        hideTimer.restart()
    }

    overMedia: true
    implicitWidth: Math.min(implicitLabelWidth + 36, 560)
    implicitHeight: label.implicitHeight + 20
    property real implicitLabelWidth: label.implicitWidth
    radius: 12
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
        width: Math.min(implicitWidth, 524)
        text: root.message
        color: Theme.mediaText
        font.pixelSize: 13
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
    }
}
