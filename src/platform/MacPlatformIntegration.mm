#include "platform/MacPlatformIntegration.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QWindow>

#import <AppKit/AppKit.h>
#import <CoreServices/CoreServices.h>

#include <array>

namespace frameviewer::macos {
namespace {

const std::array<CFStringRef, 13>& videoContentTypes()
{
    static const std::array types{
        CFSTR("public.movie"),
        CFSTR("public.video"),
        CFSTR("public.mpeg-4"),
        CFSTR("com.apple.quicktime-movie"),
        CFSTR("public.avi"),
        CFSTR("public.mpeg"),
        CFSTR("public.mpeg-2-transport-stream"),
        CFSTR("org.matroska.mkv"),
        CFSTR("org.webmproject.webm"),
        CFSTR("public.3gpp"),
        CFSTR("public.3gpp2"),
        CFSTR("com.microsoft.windows-media-wmv"),
        CFSTR("com.microsoft.advanced-systems-format"),
    };
    return types;
}

bool handlerMatches(CFStringRef contentType)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    CFStringRef handler = LSCopyDefaultRoleHandlerForContentType(
        contentType, kLSRolesViewer);
#pragma clang diagnostic pop
    if (!handler) {
        return false;
    }
    const bool matches = CFStringCompare(
                             handler,
                             CFSTR("com.frameviewer.app"),
                             kCFCompareCaseInsensitive)
        == kCFCompareEqualTo;
    CFRelease(handler);
    return matches;
}

} // namespace

bool available()
{
    return true;
}

bool installedInApplications()
{
    @autoreleasepool {
        NSString* bundlePath = NSBundle.mainBundle.bundlePath.stringByStandardizingPath;
        return [bundlePath isEqualToString:@"/Applications/FrameViewer.app"]
            || [bundlePath hasPrefix:@"/Applications/"];
    }
}

bool isDefaultVideoPlayer()
{
    return handlerMatches(CFSTR("public.movie"))
        || handlerMatches(CFSTR("public.video"));
}

QString makeDefaultVideoPlayer()
{
    if (!installedInApplications()) {
        return QCoreApplication::translate(
            "MacPlatformIntegration",
            "Move FrameViewer to Applications before making it the default video player.");
    }

    registerApplicationBundle();
    OSStatus firstError = noErr;
    for (CFStringRef contentType : videoContentTypes()) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        const OSStatus result = LSSetDefaultRoleHandlerForContentType(
            contentType,
            kLSRolesViewer,
            CFSTR("com.frameviewer.app"));
#pragma clang diagnostic pop
        if (result != noErr && firstError == noErr) {
            firstError = result;
        }
    }

    if (firstError != noErr) {
        return QCoreApplication::translate(
                   "MacPlatformIntegration",
                   "macOS could not update every video association (error %1). You can still use Finder → Get Info → Open with → Change All.")
            .arg(firstError);
    }
    return {};
}

void registerApplicationBundle()
{
    @autoreleasepool {
        NSURL* bundleURL = NSBundle.mainBundle.bundleURL;
        if (!bundleURL) {
            return;
        }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        LSRegisterURL((__bridge CFURLRef)bundleURL, true);
#pragma clang diagnostic pop
    }
}

void configureWindow(QWindow* window)
{
    if (!window) {
        return;
    }
    @autoreleasepool {
        NSView* view = (__bridge NSView*)reinterpret_cast<void*>(window->winId());
        NSWindow* nativeWindow = view.window;
        if (!nativeWindow) {
            return;
        }
        nativeWindow.titlebarAppearsTransparent = YES;
        nativeWindow.titleVisibility = NSWindowTitleHidden;
        nativeWindow.styleMask |= NSWindowStyleMaskFullSizeContentView;
        nativeWindow.backgroundColor = NSColor.blackColor;
        nativeWindow.opaque = NO;
    }
}

} // namespace frameviewer::macos
