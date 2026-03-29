#include "TelemetryLogger.h"
#include <QDateTime>

TelemetryLogger::TelemetryLogger(QObject* parent)
    : QObject(parent)
{}

TelemetryLogger::~TelemetryLogger()
{
    stop();
}

bool TelemetryLogger::start(const QString& filePath)
{
    if (m_file.isOpen())
        stop();

    m_file.setFileName(filePath);
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
        emit errorOccurred(tr("Cannot open log file: %1").arg(m_file.errorString()));
        return false;
    }

    m_stream.setDevice(&m_file);

    // Write header only if the file is new/empty
    if (m_file.pos() == 0)
        writeHeader();

    emit loggingStarted(filePath);
    return true;
}

void TelemetryLogger::stop()
{
    if (m_file.isOpen()) {
        m_stream.flush();
        m_file.close();
        emit loggingStopped();
    }
}

void TelemetryLogger::writeHeader()
{
    m_stream << "Timestamp,"
             << "Mode,Band,Input,SubBand,FreqKHz,"
             << "TX,Tuning,Alarm,FullMode,Contest,"
             << "PAOutWpep,RevPowerWpep,"
             << "SWR,GainDB,"
             << "TempDeg,TempUnit,"
             << "SupplyV,SupplyA,"
             << "CatInterface,Antenna,"
             << "DisplayCtx"
             << Qt::endl;
}

void TelemetryLogger::logStatus(const StatusPacket& s)
{
    if (!m_file.isOpen())
        return;

    const QString ts = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);

    m_stream
        << ts << ","
        << (s.operate ? "OPERATE" : "STANDBY") << ","
        << bandName(s.band) << ","
        << (s.input + 1) << ","
        << s.subBand << ","
        << s.frequencyKHz << ","
        << (s.txActive    ? 1 : 0) << ","
        << (s.tuning      ? 1 : 0) << ","
        << (s.alarmActive ? 1 : 0) << ","
        << (s.fullMode    ? 1 : 0) << ","
        << (s.contest     ? 1 : 0) << ","
        << s.paOutputWpep() << ","
        << s.reversePowerWpep() << ","
        << (s.operate ? 0.0 : s.swr()) << ","
        << (s.operate ? s.gainDb() : 0.0) << ","
        << s.temperature << ","
        << (s.tempCelsius ? "C" : "F") << ","
        << s.supplyVoltage() << ","
        << s.supplyCurrent() << ","
        << catName(s.catInterface) << ","
        << antennaName(s.antenna) << ","
        << static_cast<int>(s.displayCtx)
        << Qt::endl;
}
