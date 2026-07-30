cmake_minimum_required(VERSION 3.24)

project(FrameViewer VERSION 0.1.1 LANGUAGES CXX)

option(FRAMEVIEWER_BUILD_TESTS "Build FrameViewer tests" ON)
option(FRAMEVIEWER_ENABLE_SANITIZERS "Enable address and undefined behaviour sanitizers" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui OpenGL Quick Qml QuickControls2)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MPV REQUIRED IMPORTED_TARGET mpv)

qt_standard_project_setup(REQUIRES 6.4)
if(Qt6_VERSION VERSION_GREATER_EQUAL 6.5)
    qt_policy(SET QTP0004 NEW)
endif()

set(FRAMEVIEWER_SOURCES
    src/app/ApplicationController.cpp
    src/app/ApplicationController.hpp
    src/app/main.cpp
    src/frames/FrameIndex.cpp
    src/frames/FrameIndex.hpp
    src/frames/FrameIndexCache.cpp
    src/frames/FrameIndexCache.hpp
    src/frames/FrameIndexer.cpp
    src/frames/FrameIndexer.hpp
    src/frames/FramePositionResolver.cpp
    src/frames/FramePositionResolver.hpp
    src/models/PlayerState.hpp
    src/playback/MpvEngine.cpp
    src/playback/MpvEngine.hpp
    src/playback/PlaybackController.cpp
    src/playback/PlaybackController.hpp
    src/rendering/MpvVideoItem.cpp
    src/rendering/MpvVideoItem.hpp
)

qt_add_executable(frameviewer ${FRAMEVIEWER_SOURCES})

qt_add_qml_module(frameviewer
    URI FrameViewer
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    QML_FILES
        qml/Main.qml
        qml/VideoViewport.qml
        qml/PlaybackOverlay.qml
        qml/FrameSeekBar.qml
        qml/PlayerButton.qml
        qml/EmptyState.qml
        qml/ErrorView.qml
        qml/Toast.qml
    RESOURCES
        assets/icons/backward.svg
        assets/icons/forward.svg
        assets/icons/open.svg
        assets/icons/pause.svg
        assets/icons/play.svg
)

target_include_directories(frameviewer PRIVATE src)
target_link_libraries(frameviewer PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::OpenGL
    Qt6::Quick
    Qt6::Qml
    Qt6::QuickControls2
    PkgConfig::MPV
)
target_compile_definitions(frameviewer PRIVATE
    FRAMEVIEWER_VERSION="${PROJECT_VERSION}"
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_TO_ASCII
)

if(APPLE)
    set_target_properties(frameviewer PROPERTIES MACOSX_BUNDLE TRUE)
endif()

if(FRAMEVIEWER_ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(frameviewer PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(frameviewer PRIVATE -fsanitize=address,undefined)
endif()

if(FRAMEVIEWER_BUILD_TESTS)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()

install(TARGETS frameviewer
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

if(UNIX AND NOT APPLE)
    install(FILES packaging/linux/frameviewer.desktop
        DESTINATION share/applications
    )
    install(FILES packaging/linux/frameviewer.svg
        DESTINATION share/icons/hicolor/scalable/apps
    )
    install(FILES README.md THIRD_PARTY_NOTICES.md
        DESTINATION share/doc/frameviewer
    )
endif()

set(CPACK_PACKAGE_NAME "frameviewer")
set(CPACK_PACKAGE_VENDOR "FrameViewer")
set(CPACK_PACKAGE_CONTACT "FrameViewer")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "Native frame-accurate Qt and libmpv video viewer"
)
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "FrameViewer")
set(CPACK_DEBIAN_PACKAGE_SECTION "video")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "ffmpeg, libqt6svg6, qml6-module-qtqml, qml6-module-qtqml-workerscript, qml6-module-qtquick, qml6-module-qtquick-controls, qml6-module-qtquick-dialogs, qml6-module-qtquick-layouts, qml6-module-qtquick-templates"
)
include(CPack)
