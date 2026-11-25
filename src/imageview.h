#ifndef QTIMAGEVIEWER_IMAGEVIEW_H
#define QTIMAGEVIEWER_IMAGEVIEW_H

#include <QImage>
#include <QPixmap>
#include <QString>
#include <QWidget>

class QPainter;

namespace imgview {

/*!
 * Paints a single image on a black background: scaled down to fit the widget,
 * never scaled up beyond its own size, and centred. When there is no image to
 * paint it shows a text message instead.
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

  QString path() const;

  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;

private:
  void rescale();
  void paintMessage(QPainter *painter) const;

  QImage m_image;
  QPixmap m_scaled;
  QString m_path;
  QString m_message;
  bool m_scaledDirty = true;
};

}// namespace imgview

#endif
