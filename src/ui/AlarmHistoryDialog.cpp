#include "AlarmHistoryDialog.h"
#include "protocol/Protocol.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QLabel>

AlarmHistoryDialog::AlarmHistoryDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Alarm History"));
    setMinimumSize(480, 300);

    auto* layout = new QVBoxLayout(this);

    auto* note = new QLabel(
        tr("Alarm buffer holds up to 10 entries (most recent last). "
           "Navigate on the amplifier with ← → keys."));
    note->setWordWrap(true);
    note->setStyleSheet("color: #aaa; font-size: 10px;");
    layout->addWidget(note);

    m_table = new QTableWidget(0, 3, this);
    m_table->setHorizontalHeaderLabels({tr("Index"), tr("Input"), tr("Alarm")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->hide();
    layout->addWidget(m_table);

    auto* btnRow = new QHBoxLayout;
    btnRow->addStretch();
    auto* closeBtn = new QPushButton(tr("Close"));
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnRow->addWidget(closeBtn);
    layout->addLayout(btnRow);
}

void AlarmHistoryDialog::updateFromStatus(const StatusPacket& status)
{
    // SETUP_0 holds WRN_IDX (bits 7-4) and WRN_NO (bits 3-0)
    const uint8_t wrnNo  = status.setup[0] & 0x0F;

    m_table->setRowCount(0);

    // SETUP_1..SETUP_10 hold up to 10 warning entries
    for (uint8_t i = 0; i < wrnNo && i < 10; ++i) {
        const uint8_t entry = status.setup[1 + i];
        const uint8_t idxIn = (entry >> 7) & 0x01;
        const uint8_t code  = entry & 0x7F;

        const auto ctx = static_cast<DisplayContext>(code);
        const char* warnText = StatusPacket::warningText(ctx);

        const int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(QString::number(i + 1)));
        m_table->setItem(row, 1, new QTableWidgetItem(
            idxIn == 0 ? tr("IN 1") : tr("IN 2")));
        m_table->setItem(row, 2, new QTableWidgetItem(
            warnText[0] != '\0' ? QString::fromLatin1(warnText)
                                : QString("Code 0x%1").arg(code, 2, 16, QChar('0'))));
    }

    if (wrnNo == 0)
        m_table->insertRow(0),
        m_table->setItem(0, 2, new QTableWidgetItem(tr("No alarms recorded")));
}
