#pragma once
#include "AbstractConnection.h"
#include <QTcpSocket>
#include <QString>

class TcpConnection : public AbstractConnection
{
    Q_OBJECT
public:
    explicit TcpConnection(QObject* parent = nullptr);

    void setHost(const QString& host);
    void setPort(quint16 port);
    QString host() const { return m_host; }
    quint16 port() const { return m_port; }

    bool open()  override;
    void close() override;
    bool isOpen() const override;
    QString description() const override;
    bool write(const QByteArray& data) override;

    // TCP does not support DTR; power on/off must be done via protocol commands.
    bool supportsDtr() const override { return false; }

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onErrorOccurred(QAbstractSocket::SocketError error);

private:
    QTcpSocket m_socket;
    QString    m_host;
    quint16    m_port = 4000;

    static constexpr int k_connectTimeoutMs = 5000;
};
