#include "catch_qt.h"

#include "framelesswindow.h"
#include "imagefiles.h"
#include "imageview.h"
#include "mainwindow.h"
#include "playlist.h"

#include <catch2/catch_test_macros.hpp>

#include <QColor>
#include <QCoreApplication>
#include <QImage>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QTemporaryDir>

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

  imgview::MainWindow window(imgview::Playlist(scan.files, imgview::PlaylistOrder::Sequential), 0, false);
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
  imgview::MainWindow window(imgview::Playlist({}, imgview::PlaylistOrder::Sequential), 0, false);
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

  imgview::MainWindow window(imgview::Playlist(scan.files, imgview::PlaylistOrder::Sequential), 0, true);
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

  sendKey(&window, Qt::Key_Right);

  REQUIRE(status->text().contains(QStringLiteral("b.png")));
  REQUIRE(status->text().contains(QStringLiteral("(2/2)")));
  REQUIRE(window.windowTitle().contains(QStringLiteral("b.png")));
}

TEST_CASE("The v key toggles the overlay while the window is open", "[gui]")
{
  const QTemporaryDir directory;
  REQUIRE(directory.isValid());
  REQUIRE(writePng(directory.filePath(QStringLiteral("a.png")), Qt::red));

  const imgview::ImageScan scan = imgview::collectImages({ directory.path() }, true);
  REQUIRE(scan.files.size() == 1);

  // Started without --verbose: the details are only in the title.
  imgview::MainWindow window(imgview::Playlist(scan.files, imgview::PlaylistOrder::Sequential), 0, false);
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
  const QPoint corners[] = { QPoint(2, 2),
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
