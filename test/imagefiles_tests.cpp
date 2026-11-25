#include "catch_qt.h"

#include "imagefiles.h"

#include <catch2/catch_test_macros.hpp>

#include <QDir>
#include <QFile>
#include <QImage>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>

namespace {

bool writePng(const QString &path)
{
  QImage image(8, 8, QImage::Format_RGB32);
  image.fill(Qt::red);
  return image.save(path, "PNG");
}

bool writeText(const QString &path)
{
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly)) { return false; }
  return file.write("not an image\n") > 0;
}

}// namespace

TEST_CASE("Image file name suffixes are recognised regardless of case", "[imagefiles]")
{
  REQUIRE(imgview::hasImageSuffix(QStringLiteral("photo.png")));
  REQUIRE(imgview::hasImageSuffix(QStringLiteral("photo.PNG")));
  REQUIRE(imgview::hasImageSuffix(QStringLiteral("/some/dir/photo.jpg")));

  REQUIRE_FALSE(imgview::hasImageSuffix(QStringLiteral("notes.txt")));
  REQUIRE_FALSE(imgview::hasImageSuffix(QStringLiteral("photo")));

  // Sorted, so that the order does not depend on how Qt enumerates its plugins.
  const QStringList suffixes = imgview::supportedImageSuffixes();
  REQUIRE(suffixes.contains(QStringLiteral("png")));
  QStringList sorted = suffixes;
  sorted.sort();
  REQUIRE(suffixes == sorted);
}

TEST_CASE("A directory contributes its image files, sorted, without descending by default", "[imagefiles]")
{
  QTemporaryDir root;
  REQUIRE(root.isValid());

  const QDir dir(root.path());
  REQUIRE(writePng(dir.absoluteFilePath(QStringLiteral("b.png"))));
  REQUIRE(writePng(dir.absoluteFilePath(QStringLiteral("a.png"))));
  REQUIRE(writePng(dir.absoluteFilePath(QStringLiteral(".hidden.png"))));
  REQUIRE(writeText(dir.absoluteFilePath(QStringLiteral("notes.txt"))));
  REQUIRE(dir.mkpath(QStringLiteral("nested")));
  REQUIRE(writePng(dir.absoluteFilePath(QStringLiteral("nested/c.png"))));

  const imgview::ImageScan flat = imgview::collectImages({ root.path() }, false);
  REQUIRE(flat.warnings.isEmpty());
  REQUIRE(
    flat.files
    == QStringList{ dir.absoluteFilePath(QStringLiteral("a.png")), dir.absoluteFilePath(QStringLiteral("b.png")) });

  const imgview::ImageScan deep = imgview::collectImages({ root.path() }, true);
  REQUIRE(deep.warnings.isEmpty());
  REQUIRE(deep.files.size() == 3);
  REQUIRE(deep.files.last() == dir.absoluteFilePath(QStringLiteral("nested/c.png")));
}

TEST_CASE("Files keep their command line order and are only added once", "[imagefiles]")
{
  QTemporaryDir root;
  REQUIRE(root.isValid());

  const QDir dir(root.path());
  REQUIRE(writePng(dir.absoluteFilePath(QStringLiteral("a.png"))));
  REQUIRE(writePng(dir.absoluteFilePath(QStringLiteral("b.png"))));

  const QString a = dir.absoluteFilePath(QStringLiteral("a.png"));
  const QString b = dir.absoluteFilePath(QStringLiteral("b.png"));

  // A file, then the directory it is in, then the same file again: the file is
  // named first, so it comes first, and nothing appears twice.
  const imgview::ImageScan scan = imgview::collectImages({ b, root.path(), a }, false);
  REQUIRE(scan.warnings.isEmpty());
  REQUIRE(scan.files == QStringList{ b, a });
}

TEST_CASE("A path that does not exist is a warning, not an image", "[imagefiles]")
{
  QTemporaryDir root;
  REQUIRE(root.isValid());

  const QDir dir(root.path());
  const imgview::ImageScan scan = imgview::collectImages({ dir.absoluteFilePath(QStringLiteral("nope.png")) }, false);

  REQUIRE(scan.files.isEmpty());
  REQUIRE(scan.warnings.size() == 1);
}

TEST_CASE("A file named on the command line is shown even if its suffix says otherwise", "[imagefiles]")
{
  QTemporaryDir root;
  REQUIRE(root.isValid());

  const QDir dir(root.path());
  REQUIRE(writeText(dir.absoluteFilePath(QStringLiteral("maybe-an-image"))));

  // It is up to the viewer to say that it cannot be read; dropping an explicit
  // argument silently would be worse.
  const imgview::ImageScan scan =
    imgview::collectImages({ dir.absoluteFilePath(QStringLiteral("maybe-an-image")) }, false);
  REQUIRE(scan.warnings.isEmpty());
  REQUIRE(scan.files.size() == 1);
}

TEST_CASE("A directory symlink that points back up is not followed forever", "[imagefiles]")
{
  QTemporaryDir root;
  REQUIRE(root.isValid());

  const QDir dir(root.path());
  REQUIRE(dir.mkdir(QStringLiteral("real")));
  REQUIRE(writePng(dir.absoluteFilePath(QStringLiteral("real/a.png"))));
  REQUIRE(QFile::link(dir.absoluteFilePath(QStringLiteral("real")), dir.absoluteFilePath(QStringLiteral("link"))));

  const imgview::ImageScan scan = imgview::collectImages({ root.path() }, true);
  REQUIRE(scan.warnings.isEmpty());
  REQUIRE(scan.files.size() == 1);
}
