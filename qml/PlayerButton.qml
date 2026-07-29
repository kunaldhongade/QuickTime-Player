import QtQuick
import QtQuick.Controls

Control {
    id: root

    required property url iconSource
    required property string accessibleName
    property bool prominent: false
    property bool down: tapHandler.pressed
    signal clicked()

    implicitWidth: prominent ? 52 : 42
    implicitHeight: prominent ? 52 : 42
    hoverEnabled: true
    focusPolicy: Qt.TabFocus
    Accessible.name: accessibleName
    Accessible.role: Accessible.Button
    ToolTip.visible: hovered
    ToolTip.text: accessibleName

    Keys.onReturnPressed: clicked()
    Keys.onEnterPressed: clicked()

    TapHandler {
        id: tapHandler
        enabled: root.enabled
        onTapped: root.clicked()
    }

    background: Rectangle {
        radius: width / 2
        color: !root.enabled
               ? "transparent"
               : root.down
                 ? "#40ffffff"
                 : root.hovered || root.visualFocus
                   ? "#24ffffff"
                   : "transparent"
        border.width: root.visualFocus ? 2 : 0
        border.color: "#d8ffffff"

        Behavior on color {
            ColorAnimation { duration: 120 }
        }
    }

    contentItem: Image {
        source: root.iconSource
        sourceSize.width: root.prominent ? 30 : 24
        sourceSize.height: root.prominent ? 30 : 24
        fillMode: Image.PreserveAspectFit
        opacity: root.enabled ? 1 : 0.32
        scale: root.down ? 0.92 : 1

        Behavior on scale {
            NumberAnimation { duration: 90 }
        }
    }
}
