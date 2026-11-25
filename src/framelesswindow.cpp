#include "framelesswindow.h"

#include <QEvent>
#include <QMouseEvent>
#include <QWindow>

#include <algorithm>

namespace imgview {
namespace {

// Qt 6 renamed the accessors: QMouseEvent::position() is the old localPos() and
// globalPosition() is the old globalPos(), which was an integer point.
QPointF localPosition(const QMouseEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  return event->position();
#else
  return event->localPos();
#endif
}

QPointF globalPosition(const QMouseEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  return event->globalPosition();
#else
  return QPointF(event->globalPos());
#endif
}

}// namespace

FramelessWindow::FramelessWindow(QWidget *parent) : QWidget(parent)
{
  setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

  // Hovering has to work without a button being held down, both for the resize
  // cursors and because a press away from the edges starts a move.
  setMouseTracking(true);
}

Qt::Edges FramelessWindow::edgesAt(const QPointF &position) const
{
  if (isFullScreen()) {
    // A full screen window fills the screen: there is nothing to resize.
    return {};
  }

  Qt::Edges edges;

  if (position.x() < kResizeBorderWidth) {
    edges |= Qt::LeftEdge;
  } else if (position.x() >= width() - kResizeBorderWidth) {
    edges |= Qt::RightEdge;
  }

  if (position.y() < kResizeBorderWidth) {
    edges |= Qt::TopEdge;
  } else if (position.y() >= height() - kResizeBorderWidth) {
    edges |= Qt::BottomEdge;
  }

  return edges;
}

void FramelessWindow::showResizeCursor(Qt::Edges edges)
{
  const bool left = edges & Qt::LeftEdge;
  const bool right = edges & Qt::RightEdge;
  const bool top = edges & Qt::TopEdge;
  const bool bottom = edges & Qt::BottomEdge;

  if ((left && top) || (right && bottom)) {
    setCursor(Qt::SizeFDiagCursor);
  } else if ((right && top) || (left && bottom)) {
    setCursor(Qt::SizeBDiagCursor);
  } else if (left || right) {
    setCursor(Qt::SizeHorCursor);
  } else if (top || bottom) {
    setCursor(Qt::SizeVerCursor);
  } else {
    unsetCursor();
  }
}

QSize FramelessWindow::minimumWindowSize() const
{
  // minimumWidth() does not include what the layout needs, minimumSizeHint()
  // does; the window may be dragged down to whichever is larger.
  return { std::max(minimumWidth(), minimumSizeHint().width()), std::max(minimumHeight(), minimumSizeHint().height()) };
}

void FramelessWindow::mousePressEvent(QMouseEvent *event)
{
  if (event->button() != Qt::LeftButton || isFullScreen()) {
    QWidget::mousePressEvent(event);
    return;
  }

  m_pressGlobal = globalPosition(event).toPoint();
  m_pressGeometry = geometry();
  m_grabEdges = edgesAt(localPosition(event));
  m_grab = m_grabEdges ? Grab::Resize : Grab::Move;

  // Let the window system do the work where it can: that is what keeps snapping,
  // tiling and the native resize behaviour. Everywhere else the handlers below
  // do it themselves, which is also what happens when there is no window manager.
  m_systemGrab = false;
  if (QWindow *handle = windowHandle()) {
    m_systemGrab = m_grabEdges ? handle->startSystemResize(m_grabEdges) : handle->startSystemMove();
  }

  event->accept();
}

void FramelessWindow::mouseMoveEvent(QMouseEvent *event)
{
  if (m_grab == Grab::None) {
    showResizeCursor(edgesAt(localPosition(event)));
    QWidget::mouseMoveEvent(event);
    return;
  }

  if (!m_systemGrab) {
    if (m_grab == Grab::Move) {
      dragTo(globalPosition(event));
    } else {
      resizeTo(globalPosition(event));
    }
  }

  event->accept();
}

void FramelessWindow::mouseReleaseEvent(QMouseEvent *event)
{
  if (event->button() != Qt::LeftButton || m_grab == Grab::None) {
    QWidget::mouseReleaseEvent(event);
    return;
  }

  m_grab = Grab::None;
  m_systemGrab = false;
  m_grabEdges = {};

  // The cursor has to catch up with wherever the mouse ended up.
  showResizeCursor(edgesAt(localPosition(event)));
  event->accept();
}

void FramelessWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
  if (event->button() != Qt::LeftButton || m_grabEdges) {
    // A double click on an edge belongs to the resize, not to full screen.
    QWidget::mouseDoubleClickEvent(event);
    return;
  }

  m_grab = Grab::None;
  m_systemGrab = false;
  toggleFullScreen();
  event->accept();
}

void FramelessWindow::leaveEvent(QEvent *event)
{
  if (m_grab == Grab::None) {
    unsetCursor();
  }
  QWidget::leaveEvent(event);
}

void FramelessWindow::toggleFullScreen()
{
  if (isFullScreen()) {
    showNormal();
  } else {
    showFullScreen();
  }
}

void FramelessWindow::dragTo(const QPointF &globalPosition)
{
  move(m_pressGeometry.topLeft() + (globalPosition.toPoint() - m_pressGlobal));
}

void FramelessWindow::resizeTo(const QPointF &globalPosition)
{
  const QPoint delta = globalPosition.toPoint() - m_pressGlobal;
  const QSize minimum = minimumWindowSize();
  QRect geometry = m_pressGeometry;

  // Dragging the left or top edge moves the window as well as changing its size,
  // so the opposite edge has to stay where it is: move the edge, then clamp it.
  if (m_grabEdges & Qt::LeftEdge) {
    geometry.setLeft(std::min(geometry.left() + delta.x(), geometry.right() - minimum.width() + 1));
  } else if (m_grabEdges & Qt::RightEdge) {
    geometry.setRight(std::max(geometry.right() + delta.x(), geometry.left() + minimum.width() - 1));
  }

  if (m_grabEdges & Qt::TopEdge) {
    geometry.setTop(std::min(geometry.top() + delta.y(), geometry.bottom() - minimum.height() + 1));
  } else if (m_grabEdges & Qt::BottomEdge) {
    geometry.setBottom(std::max(geometry.bottom() + delta.y(), geometry.top() + minimum.height() - 1));
  }

  setGeometry(geometry);
}

}// namespace imgview
