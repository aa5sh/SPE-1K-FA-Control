#include "CompactWindow.h"
#include "widgets/MeterWidget.h"
#include "controller/AmplifierController.h"
#include "protocol/CommandBuilder.h"
#include "protocol/Protocol.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCloseEvent>
#include <QSettings>
#include <QFont>
#include <QFrame>

// ── Style helpers ─────────────────────────────────────────────────────────────

static const char* k_bg      = "#1a1a1a";
static const char* k_stripBg = "#0d2020";

static QString compactBtnStyle()
{
    return QStringLiteral(
        "QPushButton {"
        "  background:#1e3030; color:#a0d0c0;"
        "  border:1px solid #2a5050; border-radius:2px;"
        "  font-size:9px; font-weight:bold; min-height:22px;"
        "}"
        "QPushButton:hover   { background:#2a4040; border-color:#50a090; }"
        "QPushButton:pressed { background:#0e1a1a; }"
        "QPushButton:disabled{ background:#111818; color:#2a4040; border-color:#1a2828; }"
    );
}

static QString operateBtnStyle(bool operate)
{
    if (operate)
        return QStringLiteral(
            "QPushButton {"
            "  background:#0a280a; color:#40e040;"
            "  border:1px solid #1a7a1a; border-radius:2px;"
            "  font-size:9px; font-weight:bold; min-height:22px;"
            "}"
            "QPushButton:hover   { background:#0e3a0e; }"
            "QPushButton:disabled{ background:#060f06; color:#1a4a1a; border-color:#0a1a0a; }");
    return QStringLiteral(
        "QPushButton {"
        "  background:#282808; color:#d0c040;"
        "  border:1px solid #5a5010; border-radius:2px;"
        "  font-size:9px; font-weight:bold; min-height:22px;"
        "}"
        "QPushButton:hover   { background:#3a3a0c; }"
        "QPushButton:disabled{ background:#0f0f06; color:#3a3a1a; border-color:#1a1a0a; }");
}

static QString offBtnStyle()
{
    return QStringLiteral(
        "QPushButton {"
        "  background:#3a0a0a; color:#ff6060;"
        "  border:1px solid #7a1010; border-radius:2px;"
        "  font-size:9px; font-weight:bold; min-height:22px; min-width:22px;"
        "}"
        "QPushButton:hover   { background:#4a1010; }"
        "QPushButton:pressed { background:#200505; }"
        "QPushButton:disabled{ background:#1a0606; color:#502020; border-color:#2a0808; }");
}

// ── CompactWindow ─────────────────────────────────────────────────────────────

CompactWindow::CompactWindow(AmplifierController* controller, QWidget* parent)
    : QWidget(parent, Qt::Window)
    , m_controller(controller)
{
    setWindowTitle(tr("1K-FA — Compact"));
    setMinimumSize(480, 190);
    setAttribute(Qt::WA_QuitOnClose, false);  // closing compact doesn't quit app

    // Inherit the application-wide dark palette set by MainWindow.

    buildUi();
    restoreGeometry();

    connect(m_controller, &AmplifierController::statusUpdated,
            this, &CompactWindow::onStatusUpdated);
    connect(m_controller, &AmplifierController::connectionChanged,
            this, &CompactWindow::onConnectionChanged);

    // Initialise to disconnected state
    onConnectionChanged(m_controller->isConnected());
}

// ── UI construction ───────────────────────────────────────────────────────────

