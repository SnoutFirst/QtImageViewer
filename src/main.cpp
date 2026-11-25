#include "config.hpp"

#include "imagefiles.h"
#include "mainwindow.h"
#include "playlist.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include <cmath>
#include <cstdlib>
#include <string_view>

namespace {

constexpr int kDefaultSlideshowSeconds = 5;
constexpr int kMaximumSlideshowSeconds = 24 * 60 * 60;

//! The version CMake built this program as, from configured_files/config.hpp.in.
QString applicationVersion()
{
  const std::string_view version = QtImageViewer::cmake::project_version;
  return QString::fromLatin1(version.data(), static_cast<int>(version.size()));
}

void report(const QString &message) { QTextStream(stderr) << "QtImageViewer: " << message << '\n'; }

/*
 * Milliseconds between slides, from --timeout. Returns false when --timeout was
 * given but is not a positive number of seconds.
 */
bool slideshowInterval(const QCommandLineParser &parser,
  const QCommandLineOption &slideshowOption,
  const QCommandLineOption &timeoutOption,
  int *interval)
{
  double seconds = kDefaultSlideshowSeconds;

  if (parser.isSet(timeoutOption)) {
    bool isNumber = false;
    seconds = parser.value(timeoutOption).toDouble(&isNumber);
    if (!isNumber || !std::isfinite(seconds) || seconds <= 0.0 || seconds > kMaximumSlideshowSeconds) {
      report(QStringLiteral("--timeout %1: expected a number of seconds greater than 0 and not more than %2")
          .arg(parser.value(timeoutOption))
          .arg(kMaximumSlideshowSeconds));
      return false;
    }

    if (!parser.isSet(slideshowOption)) {
      report(QStringLiteral("note: --timeout is only used together with --slideshow"));
    }
  }

  *interval = parser.isSet(slideshowOption) ? static_cast<int>(std::lround(seconds * 1000.0)) : 0;
  return true;
}

}// namespace

int main(int argc, char *argv[])
{
  // These have to be set before the application object exists.
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

  // qApp points at this object as a QApplication *, so it cannot be const even
  // though everything this program calls on it is static.
  QApplication application(argc, argv);// NOLINT(misc-const-correctness)
  QApplication::setApplicationName(QStringLiteral("QtImageViewer"));
  QApplication::setApplicationVersion(applicationVersion());

  const QCommandLineOption noRecurseOption(
    QStringLiteral("no-recurse"), QStringLiteral("Do not look inside the subdirectories of a directory argument."));
  const QCommandLineOption randomOption({ QStringLiteral("r"), QStringLiteral("random") },
    QStringLiteral("Show the images in a random order: every image once, then shuffle again."));
  const QCommandLineOption slideshowOption(
    { QStringLiteral("s"), QStringLiteral("slideshow") }, QStringLiteral("Advance to the next image automatically."));
  const QCommandLineOption timeoutOption({ QStringLiteral("t"), QStringLiteral("timeout") },
    QStringLiteral("Seconds each image is shown in a slideshow (default: %1).").arg(kDefaultSlideshowSeconds),
    QStringLiteral("seconds"),
    QString::number(kDefaultSlideshowSeconds));
  const QCommandLineOption verboseOption({ QStringLiteral("v"), QStringLiteral("verbose") },
    QStringLiteral("Draw the file name, the position in the list and the image details over the image."));
  // -v is taken by --verbose, which is the more useful of the two; the version
  // is on -V instead of the -v addVersionOption() would use.
  const QCommandLineOption versionOption({ QStringLiteral("V"), QStringLiteral("version") },
    QStringLiteral("Displays version information."));

  QCommandLineParser parser;
  parser.setApplicationDescription(
    QStringLiteral("Shows images, in the order given or in a shuffled order, by hand or as a slideshow."));
  parser.addHelpOption();
  parser.addOption(versionOption);
  parser.addOption(noRecurseOption);
  parser.addOption(randomOption);
  parser.addOption(slideshowOption);
  parser.addOption(timeoutOption);
  parser.addOption(verboseOption);
  parser.addPositionalArgument(QStringLiteral("images"),
    QStringLiteral("Image files, and/or directories with images in them."),
    QStringLiteral("[images...]"));
  parser.process(application);

  if (parser.isSet(versionOption)) {
    QTextStream(stdout) << QApplication::applicationName() << ' ' << QApplication::applicationVersion() << '\n';
    return EXIT_SUCCESS;
  }

  const QStringList paths = parser.positionalArguments();
  if (paths.isEmpty()) {
    report(QStringLiteral("no images given, see --help"));
    return EXIT_FAILURE;
  }

  int interval = 0;
  if (!slideshowInterval(parser, slideshowOption, timeoutOption, &interval)) { return EXIT_FAILURE; }

  const imgview::ImageScan scan = imgview::collectImages(paths, !parser.isSet(noRecurseOption));
  for (const QString &warning : scan.warnings) { report(warning); }
  if (scan.files.isEmpty()) {
    report(QStringLiteral("no images found in the given arguments"));
    return EXIT_FAILURE;
  }

  const imgview::PlaylistOrder order =
    parser.isSet(randomOption) ? imgview::PlaylistOrder::Shuffled : imgview::PlaylistOrder::Sequential;

  imgview::MainWindow window(imgview::Playlist(scan.files, order), interval, parser.isSet(verboseOption));
  window.show();

  return application.exec();
}
