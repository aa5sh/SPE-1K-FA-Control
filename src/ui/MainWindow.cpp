#include "MainWindow.h"
#include "ConnectionDialog.h"
#include "AlarmHistoryDialog.h"
#include "CompactWindow.h"
#include "connection/AbstractConnection.h"
#include "connection/TcpConnection.h"
#include "widgets/MeterWidget.h"
#include "controller/AmplifierController.h"
#include "logging/TelemetryLogger.h"
#include "protocol/CommandBuilder.h"
#include "protocol/Protocol.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QCloseEvent>
#include <QStandardPaths>
#include <QFont>
#include <QFrame>
#include <QSizePolicy>
#include <QApplication>
#include <QTimer>
#include <QSettings>

// ─── Style constants ──────────────────────────────────────────────────────────

static const char* k_darkBg   = "#1a1a1a";
static const char* k_displayBg = "#111111";
static const char* k_tealBg   = "#006060";
static const char* k_tealDark = "#004444";

static QString leftBtnStyle()
{
    return QStringLiteral(
        "QPushButton {"
        "  background: #2c2c2c; color: #c8c8c8;"
        "  border: 1px solid #505050; border-radius: 3px;"
        "  font-size: 11px; font-weight: bold; padding: 2px 6px;"
        "}"
        "QPushButton:hover   { background: #3c3c3c; border-color: #808080; }"
        "QPushButton:pressed { background: #181818; }"
        "QPushButton:disabled{ color: #444; border-color: #2c2c2c; }"
    );
}

static QString rightBtnStyle()
{
    return QStringLiteral(
        "QPushButton {"
        "  background: #2c2c2c; color: #c8c8c8;"
        "  border: 1px solid #505050; border-radius: 3px;"
        "  font-size: 10px; font-weight: bold; min-height: 40px;"
        "}"
        "QPushButton:hover   { background: #3c3c3c; border-color: #808080; }"
        "QPushButton:pressed { background: #181818; }"
        "QPushButton:disabled{ color: #444; border-color: #2c2c2c; }"
    );
}

static QString bottomBtnStyle()
{
    return QStringLiteral(
        "QPushButton {"
        "  background: #005858; color: #e0e0e0;"
        "  border: 1px solid #008080; border-radius: 2px;"
        "  font-size: 9px; font-weight: bold; min-height: 24px;"
        "}"
        "QPushButton:hover   { background: #007070; }"
        "QPushButton:pressed { background: #003838; }"
        "QPushButton:disabled{ background: #1a2828; color: #3a5a5a; border-color: #2a4040; }"
    );
}

// ─── MainWindow ───────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("EXPERT Console — SPE 1K-FA Control");
    setMinimumSize(780, 440);

    // Always-dark palette: this is equipment control software; the display
    // should look like the physical unit regardless of the OS color scheme.
    // The macOS/Windows title bar continues to follow the OS automatically.
    QPalette pal;
    pal.setColor(QPalette::Window,                  QColor(30,  30,  30));
    pal.setColor(QPalette::WindowText,              QColor(210, 210, 210));
    pal.setColor(QPalette::Base,                    QColor(20,  20,  20));
    pal.setColor(QPalette::AlternateBase,           QColor(40,  40,  40));
    pal.setColor(QPalette::Text,                    QColor(210, 210, 210));
    pal.setColor(QPalette::Button,                  QColor(48,  48,  48));
    pal.setColor(QPalette::ButtonText,              QColor(210, 210, 210));
    pal.setColor(QPalette::BrightText,              Qt::white);
    pal.setColor(QPalette::Highlight,               QColor(0,   100, 180));
    pal.setColor(QPalette::HighlightedText,         Qt::white);
    pal.setColor(QPalette::ToolTipBase,             QColor(40,  40,  40));
    pal.setColor(QPalette::ToolTipText,             QColor(210, 210, 210));
    pal.setColor(QPalette::Disabled, QPalette::ButtonText,  QColor(90, 90, 90));
    pal.setColor(QPalette::Disabled, QPalette::WindowText,  QColor(90, 90, 90));
    pal.setColor(QPalette::Disabled, QPalette::Text,        QColor(90, 90, 90));
    // Apply app-wide so child dialogs (ConnectionDialog etc.) also get dark theme.
    qApp->setPalette(pal);
    setPalette(pal);

    m_controller = new AmplifierController(this);
    m_logger     = new TelemetryLogger(this);
    m_alarmDlg   = new AlarmHistoryDialog(this);
    // Parent = this so Qt auto-deletes on shutdown.
    // Qt::Window flag in CompactWindow ensures it's still a top-level window.
    m_compactWin = new CompactWindow(m_controller, this);
    connect(m_compactWin, &CompactWindow::requestFullView,
            this, &MainWindow::onSwitchToFull);

    connect(m_controller, &AmplifierController::statusUpdated,
            this, &MainWindow::onStatusUpdated);
    connect(m_controller, &AmplifierController::connectionChanged,
            this, &MainWindow::onConnectionChanged);
    connect(m_controller, &AmplifierController::errorOccurred,
            this, &MainWindow::onError);
    connect(m_controller, &AmplifierController::nakReceived,
            this, &MainWindow::onNak);
    connect(m_controller, &AmplifierController::dtrChanged,
            this, &MainWindow::onDtrChanged);
    connect(m_controller, &AmplifierController::statusUpdated,
            m_logger, &TelemetryLogger::logStatus);
    connect(m_logger, &TelemetryLogger::loggingStarted,
            this, [this](const QString& path){
                m_statusLogLabel->setText(tr("Logging: %1").arg(path));
                m_logStartAction->setEnabled(false);
                m_logStopAction->setEnabled(true);
            });
    connect(m_logger, &TelemetryLogger::loggingStopped,
            this, [this]{
                m_statusLogLabel->setText(tr("Logging: Off"));
                m_logStartAction->setEnabled(true);
                m_logStopAction->setEnabled(false);
            });

    buildMenuBar();
    buildCentralWidget();

    m_statusConnLabel = new QLabel(tr("Not connected"));
    m_statusLogLabel  = new QLabel(tr("Logging: Off"));
    statusBar()->addWidget(m_statusConnLabel, 1);
    statusBar()->addPermanentWidget(m_statusLogLabel);
    statusBar()->setStyleSheet(
        "QStatusBar { background: #111; color: #888; font-size: 10px; }"
        "QStatusBar::item { border: none; }");

    updateConnectionUi(false);

    // Schedule auto-connect after the event loop starts so the window is
    // fully shown before any connection error dialog might appear.
    QTimer::singleShot(0, this, &MainWindow::autoConnectIfConfigured);

    // Restore always-on-top from the previous session.
    // Block signals so the toggled handler (which calls show()) doesn't fire
    // during construction before the window is first shown.
    {
        QSettings qs;
        if (qs.value("view/alwaysOnTop", false).toBool()) {
            m_onTopAction->blockSignals(true);
            m_onTopAction->setChecked(true);
            m_onTopAction->blockSignals(false);
            setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
        }

        // Restore the last view mode. Use singleShot so switching happens
        // after main() calls show() on this window.
        if (qs.value("view/compactMode", false).toBool()) {
            // Pre-check the action so it reads correctly if the user somehow
            // sees the menu before the singleShot fires.
            m_compactViewAction->blockSignals(true);
            m_compactViewAction->setChecked(true);
            m_compactViewAction->blockSignals(false);
            QTimer::singleShot(0, this, &MainWindow::onSwitchToCompact);
        }
    }
}

