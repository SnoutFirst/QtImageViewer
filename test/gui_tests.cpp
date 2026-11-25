#include "catch_qt.h"

#include "framelesswindow.h"
#include "imagefiles.h"
#include "imageview.h"
#include "mainwindow.h"
#include "playlist.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <QAction>
#include <QColor>
#include <QContextMenuEvent>
#include <QCoreApplication>
#include <QImage>
#include <QKeyEvent>
#include <QLabel>
#include <QKeySequence>
#include <QMenu>
#include <QMouseEvent>
#include <QTemporaryDir>
#include <QWheelEvent>

#include <array>

namespace {

//! Size of the test images, so a test can check that the viewer reports it.
constexpr int kTestImageWidth = 48;
constexpr int kTestImageHeight = 32;

bool writePng(const QString &path, const QColor &colour)
{
  QImage image(kTestImageWidth, kTestImageHeight, QImage::Format_RGB32);
  image.fill(colour);
  return image.save(path, "PNG");
}

void sendMouse(QWidget *widget, QEvent::Type type, const QPoint &position)
{
  QMouseEvent event(type,
                    QPointF(position),
                    QPointF(widget->mapToGlobal(position)),
                    Qt::LeftButton,
                    type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton,
                    Qt::NoModifier);
  QCoreApplication::sendEvent(widget, &event);
}

//! A press, a drag to \a delta from the press position, and the release.
void drag(QWidget *widget, const QPoint &from, const QPoint &delta)
{
  sendMouse(widget, QEvent::MouseButtonPress, from);
  sendMouse(widget, QEvent::MouseMove, from + delta);
  sendMouse(widget, QEvent::MouseButtonRelease, from + delta);
}

void sendKey(QWidget *widget, int key)
{
  QKeyEvent event(QEvent::KeyPress, key, Qt::NoModifier);
  QCoreApplication::sendEvent(widget, &event);
}

//! One wheel notch: positive \a delta is away from the user (up, or forwards).
void sendWheel(QWidget *widget, int delta, Qt::KeyboardModifiers modifiers)
{
  const QPoint position(10, 10);
  QWheelEvent event(QPointF(position),
                    QPointF(widget->mapToGlobal(position)),
                    QPoint(0, 0),
                    QPoint(0, delta),
                    Qt::NoButton,
                    modifiers,
                    Qt::NoScrollPhase,
                    false);
  QCoreApplication::sendEvent(widget, &event);
}

//! A red and a green image in \a directory, which is what the window needs to page through.
void writeTwoImages(const QTemporaryDir &directory)
{
  REQUIRE(writePng(directory.filePath(QStringLiteral("a.png")), Qt::red));
  REQUIRE(writePng(directory.filePath(QStringLiteral("b.png")), Qt::green));
}

//! The images of \a directory, in the order the viewer would show them.
imgview::ImageScan scanTwoImages(const QTemporaryDir &directory)
{
  imgview::ImageScan scan = imgview::collectImages({ directory.path() }, true);
  REQUIRE(scan.files.size() == 2);
  return scan;
}

//! Checks that context menu entry \a index is the command --help lists there, with the key that runs it.
void checkMenuItem(QMenu *menu, int index)
{
  const imgview::Command command = imgview::commands()[static_cast<std::size_t>(index)];
  const QAction *action = menu->actions().at(index);

  REQUIRE(action->text() == imgview::commandDescription(command));
  REQUIRE_FALSE(action->shortcut().isEmpty());
  REQUIRE(action->shortcut() == QKeySequence(imgview::primaryKey(command)));
}

//! The options most of these tests want: nothing running, nothing drawn over the image.
imgview::MainWindow::Options viewOptions(int slideshowInterval = 0, bool verbose = false)
{
  imgview::MainWindow::Options options;
  options.slideshowInterval = slideshowInterval;
  options.verbose = verbose;
  return options;
}

}// namespace

