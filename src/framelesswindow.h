#ifndef QTIMAGEVIEWER_FRAMELESSWINDOW_H
#define QTIMAGEVIEWER_FRAMELESSWINDOW_H

#include <QPoint>
#include <QRect>
#include <QWidget>

namespace imgview {

/*!
 * A top-level window without the frame the window system would draw around it:
 * no title bar, no borders.
 *
 * The window can still be dragged and resized with the mouse. Pressing with the
 * left button near an edge or a corner resizes; pressing anywhere else moves the
 * window; a double click toggles full screen. Where the platform supports it the
 * window system performs the move or resize (QWindow::startSystemMove() and
 * QWindow::startSystemResize(), which is what keeps snapping and tiling
 * working); everywhere else this class moves or resizes the window itself, so a
 * frameless window stays usable on a bare X server or an offscreen platform.
 */
class FramelessWindow : public QWidget
{
  Q_OBJECT

public:
  explicit FramelessWindow(QWidget *parent = nullptr);

protected:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;
  void leaveEvent(QEvent *event) override;

  //! Full screen, or back to a normal window.
  void toggleFullScreen();

private:
  //! What the mouse is doing between a press and its release.
  enum class Grab {
    None,   //!< Nothing: the mouse is only hovering.
    Move,   //!< The window is dragged around.
    Resize, //!< An edge or a corner is dragged.
  };

  //! Thickness, in pixels, of the invisible border used for resizing.
  static constexpr int kResizeBorderWidth = 8;

  //! The edges and corners within kResizeBorderWidth of \a position.
  Qt::Edges edgesAt(const QPointF &position) const;
  //! The cursor that says "this edge resizes", or none for the rest of the window.
  void showResizeCursor(Qt::Edges edges);
  //! The smallest size the window may be dragged down to.
  QSize minimumWindowSize() const;

  void dragTo(const QPointF &globalPosition);
  void resizeTo(const QPointF &globalPosition);

  Grab m_grab = Grab::None;
  //! Whether the window system took over; then the mouse events below stay out of it.
  bool m_systemGrab = false;
  Qt::Edges m_grabEdges;
  //! Cursor position and window geometry when the press happened.
  QPoint m_pressGlobal;
  QRect m_pressGeometry;
};

}// namespace imgview

#endif// QTIMAGEVIEWER_FRAMELESSWINDOW_H