MainWindow::~MainWindow() = default;

void MainWindow::closeEvent(QCloseEvent* event)
{
    m_compactWin->saveGeometry();
    m_compactWin->hide();   // hide without triggering requestFullView
    m_logger->stop();
    m_controller->disconnectFromAmplifier();
    event->accept();
}

// ─── Menu ─────────────────────────────────────────────────────────────────────

void MainWindow::buildMenuBar()
{
    auto* fileMenu = menuBar()->addMenu(tr("&File"));
    m_connectAction    = fileMenu->addAction(tr("&Connect…"),   this, &MainWindow::onConnect);
    m_disconnectAction = fileMenu->addAction(tr("&Disconnect"), this, &MainWindow::onDisconnect);
    fileMenu->addSeparator();
    // Software OFF command — sends KEY_OFF (0x18) to the amp firmware.
    // Note: this is a firmware shutdown, distinct from DTR power control.
    auto* offAct = fileMenu->addAction(tr("Send Amp &Off Command"),
        this, [this]{ sendKey(KEY_OFF); });
    offAct->setToolTip(tr("Send the firmware OFF command to the amplifier.\n"
                          "Equivalent to pressing OFF on the front panel."));
    fileMenu->addSeparator();
    auto* quitAct = fileMenu->addAction(tr("&Quit"), this, &QWidget::close);
    quitAct->setShortcut(QKeySequence::Quit);

    auto* logMenu = menuBar()->addMenu(tr("&Log"));
    m_logStartAction = logMenu->addAction(tr("Start Logging…"), this, &MainWindow::onStartLogging);
    m_logStopAction  = logMenu->addAction(tr("Stop Logging"),   this, &MainWindow::onStopLogging);

    auto* viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(tr("Alarm History…"), this, &MainWindow::onShowAlarmHistory);
    viewMenu->addSeparator();
    m_compactViewAction = viewMenu->addAction(tr("Compact View"));
    m_compactViewAction->setCheckable(true);
    m_compactViewAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_K));
    m_compactViewAction->setToolTip(
        tr("Switch between the full window and the compact meter/control view.\n"
           "Shortcut: Ctrl+K"));
    connect(m_compactViewAction, &QAction::toggled, this, [this](bool on) {
        if (on) onSwitchToCompact();
        else    onSwitchToFull();
    });
    viewMenu->addSeparator();
    m_onTopAction = viewMenu->addAction(tr("Always on Top"));
    m_onTopAction->setCheckable(true);
    m_onTopAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_T));
    connect(m_onTopAction, &QAction::toggled, this, [this](bool on) {
        QSettings qs;
        qs.setValue("view/alwaysOnTop", on);
        const Qt::WindowFlags flags = windowFlags();
        if (on)
            setWindowFlags(flags | Qt::WindowStaysOnTopHint);
        else
            setWindowFlags(flags & ~Qt::WindowStaysOnTopHint);
        show();  // required after setWindowFlags to re-show the window
    });

    menuBar()->setStyleSheet(
        "QMenuBar { background: #141414; color: #ccc; }"
        "QMenuBar::item:selected { background: #2a2a2a; }"
        "QMenu { background: #1e1e1e; color: #ccc; border: 1px solid #444; }"
        "QMenu::item:selected { background: #005880; }");
}

// ─── Central widget ───────────────────────────────────────────────────────────

void MainWindow::buildCentralWidget()
{
    auto* central = new QWidget(this);
    central->setStyleSheet(QString("QWidget { background: %1; }").arg(k_darkBg));
    setCentralWidget(central);

    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Title header ──────────────────────────────────────────────────────────
    auto* titleLabel = new QLabel(
        tr("EXPERT 1K-FA Fully Automatic Solid State Linear Amplifier"));
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "QLabel { background: #141414; color: #b0b8c0; font-size: 13px;"
        "         font-weight: bold; padding: 6px; "
        "         border-bottom: 1px solid #303030; }");
    root->addWidget(titleLabel);

    // ── Main row: left buttons | center display | right buttons ───────────────
    auto* mainRow = new QHBoxLayout;
    mainRow->setContentsMargins(4, 4, 4, 4);
    mainRow->setSpacing(4);
    mainRow->addWidget(buildLeftButtons());
    mainRow->addWidget(buildCenterDisplay(), 1);
    mainRow->addWidget(buildRightButtons());
    root->addLayout(mainRow, 1);

    // ── Info strip ────────────────────────────────────────────────────────────
    root->addWidget(buildInfoStrip());

    // ── Bottom button row ─────────────────────────────────────────────────────
    root->addWidget(buildBottomButtons());
}

// ─── Left buttons (L / C / TUNE) ─────────────────────────────────────────────

