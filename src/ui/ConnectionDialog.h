#pragma once
#include "connection/AbstractConnection.h"
#include <QDialog>
#include <QTabWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <memory>

// Modal dialog for configuring and creating a connection.
// On accept(), call takeConnection() to get the configured object.
class ConnectionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ConnectionDialog(QWidget* parent = nullptr);

    // Returns the configured connection (caller takes ownership).
    // Valid only after exec() == QDialog::Accepted.
    std::unique_ptr<AbstractConnection> takeConnection();

private slots:
    void refreshPorts();
    void onAccepted();

private:
    void buildSerialTab();
    void buildTcpTab();

    QTabWidget*       m_tabs      = nullptr;

    // Serial tab
    QComboBox*        m_portCombo = nullptr;
    QCheckBox*        m_dtrCheck  = nullptr;

    // TCP tab
    QLineEdit*        m_hostEdit  = nullptr;
    QSpinBox*         m_portSpin  = nullptr;

    QDialogButtonBox* m_buttons   = nullptr;

    std::unique_ptr<AbstractConnection> m_connection;
};
