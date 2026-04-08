#pragma once
#include <QWidget>
#include <QString>

// Horizontal bar meter with green→yellow→red coloring and peak hold.
// Value and max are in display units (caller passes already-scaled doubles).
class MeterWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MeterWidget(const QString& label,
                         const QString& unit,
                         double maxValue,
                         QWidget* parent = nullptr);

    void setValue(double value);
    void setMaxValue(double maxValue);
    void setLabel(const QString& label);
    void setUnit(const QString& unit);
    void setPeakHold(bool enabled);
    void resetPeak();

    double value()    const { return m_value;    }
    double maxValue() const { return m_maxValue; }

    QSize sizeHint()    const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void timerEvent(QTimerEvent* event) override;

private:
    QString m_label;
    QString m_unit;
    double  m_value    = 0.0;
    double  m_maxValue = 100.0;
    double  m_peak     = 0.0;
    bool    m_peakHold = true;
    int     m_peakTimer = 0;

    // Thresholds as fractions of maxValue
    static constexpr double k_yellowThreshold = 0.70;
    static constexpr double k_redThreshold    = 0.90;
    static constexpr int    k_peakHoldMs      = 2000;
};
