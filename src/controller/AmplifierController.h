#pragma once
#include "protocol/StatusPacket.h"
#include <QObject>
#include <QByteArray>
#include <QTimer>
#include <memory>

class AbstractConnection;

// Owns the connection and drives the protocol state machine.
// Once connected, sends RCU_ON to enable streaming status updates.
// Parses all incoming bytes and emits typed signals.
class AmplifierController : public QObject
{
    Q_OBJECT
public:
    explicit AmplifierController(QObject* parent = nullptr);
    ~AmplifierController() override;

    // Takes ownership of connection.
    void setConnection(std::unique_ptr<AbstractConnection> conn);
    AbstractConnection* connection() const { return m_conn.get(); }

    bool connectToAmplifier();
    void disconnectFromAmplifier();
    bool isConnected() const;

    // Send a pre-built command packet.
    bool sendCommand(const QByteArray& packet);

    // DTR power control (serial only — no-op over TCP).
    // Asserting DTR high powers the amp on (3-4.5 s startup).
    // De-asserting powers it off (~1 s). While DTR is held high the
    // front-panel OFF key on the amp is disabled.
    void setDtr(bool asserted);
    bool isDtrAsserted() const { return m_dtrAsserted; }
    bool supportsDtr() const;

    const StatusPacket& lastStatus() const { return m_lastStatus; }

signals:
    void statusUpdated(StatusPacket status);
    void ackReceived();
    void nakReceived();
    void unkReceived();
    void connectionChanged(bool connected);
    void errorOccurred(QString message);
    void dtrChanged(bool asserted);

private slots:
    void onDataReceived(QByteArray data);
    void onConnectionOpened();
    void onConnectionClosed();
    void onConnectionError(QString message);
    void onRcuRetryTimer();

private:
    void processBytes(const QByteArray& data);
    void dispatchPacket(const QByteArray& raw);

    // Parser state machine
    enum class ParseState {
        WaitSyn1, WaitSyn2, WaitSyn3,
        ReadCnt, ReadData, ReadChecksum
    };

    std::unique_ptr<AbstractConnection> m_conn;
    StatusPacket m_lastStatus;
    bool         m_dtrAsserted = false;
    QTimer*      m_rcuRetryTimer = nullptr;  // fires after DTR boot delay to re-send RCU_ON

    // Parser state
    ParseState   m_parseState = ParseState::WaitSyn1;
    uint8_t      m_expectedCnt = 0;
    QByteArray   m_packetBuf;   // accumulates full packet including SYN/CNT
};