QWidget* MainWindow::buildLeftButtons()
{
    auto* w = new QWidget;
    w->setFixedWidth(72);
    auto* layout = new QVBoxLayout(w);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(4);

    auto makeBtn = [&](const QString& text, uint8_t key) -> QPushButton* {
        auto* b = new QPushButton(text);
        b->setStyleSheet(leftBtnStyle());
        b->setFixedHeight(28);
        connect(b, &QPushButton::clicked, this, [this, key]{ sendKey(key); });
        m_controlButtons.append(b);
        return b;
    };

    // Inductor
    auto* lLabel = new QLabel(tr("L"));
    lLabel->setAlignment(Qt::AlignCenter);
    lLabel->setStyleSheet("font-size: 9px;");
    layout->addWidget(lLabel);

    auto* lRow = new QHBoxLayout;
    lRow->setSpacing(3);
    lRow->addWidget(makeBtn(tr("L −"), KEY_L_MINUS));
    lRow->addWidget(makeBtn(tr("L +"), KEY_L_PLUS));
    layout->addLayout(lRow);

    layout->addSpacing(4);

    // Capacitor
    auto* cLabel = new QLabel(tr("C"));
    cLabel->setAlignment(Qt::AlignCenter);
    cLabel->setStyleSheet("font-size: 9px;");
    layout->addWidget(cLabel);

    auto* cRow = new QHBoxLayout;
    cRow->setSpacing(3);
    cRow->addWidget(makeBtn(tr("C −"), KEY_C_MINUS));
    cRow->addWidget(makeBtn(tr("C +"), KEY_C_PLUS));
    layout->addLayout(cRow);

    layout->addStretch();

    // TUNE
    auto* tuneBtn = new QPushButton(tr("TUNE"));
    tuneBtn->setStyleSheet(
        "QPushButton {"
        "  background: #1a3a20; color: #60d060;"
        "  border: 1px solid #2a6030; border-radius: 3px;"
        "  font-size: 10px; font-weight: bold;"
        "}"
        "QPushButton:hover   { background: #224a28; }"
        "QPushButton:pressed { background: #0e200e; }"
        "QPushButton:disabled{ background: #151a15; color: #2a4a2a; border-color: #1e2e1e; }");
    tuneBtn->setFixedHeight(32);
    connect(tuneBtn, &QPushButton::clicked, this, [this]{ sendKey(KEY_TUNE); });
    m_controlButtons.append(tuneBtn);
    layout->addWidget(tuneBtn);

    return w;
}

// ─── Center display ───────────────────────────────────────────────────────────

QWidget* MainWindow::buildCenterDisplay()
{
    auto* w = new QFrame;
    w->setFrameShape(QFrame::NoFrame);
    w->setStyleSheet(QString("QFrame { background: %1; border: 1px solid #303030; }").arg(k_displayBg));

    auto* layout = new QVBoxLayout(w);
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(4);

    // Mode status box
    m_modeBox = new QLabel(tr("NOT CONNECTED"));
    m_modeBox->setAlignment(Qt::AlignCenter);
    m_modeBox->setFixedHeight(26);
    m_modeBox->setStyleSheet(
        "QLabel { background: #1e1e1e; color: #606060;"
        "         border: 1px solid #303030; border-radius: 2px;"
        "         font-size: 12px; font-weight: bold; }");
    layout->addWidget(m_modeBox);

    // Setup / warning text area — hidden by default, expands to fill when shown.
    m_setupFrame = new QFrame;
    m_setupFrame->setFrameShape(QFrame::StyledPanel);
    m_setupFrame->setStyleSheet(
        "QFrame { background: #141a14; border: 1px solid #304030; border-radius: 2px; }");
    m_setupFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_setupFrame->setVisible(false);
    {
        auto* sfLayout = new QVBoxLayout(m_setupFrame);
        sfLayout->setContentsMargins(6, 4, 6, 4);
        m_setupText = new QLabel;
        m_setupText->setStyleSheet(
            "QLabel { color: #a0c0a0; font-family: monospace; font-size: 11px;"
            "         background: transparent; border: none; }");
        m_setupText->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        m_setupText->setWordWrap(false);     // preserve column alignment
        // Qt::AutoText (default) auto-detects HTML when text starts with '<'
        // so SetAntenna can return <pre>…</pre> while other cases use plain text
        sfLayout->addWidget(m_setupText);
        sfLayout->addStretch();
    }
    layout->addWidget(m_setupFrame, 1);   // stretch=1 so it fills when meters hidden

    // Meters container (scale hint + 4 bar meters) — hidden during setup display.
    m_metersWidget = new QWidget(w);
    m_metersWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    {
        auto* ml = new QVBoxLayout(m_metersWidget);
        ml->setContentsMargins(0, 0, 0, 0);
        ml->setSpacing(4);

        auto* scaleRow = new QHBoxLayout;
        scaleRow->setContentsMargins(80, 0, 70, 0);
        for (const QString& s : {tr("0"), tr("300"), tr("600"), tr("900"), tr("1200 W")}) {
            auto* l = new QLabel(s);
            l->setStyleSheet("color: #505860; font-size: 8px;");
            l->setAlignment(Qt::AlignHCenter);
            scaleRow->addWidget(l, 1);
        }
        ml->addLayout(scaleRow);

        m_meterPaOut = new MeterWidget(tr("PA OUT"), tr("W pep"), 1200.0, w);
        m_meterIpa   = new MeterWidget(tr("I PA"),   tr("A"),       50.0, w);
        m_meterPwRev = new MeterWidget(tr("PW REV"), tr("W pep"),  300.0, w);
        m_meterVpa   = new MeterWidget(tr("V PA"),   tr("V"),        60.0, w);

        ml->addWidget(m_meterPaOut);
        ml->addWidget(m_meterIpa);
        ml->addWidget(m_meterPwRev);
        ml->addWidget(m_meterVpa);
        ml->addStretch();
    }
    layout->addWidget(m_metersWidget, 1); // stretch=1 so it fills when setup hidden

    return w;
}

// ─── Right buttons (POWER / DISPLAY / OPERATE) ───────────────────────────────