TEST_CASE("The window has no frame", "[gui]")
{
  // Assertions only, so the window can be const and the test says so.
  const imgview::FramelessWindow window;

  REQUIRE(window.windowType() == Qt::Window);
  REQUIRE(window.windowFlags().testFlag(Qt::FramelessWindowHint));
  REQUIRE(window.hasMouseTracking());
}

TEST_CASE("Dragging the window moves it", "[gui]")
{
  imgview::FramelessWindow window;
  window.resize(400, 300);
  window.show();
  window.move(120, 80);

  const QPoint before = window.pos();
  drag(&window, QPoint(200, 150), QPoint(40, 25));

  REQUIRE(window.pos() == before + QPoint(40, 25));
  // ... and the size is none of the drag's business.
  REQUIRE(window.size() == QSize(400, 300));
}

TEST_CASE("Dragging near an edge resizes the window", "[gui]")
{
  imgview::FramelessWindow window;
  window.resize(400, 300);
  window.show();
  window.move(120, 80);

  SECTION("the right edge follows the mouse")
  {
    const QSize before = window.size();
    drag(&window, QPoint(399, 150), QPoint(60, 0));

    REQUIRE(window.width() == before.width() + 60);
    REQUIRE(window.height() == before.height());
    REQUIRE(window.pos() == QPoint(120, 80));
  }

  SECTION("the left edge moves the window as it shrinks it")
  {
    const QSize before = window.size();
    drag(&window, QPoint(1, 150), QPoint(60, 0));

    REQUIRE(window.width() == before.width() - 60);
    REQUIRE(window.pos().x() == 120 + 60);
    // The opposite edge stayed where it was.
    REQUIRE(window.geometry().right() == 120 + before.width() - 1);
  }

  SECTION("the bottom right corner changes both")
  {
    const QSize before = window.size();
    drag(&window, QPoint(399, 299), QPoint(20, 40));

    REQUIRE(window.size() == before + QSize(20, 40));
  }

  SECTION("the window is not dragged below its minimum size")
  {
    drag(&window, QPoint(399, 299), QPoint(-2000, -2000));

    REQUIRE(window.width() == window.minimumWidth());
    REQUIRE(window.height() == window.minimumHeight());
  }
}

TEST_CASE("The cursor says what the edges do", "[gui]")
{
  imgview::FramelessWindow window;
  window.resize(400, 300);
  window.show();

  sendMouse(&window, QEvent::MouseMove, QPoint(1, 150));
  REQUIRE(window.cursor().shape() == Qt::SizeHorCursor);

  sendMouse(&window, QEvent::MouseMove, QPoint(1, 1));
  REQUIRE(window.cursor().shape() == Qt::SizeFDiagCursor);

  sendMouse(&window, QEvent::MouseMove, QPoint(399, 1));
  REQUIRE(window.cursor().shape() == Qt::SizeBDiagCursor);

  sendMouse(&window, QEvent::MouseMove, QPoint(200, 1));
  REQUIRE(window.cursor().shape() == Qt::SizeVerCursor);

  sendMouse(&window, QEvent::MouseMove, QPoint(200, 150));
  REQUIRE(window.cursor().shape() == Qt::ArrowCursor);
}

TEST_CASE("A double click toggles full screen", "[gui]")
{
  imgview::FramelessWindow window;
  window.resize(400, 300);
  window.show();
  REQUIRE_FALSE(window.isFullScreen());

  sendMouse(&window, QEvent::MouseButtonDblClick, QPoint(200, 150));
  REQUIRE(window.isFullScreen());

  sendMouse(&window, QEvent::MouseButtonDblClick, QPoint(200, 150));
  REQUIRE_FALSE(window.isFullScreen());
}

TEST_CASE("A full screen window is neither moved nor resized", "[gui]")
{
  imgview::FramelessWindow window;
  window.resize(400, 300);
  window.show();
  window.showFullScreen();

  const QRect before = window.geometry();
  drag(&window, QPoint(200, 150), QPoint(40, 25));
  REQUIRE(window.geometry() == before);

  drag(&window, QPoint(399, 299), QPoint(-40, -25));
  REQUIRE(window.geometry() == before);
}

