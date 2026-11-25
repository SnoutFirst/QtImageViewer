#include "imageview.h"

#include <QDir>
#include <QFontMetrics>
#include <QImageReader>
#include <QPainter>
#include <QResizeEvent>

#include <utility>

namespace imgview {

ImageView::ImageView(QWidget *parent) : QWidget(parent)
{
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  setMinimumSize(64, 64);
  setAutoFillBackground(false);
  setAttribute(Qt::WA_OpaquePaintEvent, true);
}

bool ImageView::load(const QString &path)
{
  QImageReader reader(path);
  reader.setAutoTransform(true);// honour the EXIF orientation
  reader.setDecideFormatFromContent(true);// a file name without a useful suffix is still an image
  QImage image = reader.read();

  m_path = path;

  if (image.isNull()) {
    m_size = {};
    m_image = QImage();
    m_scaled = QPixmap();
    m_scaledDirty = false;
    m_message = QStringLiteral("Cannot show\n%1\n\n%2").arg(QDir::toNativeSeparators(path), reader.errorString());
    update();
    return false;
  }

  m_image = std::move(image);
  m_size = m_image.size();
  m_message.clear();
  m_scaledDirty = true;
  update();
  return true;
}

void ImageView::showMessage(const QString &message)
{
  m_path.clear();
  m_message = message;
  m_image = QImage();
  m_size = {};
  m_scaled = QPixmap();
  m_scaledDirty = false;
  update();
}

const QString &ImageView::path() const { return m_path; }

QSize ImageView::imageSize() const { return m_size; }

QSize ImageView::sizeHint() const { return { 960, 640 }; }

void ImageView::paintEvent(QPaintEvent *event)
{
  Q_UNUSED(event)

  QPainter painter(this);
  painter.fillRect(rect(), Qt::black);

  if (m_scaledDirty) { rescale(); }

  if (m_scaled.isNull()) {
    paintMessage(&painter);
    return;
  }

  const qreal ratio = m_scaled.devicePixelRatio();
  const QSize shown(
    qRound(static_cast<qreal>(m_scaled.width()) / ratio), qRound(static_cast<qreal>(m_scaled.height()) / ratio));
  painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
  painter.drawPixmap(QPoint((width() - shown.width()) / 2, (height() - shown.height()) / 2), m_scaled);
}

void ImageView::resizeEvent(QResizeEvent *event)
{
  QWidget::resizeEvent(event);
  m_scaledDirty = true;
}

/*!
 * Renders the image at the widget's size in device pixels. Images smaller than
 * the widget are left at their own size: enlarging a small picture to fill a
 * maximised window only makes it blurry.
 */
void ImageView::rescale()
{
  m_scaledDirty = false;
  m_scaled = QPixmap();

  if (m_image.isNull() || width() <= 0 || height() <= 0) { return; }

  const qreal ratio = devicePixelRatioF();
  const QSize available(qRound(static_cast<qreal>(width()) * ratio), qRound(static_cast<qreal>(height()) * ratio));

  QSize size = m_image.size();
  if (size.width() > available.width() || size.height() > available.height()) {
    size = size.scaled(available, Qt::KeepAspectRatio);
  }

  m_scaled = QPixmap::fromImage(m_image.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  m_scaled.setDevicePixelRatio(ratio);
}

void ImageView::paintMessage(QPainter *painter) const
{
  if (m_message.isEmpty()) { return; }

  painter->setPen(QColor(0xdd, 0xdd, 0xdd));

  const QRectF area = QRectF(rect()).adjusted(24, 24, -24, -24);
  const QFontMetrics metrics = painter->fontMetrics();
  const QStringList lines = m_message.split(QLatin1Char('\n'));

  qreal y = area.center().y() - (static_cast<qreal>(lines.size() * metrics.height()) / 2.0);
  for (const QString &line : lines) {
    const QString elided = metrics.elidedText(line, Qt::ElideMiddle, qRound(area.width()));
    painter->drawText(
      QRectF(area.left(), y, area.width(), static_cast<qreal>(metrics.height())), Qt::AlignCenter, elided);
    y += metrics.height();
  }
}

}// namespace imgview