QWidget* MainWindow::buildRightButtons()
{
    auto* w = new QWidget;
    w->setFixedWidth(74);
    auto* layout = new QVBoxLayout(w);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(4);

    // ── DTR amp power button (serial only) ───────────────────────────────────
    m_dtrButton = new QPushButton(tr("AMP\nOFF"));
    m_dtrButton->setFixedHeight(44);
    m_dtrButton->setEnabled(false);
    m_dtrButton->setToolTip(
        tr("Toggle amplifier mains power via DTR.\n"
           "Only available on serial connections.\n"
           "While DTR is held ON the front-panel power key is locked out."));
    m_dtrButton->setStyleSheet(
        "QPushButton {"
        "  background:#1e1e1e; color:#505050;"
        "  border: 1px solid #383838; border-radius: 3px;"
        "  font-size: 9px; font-weight: bold;"
        "}"
        "QPushButton:disabled{ background:#141414; color:#303030; border-color:#242424; }");
    connect(m_dtrButton, &QPushButton::clicked, this, &MainWindow::onDtrToggle);
    layout->addWidget(m_dtrButton);

    layout->addSpacing(4);

    auto makeBtn = [&](const QString& text, uint8_t key) -> QPushButton*
    {
        auto* b = new QPushButton(text);
        b->setStyleSheet(rightBtnStyle());
        connect(b, &QPushButton::clicked, this, [this, key]{ sendKey(key); });
        m_controlButtons.append(b);
        return b;
    };

    // POWER = KEY_MODE — cycles the output power level (HALF / FULL)
    auto* powerBtn = makeBtn(tr("POWER"), KEY_MODE);

    auto* dispBtn = makeBtn(tr("DISPLAY"), KEY_DISPLAY);

    // OPERATE always shown in green regardless of theme
    auto* operBtn = new QPushButton(tr("OPERATE"));
    operBtn->setMinimumHeight(40);
    operBtn->setStyleSheet(
        "QPushButton { background:#143a14; color:#60e060;"
        "  border: 1px solid #2a7a2a; border-radius: 3px;"
        "  font-size: 10px; font-weight: bold; }"
        "QPushButton:hover { background:#1a4a1a; }"
        "QPushButton:pressed { background:#0e200e; }"
        "QPushButton:disabled{ background:#0a1a0a; color:#204a20; border-color:#102a10; }");
    connect(operBtn, &QPushButton::clicked, this, [this]{ sendKey(KEY_OPERATE); });
    m_controlButtons.append(operBtn);

    layout->addWidget(powerBtn);
    layout->addWidget(dispBtn);
    layout->addStretch();
    layout->addWidget(operBtn);

    return w;
}

// ─── Info strip ───────────────────────────────────────────────────────────────

QWidget* MainWindow::buildInfoStrip()
{
    auto* w = new QWidget;
    w->setFixedHeight(48);
    w->setStyleSheet(QString("QWidget { background: %1; border-top: 1px solid #303030; }").arg(k_tealDark));

    auto* layout = new QHBoxLayout(w);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(0);

    auto addCell = [&](const QString& header, QLabel*& valueLabel,
                       bool stretch = false) -> QWidget*
    {
        auto* cell = new QWidget;
        cell->setStyleSheet("QWidget { background: transparent; border: none; }");
        auto* vl = new QVBoxLayout(cell);
        vl->setContentsMargins(6, 1, 6, 1);
        vl->setSpacing(1);

        auto* hdr = new QLabel(header);
        hdr->setAlignment(Qt::AlignCenter);
        hdr->setStyleSheet("color: #60a0a0; font-size: 8px; font-weight: bold;");
        vl->addWidget(hdr);

        valueLabel = new QLabel("--");
        valueLabel->setAlignment(Qt::AlignCenter);
        valueLabel->setStyleSheet(
            "color: #00e0e0; font-size: 11px; font-weight: bold;");
        vl->addWidget(valueLabel);

        auto* sep = new QFrame;
        sep->setFrameShape(QFrame::VLine);
        sep->setStyleSheet("color: #2a5050;");

        auto* row = new QHBoxLayout;
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(0);
        row->addWidget(cell, stretch ? 1 : 0);
        row->addWidget(sep);

        auto* container = new QWidget;
        container->setLayout(row);
        container->setStyleSheet("background: transparent;");
        layout->addWidget(container, stretch ? 1 : 0);

        return cell;
    };

    // For SWR/GAIN we need a mutable header label
    m_infoSwrGainHeader = new QLabel(tr("SWR"));
    m_infoSwrGainHeader->setAlignment(Qt::AlignCenter);
    m_infoSwrGainHeader->setStyleSheet("color: #60a0a0; font-size: 8px; font-weight: bold;");

    addCell(tr("IN"),   m_infoIn);
    addCell(tr("BAND"), m_infoBand);
    addCell(tr("ANT"),  m_infoAnt);
    addCell(tr("CAT"),  m_infoCat, true);
    addCell(tr("OUT"),  m_infoOut);

    // SWR/GAIN cell — manual build so we can swap the header
    {
        auto* cell = new QWidget;
        cell->setStyleSheet("QWidget { background: transparent; border: none; }");
        auto* vl = new QVBoxLayout(cell);
        vl->setContentsMargins(6, 1, 6, 1);
        vl->setSpacing(1);
        vl->addWidget(m_infoSwrGainHeader);
        m_infoSwrGain = new QLabel("--");
        m_infoSwrGain->setAlignment(Qt::AlignCenter);
        m_infoSwrGain->setStyleSheet("color: #00e0e0; font-size: 11px; font-weight: bold;");
        vl->addWidget(m_infoSwrGain);

        auto* sep = new QFrame;
        sep->setFrameShape(QFrame::VLine);
        sep->setStyleSheet("color: #2a5050;");

        layout->addWidget(cell);
        layout->addWidget(sep);
    }

    addCell(tr("TEMP"), m_infoTemp);

    return w;
}

// ─── Bottom buttons ───────────────────────────────────────────────────────────

QWidget* MainWindow::buildBottomButtons()
{
    auto* w = new QWidget;
    w->setFixedHeight(34);
    w->setStyleSheet(QString("QWidget { background: %1; border-top: 1px solid #2a5050; }").arg(k_tealDark));

    auto* layout = new QHBoxLayout(w);
    layout->setContentsMargins(4, 3, 4, 3);
    layout->setSpacing(3);

    struct BtnDef { QString label; uint8_t key; };
    const QList<BtnDef> buttons = {
        { tr("IN"),      KEY_IN         },
        { tr("← BAND"),  KEY_BAND_MINUS },
        { tr("BAND →"),  KEY_BAND_PLUS  },
        { tr("ANT"),     KEY_ANT        },
        { tr("CAT"),     KEY_CAT        },
        { tr("←"),       KEY_LEFT       },
        { tr("→"),       KEY_RIGHT      },
        { tr("SET"),     KEY_SET        },
    };

    for (const auto& def : buttons) {
        auto* b = new QPushButton(def.label);
        b->setStyleSheet(bottomBtnStyle());
        connect(b, &QPushButton::clicked, this, [this, key=def.key]{ sendKey(key); });
        m_controlButtons.append(b);
        layout->addWidget(b, 1);
    }

    return w;
}

// ─── Slots ────────────────────────────────────────────────────────────────────

void MainWindow::onConnect()
{
    ConnectionDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    auto conn = dlg.takeConnection();
    if (!conn) return;

    m_controller->setConnection(std::move(conn));
    if (!m_controller->connectToAmplifier()) {
        QMessageBox::warning(this, tr("Connection Failed"),
            tr("Could not open connection to the amplifier."));
    }
}

