pragma Singleton

import QtQuick

QtObject {
    readonly property SystemPalette systemPalette: SystemPalette {}
    readonly property bool dark: systemPalette.window.hslLightness < 0.45

    readonly property color canvas: dark ? "#111216" : "#e9edf4"
    readonly property color canvasTop: dark ? "#20232a" : "#f9fbff"
    readonly property color canvasBottom: dark ? "#090a0d" : "#dce3ed"
    readonly property color textPrimary: dark ? "#f7f8fa" : "#17191d"
    readonly property color textSecondary: dark ? "#b9bdc6" : "#59616d"
    readonly property color glassTop: dark ? "#b02a2d34" : "#b8ffffff"
    readonly property color glassBottom: dark ? "#9e111318" : "#94e9eef6"
    readonly property color glassBorder: dark ? "#42ffffff" : "#5cffffff"
    readonly property color glassInnerHighlight: dark ? "#20ffffff" : "#8affffff"
    readonly property color controlFill: dark ? "#22ffffff" : "#52000000"
    readonly property color controlHover: dark ? "#38ffffff" : "#76000000"
    readonly property color accent: "#2388ff"
    readonly property color accentHover: "#4aa0ff"
    readonly property color mediaText: "#f8f9fb"
    readonly property color mediaSecondary: "#c4c8d0"
    readonly property color mediaGlassTop: "#a921242a"
    readonly property color mediaGlassBottom: "#c20b0c0f"
    readonly property color mediaBorder: "#48ffffff"
}