TEST_CASE("The viewer shows the playlist and steps through it", "[gui]")
{
  const QTemporaryDir directory;
  REQUIRE(directory.isValid());
  REQUIRE(writePng(directory.filePath(QStringLiteral("a.png")), Qt::red));
  REQUIRE(writePng(directory.filePath(QStringLiteral("b.png")), Qt::green));

  const imgview::ImageScan scan = imgview::collectImages({ directory.path() }, true);
  REQUIRE(scan.files.size() == 2);

  imgview::MainWindow window(imgview::Playlist(scan.files, imgview::PlaylistOrder::Sequential), viewOptions());
  window.show();

  auto *view = window.findChild<imgview::ImageView *>();
  REQUIRE(view != nullptr);
  // The window drags and resizes itself, so the view must not swallow the mouse.
  REQUIRE(view->testAttribute(Qt::WA_TransparentForMouseEvents));

  // The file name goes in the window title; nothing is drawn over the image
  // unless verbose mode was asked for (the overlay exists, but is hidden).
  const auto *status = window.findChild<QLabel *>(QStringLiteral("status"));
  REQUIRE(status != nullptr);
  REQUIRE_FALSE(status->isVisible());
  REQUIRE(window.windowTitle().contains(QStringLiteral("a.png")));
  REQUIRE(window.windowTitle().contains(QStringLiteral("(1/2)")));

  SECTION("the next image is one key (or one wheel click) away")
  {
    sendKey(&window, Qt::Key_Right);

    REQUIRE(view->path() == scan.files.at(1));
    REQUIRE(window.windowTitle().contains(QStringLiteral("b.png")));
    REQUIRE(window.windowTitle().contains(QStringLiteral("(2/2)")));
  }

  SECTION("and the one before it is as well")
  {
    sendKey(&window, Qt::Key_Right);
    sendKey(&window, Qt::Key_Left);

    REQUIRE(view->path() == scan.files.at(0));
  }

  SECTION("Escape closes a window that is not full screen")
  {
    sendKey(&window, Qt::Key_Escape);

    REQUIRE_FALSE(window.isVisible());
  }
}

TEST_CASE("A viewer without images says so instead of showing nothing", "[gui]")
{
  imgview::MainWindow window(imgview::Playlist({}, imgview::PlaylistOrder::Sequential), viewOptions());
  window.show();

  // Nothing to step to, and nothing to crash on.
  sendKey(&window, Qt::Key_Right);
  sendKey(&window, Qt::Key_Left);
  sendKey(&window, Qt::Key_R);

  REQUIRE(window.windowTitle() == QStringLiteral("QtImageViewer"));
}

TEST_CASE("Verbose mode draws the file name and the image details over the image", "[gui]")
{
  const QTemporaryDir directory;
  REQUIRE(directory.isValid());
  REQUIRE(writePng(directory.filePath(QStringLiteral("a.png")), Qt::red));
  REQUIRE(writePng(directory.filePath(QStringLiteral("b.png")), Qt::green));

  const imgview::ImageScan scan = imgview::collectImages({ directory.path() }, true);
  REQUIRE(scan.files.size() == 2);

  imgview::MainWindow window(imgview::Playlist(scan.files, imgview::PlaylistOrder::Sequential), viewOptions(0, true));
  window.show();

  auto *status = window.findChild<QLabel *>(QStringLiteral("status"));
  REQUIRE(status != nullptr);
  REQUIRE(status->isVisible());
  REQUIRE(status->text().contains(QStringLiteral("a.png")));
  REQUIRE(status->text().contains(QStringLiteral("(1/2)")));
  // The details the title bar cannot carry: the pixel size and the file size.
  REQUIRE(status->text().contains(
    QStringLiteral("%1x%2").arg(kTestImageWidth).arg(kTestImageHeight)));
  REQUIRE(status->text().contains(QStringLiteral("bytes")));
  // At the fit there is no zoom to report; once there is one, it is shown.
  REQUIRE_FALSE(status->text().contains(QLatin1Char('%')));

  sendKey(&window, Qt::Key_Plus);
  REQUIRE(status->text().contains(QStringLiteral("125%")));

  sendKey(&window, Qt::Key_Right);

  REQUIRE(status->text().contains(QStringLiteral("b.png")));
  REQUIRE(status->text().contains(QStringLiteral("(2/2)")));
  REQUIRE(window.windowTitle().contains(QStringLiteral("b.png")));
  // A new image is back at the fit, so there is nothing zoomed to report.
  REQUIRE_FALSE(status->text().contains(QLatin1Char('%')));
}

