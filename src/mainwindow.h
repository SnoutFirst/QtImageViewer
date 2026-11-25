#ifndef QTIMAGEVIEWER_MAINWINDOW_H
#define QTIMAGEVIEWER_MAINWINDOW_H

#include "framelesswindow.h"
#include "playlist.h"

#include <QTimer>

class QKeyEvent;
class QLabel;
class QResizeEvent;
class QWheelEvent;

namespace imgview {

class ImageView;

/*!
 * Shows one image of a playlist and moves through it, on request or on a timer.
 *
 * The window has no title bar, so where the file name and the position in the
 * list go is a label in the corner of the window; it fades out of the way by
 * itself. The window title carries the same text for task bars and window lists.
 */
class MainWindow : public FramelessWindow
{
  Q_OBJECT

public:
  /*!
   * \param playlist           The images to show, and in which order.
   * \param slideshowInterval  Milliseconds between automatic advances; 0 turns the slideshow off.
   */
  MainWindow(Playlist playlist, int slideshowInterval, QWidget *parent = nullptr);

protected:
  void keyPressEvent(QKeyEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;

private:
  void showNext(bool automatic);
  void showPrevious();
  void showCurrent();
  void restartCycle();

  //! Puts \a text in the corner overlay and in the window title.
  void setStatus(const QString &text);
  void placeStatus();

  Playlist m_playlist;
  ImageView *m_view;
  QLabel *m_status;
  //! Hides the corner overlay again after a while.
  QTimer m_statusTimeout;
  QTimer m_slideshow;
};

}// namespace imgview

#endif
