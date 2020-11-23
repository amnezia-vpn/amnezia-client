#include "customshadoweffect.h"
#include <QPainter>

CustomShadowEffect::CustomShadowEffect(QObject *parent) :
    QGraphicsEffect(parent),
    _distance(4.0f),
    _blurRadius(10.0f),
    _color(0, 0, 0, 80)
{
}

QT_BEGIN_NAMESPACE
  extern Q_WIDGETS_EXPORT void qt_blurImage(QPainter *p, QImage &blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0 );
QT_END_NAMESPACE

void CustomShadowEffect::draw(QPainter* painter)
{
    // if nothing to show outside the item, just draw source
    if ((blurRadius() + distance()) <= 0) {
        drawSource(painter);
        return;
    }

    PixmapPadMode mode = QGraphicsEffect::PadToEffectiveBoundingRect;
    QPoint offset;
    const QPixmap px = sourcePixmap(Qt::DeviceCoordinates, &offset, mode);

    // return if no source
    if (px.isNull())
        return;

    // save world transform
    QTransform restoreTransform = painter->worldTransform();
    painter->setWorldTransform(QTransform());

    // Calculate size for the background image
    QSize szi(px.size().width() + 2 * distance(), px.size().height() + 2 * distance());

    QImage tmp(szi, QImage::Format_ARGB32_Premultiplied);
    QPixmap scaled = px.scaled(szi);
    tmp.fill(0);
    QPainter tmpPainter(&tmp);
    tmpPainter.setCompositionMode(QPainter::CompositionMode_Source);
    tmpPainter.drawPixmap(QPointF(-distance(), -distance()), scaled);
    tmpPainter.end();

    // blur the alpha channel
    QImage blurred(tmp.size(), QImage::Format_ARGB32_Premultiplied);
    blurred.fill(0);
    QPainter blurPainter(&blurred);
    qt_blurImage(&blurPainter, tmp, blurRadius(), false, true);
    blurPainter.end();

    tmp = blurred;

    // blacken the image...
    tmpPainter.begin(&tmp);
    tmpPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    tmpPainter.fillRect(tmp.rect(), color());
    tmpPainter.end();

    // draw the blurred shadow...
    painter->drawImage(offset, tmp);

    // draw the actual pixmap...
    painter->drawPixmap(offset, px, QRectF());

    // restore world transform
    painter->setWorldTransform(restoreTransform);
}

QRectF CustomShadowEffect::boundingRectFor(const QRectF& rect) const
{
    qreal delta = blurRadius() + distance();
    return rect.united(rect.adjusted(-delta, -delta, delta, delta));
}
