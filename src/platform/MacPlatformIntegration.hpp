#pragma once

#include <QString>

class QWindow;

namespace frameviewer::macos {

[[nodiscard]] bool available();
[[nodiscard]] bool installedInApplications();
[[nodiscard]] bool isDefaultVideoPlayer();
[[nodiscard]] QString makeDefaultVideoPlayer();
void registerApplicationBundle();
void configureWindow(QWindow* window);

} // namespace frameviewer::macos