TEST_CASE("The v key toggles the overlay while the window is open", "[gui]")
{
  const QTemporaryDir directory;
  REQUIRE(directory.isValid());
  REQUIRE(writePng(directory.filePath(QStringLiteral("a.png")), Qt::red));

  const imgview::ImageScan scan = imgview::collectImages({ directory.path() }, true);
  REQUIRE(scan.files.size() == 1);

  // Started without --verbose: the details are only in the title.
  imgview::MainWindow window(imgview::Playlist(scan.files, imgview::PlaylistOrder::Sequential), viewOptions());
  window.show();

  auto *status = window.findChild<QLabel *>(QStringLiteral("status"));
  REQUIRE(status != nullptr);
  REQUIRE_FALSE(status->isVisible());

  SECTION("pressing v draws the details, a second time takes them away again")
  {
    sendKey(&window, Qt::Key_V);
    REQUIRE(status->isVisible());
    REQUIRE(status->text().contains(QStringLiteral("a.png")));
    REQUIRE(status->text().contains(
      QStringLiteral("%1x%2").arg(kTestImageWidth).arg(kTestImageHeight)));

    sendKey(&window, Qt::Key_V);
    REQUIRE_FALSE(status->isVisible());
    // The title is not part of the overlay, so it stays put either way.
    REQUIRE(window.windowTitle().contains(QStringLiteral("a.png")));
  }
}

TEST_CASE("A small image is scaled up to fill the window", "[gui]")
{
  const QTemporaryDir directory;
  REQUIRE(directory.isValid());
  REQUIRE(writePng(directory.filePath(QStringLiteral("a.png")), Qt::red));

  imgview::ImageView view;
  REQUIRE(view.load(directory.filePath(QStringLiteral("a.png"))));
  // 3:2 is the image's own aspect ratio, and 240x160 is much larger than 48x32.
  view.resize(240, 160);
  view.show();

  const QImage shown = view.grab().toImage();
  REQUIRE_FALSE(shown.isNull());

  // Every corner is image, not the black background: the picture was blown up
  // to the window instead of sitting in the middle at its own 48x32 size.
  const std::array<QPoint, 4> corners{ QPoint(2, 2),
    QPoint(shown.width() - 3, 2),
    QPoint(2, shown.height() - 3),
    QPoint(shown.width() - 3, shown.height() - 3) };
  for (const QPoint &corner : corners) {
    REQUIRE(shown.pixelColor(corner).red() > 0x80);
  }
}

TEST_CASE("Scaling up keeps the aspect ratio", "[gui]")
{
  const QTemporaryDir directory;
  REQUIRE(directory.isValid());
  REQUIRE(writePng(directory.filePath(QStringLiteral("a.png")), Qt::red));

  imgview::ImageView view;
  REQUIRE(view.load(directory.filePath(QStringLiteral("a.png"))));
  view.resize(200, 200);
  view.show();

  const QImage shown = view.grab().toImage();
  REQUIRE_FALSE(shown.isNull());

  // A 3:2 image in a square window is as wide as the window and black above and
  // below, rather than stretched to fill it.
  REQUIRE(shown.pixelColor(shown.width() / 2, 2).red() < 0x20);
  REQUIRE(shown.pixelColor(shown.width() / 2, shown.height() / 2).red() > 0x80);
  REQUIRE(shown.pixelColor(shown.width() / 2, shown.height() - 3).red() < 0x20);
}

