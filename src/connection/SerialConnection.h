#pragma once
#include "AbstractConnection.h"
#include <QSerialPort>
#include <QString>

class SerialConnection : public AbstractConnection
{
    Q_OBJECT
public:
    explicit SerialConnection(QObject* parent = nullptr);

    void setPortName(const QString& port);
    QString portName() const;

    bool open()  override;
    void close() override;
    bool isOpen() const override;
    QString description() const override;
    bool write(const QByteArray& data) override;

    bool supportsDtr() const override { return true; }
    void setDtr(bool asserted) override;

private slots:
    void onReadyRead();
    void onErrorOccurred(QSerialPort::SerialPortError error);

private:
    QSerialPort m_port;
};