void MainWindow::onDisconnect()
{
    m_controller->disconnectFromAmplifier();
}

void MainWindow::onConnectionChanged(bool connected)
{
    updateConnectionUi(connected);

    if (connected) {
        const QString desc = m_controller->connection()
                             ? m_controller->connection()->description() : QString();
        m_statusConnLabel->setText(tr("Connected: %1").arg(desc));

        // Configure DTR button for serial connections
        const bool hasDtr = m_controller->supportsDtr();
        m_dtrButton->setEnabled(hasDtr);
        if (hasDtr) {
            // Start with DTR de-asserted (amp off); user explicitly turns it on
            onDtrChanged(false);
        } else {
            m_dtrButton->setText(tr("AMP PWR\n(TCP)"));
            m_dtrButton->setToolTip(tr("DTR power control is not available over TCP."));
            m_dtrButton->setStyleSheet(
                "QPushButton { background:#1a1a1a; color:#404040;"
                "  border:1px solid #303030; border-radius:3px;"
                "  font-size:9px; font-weight:bold; }");
        }
        m_modeBox->setText(tr("CONNECTED — WAITING FOR STATUS"));
        m_modeBox->setStyleSheet(
            "QLabel { background: #1a2a1a; color: #608060;"
            "         border: 1px solid #305030; border-radius: 2px;"
            "         font-size: 12px; font-weight: bold; }");
    } else {
        m_statusConnLabel->setText(tr("Not connected"));
        m_dtrButton->setEnabled(false);
        m_dtrButton->setText(tr("AMP\nOFF"));
        m_dtrButton->setStyleSheet(
            "QPushButton { background:#1e1e1e; color:#505050;"
            "  border:1px solid #383838; border-radius:3px;"
            "  font-size:9px; font-weight:bold; }"
            "QPushButton:disabled{ background:#141414; color:#303030; border-color:#242424; }");
        m_modeBox->setText(tr("NOT CONNECTED"));
        m_modeBox->setStyleSheet(
            "QLabel { background: #1e1e1e; color: #606060;"
            "         border: 1px solid #303030; border-radius: 2px;"
            "         font-size: 12px; font-weight: bold; }");
        m_setupFrame->setVisible(false);
        m_metersWidget->setVisible(true);
        m_meterPaOut->setValue(0); m_meterIpa->setValue(0);
        m_meterPwRev->setValue(0); m_meterVpa->setValue(0);
        m_infoIn->setText("--");   m_infoBand->setText("--");
        m_infoAnt->setText("--");  m_infoCat->setText("--");
        m_infoOut->setText("--");  m_infoSwrGain->setText("--");
        m_infoTemp->setText("--");
    }
}

void MainWindow::onStatusUpdated(const StatusPacket& s)
{
    // ── Mode box ──────────────────────────────────────────────────────────────
    updateModeBox(s);

    // ── Setup / warning text area ─────────────────────────────────────────────
    updateSetupText(s);

    // ── Meters ────────────────────────────────────────────────────────────────
    const double paMax = s.operate ? (s.fullMode ? 1200.0 : 600.0) : 200.0;
    m_meterPaOut->setMaxValue(paMax);
    m_meterPaOut->setValue(s.paOutputWpep());
    m_meterIpa->setValue(s.supplyCurrent());
    m_meterPwRev->setValue(s.reversePowerWpep());
    // V PA is only meaningful while in operate; zero it in standby
    m_meterVpa->setValue(s.operate ? s.supplyVoltage() : 0.0);

    // ── Info strip ────────────────────────────────────────────────────────────
    m_infoIn->setText(QString("IN %1").arg(s.input + 1));
    m_infoBand->setText(QString::fromLatin1(bandName(s.band)));
    m_infoAnt->setText(QString::fromLatin1(antennaName(s.antenna)).remove("ANT "));
    m_infoCat->setText(QString::fromLatin1(catName(s.catInterface)));
    m_infoOut->setText(s.fullMode ? tr("FULL") : tr("HALF"));

    if (!s.operate) {
        m_infoSwrGainHeader->setText(tr("SWR"));
        if (s.swrOrGainRaw == SWR_NO_SIGNAL)
            m_infoSwrGain->setText("--");
        else if (s.swrOrGainRaw == SWR_INFINITY)
            m_infoSwrGain->setText("∞");
        else
            m_infoSwrGain->setText(QString("%1:1").arg(s.swr(), 0, 'f', 2));
    } else {
        m_infoSwrGainHeader->setText(tr("PA GAIN"));
        m_infoSwrGain->setText(QString("%1 dB").arg(s.gainDb(), 0, 'f', 1));
    }

    m_infoTemp->setText(QString("%1°%2")
        .arg(s.temperature)
        .arg(s.tempCelsius ? 'C' : 'F'));

    // ── TCP "amp is on" indicator ─────────────────────────────────────────────
    // Over TCP we have no DTR control, but receiving a STATUS packet proves the
    // amp is powered and responding.  Light the button green so the operator
    // can see the amp is live (button stays disabled — clicking does nothing).
    if (!m_controller->supportsDtr()) {
        m_dtrButton->setText(tr("AMP ON\n(TCP)"));
        m_dtrButton->setToolTip(
            tr("Amplifier is on and responding.\n"
               "Physical power control via DTR is not available over TCP."));
        m_dtrButton->setStyleSheet(
            "QPushButton {"
            "  background:#143a14; color:#60e060;"
            "  border:1px solid #2a7a2a; border-radius:3px;"
            "  font-size:9px; font-weight:bold;"
            "}"
            "QPushButton:disabled{ background:#0d260d; color:#3a8a3a;"
            "  border-color:#1a5a1a; }");
    }

    // ── Alarm history auto-update ─────────────────────────────────────────────
    if (s.displayCtx == DisplayContext::AlarmHistory && m_alarmDlg->isVisible())
        m_alarmDlg->updateFromStatus(s);
}

