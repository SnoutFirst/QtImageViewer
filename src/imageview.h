#ifndef QTIMAGEVIEWER_IMAGEVIEW_H
#define QTIMAGEVIEWER_IMAGEVIEW_H

#include <QImage>
#include <QPixmap>
#include <QString>
#include <QWidget>

class QPainter;

namespace imgview {

/*!
 * Paints a single image on a black background: scaled (up or down, always with
 * its aspect ratio) to fill as much of the widget as it can, and centred. When
 * there is no image to paint it shows a text message instead.
 */
class ImageView : public QWidget
{
  Q_OBJECT

public:
  explicit ImageView(QWidget *parent = nullptr);

  //! Loads \a path, or shows why it cannot be shown and returns false.
  bool load(const QString &path);
  //! Throws the image away and shows \a message instead.
  void showMessage(const QString &message);

  //! One zoom step in; 1.0 is "as large as the window fits", which is where every image starts.
  void zoomIn();
  //! One zoom step out.
  void zoomOut();
  //! The current zoom, as a multiple of the fitted size.
  [[nodiscard]] qreal zoom() const;

  [[nodiscard]] const QString &path() const;
  //! Size of the decoded image; empty when there is nothing (decodable) loaded.
  [[nodiscard]] QSize imageSize() const;

  [[nodiscard]] QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;

private:
  void rescale();
  void setZoom(qreal zoom);
  void paintMessage(QPainter *painter) const;

  QImage m_image;
  QPixmap m_scaled;
  QSize m_size;
  QString m_path;
  QString m_message;
  //! Multiple of the fitted size; 1.0 until the user zooms.
  qreal m_zoom = 1.0;
  bool m_scaledDirty = true;
};

}// namespace imgview

#endif
