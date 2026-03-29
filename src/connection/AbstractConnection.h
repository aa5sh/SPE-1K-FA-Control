#pragma once
#include <QObject>
#include <QByteArray>
#include <QString>

// Pure interface for a connection to the amplifier.
// Concrete implementations: SerialConnection, TcpConnection.
class AbstractConnection : public QObject
{
    Q_OBJECT
public:
    explicit AbstractConnection(QObject* parent = nullptr) : QObject(parent) {}
    ~AbstractConnection() override = default;

    virtual bool open()  = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;

    // Returns a human-readable description of the connection endpoint.
    virtual QString description() const = 0;

    // Write raw bytes; returns false if not connected.
    virtual bool write(const QByteArray& data) = 0;

    // True if this connection type supports DTR (serial only).
    virtual bool supportsDtr() const { return false; }
    virtual void setDtr(bool /*asserted*/) {}

signals:
    void dataReceived(QByteArray data);
    void connectionOpened();
    void connectionClosed();
    void errorOccurred(QString message);
};