void MainWindow::updateModeBox(const StatusPacket& s)
{
    QString text;
    QString style;

    if (s.alarmActive) {
        text  = tr("⚠  ALARM  ⚠");
        style = "background:#3a0a0a; color:#ff4040; border: 1px solid #7a1010;";
    } else if (s.tuning) {
        text  = tr("TUNING…");
        style = "background:#0a2a3a; color:#40c0ff; border: 1px solid #106080;";
    } else if (s.operate) {
        text  = tr("OPERATE MODE");
        style = "background:#0a280a; color:#40e040; border: 1px solid #106010;";
    } else {
        // Standby — show what the display context is
        switch (s.displayCtx) {
        case DisplayContext::Logo:
            text  = tr("STANDBY");
            style = "background:#1e1e1e; color:#808080; border: 1px solid #383838;";
            break;
        case DisplayContext::SetupOptions:
        case DisplayContext::SetAntenna:
        case DisplayContext::SetCat:
        case DisplayContext::SetYaesu:
        case DisplayContext::SetIcom:
        case DisplayContext::SetTenTec:
        case DisplayContext::SetBaudrate:
        case DisplayContext::ManualTune:
        case DisplayContext::Backlight:
            text  = tr("SETUP MODE");
            style = "background:#2a2000; color:#e0c000; border: 1px solid #605000;";
            break;
        case DisplayContext::Shutdown:
            text  = tr("SHUTTING DOWN…");
            style = "background:#2a0a0a; color:#e04040; border: 1px solid #601010;";
            break;
        default: {
            const char* warn = StatusPacket::warningText(s.displayCtx);
            if (warn[0]) {
                text  = QString::fromLatin1(warn);
                style = "background:#2a1400; color:#ff8020; border: 1px solid #604010;";
            } else {
                text  = tr("STANDBY");
                style = "background:#1e1e1e; color:#808080; border: 1px solid #383838;";
            }
            break;
        }
        }
    }

    m_modeBox->setText(text);
    m_modeBox->setStyleSheet(
        "QLabel { " + style + " border-radius: 2px; font-size: 12px; font-weight: bold; }");
}

void MainWindow::updateSetupText(const StatusPacket& s)
{
    const QString text = formatSetupText(s);
    const bool show = !text.isEmpty();
    // Hide meters when showing setup/warning text so the text area can expand.
    m_metersWidget->setVisible(!show);
    m_setupFrame->setVisible(show);
    if (show)
        m_setupText->setText(text);
}

