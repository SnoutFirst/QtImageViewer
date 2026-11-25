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
 * corner of the image. The "v" key turns that overlay on and off while the window
 * is open, whichever way it was started.
 */
class MainWindow : public FramelessWindow
{
  Q_OBJECT

public:
  /*!
   * \param playlist           The images to show, and in which order.
   * \param slideshowInterval  Milliseconds between automatic advances; 0 turns the slideshow off.
   * \param verbose            Whether to draw the file name and the image details over the image
   *                           from the start; the "v" key toggles it while the window is open.
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
  //! Flips the overlay on or off, which is what the "v" key is for.
  void toggleVerbose();

  //! Puts the current file name in the window title, and the details over the image when verbose.
  void refreshStatus();
  void placeStatus();

  Playlist m_playlist;
  ImageView *m_view;
  //! The corner overlay; it exists either way and is only shown in verbose mode.
  QLabel *m_status;
  QTimer m_slideshow;
  bool m_verbose;
};

}// namespace imgview

#endif
