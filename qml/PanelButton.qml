import QtQuick
import QtQuick.Controls

Button {
    id: root

    property bool mediaStyle: true
    property bool accent: false

    implicitHeight: 30
    implicitWidth: Math.max(70, contentItem.implicitWidth + 20)
    leftPadding: 10
    rightPadding: 10
    focusPolicy: Qt.TabFocus

    contentItem: Text {
        text: root.text
        color: !root.enabled
               ? (root.mediaStyle ? "#70f2f2f2" : Theme.textSecondary)
               : root.accent || root.mediaStyle ? "#f8f9fb" : Theme.textPrimary
        font.pixelSize: 12
        font.weight: Font.Medium
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: 8
        color: !root.enabled
               ? (root.mediaStyle ? "#0dffffff" : "#13000000")
               : root.down
                 ? (root.accent ? "#1871d2" : root.mediaStyle ? "#35ffffff" : Theme.controlHover)
                 : root.hovered || root.visualFocus
                   ? (root.accent ? Theme.accentHover : root.mediaStyle ? "#26ffffff" : Theme.controlHover)
                   : (root.accent ? Theme.accent : root.mediaStyle ? "#17ffffff" : Theme.controlFill)
        border.width: root.visualFocus ? 2 : 1
        border.color: root.visualFocus
                      ? (root.mediaStyle ? "#d8ffffff" : Theme.accent)
                      : root.mediaStyle ? "#20ffffff" : Theme.glassBorder

        Behavior on color {
            ColorAnimation { duration: 120 }
        }
    }
}
