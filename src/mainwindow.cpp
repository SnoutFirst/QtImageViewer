#include "mainwindow.h"

#include "imageview.h"

#include <QAction>
#include <QContextMenuEvent>
#include <QFileInfo>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QLocale>
#include <QMenu>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <optional>
#include <utility>

namespace imgview {
namespace {

//! Distance between the overlay and the bottom left corner, in pixels.
constexpr int kStatusMargin = 12;

}// namespace

MainWindow::MainWindow(Playlist playlist, Options options, QWidget *parent)
  : FramelessWindow(parent), m_playlist(std::move(playlist)), m_view(new ImageView(this)),
    m_status(new QLabel(this)), m_menu(new QMenu(this)), m_slideshow(this), m_verbose(options.verbose)
{
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(m_view);

  // The window owns the mouse (it drags and resizes itself) and the keyboard;
  // the view only paints. A window this small looks odd, so ask for a bit more.
  m_view->setFocusPolicy(Qt::NoFocus);
  m_view->setAttribute(Qt::WA_TransparentForMouseEvents, true);
  setFocusPolicy(Qt::StrongFocus);
  setMinimumSize(200, 150);

  // The overlay is a child of the window rather than part of the layout: it
  // floats over the image instead of taking a strip of the window for itself.
  // It exists from the start and is merely hidden, so "v" can bring it back.
  m_status->setObjectName(QStringLiteral("status"));
  m_status->setAttribute(Qt::WA_TransparentForMouseEvents, true);
  m_status->setStyleSheet(
    QStringLiteral("QLabel { color: white; background: rgba(0, 0, 0, 170); padding: 4px 8px; border-radius: 4px; }"));

  buildMenu();

  resize(m_view->sizeHint());

  showCurrent();

  m_slideshow.setInterval(options.slideshowInterval > 0 ? options.slideshowInterval : kDefaultSlideshowInterval);
  connect(&m_slideshow, &QTimer::timeout, this, [this] { showNext(true); });
  if (options.startSlideshow) { m_slideshow.start(); }
}

bool MainWindow::slideshowRunning() const { return m_slideshow.isActive(); }

void MainWindow::buildMenu()
{
  for (const Command command : commands()) {
    QAction *action = m_menu->addAction(commandDescription(command));
    // The menu draws this itself, in its shortcut column -- and while the menu
    // is open it works, which is no bad thing.
    action->setShortcut(QKeySequence(primaryKey(command)));
    connect(action, &QAction::triggered, this, [this, command] { perform(command); });
  }
}

void MainWindow::perform(Command command)
{
  switch (command) {
  case Command::NextImage:
    showNext(false);
    break;
  case Command::PreviousImage:
    showPrevious();
    break;
  case Command::ToggleSlideshow:
    toggleSlideshow();
    break;
  case Command::ToggleFullScreen:
    toggleFullScreen();
    break;
  case Command::ToggleVerbose:
    toggleVerbose();
    break;
  case Command::RestartCycle:
    restartCycle();
    break;
  case Command::ZoomIn:
    m_view->zoomIn();
    refreshStatus();
    break;
  case Command::ZoomOut:
    m_view->zoomOut();
    refreshStatus();
    break;
  case Command::Quit:
    close();
    break;
  }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
  // Escape leaves full screen first, and only quits when there is no full
  // screen to leave: it is the one key that does something different depending
  // on where the window is, which is why it is not in the table.
  if (event->key() == Qt::Key_Escape) {
    if (isFullScreen()) {
      showNormal();
    } else {
      close();
    }
    event->accept();
    return;
  }

  if (const std::optional<Command> command = commandForKey(event->key())) {
    perform(*command);
    event->accept();
    return;
  }

  QWidget::keyPressEvent(event);
}

void MainWindow::wheelEvent(QWheelEvent *event)
{
  const int delta = event->angleDelta().y();
  if (delta == 0) {
    QWidget::wheelEvent(event);
    return;
  }

  // The wheel zooms; with Control held it steps through the playlist instead,
  // which is where the wheel used to be and what a touchpad still expects from
  // it. Either way the step is in the direction the wheel was rolled.
  if (event->modifiers().testFlag(Qt::ControlModifier)) {
    if (delta > 0) {
      showPrevious();
    } else {
      showNext(false);
    }
  } else if (delta > 0) {
    perform(Command::ZoomIn);
  } else {
    perform(Command::ZoomOut);
  }
  event->accept();
}

void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
  m_menu->popup(event->globalPos());
  event->accept();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
  FramelessWindow::resizeEvent(event);
  placeStatus();
}

void MainWindow::toggleSlideshow()
{
  if (m_slideshow.isActive()) {
    m_slideshow.stop();
  } else {
    m_slideshow.start();
  }
}

void MainWindow::showNext(bool automatic)
{
  if (m_playlist.isEmpty()) { return; }

  m_playlist.advance();
  showCurrent();

  if (!automatic && m_slideshow.isActive()) {
    // A manual step restarts the countdown: the user just looked at this image
    // and should get the whole interval to look at the next one.
    m_slideshow.start();
  }
}

void MainWindow::showPrevious()
{
  if (m_playlist.isEmpty()) { return; }

  m_playlist.retreat();
  showCurrent();

  if (m_slideshow.isActive()) { m_slideshow.start(); }
}

void MainWindow::restartCycle()
{
  if (m_playlist.isEmpty()) { return; }

  m_playlist.restart();
  showCurrent();
}

void MainWindow::showCurrent()
{
  if (m_playlist.isEmpty()) {
    m_view->showMessage(QStringLiteral("No images to show"));
    refreshStatus();
    return;
  }

  // A file that cannot be read is reported by the view itself; the slideshow
  // keeps going rather than stalling on it.
  m_view->load(m_playlist.current());
  refreshStatus();
}

void MainWindow::toggleVerbose()
{
  m_verbose = !m_verbose;
  refreshStatus();
}

void MainWindow::refreshStatus()
{
  if (m_playlist.isEmpty()) {
    setWindowTitle(QStringLiteral("QtImageViewer"));
    m_status->clear();
    m_status->hide();
    return;
  }

  const QFileInfo file(m_playlist.current());
  const QString text =
    QStringLiteral("%1 (%2/%3)").arg(file.fileName()).arg(m_playlist.position()).arg(m_playlist.size());

  // Task bars and window lists show the title, where the details would only be
  // in the way; the overlay is the place for them.
  setWindowTitle(text);

  if (!m_verbose) {
    m_status->hide();
    return;
  }

  // Verbose mode is the place for the details that would be in the way of the
  // picture: how big it is, how far it is zoomed, and how big the file is.
  QString overlay = text;
  const QSize imageSize = m_view->imageSize();
  if (imageSize.isValid()) {
    overlay += QStringLiteral("  %1x%2").arg(imageSize.width()).arg(imageSize.height());
  }
  if (m_view->zoom() != 1.0) {
    overlay += QStringLiteral("  %1%").arg(qRound(m_view->zoom() * 100.0));
  }
  if (file.size() > 0) {
    overlay += QStringLiteral("  %1").arg(QLocale().formattedDataSize(file.size(), 1));
  }

  m_status->setText(overlay);
  m_status->adjustSize();
  placeStatus();
  m_status->raise();
  m_status->show();
}

void MainWindow::placeStatus()
{
  m_status->move(kStatusMargin, height() - m_status->height() - kStatusMargin);
}

}// namespace imgview
