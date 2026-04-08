#pragma once
#include "protocol/StatusPacket.h"
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <cstdint>

class AmplifierController;
class MeterWidget;

// Compact operation window — shows meters and key controls only.
// Activated via View → Compact View; hides the main window while visible.
// Emits requestFullView() when the user wants to switch back.
// Window position is persisted via QSettings.
class CompactWindow : public QWidget
{
    Q_OBJECT
public:
    explicit CompactWindow(AmplifierController* controller, QWidget* parent = nullptr);

    void saveGeometry();
    void restoreGeometry();

signals:
    // Emitted when the user clicks "Full View" or closes this window.
    // MainWindow connects this to its onSwitchToFull() slot.
    void requestFullView();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onStatusUpdated(const StatusPacket& s);
    void onConnectionChanged(bool connected);

private:
    void buildUi();
    void sendKey(uint8_t key);

    AmplifierController* m_controller = nullptr;

    // ── Status strip ──────────────────────────────────────────────────────────
    QLabel* m_outLabel   = nullptr;   // "OUT: 1000 W" / "STANDBY" / mode text
    QLabel* m_txLabel    = nullptr;   // "TX" pill — hidden when not transmitting
    QLabel* m_bandLabel  = nullptr;
    QLabel* m_antLabel   = nullptr;
    QLabel* m_inLabel    = nullptr;
    QLabel* m_tempLabel  = nullptr;
    QLabel* m_swrLabel   = nullptr;   // "SWR 1.05" or "GAIN 12.3 dB"
    QLabel* m_connDot    = nullptr;   // ● connection indicator

    // ── Meters ────────────────────────────────────────────────────────────────
    MeterWidget* m_meterPwr   = nullptr;   // PWR OUT  (W pep)
    MeterWidget* m_meterSwr   = nullptr;   // SWR (standby) | PA GAIN (operate)
    MeterWidget* m_meterVolts = nullptr;   // V PA
    MeterWidget* m_meterAmps  = nullptr;   // I PA

    // ── Buttons ───────────────────────────────────────────────────────────────
    QPushButton* m_operateBtn = nullptr;   // label switches OPERATE ↔ STANDBY
    QPushButton* m_modeBtn    = nullptr;   // label switches HALF ↔ FULL
    QList<QPushButton*> m_allButtons;      // all control buttons (for enabled state)
};
