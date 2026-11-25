#ifndef QTIMAGEVIEWER_MAINWINDOW_H
#define QTIMAGEVIEWER_MAINWINDOW_H

#include "playlist.h"

#include <QTimer>
#include <QWidget>

class QKeyEvent;
class QWheelEvent;

namespace imgview {

class ImageView;

/*!
 * Shows one image of a playlist and moves through it, on request or on a timer.
 */
class MainWindow : public QWidget
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

private:
  void showNext(bool automatic);
  void showPrevious();
  void showCurrent();
  void restartCycle();
  void toggleFullScreen();

  Playlist m_playlist;
  ImageView *m_view;
  QTimer m_slideshow;
};

}// namespace imgview

#endif
