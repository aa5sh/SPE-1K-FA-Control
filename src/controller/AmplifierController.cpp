#include "AmplifierController.h"
#include "connection/AbstractConnection.h"
#include "protocol/CommandBuilder.h"
#include "protocol/Protocol.h"

AmplifierController::AmplifierController(QObject* parent)
    : QObject(parent)
{
    m_rcuRetryTimer = new QTimer(this);
    m_rcuRetryTimer->setInterval(1000);      // retry every 1 s
    m_rcuRetryTimer->setSingleShot(false);   // repeating
    connect(m_rcuRetryTimer, &QTimer::timeout, this, &AmplifierController::onRcuRetryTimer);
}

AmplifierController::~AmplifierController()
{
    disconnectFromAmplifier();
}

void AmplifierController::setConnection(std::unique_ptr<AbstractConnection> conn)
{
    if (m_conn && m_conn->isOpen())
        disconnectFromAmplifier();

    m_conn = std::move(conn);

    if (m_conn) {
        connect(m_conn.get(), &AbstractConnection::dataReceived,
                this, &AmplifierController::onDataReceived);
        connect(m_conn.get(), &AbstractConnection::connectionOpened,
                this, &AmplifierController::onConnectionOpened);
        connect(m_conn.get(), &AbstractConnection::connectionClosed,
                this, &AmplifierController::onConnectionClosed);
        connect(m_conn.get(), &AbstractConnection::errorOccurred,
                this, &AmplifierController::onConnectionError);
    }
}

bool AmplifierController::connectToAmplifier()
{
    if (!m_conn)
        return false;

    // Reset parser
    m_parseState   = ParseState::WaitSyn1;
    m_expectedCnt  = 0;
    m_packetBuf.clear();

    return m_conn->open();
}

void AmplifierController::disconnectFromAmplifier()
{
    m_rcuRetryTimer->stop();
    if (m_conn && m_conn->isOpen()) {
        m_conn->write(CommandBuilder::rcuOff());
        m_conn->close();
    }
}

bool AmplifierController::isConnected() const
{
    return m_conn && m_conn->isOpen();
}

bool AmplifierController::sendCommand(const QByteArray& packet)
{
    if (!isConnected())
        return false;
    return m_conn->write(packet);
}

bool AmplifierController::supportsDtr() const
{
    return m_conn && m_conn->supportsDtr();
}

void AmplifierController::setDtr(bool asserted)
{
    if (!m_conn || !m_conn->supportsDtr())
        return;
    m_dtrAsserted = asserted;
    m_conn->setDtr(asserted);
    emit dtrChanged(asserted);

    if (asserted) {
        // Amp boot takes 7-10 s after DTR asserted.  Keep sending RCU_ON every
        // second until the amp responds with a STATUS packet (timer stops then).
        m_rcuRetryTimer->start();
    } else {
        m_rcuRetryTimer->stop();
    }
}

// ── Private slots ─────────────────────────────────────────────────────────────

void AmplifierController::onConnectionOpened()
{
    emit connectionChanged(true);
    // Send immediately and start repeating every 1 s until first STATUS confirms
    // streaming is active.  Covers both TCP (amp already on) and serial (amp may
    // still be booting when the port opens).
    m_conn->write(CommandBuilder::rcuOn());
    m_rcuRetryTimer->start();
}

void AmplifierController::onConnectionClosed()
{
    m_rcuRetryTimer->stop();
    emit connectionChanged(false);
}

void AmplifierController::onConnectionError(QString message)
{
    emit errorOccurred(message);
    emit connectionChanged(false);
}

void AmplifierController::onDataReceived(QByteArray data)
{
    processBytes(data);
}

void AmplifierController::onRcuRetryTimer()
{
    if (m_conn && m_conn->isOpen())
        m_conn->write(CommandBuilder::rcuOn());
}

// ── Protocol parser (state machine) ──────────────────────────────────────────

void AmplifierController::processBytes(const QByteArray& data)
{
    for (const char ch : data) {
        const auto b = static_cast<uint8_t>(ch);

        switch (m_parseState) {
        case ParseState::WaitSyn1:
            if (b == AMP_SYN) {
                m_packetBuf.clear();
                m_packetBuf.append(ch);
                m_parseState = ParseState::WaitSyn2;
            }
            break;

        case ParseState::WaitSyn2:
            if (b == AMP_SYN) {
                m_packetBuf.append(ch);
                m_parseState = ParseState::WaitSyn3;
            } else {
                m_parseState = ParseState::WaitSyn1;
            }
            break;

        case ParseState::WaitSyn3:
            if (b == AMP_SYN) {
                m_packetBuf.append(ch);
                m_parseState = ParseState::ReadCnt;
            } else {
                m_parseState = ParseState::WaitSyn1;
            }
            break;

        case ParseState::ReadCnt:
            m_expectedCnt = b;
            m_packetBuf.append(ch);
            m_parseState = ParseState::ReadData;
            break;

        case ParseState::ReadData:
            m_packetBuf.append(ch);
            // packetBuf already has 3 SYN + 1 CNT; data starts at index 4
            if (m_packetBuf.size() == static_cast<qsizetype>(4 + m_expectedCnt)) {
                m_parseState = ParseState::ReadChecksum;
            }
            break;

        case ParseState::ReadChecksum:
            m_packetBuf.append(ch);
            dispatchPacket(m_packetBuf);
            m_parseState = ParseState::WaitSyn1;
            break;
        }
    }
}

void AmplifierController::dispatchPacket(const QByteArray& raw)
{
    // Single-byte response packets (ACK/NAK/UNK have CNT=1)
    if (m_expectedCnt == 1 && raw.size() == 6) {
        const auto code = static_cast<uint8_t>(raw[4]);
        if (code == RESP_ACK) { emit ackReceived(); return; }
        if (code == RESP_NAK) { emit nakReceived(); return; }
        if (code == RESP_UNK) { emit unkReceived(); return; }
    }

    // STATUS packet (CNT = 0x1E = 30)
    if (m_expectedCnt == STATUS_CNT && raw.size() == STATUS_TOTAL_BYTES) {
        StatusPacket pkt;
        if (StatusPacket::decode(raw, pkt)) {
            // RCU streaming confirmed — no need to keep retrying RCU_ON
            m_rcuRetryTimer->stop();
            m_lastStatus = pkt;
            emit statusUpdated(pkt);
        }
    }
}
