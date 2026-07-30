import QtQuick
import QtQuick.Controls

Button {
    id: root

    implicitHeight: 30
    implicitWidth: Math.max(70, contentItem.implicitWidth + 20)
    leftPadding: 10
    rightPadding: 10
    focusPolicy: Qt.TabFocus

    contentItem: Text {
        text: root.text
        color: root.enabled ? "#f2f2f2" : "#70f2f2f2"
        font.pixelSize: 12
        font.weight: Font.Medium
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: 8
        color: !root.enabled
               ? "#0dffffff"
               : root.down
                 ? "#35ffffff"
                 : root.hovered || root.visualFocus
                   ? "#26ffffff"
                   : "#17ffffff"
        border.width: root.visualFocus ? 2 : 1
        border.color: root.visualFocus ? "#d8ffffff" : "#20ffffff"

        Behavior on color {
            ColorAnimation { duration: 120 }
        }
    }
}
