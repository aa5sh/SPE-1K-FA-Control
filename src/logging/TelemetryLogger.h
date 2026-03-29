#pragma once
#include "protocol/StatusPacket.h"
#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QString>

// Logs StatusPacket telemetry to a CSV file.
// File is created on start(); each statusUpdated() call appends a row.
class TelemetryLogger : public QObject
{
    Q_OBJECT
public:
    explicit TelemetryLogger(QObject* parent = nullptr);
    ~TelemetryLogger() override;

    bool start(const QString& filePath);
    void stop();
    bool isLogging() const { return m_file.isOpen(); }
    QString currentFile() const { return m_file.fileName(); }

public slots:
    void logStatus(const StatusPacket& status);

signals:
    void loggingStarted(QString filePath);
    void loggingStopped();
    void errorOccurred(QString message);

private:
    void writeHeader();

    QFile       m_file;
    QTextStream m_stream;
};
