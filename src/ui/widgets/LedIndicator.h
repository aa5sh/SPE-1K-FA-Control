#pragma once
#include <QWidget>
#include <QColor>
#include <QString>

// A small circular LED indicator widget with a label beneath it.
class LedIndicator : public QWidget
{
    Q_OBJECT
public:
    explicit LedIndicator(const QString& label, QWidget* parent = nullptr);

    void setOn(bool on);
    void setColors(QColor onColor, QColor offColor);
    bool isOn() const { return m_on; }

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QString m_label;
    bool    m_on       = false;
    QColor  m_onColor  = QColor(0, 220, 60);
    QColor  m_offColor = QColor(30, 60, 30);
};