TEST_CASE("Enter and s start and stop the slideshow", "[gui]")
{
  const QTemporaryDir directory;
  REQUIRE(directory.isValid());
  writeTwoImages(directory);
  const imgview::ImageScan scan = scanTwoImages(directory);

  // An interval long enough that the slideshow cannot advance during the test:
  // what is being tested is the switch, not the timer firing.
  imgview::MainWindow window(imgview::Playlist(scan.files, imgview::PlaylistOrder::Sequential), viewOptions(600000));
  window.show();
  REQUIRE_FALSE(window.slideshowRunning());

  for (const int key : { Qt::Key_Return, Qt::Key_Enter, Qt::Key_S }) {
    sendKey(&window, key);
    REQUIRE(window.slideshowRunning());

    sendKey(&window, key);
    REQUIRE_FALSE(window.slideshowRunning());
  }
}

TEST_CASE("The wheel zooms in and out", "[gui]")
{
  const QTemporaryDir directory;
  REQUIRE(directory.isValid());
  writeTwoImages(directory);
  const imgview::ImageScan scan = scanTwoImages(directory);

  imgview::MainWindow window(imgview::Playlist(scan.files, imgview::PlaylistOrder::Sequential), viewOptions());
  window.show();

  auto *view = window.findChild<imgview::ImageView *>();
  REQUIRE(view != nullptr);

  sendWheel(&window, 120, Qt::NoModifier);
  REQUIRE(view->zoom() > 1.0);
  // Zooming is not paging: the same image, only larger.
  REQUIRE(view->path() == scan.files.at(0));

  sendWheel(&window, -120, Qt::NoModifier);
  REQUIRE(view->zoom() == Catch::Approx(1.0));
  REQUIRE(view->path() == scan.files.at(0));
}

TEST_CASE("Control turns the wheel into paging", "[gui]")
{
  const QTemporaryDir directory;
  REQUIRE(directory.isValid());
  writeTwoImages(directory);
  const imgview::ImageScan scan = scanTwoImages(directory);

  imgview::MainWindow window(imgview::Playlist(scan.files, imgview::PlaylistOrder::Sequential), viewOptions());
  window.show();

  auto *view = window.findChild<imgview::ImageView *>();
  REQUIRE(view != nullptr);

  sendWheel(&window, -120, Qt::ControlModifier);
  REQUIRE(view->path() == scan.files.at(1));
  REQUIRE(view->zoom() == Catch::Approx(1.0));

  sendWheel(&window, 120, Qt::ControlModifier);
  REQUIRE(view->path() == scan.files.at(0));
}

TEST_CASE("Zooming is per image, and comes back to the fit", "[gui]")
{
  const QTemporaryDir directory;
  REQUIRE(directory.isValid());
  writeTwoImages(directory);
  const imgview::ImageScan scan = scanTwoImages(directory);

  imgview::MainWindow window(imgview::Playlist(scan.files, imgview::PlaylistOrder::Sequential), viewOptions());
  window.show();

  auto *view = window.findChild<imgview::ImageView *>();
  REQUIRE(view != nullptr);

  sendWheel(&window, 120, Qt::NoModifier);
  sendWheel(&window, 120, Qt::NoModifier);
  REQUIRE(view->zoom() > 1.5);

  sendWheel(&window, -120, Qt::NoModifier);
  sendWheel(&window, -120, Qt::NoModifier);
  // Not "about one": the fitted size is what the two directions have to meet at,
  // or the picture would sit a fraction of a pixel off the window from now on.
  REQUIRE(view->zoom() == 1.0);

  sendWheel(&window, 120, Qt::NoModifier);
  sendKey(&window, Qt::Key_Right);
  REQUIRE(view->path() == scan.files.at(1));
  REQUIRE(view->zoom() == 1.0);
}

