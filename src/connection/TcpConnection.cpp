#include "TcpConnection.h"
#include <QEventLoop>
#include <QTimer>

TcpConnection::TcpConnection(QObject* parent)
    : AbstractConnection(parent)
{
    connect(&m_socket, &QTcpSocket::connected,
            this, &TcpConnection::onConnected);
    connect(&m_socket, &QTcpSocket::disconnected,
            this, &TcpConnection::onDisconnected);
    connect(&m_socket, &QTcpSocket::readyRead,
            this, &TcpConnection::onReadyRead);
    connect(&m_socket, &QAbstractSocket::errorOccurred,
            this, &TcpConnection::onErrorOccurred);
}

void TcpConnection::setHost(const QString& host) { m_host = host; }
void TcpConnection::setPort(quint16 port)        { m_port = port; }

bool TcpConnection::open()
{
    if (m_host.isEmpty())
        return false;

    m_socket.connectToHost(m_host, m_port);

    // Block briefly for the connection attempt
    if (!m_socket.waitForConnected(k_connectTimeoutMs)) {
        emit errorOccurred(m_socket.errorString());
        return false;
    }
    return true;
}

void TcpConnection::close()
{
    if (m_socket.state() != QAbstractSocket::UnconnectedState) {
        m_socket.disconnectFromHost();
        if (m_socket.state() != QAbstractSocket::UnconnectedState) {
            // Some serial-to-TCP adapters don't perform a clean TCP teardown;
            // abort() forces the close and ensures the disconnected signal fires.
            if (!m_socket.waitForDisconnected(1000))
                m_socket.abort();
        }
    }
}

bool TcpConnection::isOpen() const
{
    return m_socket.state() == QAbstractSocket::ConnectedState;
}

QString TcpConnection::description() const
{
    return QString("%1:%2").arg(m_host).arg(m_port);
}

bool TcpConnection::write(const QByteArray& data)
{
    if (!isOpen())
        return false;
    return m_socket.write(data) == data.size();
}

void TcpConnection::onConnected()
{
    emit connectionOpened();
}

void TcpConnection::onDisconnected()
{
    emit connectionClosed();
}

void TcpConnection::onReadyRead()
{
    const QByteArray data = m_socket.readAll();
    if (!data.isEmpty())
        emit dataReceived(data);
}

void TcpConnection::onErrorOccurred(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)
    emit errorOccurred(m_socket.errorString());
}
