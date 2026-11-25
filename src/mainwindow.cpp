#include "mainwindow.h"

#include "imageview.h"

#include <QFileInfo>
#include <QKeyEvent>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <utility>

namespace imgview {

MainWindow::MainWindow(Playlist playlist, int slideshowInterval, QWidget *parent)
  : QWidget(parent), m_playlist(std::move(playlist)), m_view(new ImageView(this)), m_slideshow(this)
{
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(m_view);

  // The window owns the keyboard; the view only paints.
  m_view->setFocusPolicy(Qt::NoFocus);
  setFocusPolicy(Qt::StrongFocus);

  resize(m_view->sizeHint());

  showCurrent();

  if (slideshowInterval > 0) {
    m_slideshow.setInterval(slideshowInterval);
    connect(&m_slideshow, &QTimer::timeout, this, [this] { showNext(true); });
    m_slideshow.start();
  }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
  switch (event->key()) {
  case Qt::Key_Right:
  case Qt::Key_Down:
  case Qt::Key_Space:
  case Qt::Key_PageDown:
  case Qt::Key_N:
    showNext(false);
    break;
  case Qt::Key_Left:
  case Qt::Key_Up:
  case Qt::Key_Backspace:
  case Qt::Key_PageUp:
  case Qt::Key_P:
    showPrevious();
    break;
  case Qt::Key_R:
    restartCycle();
    break;
  case Qt::Key_F:
  case Qt::Key_F11:
    toggleFullScreen();
    break;
  case Qt::Key_Q:
    close();
    break;
  case Qt::Key_Escape:
    // Escape leaves full screen first, and only quits when there is no full
    // screen to leave.
    if (isFullScreen()) {
      showNormal();
    } else {
      close();
    }
    break;
  default:
    QWidget::keyPressEvent(event);
    return;
  }
  event->accept();
}

void MainWindow::wheelEvent(QWheelEvent *event)
{
  const int delta = event->angleDelta().y();
  if (delta == 0) {
    QWidget::wheelEvent(event);
    return;
  }

  if (delta > 0) {
    showPrevious();
  } else {
    showNext(false);
  }
  event->accept();
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

void MainWindow::toggleFullScreen()
{
  if (isFullScreen()) {
    showNormal();
  } else {
    showFullScreen();
  }
}

void MainWindow::showCurrent()
{
  if (m_playlist.isEmpty()) {
    m_view->showMessage(QStringLiteral("No images to show"));
    setWindowTitle(QStringLiteral("QtImageViewer"));
    return;
  }

  const QString path = m_playlist.current();

  // A file that cannot be read is reported by the view itself; the slideshow
  // keeps going rather than stalling on it.
  m_view->load(path);

  setWindowTitle(
    QStringLiteral("%1 (%2/%3)").arg(QFileInfo(path).fileName()).arg(m_playlist.position()).arg(m_playlist.size()));
}

}// namespace imgview