QString MainWindow::formatSetupText(const StatusPacket& s) const
{
    // Show setup menu content mirroring what the amp LCD displays
    switch (s.displayCtx) {
    case DisplayContext::SetupOptions: {
        // Items 4-7 are toggle settings whose current state comes from the
        // FLAGS / STATUS_CODE bytes — show the value next to the name so the
        // operator can see the current setting without pressing into the sub-menu.
        struct Item { const char* name; QString value; };
        const Item items[9] = {
            {"ANTENNA",     {}},
            {"CAT",         {}},
            {"MANUAL TUNE", {}},
            {"BACKLIGHT",   {}},
            {"CONTEST",     s.contest        ? "ON"      : "OFF"},
            {"BEEP",        s.beep           ? "ON"      : "OFF"},
            {"START",       s.startupOperate ? "OPERATE" : "STANDBY"},
            {"TEMP.",       s.tempCelsius    ? "\xc2\xb0\x43" : "\xc2\xb0\x46"},
            {"QUIT",        {}},
        };
        const uint8_t sel = s.setup[1] & 0x0F;
        QString out = "SETUP OPTIONS:\n";
        for (uint8_t i = 0; i < 9; ++i) {
            const QString suffix = items[i].value.isEmpty()
                                   ? QString()
                                   : QString(": %1").arg(items[i].value);
            out += QString(i == sel ? " \xe2\x96\xba %1%2\n" : "   %1%2\n")
                       .arg(items[i].name).arg(suffix);
        }
        return out.trimmed();
    }
    case DisplayContext::SetCat: {
        const char* items[] = {
            "SPE","ICOM","KENWOOD","YAESU","TEN-TEC","FLEX-RADIO","RS-232","NONE"
        };
        const uint8_t sel = s.setup[1] & 0x0F;
        QString out = QString("SET CAT (IN %1):\n").arg(s.input + 1);
        for (uint8_t i = 0; i < 8; ++i)
            out += QString(i == sel ? " ► %1\n" : "   %1\n").arg(items[i]);
        return out.trimmed();
    }
    case DisplayContext::SetBaudrate: {
        // S[1] = selected baud rate index (0-based); 9600 is max
        static const char* kBauds[] = { "1200","2400","4800","9600" };
        constexpr int nBauds = static_cast<int>(std::size(kBauds));
        const uint8_t sel = s.setup[1];
        QString out = QString("SET BAUD RATE (IN %1):\n").arg(s.input + 1);
        out += QString("CAT: %1\n").arg(catName(s.catInterface));
        for (int i = 0; i < nBauds; ++i)
            out += QString(i == sel ? " ► %1\n" : "   %1\n").arg(kBauds[i]);
        return out.trimmed();
    }
    case DisplayContext::SetYaesu: {
        // S[1] = selected Yaesu model index (0-based)
        // List matches amp LCD 3-column grid (col-major order, 5 rows × 3 cols)
        static const char* kModels[] = {
            "FT100","FT757 GX2","FT817/847","FT840/890","FT897",
            "FT900","FT920","FT990","FT1000","FT1000 MP1",
            "FT1000 MP2","FT1000 MP3","FT2000","FT9000D","Band_Data"
        };
        constexpr int nModels = static_cast<int>(std::size(kModels));
        const uint8_t sel = s.setup[1];
        QString out = QString("SET YAESU (IN %1):\n").arg(s.input + 1);
        for (int i = 0; i < nModels; ++i)
            out += QString(i == sel ? " ► %1\n" : "   %1\n").arg(kModels[i]);
        return out.trimmed();
    }
    case DisplayContext::SetIcom: {
        // S[1] = CI-V address (hex byte) — NOT a model list
        // The amp lets you scroll through CI-V hex addresses to match your radio
        const uint8_t addr = s.setup[1];
        QString out = QString("SET ICOM CI-V (IN %1):\n").arg(s.input + 1);
        out += QString("CI-V Address: 0x%1\n\n").arg(addr, 2, 16, QChar('0')).toUpper();
        // Common CI-V addresses for reference
        static const struct { uint8_t addr; const char* model; } kRef[] = {
            {0x04,"IC-735"}, {0x38,"IC-720"}, {0x44,"IC-738"},
            {0x4E,"IC-706 MkII"}, {0x50,"IC-756"}, {0x58,"IC-706/706MkIIG"},
            {0x5C,"IC-756PRO"}, {0x5E,"IC-718"}, {0x64,"IC-756PRO2"},
            {0x66,"IC-746/746PRO/7400"}, {0x6A,"IC-7800"}, {0x6E,"IC-756PRO3"},
            {0x70,"IC-7000"}, {0x74,"IC-7700"}, {0x76,"IC-7200"},
            {0x7A,"IC-7600"}, {0x7C,"IC-9100"}, {0x80,"IC-7410"},
            {0x88,"IC-7100"}, {0x94,"IC-7300"},
        };
        out += "Common CI-V addresses:\n";
        for (const auto& r : kRef)
            out += QString("%1 0x%2 — %3\n")
                       .arg(r.addr == addr ? "►" : " ")
                       .arg(r.addr, 2, 16, QChar('0')).toUpper()
                       .arg(r.model);
        return out.trimmed();
    }
    case DisplayContext::SetTenTec: {
        // S[1] = selected Ten-Tec model index (0-based)
        static const char* kModels[] = {
            "Orion","Orion II","Omni VI","Omni VII","Eagle","Argonaut VI"
        };
        constexpr int nModels = static_cast<int>(std::size(kModels));
        const uint8_t sel = s.setup[1];
        QString out = QString("SET TEN-TEC (IN %1):\n").arg(s.input + 1);
        for (int i = 0; i < nModels; ++i)
            out += QString(i == sel ? " ► %1\n" : "   %1\n").arg(kModels[i]);
        return out.trimmed();
    }
    case DisplayContext::SetAntenna: {
        // S[0]: bandIdx = raw >> 1  (0-9 = 160m..6m, >=10 = SAVE)
        //        pos    = raw & 1   (0 = editing Opt1, 1 = editing Opt2)
        //
        // S[bandIdx+1]: one byte per band
        //   bits[2:0] = Opt1 antenna (0=ANT1..3=ANT4, >=4=None)
        //   bits[6:4] = Opt2 antenna (same encoding)
        //   bit3, bit7 = preset flags (ignored for display)
        const uint8_t raw0    = s.setup[0];
        const int     bandIdx = raw0 >> 1;
        const int     pos     = raw0 & 0x01;  // 0=Opt1, 1=Opt2

        static const char* kBands[10] = {
            "160m","80m","40m","30m","20m","17m","15m","12m","10m","6m"
        };

        // 0=ANT1, 1=ANT2, 2=ANT3, 3=ANT4, >=4=None
        auto antName = [](int raw) -> QString {
            return (raw <= 3) ? QString::number(raw + 1) : QString("-");
        };

        auto cellHtml = [&](int b) -> QString {
            const uint8_t v      = s.setup[static_cast<size_t>(b + 1)];
            const int     opt1   = v & 0x07;          // bits[2:0]
            const int     opt2   = (v >> 4) & 0x07;   // bits[6:4]
            const QString s1     = antName(opt1).toHtmlEscaped();
            const QString s2     = antName(opt2).toHtmlEscaped();
            const bool    curHere = (b == bandIdx);
            const QString band   = QString(kBands[b]).leftJustified(4).toHtmlEscaped();

            const QString hl = "background-color:#2a4a2a;color:#b0ffb0;";
            QString o1html = curHere && pos == 0
                             ? "<span style='" + hl + "'>" + s1 + "</span>" : s1;
            QString o2html = curHere && pos == 1
                             ? "<span style='" + hl + "'>" + s2 + "</span>" : s2;

            return QString(" %1:%2/%3 ").arg(band).arg(o1html).arg(o2html);
        };

        const QString saveHtml = (bandIdx >= 10)
            ? "<span style='background-color:#2a4a2a;color:#b0ffb0;'> SAVE </span>"
            : " SAVE ";

        const QString header = " BAND :O1/O2 ";
        QString html = "<pre style='margin:0;'>";
        html += "SET ANTENNA:\n";
        html += header + " " + header + " " + header + "\n";
        html += cellHtml(0) + " " + cellHtml(4) + " " + cellHtml(8) + "\n";
        html += cellHtml(1) + " " + cellHtml(5) + " " + cellHtml(9) + "\n";
        html += cellHtml(2) + " " + cellHtml(6) + " " + saveHtml   + "\n";
        html += cellHtml(3) + " " + cellHtml(7) + "\n";

        if (bandIdx < 10) {
            const uint8_t bv  = s.setup[static_cast<size_t>(bandIdx + 1)];
            const int     raw = (pos == 0) ? (bv & 0x07) : ((bv >> 4) & 0x07);
            const QString cur = (raw <= 3) ? QString("ANT%1").arg(raw + 1) : "None";
            html += QString("\nEditing %1 Option %2 \xe2\x80\x94 currently %3")
                        .arg(kBands[bandIdx]).arg(pos + 1).arg(cur).toHtmlEscaped();
        } else {
            html += "\nPress SET to save changes";
        }

        html += "</pre>";
        return html;
    }
    case DisplayContext::ManualTune: {
        const uint8_t lOut = s.setup[1] & 0x7F;
        const uint16_t cOutRaw = static_cast<uint16_t>(s.setup[2]) |
                                 (static_cast<uint16_t>(s.setup[3] & 0x03) << 8);
        // Decode Cout from weighted bits
        static const double cWeights[] = {3.6,6.4,12.1,18.9,40.8,81.5,158.0,321.5,641.6,1250.0};
        double cOut = 0.0;
        for (int i = 0; i < 10; ++i)
            if (cOutRaw & (1 << i)) cOut += cWeights[i];
        return QString("MANUAL TUNE:\n  L = %1 µH\n  C = %2 pF")
            .arg(lOut / 10.0, 0, 'f', 1)
            .arg(cOut, 0, 'f', 1);
    }
    case DisplayContext::Backlight: {
        // s.setup[1] = raw brightness (0–255)
        const int level  = static_cast<int>(s.setup[1]);
        const int barLen = 24;
        const int pos    = static_cast<int>((level / 255.0) * (barLen - 1));
        // Render slider matching amp LCD: - [====|====] +
        QString bar;
        for (int i = 0; i < barLen; ++i)
            bar += (i == pos) ? '|' : '.';
        const int pct = static_cast<int>((level / 255.0) * 100 + 0.5);
        return QString("DISPLAY BACKLIGHT\n- [%1] +\nLevel: %2%  [SET]:SAVE")
                   .arg(bar).arg(pct);
    }
    case DisplayContext::DataStored: {
        return QString("DATA STORED");
    }
    case DisplayContext::CatInfo: {
        // Shown when the amp LCD cycles to the CAT / power-control page.
        // setup[1] carries the PC (power-control) level sent by the exciter
        // (0–100 for Flex-Radio/Kenwood/Yaesu; typically 0 for other types).
        QString out = QString("CAT INFO  [IN %1]:\n").arg(s.input + 1);
        out += QString("  Interface : %1\n").arg(catName(s.catInterface));
        if (s.catInterface != CatInterface::NONE &&
            s.catInterface != CatInterface::SPE) {
            const uint8_t pc = s.setup[1];
            out += QString("  PC        : %1\n").arg(pc, 3, 10, QChar('0'));
        }
        out += QString("  Frequency : %1 MHz").arg(s.frequencyMHz(), 0, 'f', 3);
        return out.trimmed();
    }
    case DisplayContext::AlarmHistory: {
        const uint8_t wrnNo = s.setup[0] & 0x0F;
        if (wrnNo == 0) return "ALARM HISTORY: (empty)";
        QString out = QString("ALARM HISTORY (%1 entries):\n").arg(wrnNo);
        for (uint8_t i = 0; i < wrnNo && i < 10; ++i) {
            const uint8_t entry = s.setup[1 + i];
            const char* in = (entry >> 7) ? "IN2" : "IN1";
            const auto ctx = static_cast<DisplayContext>(entry & 0x7F);
            const char* msg = StatusPacket::warningText(ctx);
            out += QString(" %1. [%2] %3\n").arg(i+1).arg(in).arg(msg);
        }
        return out.trimmed();
    }
    default:
        return {};
    }
}