void CompactWindow::buildUi()
{
    setStyleSheet(QString("QWidget { background: %1; }").arg(k_bg));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Status strip ─────────────────────────────────────────────────────────
    auto* strip = new QWidget;
    strip->setFixedHeight(26);
    strip->setStyleSheet(
        QString("QWidget { background: %1; border-bottom: 1px solid #1a4040; }").arg(k_stripBg));
    auto* stripLayout = new QHBoxLayout(strip);
    stripLayout->setContentsMargins(6, 0, 6, 0);
    stripLayout->setSpacing(6);

    // Output / mode label — stretches to take available space on the left
    m_outLabel = new QLabel(tr("NOT CONNECTED"));
    m_outLabel->setStyleSheet("color:#808080; font-size:10px; font-weight:bold;");
    stripLayout->addWidget(m_outLabel, 1);

    // TX indicator pill — hidden unless transmitting
    m_txLabel = new QLabel(tr("TX"));
    m_txLabel->setAlignment(Qt::AlignCenter);
    m_txLabel->setFixedWidth(26);
    m_txLabel->setStyleSheet(
        "color:#ff4040; font-size:8px; font-weight:bold;"
        "background:#3a0000; border:1px solid #7a0000; border-radius:2px;");
    m_txLabel->setVisible(false);
    stripLayout->addWidget(m_txLabel);

    auto addPill = [&](QLabel*& lbl, const QString& init) {
        lbl = new QLabel(init);
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setStyleSheet(
            "color:#00c0c0; font-size:9px; font-weight:bold;"
            "background:#0a2828; border:1px solid #1a4848; border-radius:2px;"
            "padding:0px 4px;");
        stripLayout->addWidget(lbl);
    };

    addPill(m_bandLabel, "--");
    addPill(m_antLabel,  "ANT --");
    addPill(m_inLabel,   "IN --");
    addPill(m_tempLabel, "--");
    addPill(m_swrLabel,  "SWR --");

    // Connection dot
    m_connDot = new QLabel("\xe2\x97\x8f");  // ●
    m_connDot->setStyleSheet("color:#404040; font-size:12px; padding:0 2px;");
    stripLayout->addWidget(m_connDot);

    root->addWidget(strip);

    // ── Meters ────────────────────────────────────────────────────────────────
    auto* metersWidget = new QWidget;
    metersWidget->setStyleSheet("QWidget { background: #161616; }");
    auto* ml = new QVBoxLayout(metersWidget);
    ml->setContentsMargins(4, 4, 4, 2);
    ml->setSpacing(3);

    m_meterPwr   = new MeterWidget(tr("PWR OUT"), tr("W pep"), 1200.0, metersWidget);
    m_meterSwr   = new MeterWidget(tr("SWR"),     QString(),     3.0,  metersWidget);
    m_meterVolts = new MeterWidget(tr("V PA"),    tr("V"),      60.0,  metersWidget);
    m_meterAmps  = new MeterWidget(tr("I PA"),    tr("A"),      50.0,  metersWidget);

    ml->addWidget(m_meterPwr);
    ml->addWidget(m_meterSwr);

    // Volts and Amps side by side
    auto* vaRow = new QHBoxLayout;
    vaRow->setSpacing(4);
    vaRow->addWidget(m_meterVolts);
    vaRow->addWidget(m_meterAmps);
    ml->addLayout(vaRow);

    root->addWidget(metersWidget, 1);

    // ── Button row ────────────────────────────────────────────────────────────
    auto* btnWidget = new QWidget;
    btnWidget->setFixedHeight(32);
    btnWidget->setStyleSheet(
        QString("QWidget { background: %1; border-top: 1px solid #1a3030; }").arg(k_stripBg));
    auto* bl = new QHBoxLayout(btnWidget);
    bl->setContentsMargins(4, 3, 4, 3);
    bl->setSpacing(3);

    // OPERATE / STANDBY toggle button
    m_operateBtn = new QPushButton(tr("OPERATE"));
    m_operateBtn->setStyleSheet(operateBtnStyle(false));
    connect(m_operateBtn, &QPushButton::clicked, this, [this]{ sendKey(KEY_OPERATE); });
    m_allButtons.append(m_operateBtn);
    bl->addWidget(m_operateBtn, 1);

    // HALF / FULL toggle
    m_modeBtn = new QPushButton(tr("HALF"));
    m_modeBtn->setStyleSheet(compactBtnStyle());
    connect(m_modeBtn, &QPushButton::clicked, this, [this]{ sendKey(KEY_MODE); });
    m_allButtons.append(m_modeBtn);
    bl->addWidget(m_modeBtn, 1);

    // Standard nav buttons
    struct BtnDef { QString label; uint8_t key; };
    const QList<BtnDef> navBtns = {
        { tr("ANT"),    KEY_ANT         },
        { tr("TUNE"),   KEY_TUNE        },
        { tr("IN"),     KEY_IN          },
        { tr("\xe2\x86\x90"),  KEY_LEFT        },   // ←
        { tr("\xe2\x86\x92"),  KEY_RIGHT       },   // →
        { tr("BAND\xe2\x88\x92"), KEY_BAND_MINUS },  // BAND−
        { tr("BAND+"),  KEY_BAND_PLUS   },
    };
    for (const auto& def : navBtns) {
        auto* b = new QPushButton(def.label);
        b->setStyleSheet(compactBtnStyle());
        connect(b, &QPushButton::clicked, this, [this, key=def.key]{ sendKey(key); });
        m_allButtons.append(b);
        bl->addWidget(b, 1);
    }

    // OFF button (red)
    auto* offBtn = new QPushButton(tr("OFF"));
    offBtn->setStyleSheet(offBtnStyle());
    offBtn->setToolTip(tr("Send firmware OFF command (standby).\n"
                          "Does not cut mains power in TCP/remote mode."));
    connect(offBtn, &QPushButton::clicked, this, [this]{ sendKey(KEY_OFF); });
    m_allButtons.append(offBtn);
    bl->addWidget(offBtn);

    // Separator line before the view-switch button
    auto* sep = new QFrame;
    sep->setFrameShape(QFrame::VLine);
    sep->setStyleSheet("color:#2a5050;");
    bl->addWidget(sep);

    // Full View button — switches back to the main window (not a control button,
    // so it's always enabled even when disconnected)
    auto* fullBtn = new QPushButton(tr("\xe2\x96\xa3 Full"));  // ▣ Full
    fullBtn->setFixedWidth(46);
    fullBtn->setStyleSheet(
        "QPushButton {"
        "  background:#222222; color:#808080;"
        "  border:1px solid #404040; border-radius:2px;"
        "  font-size:8px; font-weight:bold; min-height:22px;"
        "}"
        "QPushButton:hover { background:#303030; color:#b0b0b0; border-color:#606060; }"
        "QPushButton:pressed { background:#141414; }");
    fullBtn->setToolTip(tr("Switch back to the full main window (Ctrl+K)."));
    connect(fullBtn, &QPushButton::clicked, this, &CompactWindow::requestFullView);
    bl->addWidget(fullBtn);

    root->addWidget(btnWidget);
}

