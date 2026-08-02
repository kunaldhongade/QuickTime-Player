#include "platform/ExecutableLocator.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace frameviewer {

QString locateExecutable(const QString& name)
{
#ifdef Q_OS_MACOS
    const QString bundled = QDir(QCoreApplication::applicationDirPath())
                                .absoluteFilePath(QStringLiteral("../Helpers/%1").arg(name));
    const QFileInfo information(bundled);
    if (information.exists() && information.isFile() && information.isExecutable()) {
        return information.absoluteFilePath();
    }
#endif
    return QStandardPaths::findExecutable(name);
}

} // namespace frameviewer