void MainWindow::onError(const QString& message)
{
    statusBar()->showMessage(tr("Error: %1").arg(message), 5000);
}

void MainWindow::onNak()
{
    statusBar()->showMessage(tr("NAK — command rejected by amplifier"), 3000);
}

void MainWindow::onStartLogging()
{
    const QString defaultPath =
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
        + "/spe1kfa_"
        + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")
        + ".csv";

    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save Telemetry Log"), defaultPath,
        tr("CSV files (*.csv);;All files (*)"));
    if (path.isEmpty()) return;

    if (!m_logger->start(path))
        QMessageBox::warning(this, tr("Log Error"),
            tr("Could not open log file:\n%1").arg(path));
}

void MainWindow::onStopLogging()
{
    m_logger->stop();
}

void MainWindow::onShowAlarmHistory()
{
    m_alarmDlg->show();
    m_alarmDlg->raise();
    m_alarmDlg->activateWindow();
}

// ─── Helpers ──────────────────────────────────────────────────────────────────

void MainWindow::updateConnectionUi(bool connected)
{
    m_connectAction->setEnabled(!connected);
    m_disconnectAction->setEnabled(connected);
    m_logStartAction->setEnabled(connected && !m_logger->isLogging());
    m_logStopAction->setEnabled(m_logger->isLogging());

    for (auto* b : m_controlButtons)
        b->setEnabled(connected);
}

void MainWindow::sendKey(uint8_t keyCode)
{
    m_controller->sendCommand(CommandBuilder::keyPress(keyCode));
}

void MainWindow::onDtrToggle()
{
    // Toggle DTR: if currently asserted → de-assert (power off amp),
    // if de-asserted → assert (power on amp, 3-4.5 s startup).
    const bool newState = !m_controller->isDtrAsserted();

    if (newState) {
        // Confirm before powering on — DTR locks out front-panel OFF key
        const auto reply = QMessageBox::question(
            this,
            tr("Power On Amplifier"),
            tr("Assert DTR to power on the amplifier?\n\n"
               "Note: while DTR is held ON, the front-panel power key\n"
               "on the amplifier is disabled. Allow 3–5 seconds for startup."),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes);
        if (reply != QMessageBox::Yes)
            return;
    }

    m_controller->setDtr(newState);
}

void MainWindow::onSwitchToCompact()
{
    QSettings qs;
    qs.setValue("view/compactMode", true);
    m_compactViewAction->blockSignals(true);
    m_compactViewAction->setChecked(true);
    m_compactViewAction->blockSignals(false);
    m_compactWin->restoreGeometry();
    m_compactWin->show();
    m_compactWin->raise();
    hide();
}

void MainWindow::onSwitchToFull()
{
    QSettings qs;
    qs.setValue("view/compactMode", false);
    m_compactViewAction->blockSignals(true);
    m_compactViewAction->setChecked(false);
    m_compactViewAction->blockSignals(false);
    m_compactWin->saveGeometry();
    m_compactWin->hide();
    show();
    raise();
    activateWindow();
}

void MainWindow::autoConnectIfConfigured()
{
    QSettings qs;
    if (!qs.value("conn/tcp/autoConnect", false).toBool())
        return;
    const QString host = qs.value("conn/tcp/host").toString().trimmed();
    if (host.isEmpty())
        return;
    const quint16 port = static_cast<quint16>(qs.value("conn/tcp/port", 4000).toInt());

    auto* conn = new TcpConnection;
    conn->setHost(host);
    conn->setPort(port);
    m_controller->setConnection(std::unique_ptr<AbstractConnection>(conn));
    if (!m_controller->connectToAmplifier()) {
        QMessageBox::warning(this, tr("Auto-Connect Failed"),
            tr("Could not connect to %1:%2.\n"
               "Use File \xe2\x86\x92 Connect to try again.")
            .arg(host).arg(port));
    }
}

void MainWindow::onDtrChanged(bool asserted)
{
    if (asserted) {
        m_dtrButton->setText(tr("AMP\nON"));
        m_dtrButton->setToolTip(
            tr("DTR asserted — amplifier powering on (allow 3–5 s).\n"
               "Click to de-assert DTR and power off the amplifier."));
        m_dtrButton->setStyleSheet(
            "QPushButton {"
            "  background:#143a14; color:#60e060;"
            "  border:1px solid #2a7a2a; border-radius:3px;"
            "  font-size:9px; font-weight:bold;"
            "}"
            "QPushButton:hover { background:#1a4a1a; }");
    } else {
        m_dtrButton->setText(tr("AMP\nOFF"));
        m_dtrButton->setToolTip(
            tr("DTR de-asserted — amplifier is off.\n"
               "Click to assert DTR and power on the amplifier."));
        m_dtrButton->setStyleSheet(
            "QPushButton {"
            "  background:#3a1414; color:#e06060;"
            "  border:1px solid #7a2a2a; border-radius:3px;"
            "  font-size:9px; font-weight:bold;"
            "}"
            "QPushButton:hover { background:#4a1a1a; }");
    }
}
