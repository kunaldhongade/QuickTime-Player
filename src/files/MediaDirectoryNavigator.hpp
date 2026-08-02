#pragma once

#include <QString>
#include <QStringList>

namespace frameviewer {

class MediaDirectoryNavigator {
public:
    void setCurrentFile(const QString& mediaPath);

    [[nodiscard]] bool canOpenPrevious() const;
    [[nodiscard]] bool canOpenNext() const;
    [[nodiscard]] QString previousPath() const;
    [[nodiscard]] QString nextPath() const;
    [[nodiscard]] const QStringList& files() const;
    [[nodiscard]] qsizetype currentIndex() const;

    [[nodiscard]] static bool isSupportedVideoFile(const QString& path);
    [[nodiscard]] static const QStringList& supportedFilePatterns();

private:
    QStringList m_files;
    qsizetype m_currentIndex = -1;
};

} // namespace frameviewer
