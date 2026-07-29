import QtQuick
import FrameViewer.Native

Item {
    id: root

    required property var engine

    Rectangle {
        anchors.fill: parent
        color: "black"
    }

    MpvVideoItem {
        anchors.fill: parent
        engine: root.engine
    }
}
