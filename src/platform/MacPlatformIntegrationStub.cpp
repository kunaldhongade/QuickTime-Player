#include "platform/MacPlatformIntegration.hpp"

namespace frameviewer::macos {

bool available()
{
    return false;
}

bool installedInApplications()
{
    return false;
}

bool isDefaultVideoPlayer()
{
    return false;
}

QString makeDefaultVideoPlayer()
{
    return QStringLiteral("macOS integration is unavailable on this platform.");
}

void registerApplicationBundle() {}

void configureWindow(QWindow*) {}

} // namespace frameviewer::macos
