#include "ConnectionDialog.h"
#include "connection/SerialConnection.h"
#include "connection/TcpConnection.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSerialPortInfo>
#include <QSettings>

ConnectionDialog::ConnectionDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Connect to Amplifier"));
    setModal(true);
    setMinimumWidth(360);

    auto* layout = new QVBoxLayout(this);

    m_tabs = new QTabWidget(this);
    buildSerialTab();
    buildTcpTab();
    layout->addWidget(m_tabs);

    m_buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttons->button(QDialogButtonBox::Ok)->setText(tr("Connect"));
    layout->addWidget(m_buttons);

    connect(m_buttons, &QDialogButtonBox::accepted, this, &ConnectionDialog::onAccepted);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Restore last settings
    QSettings s;
    m_tabs->setCurrentIndex(s.value("conn/tab", 0).toInt());
    m_hostEdit->setText(s.value("conn/tcp/host", "").toString());
    m_portSpin->setValue(s.value("conn/tcp/port", 4000).toInt());
    const QString lastPort = s.value("conn/serial/port", "").toString();
    const int idx = m_portCombo->findText(lastPort);
    if (idx >= 0) m_portCombo->setCurrentIndex(idx);
    m_dtrCheck->setChecked(s.value("conn/serial/dtr", false).toBool());
}

void ConnectionDialog::buildSerialTab()
{
    auto* w      = new QWidget;
    auto* layout = new QFormLayout(w);
    layout->setSpacing(10);

    m_portCombo = new QComboBox;
    m_portCombo->setMinimumWidth(200);

    auto* refreshBtn = new QPushButton(tr("Refresh"));
    auto* portRow    = new QHBoxLayout;
    portRow->addWidget(m_portCombo, 1);
    portRow->addWidget(refreshBtn);

    layout->addRow(tr("Port:"), portRow);

    auto* baudLabel = new QLabel(tr("9600 Baud, 8N1 (fixed)"));
    baudLabel->setEnabled(false);
    layout->addRow(tr("Speed:"), baudLabel);

    m_dtrCheck = new QCheckBox(tr("Use DTR for power control"));
    m_dtrCheck->setToolTip(
        tr("Assert DTR high to power on the amplifier.\n"
           "Note: the amplifier will own the power key while DTR is held high."));
    layout->addRow(QString(), m_dtrCheck);

    connect(refreshBtn, &QPushButton::clicked, this, &ConnectionDialog::refreshPorts);
    refreshPorts();

    m_tabs->addTab(w, tr("Serial"));
}

void ConnectionDialog::buildTcpTab()
{
    auto* w      = new QWidget;
    auto* layout = new QFormLayout(w);
    layout->setSpacing(10);

    m_hostEdit = new QLineEdit;
    m_hostEdit->setPlaceholderText(tr("192.168.1.100"));
    layout->addRow(tr("Host / IP:"), m_hostEdit);

    m_portSpin = new QSpinBox;
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(4000);
    layout->addRow(tr("Port:"), m_portSpin);

    auto* note = new QLabel(
        tr("Connect via a serial-to-TCP adapter.\n"
           "Power on/off via DTR is not available over TCP."));
    note->setStyleSheet("color: #888; font-size: 10px;");
    note->setWordWrap(true);
    layout->addRow(note);

    m_tabs->addTab(w, tr("TCP"));
}

void ConnectionDialog::refreshPorts()
{
    const QString current = m_portCombo->currentText();
    m_portCombo->clear();

    const auto ports = QSerialPortInfo::availablePorts();
    for (const auto& info : ports)
        m_portCombo->addItem(info.portName());

    if (!current.isEmpty()) {
        const int idx = m_portCombo->findText(current);
        if (idx >= 0) m_portCombo->setCurrentIndex(idx);
    }
}

void ConnectionDialog::onAccepted()
{
    QSettings s;
    s.setValue("conn/tab", m_tabs->currentIndex());

    if (m_tabs->currentIndex() == 0) {
        // Serial
        if (m_portCombo->currentText().isEmpty()) return;

        s.setValue("conn/serial/port", m_portCombo->currentText());
        s.setValue("conn/serial/dtr",  m_dtrCheck->isChecked());

        auto* conn = new SerialConnection;
        conn->setPortName(m_portCombo->currentText());
        m_connection.reset(conn);
    } else {
        // TCP
        if (m_hostEdit->text().trimmed().isEmpty()) return;

        s.setValue("conn/tcp/host", m_hostEdit->text().trimmed());
        s.setValue("conn/tcp/port", m_portSpin->value());

        auto* conn = new TcpConnection;
        conn->setHost(m_hostEdit->text().trimmed());
        conn->setPort(static_cast<quint16>(m_portSpin->value()));
        m_connection.reset(conn);
    }

    accept();
}

std::unique_ptr<AbstractConnection> ConnectionDialog::takeConnection()
{
    return std::move(m_connection);
}
