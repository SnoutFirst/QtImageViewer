#include "imageview.h"

#include <QDir>
#include <QFontMetrics>
#include <QImageReader>
#include <QPainter>
#include <QResizeEvent>

#include <algorithm>
#include <cmath>
#include <utility>

namespace imgview {
namespace {

//! How much one zoom step changes the picture, and how far the steps may go.
constexpr qreal kZoomStep = 1.25;
constexpr qreal kMinimumZoom = 0.1;
constexpr qreal kMaximumZoom = 10.0;

//! Up to this many pixels in the scaled copy; a 48 megapixel photograph zoomed
//! ten times over would otherwise ask Qt for a pixmap of two gigabytes.
constexpr qint64 kMaximumScaledPixels = 64LL * 1024 * 1024;

}// namespace

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
  // Every image starts at the fit, so stepping through a directory does not
  // leave the next picture at whatever zoom the last one was looked at.
  m_zoom = 1.0;
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
  m_zoom = 1.0;
  m_scaledDirty = false;
  update();
}

void ImageView::zoomIn() { setZoom(m_zoom * kZoomStep); }

void ImageView::zoomOut() { setZoom(m_zoom / kZoomStep); }

qreal ImageView::zoom() const { return m_zoom; }

void ImageView::setZoom(qreal zoom)
{
  const qreal clamped = std::clamp(zoom, kMinimumZoom, kMaximumZoom);

  // Stepping in and back out has to come back to the fit exactly, or the picture
  // stays a fraction of a pixel off the window for the rest of the session.
  const qreal settled = qAbs(clamped - 1.0) < 0.001 ? 1.0 : clamped;
  if (settled == m_zoom) { return; }

  m_zoom = settled;
  m_scaledDirty = true;
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
 * Renders the image at the widget's size in device pixels, as large as it fits:
 * the image is scaled up to fill the window as well as down, so a small picture
 * cannot get lost in the middle of a maximised window (fbi's "-a", effectively).
 * The aspect ratio is never touched, so the letterboxing that follows from it
 * stays black. Zooming multiplies that fitted size, which is why 1.0 -- the
 * value every image starts at -- is always exactly "the whole window".
 */
void ImageView::rescale()
{
  m_scaledDirty = false;
  m_scaled = QPixmap();

  if (m_image.isNull() || width() <= 0 || height() <= 0) { return; }

  const qreal ratio = devicePixelRatioF();
  const QSize available(qRound(static_cast<qreal>(width()) * ratio), qRound(static_cast<qreal>(height()) * ratio));

  const QSize fitted = m_image.size().scaled(available, Qt::KeepAspectRatio);

  // A zoomed image is usually larger than the widget: it is still drawn from the
  // middle, and the widget clips what hangs over the edges.
  QSize size(qRound(static_cast<qreal>(fitted.width()) * m_zoom), qRound(static_cast<qreal>(fitted.height()) * m_zoom));
  size = size.expandedTo(QSize(1, 1));

  const qint64 pixels = static_cast<qint64>(size.width()) * size.height();
  if (pixels > kMaximumScaledPixels) {
    const qreal shrink = std::sqrt(static_cast<qreal>(kMaximumScaledPixels) / static_cast<qreal>(pixels));
    size = QSize(qRound(static_cast<qreal>(size.width()) * shrink), qRound(static_cast<qreal>(size.height()) * shrink))
             .expandedTo(QSize(1, 1));
  }

  // IgnoreAspectRatio because \a size came out of a ratio-preserving computation
  // already; asking for KeepAspectRatio again could only round it down further.
  m_scaled = QPixmap::fromImage(m_image.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
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
