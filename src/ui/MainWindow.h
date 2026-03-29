#pragma once
#include "protocol/StatusPacket.h"
#include <QMainWindow>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QFrame>

class AmplifierController;
class TelemetryLogger;
class MeterWidget;
class AlarmHistoryDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onConnect();
    void onDisconnect();
    void onConnectionChanged(bool connected);
    void onStatusUpdated(const StatusPacket& status);
    void onError(const QString& message);
    void onStartLogging();
    void onStopLogging();
    void onShowAlarmHistory();
    void onNak();
    void onDtrToggle();
    void onDtrChanged(bool asserted);

private:
    void buildMenuBar();
    void buildCentralWidget();
    QWidget* buildLeftButtons();
    QWidget* buildCenterDisplay();
    QWidget* buildRightButtons();
    QWidget* buildInfoStrip();
    QWidget* buildBottomButtons();
    void     updateConnectionUi(bool connected);
    void     sendKey(uint8_t keyCode);
    void     updateModeBox(const StatusPacket& s);
    void     updateSetupText(const StatusPacket& s);
    QString  formatSetupText(const StatusPacket& s) const;

    // Controller + logger
    AmplifierController* m_controller = nullptr;
    TelemetryLogger*     m_logger     = nullptr;
    AlarmHistoryDialog*  m_alarmDlg   = nullptr;

    // Menu actions
    QAction* m_connectAction    = nullptr;
    QAction* m_disconnectAction = nullptr;
    QAction* m_logStartAction   = nullptr;
    QAction* m_logStopAction    = nullptr;
    QAction* m_onTopAction      = nullptr;

    // Center display widgets
    QLabel*  m_modeBox    = nullptr;  // "STANDBY" / "OPERATE MODE" / "SETUP MODE"
    QLabel*  m_setupText  = nullptr;  // multi-line setup/warning text area
    QFrame*  m_setupFrame = nullptr;  // visible only when showing setup/warning text

    // Bar meters (grouped so they can be hidden together during setup display)
    QWidget*     m_metersWidget = nullptr;  // container: scale row + 4 meters
    MeterWidget* m_meterPaOut   = nullptr;  // PA OUT (W pep)
    MeterWidget* m_meterIpa     = nullptr;  // I PA   (A)
    MeterWidget* m_meterPwRev   = nullptr;  // PW REV (W pep)
    MeterWidget* m_meterVpa     = nullptr;  // V PA   (V)

    // Info strip labels (values)
    QLabel* m_infoIn   = nullptr;
    QLabel* m_infoBand = nullptr;
    QLabel* m_infoAnt  = nullptr;
    QLabel* m_infoCat  = nullptr;
    QLabel* m_infoOut  = nullptr;
    QLabel* m_infoSwrGain = nullptr;
    QLabel* m_infoTemp = nullptr;
    QLabel* m_infoSwrGainHeader = nullptr;  // switches between "SWR" and "PA GAIN"

    // DTR amp power button (right panel, serial connections only)
    QPushButton* m_dtrButton = nullptr;

    // Bottom buttons (enabled state)
    QList<QPushButton*> m_controlButtons;

    // Status bar
    QLabel* m_statusConnLabel = nullptr;
    QLabel* m_statusLogLabel  = nullptr;
};
