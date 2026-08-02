import QtQuick

Rectangle {
    id: root

    property bool overMedia: false

    radius: 22
    color: "transparent"
    border.width: 1
    border.color: overMedia ? Theme.mediaBorder : Theme.glassBorder
    gradient: Gradient {
        GradientStop {
            position: 0
            color: root.overMedia ? Theme.mediaGlassTop : Theme.glassTop
        }
        GradientStop {
            position: 1
            color: root.overMedia ? Theme.mediaGlassBottom : Theme.glassBottom
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: Math.max(0, parent.radius - 1)
        color: "transparent"
        border.width: 1
        border.color: Theme.glassInnerHighlight
        opacity: 0.72
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: parent.radius
        anchors.rightMargin: parent.radius
        height: 1
        color: "#a8ffffff"
        opacity: 0.58
    }
}
