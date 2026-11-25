#include "imagefiles.h"

#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QSet>

namespace imgview {
namespace {

  const QSet<QString> &imageSuffixes()
  {
    static const QSet<QString> suffixes = [] {
      QSet<QString> result;
      const QList<QByteArray> formats = QImageReader::supportedImageFormats();
      for (const QByteArray &format : formats) { result.insert(QString::fromLatin1(format).toLower()); }
      return result;
    }();
    return suffixes;
  }

  //! Whether \a name is one that a directory scan should leave alone.
  bool isHiddenName(const QString &name)
  {
    // QDir drops the entries the platform calls hidden (the hidden attribute on
    // Windows) and the leading dots here, so a name starting with a dot is skipped
    // everywhere: ".thumbnails" or ".DS_Store.png" is not a photo on any platform.
    return name.startsWith(QLatin1Char('.'));
  }

  //! Identity of \a path for duplicate detection; resolves symlinks when it can.
  QString identityOf(const QString &path)
  {
    const QString canonical = QFileInfo(path).canonicalFilePath();
    return canonical.isEmpty() ? path : canonical;
  }

  void addFile(const QString &path, QSet<QString> *seen, ImageScan *scan)
  {
    const QString identity = identityOf(path);
    if (seen->contains(identity)) { return; }
    seen->insert(identity);
    scan->files.append(path);
  }

  void scanDirectory(const QString &directory,
    bool recursive,
    QSet<QString> *visited,
    QSet<QString> *seen,
    ImageScan *scan)
  {
    const QString identity = identityOf(directory);
    if (visited->contains(identity)) {
      return;// a directory symlink that leads back into itself
    }
    visited->insert(identity);

    const QDir dir(directory);
    if (!dir.isReadable()) {
      scan->warnings.append(QStringLiteral("cannot read directory: %1").arg(QDir::toNativeSeparators(directory)));
      return;
    }

    const QStringList names = dir.entryList(QDir::Files | QDir::Readable, QDir::Name);
    for (const QString &name : names) {
      if (isHiddenName(name)) { continue; }
      const QString path = dir.absoluteFilePath(name);
      if (hasImageSuffix(path)) { addFile(path, seen, scan); }
    }

    if (!recursive) { return; }

    const QStringList subdirectories = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString &name : subdirectories) {
      if (isHiddenName(name)) { continue; }
      scanDirectory(dir.absoluteFilePath(name), true, visited, seen, scan);
    }
  }

}// namespace

bool hasImageSuffix(const QString &path) { return imageSuffixes().contains(QFileInfo(path).suffix().toLower()); }

QStringList supportedImageSuffixes()
{
  QStringList suffixes = imageSuffixes().values();
  suffixes.sort();
  return suffixes;
}

ImageScan collectImages(const QStringList &paths, bool recursive)
{
  ImageScan scan;
  QSet<QString> seen;
  QSet<QString> visited;

  for (const QString &path : paths) {
    const QFileInfo info(path);

    if (info.isDir()) {
      scanDirectory(info.absoluteFilePath(), recursive, &visited, &seen, &scan);
    } else if (info.isFile()) {
      addFile(info.absoluteFilePath(), &seen, &scan);
    } else if (!info.exists()) {
      scan.warnings.append(QStringLiteral("no such file or directory: %1").arg(QDir::toNativeSeparators(path)));
    } else {
      scan.warnings.append(QStringLiteral("not a regular file or directory: %1").arg(QDir::toNativeSeparators(path)));
    }
  }

  return scan;
}

}// namespace imgview
