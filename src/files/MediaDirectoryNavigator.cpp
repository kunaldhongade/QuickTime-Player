#include "files/MediaDirectoryNavigator.hpp"

#include <QDir>
#include <QFileInfo>
#include <QStringView>

#include <algorithm>

namespace frameviewer {
namespace {

const QStringList& supportedSuffixes()
{
    static const QStringList suffixes{
        QStringLiteral("avi"),
        QStringLiteral("m2ts"),
        QStringLiteral("m4v"),
        QStringLiteral("mkv"),
        QStringLiteral("mov"),
        QStringLiteral("mp4"),
        QStringLiteral("mpeg"),
        QStringLiteral("mpg"),
        QStringLiteral("mts"),
        QStringLiteral("ts"),
        QStringLiteral("webm"),
    };
    return suffixes;
}

bool isAsciiDigit(QChar character)
{
    return character >= QChar(u'0') && character <= QChar(u'9');
}

bool naturalFilenameLessThan(const QString& leftFilename, const QString& rightFilename)
{
    const QString left = leftFilename.toCaseFolded();
    const QString right = rightFilename.toCaseFolded();
    qsizetype leftPosition = 0;
    qsizetype rightPosition = 0;

    while (leftPosition < left.size() && rightPosition < right.size()) {
        if (isAsciiDigit(left.at(leftPosition)) && isAsciiDigit(right.at(rightPosition))) {
            const qsizetype leftRunStart = leftPosition;
            const qsizetype rightRunStart = rightPosition;
            while (leftPosition < left.size() && isAsciiDigit(left.at(leftPosition))) {
                ++leftPosition;
            }
            while (rightPosition < right.size() && isAsciiDigit(right.at(rightPosition))) {
                ++rightPosition;
            }

            qsizetype leftSignificant = leftRunStart;
            qsizetype rightSignificant = rightRunStart;
            while (leftSignificant < leftPosition
                   && left.at(leftSignificant) == QChar(u'0')) {
                ++leftSignificant;
            }
            while (rightSignificant < rightPosition
                   && right.at(rightSignificant) == QChar(u'0')) {
                ++rightSignificant;
            }

            const qsizetype leftDigits = leftPosition - leftSignificant;
            const qsizetype rightDigits = rightPosition - rightSignificant;
            if (leftDigits != rightDigits) {
                return leftDigits < rightDigits;
            }
            const int numberComparison =
                QStringView(left).mid(leftSignificant, leftDigits).compare(
                    QStringView(right).mid(rightSignificant, rightDigits));
            if (numberComparison != 0) {
                return numberComparison < 0;
            }

            const qsizetype leftRunLength = leftPosition - leftRunStart;
            const qsizetype rightRunLength = rightPosition - rightRunStart;
            if (leftRunLength != rightRunLength) {
                return leftRunLength < rightRunLength;
            }
            continue;
        }

        if (left.at(leftPosition) != right.at(rightPosition)) {
            return left.at(leftPosition) < right.at(rightPosition);
        }
        ++leftPosition;
        ++rightPosition;
    }
    return left.size() < right.size();
}

} // namespace

void MediaDirectoryNavigator::setCurrentFile(const QString& mediaPath)
{
    m_files.clear();
    m_currentIndex = -1;

    const QFileInfo current(mediaPath);
    if (!current.exists() || !current.isFile()) {
        return;
    }

    const QString currentPath = current.absoluteFilePath();
    const QFileInfoList entries =
        current.absoluteDir().entryInfoList(QDir::Files | QDir::Readable | QDir::NoDotAndDotDot);
    for (const QFileInfo& entry : entries) {
        if (isSupportedVideoFile(entry.absoluteFilePath())
            || entry.absoluteFilePath() == currentPath) {
            m_files.append(entry.absoluteFilePath());
        }
    }

    std::sort(m_files.begin(), m_files.end(), [](const QString& left, const QString& right) {
        return naturalFilenameLessThan(QFileInfo(left).fileName(),
                                       QFileInfo(right).fileName());
    });
    m_currentIndex = m_files.indexOf(currentPath);
}

bool MediaDirectoryNavigator::canOpenPrevious() const
{
    return m_currentIndex > 0;
}

bool MediaDirectoryNavigator::canOpenNext() const
{
    return m_currentIndex >= 0 && m_currentIndex + 1 < m_files.count();
}

QString MediaDirectoryNavigator::previousPath() const
{
    return canOpenPrevious() ? m_files.at(m_currentIndex - 1) : QString{};
}

QString MediaDirectoryNavigator::nextPath() const
{
    return canOpenNext() ? m_files.at(m_currentIndex + 1) : QString{};
}

const QStringList& MediaDirectoryNavigator::files() const
{
    return m_files;
}

qsizetype MediaDirectoryNavigator::currentIndex() const
{
    return m_currentIndex;
}

bool MediaDirectoryNavigator::isSupportedVideoFile(const QString& path)
{
    return supportedSuffixes().contains(QFileInfo(path).suffix().toLower());
}

} // namespace frameviewer
