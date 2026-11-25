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
 * The window has no title bar, so the window title carries the file name and the
 * position in the list for task bars and window lists. In verbose mode the same
 * text, plus the image size and the file size, is also drawn over the bottom left
 * corner of the image.
 */
class MainWindow : public FramelessWindow
{
  Q_OBJECT

public:
  /*!
   * \param playlist           The images to show, and in which order.
   * \param slideshowInterval  Milliseconds between automatic advances; 0 turns the slideshow off.
   * \param verbose            Whether to draw the file name and the image details over the image.
   */
  MainWindow(Playlist playlist, int slideshowInterval, bool verbose, QWidget *parent = nullptr);

protected:
  void keyPressEvent(QKeyEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;

private:
  void showNext(bool automatic);
  void showPrevious();
  void showCurrent();
  void restartCycle();

  //! Puts \a text in the window title, and in the overlay when there is one.
  void setStatus(const QString &text);
  void placeStatus();

  Playlist m_playlist;
  ImageView *m_view;
  //! The corner overlay, or nullptr unless the window was asked to be verbose.
  QLabel *m_status;
  QTimer m_slideshow;
};

}// namespace imgview

#endif
