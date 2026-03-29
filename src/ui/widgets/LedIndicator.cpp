#include "LedIndicator.h"
#include <QPainter>
#include <QPainterPath>
#include <QFontMetrics>

LedIndicator::LedIndicator(const QString& label, QWidget* parent)
    : QWidget(parent), m_label(label)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void LedIndicator::setOn(bool on)
{
    if (m_on == on) return;
    m_on = on;
    update();
}

void LedIndicator::setColors(QColor onColor, QColor offColor)
{
    m_onColor  = onColor;
    m_offColor = offColor;
    update();
}

QSize LedIndicator::sizeHint() const
{
    QFontMetrics fm(font());
    const int w = qMax(40, fm.horizontalAdvance(m_label) + 4);
    return QSize(w, 36);
}

void LedIndicator::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int ledSize = 14;
    const int cx = width() / 2;

    // LED circle
    const QColor base = m_on ? m_onColor : m_offColor;
    QRadialGradient grad(cx - ledSize/4, ledSize/4 + 2, ledSize/2,
                         cx, ledSize/2 + 2, ledSize);
    grad.setColorAt(0.0, base.lighter(160));
    grad.setColorAt(1.0, base.darker(140));

    p.setPen(Qt::NoPen);
    p.setBrush(grad);
    p.drawEllipse(cx - ledSize/2, 2, ledSize, ledSize);

    // Subtle border
    p.setPen(QPen(base.darker(200), 1));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(cx - ledSize/2, 2, ledSize, ledSize);

    // Label
    p.setPen(QColor(200, 200, 200));
    p.setFont(font());
    p.drawText(0, ledSize + 4, width(), height() - ledSize - 4,
               Qt::AlignHCenter | Qt::AlignTop, m_label);
}