// ── Slots ─────────────────────────────────────────────────────────────────────

void CompactWindow::onConnectionChanged(bool connected)
{
    for (auto* b : m_allButtons)
        b->setEnabled(connected);

    m_connDot->setStyleSheet(
        connected ? "color:#00e040; font-size:12px; padding:0 2px;"
                  : "color:#404040; font-size:12px; padding:0 2px;");

    if (!connected) {
        m_outLabel->setText(tr("NOT CONNECTED"));
        m_outLabel->setStyleSheet("color:#505050; font-size:10px; font-weight:bold;");
        m_txLabel->setVisible(false);
        m_bandLabel->setText("--");
        m_antLabel->setText("ANT --");
        m_inLabel->setText("IN --");
        m_tempLabel->setText("--");
        m_swrLabel->setText("SWR --");
        m_meterPwr->setValue(0);
        m_meterSwr->setValue(0);
        m_meterVolts->setValue(0);
        m_meterAmps->setValue(0);
    }
}

void CompactWindow::onStatusUpdated(const StatusPacket& s)
{
    // ── TX indicator ──────────────────────────────────────────────────────────
    m_txLabel->setVisible(s.txActive);

    // ── Mode / output label ───────────────────────────────────────────────────
    QString outText;
    QString outStyle;
    if (s.alarmActive) {
        outText  = tr("⚠ ALARM");
        outStyle = "color:#ff4040; font-size:10px; font-weight:bold;";
    } else if (s.tuning) {
        outText  = tr("TUNING…");
        outStyle = "color:#40c0ff; font-size:10px; font-weight:bold;";
    } else if (s.operate) {
        outText  = tr("OUT: %1 W").arg(s.paOutputWpep(), 0, 'f', 1);
        outStyle = "color:#40e040; font-size:10px; font-weight:bold;";
    } else {
        outText  = tr("STANDBY");
        outStyle = "color:#808080; font-size:10px; font-weight:bold;";
    }
    m_outLabel->setText(outText);
    m_outLabel->setStyleSheet(outStyle);

    // ── OPERATE button label tracks mode ─────────────────────────────────────
    m_operateBtn->setText(s.operate ? tr("STANDBY") : tr("OPERATE"));
    m_operateBtn->setStyleSheet(operateBtnStyle(s.operate));

    // ── HALF/FULL button ──────────────────────────────────────────────────────
    m_modeBtn->setText(s.fullMode ? tr("FULL") : tr("HALF"));

    // ── Info pills ────────────────────────────────────────────────────────────
    m_bandLabel->setText(QString::fromLatin1(bandName(s.band)));
    m_antLabel->setText(QString("ANT %1").arg(static_cast<int>(s.antenna) + 1));
    m_inLabel->setText(QString("IN %1").arg(s.input + 1));
    m_tempLabel->setText(QString("%1\xc2\xb0%2")
        .arg(s.temperature)
        .arg(s.tempCelsius ? 'C' : 'F'));

    if (!s.operate) {
        if (s.swrOrGainRaw == SWR_NO_SIGNAL)
            m_swrLabel->setText("SWR --");
        else if (s.swrOrGainRaw == SWR_INFINITY)
            m_swrLabel->setText("SWR \xe2\x88\x9e");   // ∞
        else
            m_swrLabel->setText(QString("SWR %1").arg(s.swr(), 0, 'f', 2));
    } else {
        m_swrLabel->setText(QString("GAIN %1dB").arg(s.gainDb(), 0, 'f', 1));
    }

    // ── Meters ────────────────────────────────────────────────────────────────
    const double pwrMax = s.operate ? (s.fullMode ? 1200.0 : 600.0) : 200.0;
    m_meterPwr->setMaxValue(pwrMax);
    m_meterPwr->setValue(s.paOutputWpep());

    // SWR meter: standby = SWR (0–3.0); operate = PA gain (0–30 dB)
    if (!s.operate) {
        m_meterSwr->setLabel(tr("SWR"));
        m_meterSwr->setUnit(QString());
        m_meterSwr->setMaxValue(3.0);
        m_meterSwr->setValue(
            (s.swrOrGainRaw == SWR_NO_SIGNAL || s.swrOrGainRaw == SWR_INFINITY)
            ? 0.0 : s.swr());
    } else {
        m_meterSwr->setLabel(tr("PA GAIN"));
        m_meterSwr->setUnit(tr("dB"));
        m_meterSwr->setMaxValue(30.0);
        m_meterSwr->setValue(s.gainDb());
    }

    m_meterVolts->setValue(s.operate ? s.supplyVoltage() : 0.0);
    m_meterAmps->setValue(s.supplyCurrent());
}

// ── Helpers ───────────────────────────────────────────────────────────────────

void CompactWindow::sendKey(uint8_t key)
{
    m_controller->sendCommand(CommandBuilder::keyPress(key));
}

void CompactWindow::closeEvent(QCloseEvent* event)
{
    // Don't actually close — signal the main window to take over instead.
    saveGeometry();
    event->ignore();
    emit requestFullView();
}

void CompactWindow::saveGeometry()
{
    QSettings qs;
    qs.setValue("compact/geometry", QWidget::saveGeometry());
}

void CompactWindow::restoreGeometry()
{
    QSettings qs;
    const QByteArray geo = qs.value("compact/geometry").toByteArray();
    if (!geo.isEmpty())
        QWidget::restoreGeometry(geo);
}
