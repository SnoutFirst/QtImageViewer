#include "mainwindow.h"

#include "imageview.h"

#include <QFileInfo>
#include <QKeyEvent>
#include <QLabel>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <utility>

namespace imgview {
namespace {

//! How long the file name overlay stays on screen, in milliseconds.
constexpr int kStatusTimeout = 2500;
//! Distance between the overlay and the bottom left corner, in pixels.
constexpr int kStatusMargin = 12;

}// namespace

MainWindow::MainWindow(Playlist playlist, int slideshowInterval, QWidget *parent)
  : FramelessWindow(parent), m_playlist(std::move(playlist)), m_view(new ImageView(this)), m_status(new QLabel(this)),
    m_slideshow(this)
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
  m_status->setObjectName(QStringLiteral("status"));
  m_status->setAttribute(Qt::WA_TransparentForMouseEvents, true);
  m_status->setStyleSheet(
    QStringLiteral("QLabel { color: white; background: rgba(0, 0, 0, 170); padding: 4px 8px; border-radius: 4px; }"));
  m_status->hide();
  m_statusTimeout.setSingleShot(true);
  m_statusTimeout.setInterval(kStatusTimeout);
  connect(&m_statusTimeout, &QTimer::timeout, m_status, &QWidget::hide);

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

void MainWindow::resizeEvent(QResizeEvent *event)
{
  FramelessWindow::resizeEvent(event);
  placeStatus();
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
    setStatus(QString());
    return;
  }

  const QString path = m_playlist.current();

  // A file that cannot be read is reported by the view itself; the slideshow
  // keeps going rather than stalling on it.
  m_view->load(path);

  setStatus(QStringLiteral("%1 (%2/%3)")
              .arg(QFileInfo(path).fileName())
              .arg(m_playlist.position())
              .arg(m_playlist.size()));
}

void MainWindow::setStatus(const QString &text)
{
  if (text.isEmpty()) {
    m_statusTimeout.stop();
    m_status->hide();
    setWindowTitle(QStringLiteral("QtImageViewer"));
    return;
  }

  setWindowTitle(text);
  m_status->setText(text);
  m_status->adjustSize();
  placeStatus();
  m_status->raise();
  m_status->show();
  m_statusTimeout.start();
}

void MainWindow::placeStatus()
{
  if (m_status == nullptr) { return; }

  m_status->move(kStatusMargin, height() - m_status->height() - kStatusMargin);
}

}// namespace imgview
