#pragma once
#include "protocol/StatusPacket.h"
#include <QDialog>
#include <QTableWidget>

class AlarmHistoryDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AlarmHistoryDialog(QWidget* parent = nullptr);

    // Update the displayed alarm history from the latest STATUS packet
    // (called when DISPLAY_CTX == AlarmHistory).
    void updateFromStatus(const StatusPacket& status);

private:
    QTableWidget* m_table = nullptr;
};
