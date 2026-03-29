#include "MeterWidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QFontMetrics>
#include <algorithm>
#include <cmath>

MeterWidget::MeterWidget(const QString& label,
                         const QString& unit,
                         double maxValue,
                         QWidget* parent)
    : QWidget(parent)
    , m_label(label)
    , m_unit(unit)
    , m_maxValue(maxValue)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setPeakHold(true);
}

void MeterWidget::setValue(double value)
{
    value = std::clamp(value, 0.0, m_maxValue);
    m_value = value;

    if (m_peakHold && value >= m_peak) {
        m_peak = value;
        if (m_peakTimer) killTimer(m_peakTimer);
        m_peakTimer = startTimer(k_peakHoldMs);
    }
    update();
}

void MeterWidget::setMaxValue(double maxValue)
{
    m_maxValue = maxValue;
    m_peak = std::min(m_peak, maxValue);
    update();
}

void MeterWidget::setPeakHold(bool enabled)
{
    m_peakHold = enabled;
    if (!enabled) resetPeak();
}

void MeterWidget::resetPeak()
{
    m_peak = 0.0;
    if (m_peakTimer) { killTimer(m_peakTimer); m_peakTimer = 0; }
    update();
}

void MeterWidget::timerEvent(QTimerEvent*)
{
    // Peak hold expired — decay
    killTimer(m_peakTimer);
    m_peakTimer = 0;
    m_peak = m_value;
    update();
}

QSize MeterWidget::sizeHint() const    { return {260, 38}; }
QSize MeterWidget::minimumSizeHint() const { return {160, 34}; }

void MeterWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int margin  = 6;
    const int labelW  = 80;
    const int valueW  = 64;
    const int barH    = 14;
    const int barTop  = (height() - barH) / 2;
    const int barLeft = margin + labelW;
    const int barRight= width() - margin - valueW;
    const int barW    = barRight - barLeft;

    // Background
    p.fillRect(rect(), QColor(28, 28, 28));

    // Label
    p.setPen(QColor(180, 180, 180));
    QFont lf = font();
    lf.setPointSize(8);
    p.setFont(lf);
    p.drawText(margin, 0, labelW - 4, height(),
               Qt::AlignVCenter | Qt::AlignRight, m_label);

    // Bar background (recessed look)
    const QRect barRect(barLeft, barTop, barW, barH);
    p.fillRect(barRect, QColor(10, 10, 10));
    p.setPen(QColor(60, 60, 60));
    p.drawRect(barRect);

    if (m_maxValue > 0.0) {
        const double fraction = m_value / m_maxValue;
        const int fillW = static_cast<int>(fraction * barW);

        if (fillW > 0) {
            // Three-zone color bar drawn as gradient segments
            const int yellowStart = static_cast<int>(k_yellowThreshold * barW);
            const int redStart    = static_cast<int>(k_redThreshold    * barW);

            // Green zone
            if (fillW > 0) {
                const int gw = std::min(fillW, yellowStart);
                QLinearGradient gg(barLeft, 0, barLeft + yellowStart, 0);
                gg.setColorAt(0.0, QColor(0, 160, 40));
                gg.setColorAt(1.0, QColor(0, 200, 60));
                p.fillRect(QRect(barLeft, barTop + 1, gw, barH - 2), gg);
            }
            // Yellow zone
            if (fillW > yellowStart) {
                const int yw = std::min(fillW, redStart) - yellowStart;
                QLinearGradient yg(barLeft + yellowStart, 0, barLeft + redStart, 0);
                yg.setColorAt(0.0, QColor(200, 180, 0));
                yg.setColorAt(1.0, QColor(230, 140, 0));
                p.fillRect(QRect(barLeft + yellowStart, barTop + 1, yw, barH - 2), yg);
            }
            // Red zone
            if (fillW > redStart) {
                const int rw = fillW - redStart;
                QLinearGradient rg(barLeft + redStart, 0, barLeft + barW, 0);
                rg.setColorAt(0.0, QColor(220, 60, 0));
                rg.setColorAt(1.0, QColor(255, 20, 0));
                p.fillRect(QRect(barLeft + redStart, barTop + 1, rw, barH - 2), rg);
            }
        }

        // Peak hold marker
        if (m_peakHold && m_peak > 0.0) {
            const int px = barLeft + static_cast<int>((m_peak / m_maxValue) * barW);
            p.setPen(QPen(QColor(255, 255, 200), 2));
            p.drawLine(px, barTop + 1, px, barTop + barH - 2);
        }

        // Scale ticks
        p.setPen(QColor(80, 80, 80));
        for (int i = 1; i < 10; ++i) {
            const int tx = barLeft + (barW * i / 10);
            p.drawLine(tx, barTop, tx, barTop + 3);
            p.drawLine(tx, barTop + barH - 3, tx, barTop + barH);
        }
    }

    // Value readout
    p.setPen(QColor(220, 220, 220));
    QFont vf = font();
    vf.setPointSize(8);
    vf.setBold(true);
    p.setFont(vf);
    const QString txt = QString("%1 %2")
                        .arg(m_value, 0, 'f', (m_maxValue >= 100 ? 0 : 1))
                        .arg(m_unit);
    p.drawText(barRight + 4, 0, valueW - 4, height(),
               Qt::AlignVCenter | Qt::AlignLeft, txt);
}
