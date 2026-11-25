#ifndef QTIMAGEVIEWER_MAINWINDOW_H
#define QTIMAGEVIEWER_MAINWINDOW_H

#include "framelesswindow.h"
#include "keymap.h"
#include "playlist.h"

#include <QTimer>

class QContextMenuEvent;
class QKeyEvent;
class QLabel;
class QMenu;
class QResizeEvent;
class QWheelEvent;

namespace imgview {

class ImageView;

//! Slideshow interval used when the command line did not give one.
constexpr int kDefaultSlideshowInterval = 5000;

/*!
 * Shows one image of a playlist and moves through it, on request or on a timer.
 *
 * The window has no title bar, so the window title carries the file name and the
 * position in the list for task bars and window lists. In verbose mode the same
 * text, plus the image size, the zoom and the file size, is also drawn over the
 * bottom left corner of the image. The "v" key turns that overlay on and off
 * while the window is open, whichever way it was started.
 *
 * The keys are the ones keymap.cpp lists, and are handled by perform(); the
 * context menu offers the same commands, with the same keys, in the same order.
 */
class MainWindow : public FramelessWindow
{
  Q_OBJECT

public:
  //! How the window starts; the keys and the context menu change all of it later.
  struct Options
  {
    //! Milliseconds between images while the slideshow runs; 0 means kDefaultSlideshowInterval.
    int slideshowInterval = 0;
    //! Whether the slideshow runs from the start (--slideshow).
    bool startSlideshow = false;
    //! Whether the details are drawn over the image from the start (--verbose).
    bool verbose = false;
  };

  MainWindow(Playlist playlist, Options options, QWidget *parent = nullptr);

  //! Whether the slideshow is running: it is started and stopped by a key, the menu or --slideshow.
  [[nodiscard]] bool slideshowRunning() const;

protected:
  void keyPressEvent(QKeyEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  void contextMenuEvent(QContextMenuEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;

private:
  void perform(Command command);

  void showNext(bool automatic);
  void showPrevious();
  void showCurrent();
  void restartCycle();
  void toggleSlideshow();
  //! Flips the overlay on or off, which is what the "v" key is for.
  void toggleVerbose();

  //! Puts the current file name in the window title, and the details over the image when verbose.
  void refreshStatus();
  void placeStatus();

  //! The right-click menu: one entry per command, showing the key that runs it.
  void buildMenu();

  Playlist m_playlist;
  ImageView *m_view;
  //! The corner overlay; it exists either way and is only shown in verbose mode.
  QLabel *m_status;
  QMenu *m_menu;
  QTimer m_slideshow;
  bool m_verbose;
};

}// namespace imgview

#endif