TEST_CASE("A zoomed image is drawn larger than the window", "[gui]")
{
  const QTemporaryDir directory;
  REQUIRE(directory.isValid());
  REQUIRE(writePng(directory.filePath(QStringLiteral("a.png")), Qt::red));

  imgview::ImageView view;
  REQUIRE(view.load(directory.filePath(QStringLiteral("a.png"))));
  view.resize(200, 200);
  view.show();

  // At the fit, the 3:2 image leaves a black band at the top.
  REQUIRE(view.grab().toImage().pixelColor(100, 2).red() < 0x20);

  view.zoomIn();
  view.zoomIn();
  REQUIRE(view.zoom() > 1.5);

  // Zoomed past the window, the image covers the corners and hangs over the edges.
  const QImage zoomed = view.grab().toImage();
  REQUIRE(zoomed.pixelColor(2, 2).red() > 0x80);
  REQUIRE(zoomed.pixelColor(zoomed.width() - 3, zoomed.height() - 3).red() > 0x80);

  // ... and zooming all the way back in the other direction restores the fit.
  view.zoomOut();
  view.zoomOut();
  REQUIRE(view.zoom() == 1.0);
  REQUIRE(view.grab().toImage().pixelColor(100, 2).red() < 0x20);
}

TEST_CASE("The context menu offers each command with the key that runs it", "[gui]")
{
  const QTemporaryDir directory;
  REQUIRE(directory.isValid());
  writeTwoImages(directory);
  const imgview::ImageScan scan = scanTwoImages(directory);

  imgview::MainWindow window(imgview::Playlist(scan.files, imgview::PlaylistOrder::Sequential), viewOptions());
  window.show();

  auto *menu = window.findChild<QMenu *>();
  REQUIRE(menu != nullptr);
  REQUIRE(menu->actions().size() == static_cast<int>(imgview::commands().size()));

  // One entry per command, in the same order as --help, each showing the key that
  // runs it: that is what makes the menu a reminder rather than a second way in.
  for (int index = 0; index < menu->actions().size(); ++index) { checkMenuItem(menu, index); }
}

TEST_CASE("Choosing a menu entry does what the key does", "[gui]")
{
  const QTemporaryDir directory;
  REQUIRE(directory.isValid());
  writeTwoImages(directory);
  const imgview::ImageScan scan = scanTwoImages(directory);

  imgview::MainWindow window(imgview::Playlist(scan.files, imgview::PlaylistOrder::Sequential), viewOptions());
  window.show();

  auto *view = window.findChild<imgview::ImageView *>();
  auto *menu = window.findChild<QMenu *>();
  REQUIRE(view != nullptr);
  REQUIRE(menu != nullptr);

  QAction *next = menu->actions().at(0);
  REQUIRE(next->text() == QStringLiteral("Next image"));
  next->trigger();

  REQUIRE(view->path() == scan.files.at(1));
}

TEST_CASE("A right click opens the context menu", "[gui]")
{
  const QTemporaryDir directory;
  REQUIRE(directory.isValid());
  writeTwoImages(directory);
  const imgview::ImageScan scan = scanTwoImages(directory);

  imgview::MainWindow window(imgview::Playlist(scan.files, imgview::PlaylistOrder::Sequential), viewOptions());
  window.show();

  auto *menu = window.findChild<QMenu *>();
  REQUIRE(menu != nullptr);
  REQUIRE_FALSE(menu->isVisible());

  const QPoint position(10, 10);
  QContextMenuEvent event(QContextMenuEvent::Mouse, position, window.mapToGlobal(position));
  QCoreApplication::sendEvent(&window, &event);

  REQUIRE(menu->isVisible());
  menu->hide();
}
