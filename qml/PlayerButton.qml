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

    background: GlassPanel {
        radius: width / 2
        overMedia: true
        opacity: !root.enabled
                 ? 0
                 : root.down
                   ? 0.88
                   : root.hovered || root.visualFocus || root.prominent
                     ? 0.62
                     : 0
        border.width: root.visualFocus ? 2 : 0
        border.color: "#d8ffffff"

        Behavior on opacity {
            NumberAnimation { duration: 120 }
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
