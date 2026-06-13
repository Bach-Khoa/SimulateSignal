#include "mainwindow.h"
#include "datareader.h"

#include <QApplication>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QRadioButton>
#include <QPushButton>
#include <QProgressBar>
#include <QTabWidget>
#include <QTextEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <QMenuBar>
#include <QSerialPortInfo>
#include <QScrollBar>
#include <QButtonGroup>
#include <QStackedWidget>
#include <QFrame>

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChart>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLegend>

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("IMU 5A Simulator — KERNEL ICD 125 Hz RS422");
    setMinimumSize(920, 720);
    resize(1100, 780);

    m_transport = new SerialTransport(this);
    m_guide     = new GuideDialog(this);

    buildUi();
    onRefreshPorts();

    statusBar()->showMessage("Ready — select CSV file or use Manual Input, then choose a COM port.");
}

MainWindow::~MainWindow() {
    if (m_engine && m_engine->isRunning()) {
        m_engine->requestStop();
        m_engine->wait(3000);
    }
    m_transport->close();
}

// ---------------------------------------------------------------------------
// UI build
// ---------------------------------------------------------------------------

void MainWindow::buildUi() {
    auto* helpMenu = menuBar()->addMenu("&Help");
    auto* guideAct = helpMenu->addAction("&Guide (F1)");
    guideAct->setShortcut(Qt::Key_F1);
    connect(guideAct, &QAction::triggered, this, [this]{ showGuide(0); });

    auto* central = new QWidget(this);
    setCentralWidget(central);

    auto* mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(6);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    auto* topRow = new QHBoxLayout;
    topRow->addWidget(buildInputGroup(),     3);
    topRow->addWidget(buildTransportGroup(), 2);
    topRow->addWidget(buildControlGroup(),   2);
    mainLayout->addLayout(topRow);

    mainLayout->addWidget(buildBottomTabs(), 1);
}

QWidget* MainWindow::buildInputGroup() {
    auto* box = new QGroupBox("Data Input");
    auto* lay = new QVBoxLayout(box);
    lay->setSpacing(6);
    lay->setContentsMargins(8, 8, 8, 8);

    // --- Source selector ---
    auto* srcRow = new QHBoxLayout;
    srcRow->addWidget(new QLabel("Source:"));
    m_radioCsv    = new QRadioButton("CSV File");
    m_radioManual = new QRadioButton("Manual Input");
    m_radioCsv->setChecked(true);
    auto* srcGrp = new QButtonGroup(this);
    srcGrp->addButton(m_radioCsv);
    srcGrp->addButton(m_radioManual);
    srcRow->addWidget(m_radioCsv);
    srcRow->addWidget(m_radioManual);
    srcRow->addStretch();
    lay->addLayout(srcRow);

    // --- Stacked widget ---
    m_inputStack = new QStackedWidget;

    // ---- Page 0: CSV mode ----
    auto* csvPage = new QWidget;
    auto* csvLay  = new QFormLayout(csvPage);
    csvLay->setSpacing(6);

    auto* fileRow = new QHBoxLayout;
    m_fileEdit = new QLineEdit;
    m_fileEdit->setPlaceholderText("Select CSV file...");
    m_fileEdit->setReadOnly(true);
    auto* browseBtn   = new QPushButton("Browse");
    auto* templateBtn = new QPushButton("Template");
    connect(browseBtn,   &QPushButton::clicked, this, &MainWindow::onBrowse);
    connect(templateBtn, &QPushButton::clicked, this, &MainWindow::onTemplate);
    fileRow->addWidget(m_fileEdit, 1);
    fileRow->addWidget(browseBtn);
    fileRow->addWidget(templateBtn);
    csvLay->addRow("File:", fileRow);

    m_framesLabel = new QLabel("Loaded: 0 frames");
    csvLay->addRow("", m_framesLabel);

    m_inputStack->addWidget(csvPage);

    // ---- Page 1: Manual input mode ----
    auto* manPage = new QWidget;
    auto* manLay  = new QGridLayout(manPage);
    manLay->setSpacing(4);
    manLay->setContentsMargins(2, 2, 2, 2);

    auto mkDbl = [](double lo, double hi, double val, int dec) {
        auto* sb = new QDoubleSpinBox;
        sb->setRange(lo, hi);
        sb->setDecimals(dec);
        sb->setValue(val);
        sb->setMaximumWidth(100);
        return sb;
    };

    m_spnHeading = mkDbl(0,    360,    0.0,  2);
    m_spnPitch   = mkDbl(-90,  90,     0.0,  2);
    m_spnRollM   = mkDbl(-180, 180,    0.0,  2);
    m_spnGx      = mkDbl(-2000,2000,   0.0,  3);
    m_spnGy      = mkDbl(-2000,2000,   0.0,  3);
    m_spnGz      = mkDbl(-2000,2000,   0.0,  3);
    m_spnAx      = mkDbl(-16,  16,     0.0,  4);
    m_spnAy      = mkDbl(-16,  16,     0.0,  4);
    m_spnAz      = mkDbl(-16,  16,     1.0,  4);
    m_spnTemp    = mkDbl(-40,  85,    25.0,  1);
    m_spnUsw     = new QSpinBox;
    m_spnUsw->setRange(0, 65535);
    m_spnUsw->setValue(0);
    m_spnUsw->setMaximumWidth(100);

    // Row 0: Orientation header
    auto* oriHdr = new QLabel("<b>Orientation (deg)</b>");
    manLay->addWidget(oriHdr, 0, 0, 1, 6);

    // Row 1: Heading / Pitch / Roll
    manLay->addWidget(new QLabel("Heading:"), 1, 0);
    manLay->addWidget(m_spnHeading,           1, 1);
    manLay->addWidget(new QLabel("Pitch:"),   1, 2);
    manLay->addWidget(m_spnPitch,             1, 3);
    manLay->addWidget(new QLabel("Roll:"),    1, 4);
    manLay->addWidget(m_spnRollM,             1, 5);

    // Row 2: Gyro header
    auto* gyrHdr = new QLabel("<b>Gyro (dps)</b>");
    manLay->addWidget(gyrHdr, 2, 0, 1, 6);

    // Row 3: Gx / Gy / Gz
    manLay->addWidget(new QLabel("Gx:"),  3, 0);
    manLay->addWidget(m_spnGx,            3, 1);
    manLay->addWidget(new QLabel("Gy:"),  3, 2);
    manLay->addWidget(m_spnGy,            3, 3);
    manLay->addWidget(new QLabel("Gz:"),  3, 4);
    manLay->addWidget(m_spnGz,            3, 5);

    // Row 4: Accel header
    auto* accHdr = new QLabel("<b>Accel (g)</b>");
    manLay->addWidget(accHdr, 4, 0, 1, 6);

    // Row 5: Ax / Ay / Az
    manLay->addWidget(new QLabel("Ax:"),  5, 0);
    manLay->addWidget(m_spnAx,            5, 1);
    manLay->addWidget(new QLabel("Ay:"),  5, 2);
    manLay->addWidget(m_spnAy,            5, 3);
    manLay->addWidget(new QLabel("Az:"),  5, 4);
    manLay->addWidget(m_spnAz,            5, 5);

    // Row 6: Temp / USW
    manLay->addWidget(new QLabel("Temp (°C):"), 6, 0);
    manLay->addWidget(m_spnTemp,                6, 1);
    manLay->addWidget(new QLabel("USW:"),       6, 2);
    manLay->addWidget(m_spnUsw,                 6, 3);

    m_inputStack->addWidget(manPage);

    lay->addWidget(m_inputStack);

    // --- Loop / One-shot (chung cho cả CSV và Manual) ---
    auto* modeRow = new QHBoxLayout;
    modeRow->addWidget(new QLabel("Repeat:"));
    m_radioLoop = new QRadioButton("Loop (continuous)");
    m_radioOnce = new QRadioButton("One-shot");
    m_radioLoop->setChecked(true);
    auto* modeGrp = new QButtonGroup(this);
    modeGrp->addButton(m_radioLoop);
    modeGrp->addButton(m_radioOnce);
    modeRow->addWidget(m_radioLoop);
    modeRow->addWidget(m_radioOnce);
    modeRow->addStretch();
    lay->addLayout(modeRow);

    // Switch pages on source toggle
    connect(m_radioCsv, &QRadioButton::toggled, this, [this](bool on) {
        if (on) m_inputStack->setCurrentIndex(0);
    });
    connect(m_radioManual, &QRadioButton::toggled, this, [this](bool on) {
        if (on) m_inputStack->setCurrentIndex(1);
    });

    // Connect spinboxes for live update while running
    auto connectDbl = [this](QDoubleSpinBox* sb) {
        connect(sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &MainWindow::onManualValueChanged);
    };
    connectDbl(m_spnHeading); connectDbl(m_spnPitch); connectDbl(m_spnRollM);
    connectDbl(m_spnGx);      connectDbl(m_spnGy);    connectDbl(m_spnGz);
    connectDbl(m_spnAx);      connectDbl(m_spnAy);    connectDbl(m_spnAz);
    connectDbl(m_spnTemp);
    connect(m_spnUsw, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onManualValueChanged);

    return box;
}

QWidget* MainWindow::buildTransportGroup() {
    auto* box = new QGroupBox("Transport (RS422)");
    auto* lay = new QFormLayout(box);
    lay->setSpacing(5);

    auto* portRow = new QHBoxLayout;
    m_portCombo = new QComboBox;
    m_portCombo->setMinimumWidth(90);
    auto* refreshBtn = new QPushButton("⟳");
    refreshBtn->setFixedWidth(30);
    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::onRefreshPorts);
    portRow->addWidget(m_portCombo, 1);
    portRow->addWidget(refreshBtn);
    lay->addRow("Port:", portRow);

    m_baudCombo = new QComboBox;
    for (int b : {921600, 460800, 230400, 115200, 57600})
        m_baudCombo->addItem(QString::number(b), b);
    m_baudCombo->setCurrentIndex(0);
    lay->addRow("Baud rate:", m_baudCombo);

    lay->addRow(new QLabel("<b>Scale Factors (KERNEL ICD)</b>"));

    auto makeScale = [](double val, int dec, const QString& suffix) {
        auto* sb = new QDoubleSpinBox;
        sb->setRange(1e-6, 1.0);
        sb->setDecimals(dec);
        sb->setSingleStep(val * 0.1);
        sb->setValue(val);
        sb->setSuffix(" " + suffix);
        return sb;
    };
    m_angleLsb = makeScale(0.01,    2, "deg/LSB");
    m_gyroLsb  = makeScale(0.00001, 6, "dps/LSB");
    m_accelLsb = makeScale(0.00025, 6, "g/LSB");
    m_tempLsb  = makeScale(0.1,     2, "°C/LSB");
    lay->addRow("Angle:",  m_angleLsb);
    lay->addRow("Gyro:",   m_gyroLsb);
    lay->addRow("Accel:",  m_accelLsb);
    lay->addRow("Temp:",   m_tempLsb);

    return box;
}

QWidget* MainWindow::buildControlGroup() {
    auto* box = new QGroupBox("Control & Status");
    auto* lay = new QVBoxLayout(box);
    lay->setSpacing(6);

    auto* btnRow = new QHBoxLayout;
    m_startBtn = new QPushButton("▶  Start");
    m_stopBtn  = new QPushButton("■  Stop");
    m_startBtn->setStyleSheet("QPushButton { background: #2e7d32; color: white; font-weight: bold; padding: 6px; }");
    m_stopBtn ->setStyleSheet("QPushButton { background: #c62828; color: white; font-weight: bold; padding: 6px; }");
    m_stopBtn->setEnabled(false);
    auto* guideBtn = new QPushButton("?  Guide");
    connect(m_startBtn, &QPushButton::clicked, this, &MainWindow::onStart);
    connect(m_stopBtn,  &QPushButton::clicked, this, &MainWindow::onStop);
    connect(guideBtn,   &QPushButton::clicked, this, [this]{ showGuide(0); });
    btnRow->addWidget(m_startBtn);
    btnRow->addWidget(m_stopBtn);
    btnRow->addWidget(guideBtn);
    lay->addLayout(btnRow);

    m_progress = new QProgressBar;
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    m_progress->setTextVisible(true);
    lay->addWidget(m_progress);

    auto* grid = new QFormLayout;
    grid->setSpacing(3);
    m_rowLabel  = new QLabel("Row: 0 / 0");
    m_sentLabel = new QLabel("Sent: 0 packets");
    m_freqLabel = new QLabel("Freq: — Hz");
    grid->addRow("Progress:", m_rowLabel);
    grid->addRow("Total:",    m_sentLabel);
    grid->addRow("Rate:",     m_freqLabel);
    lay->addLayout(grid);
    lay->addStretch();

    return box;
}

// ---------------------------------------------------------------------------
// CANoe-style signal list (left panel)
// ---------------------------------------------------------------------------
QWidget* MainWindow::buildSignalList() {
    auto* panel = new QFrame;
    panel->setFrameShape(QFrame::StyledPanel);
    panel->setFixedWidth(175);
    panel->setStyleSheet("background:#f5f5f5;");

    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(2);

    auto* hdr = new QLabel("  Signals");
    hdr->setStyleSheet("background:#1f4e79; color:white; font-weight:bold;"
                       "padding:4px; font-size:11px;");
    layout->addWidget(hdr);

    struct Entry { QColor color; QString name; };
    const QList<Entry> entries = {
        { QColor(220,50,50),   "Heading (deg)" },
        { QColor(50,180,50),   "Pitch (deg)"   },
        { QColor(50,100,220),  "Roll (deg)"    },
        { QColor(160,160,160), "──────────"    },
        { QColor(220,50,50),   "Gx (dps)"      },
        { QColor(50,180,50),   "Gy (dps)"      },
        { QColor(50,100,220),  "Gz (dps)"      },
        { QColor(160,160,160), "──────────"    },
        { QColor(220,50,50),   "Ax (g)"        },
        { QColor(50,180,50),   "Ay (g)"        },
        { QColor(50,100,220),  "Az (g)"        },
        { QColor(160,160,160), "──────────"    },
        { QColor(230,130,0),   "Temp (°C)"     },
    };

    for (const auto& e : entries) {
        if (e.name.startsWith("──")) {
            auto* sep = new QFrame;
            sep->setFrameShape(QFrame::HLine);
            sep->setStyleSheet("color:#cccccc;");
            layout->addWidget(sep);
            continue;
        }
        auto* row = new QWidget;
        auto* rl  = new QHBoxLayout(row);
        rl->setContentsMargins(2, 1, 2, 1);
        rl->setSpacing(5);
        auto* swatch = new QLabel;
        swatch->setFixedSize(14, 14);
        swatch->setStyleSheet(QString("background:%1; border:1px solid #888;")
                                  .arg(e.color.name()));
        auto* lbl = new QLabel(e.name);
        lbl->setFont(QFont("", 9));
        rl->addWidget(swatch);
        rl->addWidget(lbl, 1);
        layout->addWidget(row);
    }

    layout->addStretch();
    return panel;
}

// ---------------------------------------------------------------------------
// Main chart area — CANoe style stacked lanes
// ---------------------------------------------------------------------------
QWidget* MainWindow::buildBottomTabs() {
    auto* outerSplit = new QSplitter(Qt::Horizontal);
    outerSplit->setHandleWidth(2);

    outerSplit->addWidget(buildSignalList());

    auto* laneSplit = new QSplitter(Qt::Vertical);
    laneSplit->setHandleWidth(1);
    laneSplit->setStyleSheet("QSplitter::handle { background:#cccccc; }");

    struct LaneDef {
        QChart*&      chart;
        QLineSeries*& s0;
        QLineSeries*& s1;
        QLineSeries*& s2;
        QStringList   names;
        QList<QColor> colors;
        QString       yLabel;
        bool          isLast;
    };

    QLineSeries *_d1 = nullptr, *_d2 = nullptr;
    QList<LaneDef> lanes = {
        { m_oriChart,   m_sRoll,  m_sPitch, m_sYaw,
          {"Heading","Pitch","Roll"},
          {QColor(220,50,50), QColor(50,180,50), QColor(50,100,220)},
          "deg", false },
        { m_gyroChart, m_sGx, m_sGy, m_sGz,
          {"Gx","Gy","Gz"},
          {QColor(220,50,50), QColor(50,180,50), QColor(50,100,220)},
          "dps", false },
        { m_accelChart, m_sAx, m_sAy, m_sAz,
          {"Ax","Ay","Az"},
          {QColor(220,50,50), QColor(50,180,50), QColor(50,100,220)},
          "g", false },
        { m_tempChart, m_sTemp, _d1, _d2,
          {"Temp"},
          {QColor(230,130,0)},
          "°C", true },
    };

    for (auto& ld : lanes) {
        QChartView* view = makeChartView(ld.chart, ld.s0, ld.s1, ld.s2,
                                         ld.names, ld.colors);
        styleSignalLane(ld.chart, ld.yLabel, ld.isLast);
        view->setMinimumHeight(80);
        view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        view->setRenderHint(QPainter::Antialiasing);
        laneSplit->addWidget(view);
    }
    laneSplit->setSizes({160, 160, 160, 130});
    outerSplit->addWidget(laneSplit);

    auto* logPanel  = new QWidget;
    auto* logLayout = new QVBoxLayout(logPanel);
    logLayout->setContentsMargins(2, 0, 0, 0);
    logLayout->setSpacing(2);
    auto* logHdr = new QLabel("  Packet Log (KERNEL ICD LE)");
    logHdr->setStyleSheet(
        "background:#2e75b6; color:white; font-weight:bold; padding:3px; font-size:10px;");
    logLayout->addWidget(logHdr);
    m_hexLog = new QTextEdit;
    m_hexLog->setReadOnly(true);
    m_hexLog->setFont(QFont("Courier New", 8));
    m_hexLog->setPlaceholderText("Hex dump...");
    logLayout->addWidget(m_hexLog, 1);
    auto* clearBtn = new QPushButton("Clear");
    clearBtn->setFixedHeight(20);
    connect(clearBtn, &QPushButton::clicked, m_hexLog, &QTextEdit::clear);
    logLayout->addWidget(clearBtn);
    outerSplit->addWidget(logPanel);

    outerSplit->setSizes({175, 680, 240});
    return outerSplit;
}

// ---------------------------------------------------------------------------
// Style lane
// ---------------------------------------------------------------------------
void MainWindow::styleSignalLane(QChart* c, const QString& yLabel, bool showXAxis) {
    c->setTitle(QString());
    c->setMargins(QMargins(0, 2, 4, 2));
    c->setBackgroundRoundness(0);
    c->setBackgroundBrush(Qt::white);
    c->legend()->setVisible(false);

    if (!c->axes(Qt::Horizontal).isEmpty()) {
        auto* axX = qobject_cast<QValueAxis*>(c->axes(Qt::Horizontal).first());
        if (axX) {
            axX->setLabelsVisible(showXAxis);
            axX->setTitleVisible(showXAxis);
            if (showXAxis) axX->setTitleText("Sample");
            axX->setGridLineVisible(true);
            axX->setGridLineColor(QColor(230, 230, 230));
            axX->setMinorGridLineVisible(false);
            axX->setTickCount(11);
        }
    }

    if (!c->axes(Qt::Vertical).isEmpty()) {
        auto* axY = qobject_cast<QValueAxis*>(c->axes(Qt::Vertical).first());
        if (axY) {
            axY->setTitleText(yLabel);
            axY->setTitleFont(QFont("", 8, QFont::Bold));
            axY->setTitleVisible(true);
            axY->setLabelsFont(QFont("", 8));
            axY->setLabelFormat("%.2f");
            axY->setTickCount(3);
            axY->setGridLineColor(QColor(230, 230, 230));
            axY->setMinorGridLineVisible(false);
        }
    }
}

QChartView* MainWindow::makeChartView(QChart*& chartOut,
                                       QLineSeries*& s0,
                                       QLineSeries*& s1,
                                       QLineSeries*& s2,
                                       const QStringList& names,
                                       const QList<QColor>& colors)
{
    chartOut = new QChart;
    chartOut->setAnimationOptions(QChart::NoAnimation);
    chartOut->legend()->setVisible(true);
    chartOut->legend()->setAlignment(Qt::AlignBottom);

    auto addSeries = [&](QLineSeries*& s, int i) {
        if (i >= names.size()) return;
        s = new QLineSeries;
        s->setName(names[i]);
        QPen pen(colors[i]);
        pen.setWidth(2);
        s->setPen(pen);
        for (int x = 0; x < CHART_POINTS; ++x) s->append(x, 0.0);
        chartOut->addSeries(s);
    };

    addSeries(s0, 0);
    addSeries(s1, 1);
    addSeries(s2, 2);

    chartOut->createDefaultAxes();
    if (auto* axisX = qobject_cast<QValueAxis*>(chartOut->axes(Qt::Horizontal).first()))
        axisX->setVisible(false);

    auto* view = new QChartView(chartOut);
    view->setRenderHint(QPainter::Antialiasing);
    return view;
}

// ---------------------------------------------------------------------------
// Chart update
// ---------------------------------------------------------------------------

void MainWindow::appendChartPoint(QLineSeries* s, double y) {
    if (!s) return;
    auto pts = s->points();
    for (int i = 0; i < pts.size() - 1; ++i)
        s->replace(i, i, pts[i + 1].y());
    s->replace(pts.size() - 1, pts.size() - 1, y);
}

void MainWindow::updateAllCharts(const ImuFrame& f) {
    appendChartPoint(m_sRoll,  f.yaw);    // Heading → first Ori series
    appendChartPoint(m_sPitch, f.pitch);
    appendChartPoint(m_sYaw,   f.roll);   // Roll → third Ori series

    appendChartPoint(m_sGx, f.gx);
    appendChartPoint(m_sGy, f.gy);
    appendChartPoint(m_sGz, f.gz);

    appendChartPoint(m_sAx, f.ax);
    appendChartPoint(m_sAy, f.ay);
    appendChartPoint(m_sAz, f.az);

    appendChartPoint(m_sTemp, f.temp);

    auto rescaleChart = [](QChart* c) {
        if (!c || c->axes(Qt::Vertical).isEmpty()) return;
        auto* axisY = qobject_cast<QValueAxis*>(c->axes(Qt::Vertical).first());
        if (!axisY) return;
        double lo =  1e18, hi = -1e18;
        for (auto* series : c->series()) {
            auto* ls = qobject_cast<QLineSeries*>(series);
            if (!ls) continue;
            for (const QPointF& pt : ls->points()) {
                if (pt.y() < lo) lo = pt.y();
                if (pt.y() > hi) hi = pt.y();
            }
        }
        if (hi <= lo) { lo -= 1.0; hi += 1.0; }
        double margin = (hi - lo) * 0.1;
        axisY->setRange(lo - margin, hi + margin);
    };

    for (QChart* c : {m_oriChart, m_gyroChart, m_accelChart, m_tempChart})
        rescaleChart(c);
}

// ---------------------------------------------------------------------------
// Manual frame helpers
// ---------------------------------------------------------------------------

ImuFrame MainWindow::getManualFrame() const {
    ImuFrame f;
    f.yaw   = static_cast<float>(m_spnHeading->value());
    f.pitch = static_cast<float>(m_spnPitch->value());
    f.roll  = static_cast<float>(m_spnRollM->value());
    f.gx    = static_cast<float>(m_spnGx->value());
    f.gy    = static_cast<float>(m_spnGy->value());
    f.gz    = static_cast<float>(m_spnGz->value());
    f.ax    = static_cast<float>(m_spnAx->value());
    f.ay    = static_cast<float>(m_spnAy->value());
    f.az    = static_cast<float>(m_spnAz->value());
    f.temp  = static_cast<float>(m_spnTemp->value());
    f.usw   = static_cast<uint16_t>(m_spnUsw->value());
    return f;
}

void MainWindow::onManualValueChanged() {
    if (m_engine && m_engine->isRunning() && m_radioManual->isChecked())
        m_engine->setLiveFrame(getManualFrame());
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void MainWindow::onBrowse() {
    QString path = QFileDialog::getOpenFileName(this,
        "Select CSV File", QString(), "CSV Files (*.csv);;All Files (*.*)");
    if (path.isEmpty()) return;

    m_fileEdit->setText(path);
    QString err;
    m_frames = DataReader::readCsv(path, err);
    if (!err.isEmpty()) {
        QMessageBox::warning(this, "Load Error", err);
        m_framesLabel->setText("Loaded: 0 frames");
        m_frames.clear();
        return;
    }
    m_framesLabel->setText(QString("Loaded: %1 frames (%.2f s)")
                               .arg(m_frames.size())
                               .arg(m_frames.size() / 125.0));
    m_progress->setValue(0);
    statusBar()->showMessage(QString("Loaded %1 frames from %2")
                                 .arg(m_frames.size()).arg(path));
}

void MainWindow::onTemplate() {
    QString path = QFileDialog::getSaveFileName(this,
        "Save Template CSV", "imu_template.csv", "CSV Files (*.csv)");
    if (path.isEmpty()) return;

    QString err;
    if (!DataReader::writeTemplate(path, err)) {
        QMessageBox::warning(this, "Template Error", err);
        return;
    }
    statusBar()->showMessage("Template saved: " + path);
    QMessageBox::information(this, "Template Created",
        QString("Template CSV saved to:\n%1\n\nEdit it, then load with Browse.").arg(path));
}

void MainWindow::onRefreshPorts() {
    m_portCombo->clear();
    for (const auto& info : QSerialPortInfo::availablePorts())
        m_portCombo->addItem(info.portName());
    if (m_portCombo->count() == 0)
        m_portCombo->addItem("(no ports found)");
}

void MainWindow::onStart() {
    bool isManual = m_radioManual->isChecked();

    if (!isManual && m_frames.isEmpty()) {
        QMessageBox::warning(this, "No Data", "Please load a CSV file first.");
        return;
    }
    if (m_portCombo->currentText().startsWith("(")) {
        QMessageBox::warning(this, "No Port", "Please select a valid COM port.");
        return;
    }

    m_scale.angle_lsb = m_angleLsb->value();
    m_scale.gyro_lsb  = m_gyroLsb->value();
    m_scale.accel_lsb = m_accelLsb->value();
    m_scale.temp_lsb  = m_tempLsb->value();

    int baud = m_baudCombo->currentData().toInt();
    if (!m_transport->open(m_portCombo->currentText(), baud)) {
        QMessageBox::critical(this, "Port Error",
            QString("Cannot open %1:\n%2").arg(m_portCombo->currentText())
                                          .arg(m_transport->lastError()));
        return;
    }

    if (m_engine) { m_engine->requestStop(); m_engine->wait(2000); delete m_engine; }
    m_engine = new SimulatorEngine(this);

    connect(m_engine, &SimulatorEngine::statsUpdated,  this, &MainWindow::onStatsUpdated);
    connect(m_engine, &SimulatorEngine::packetReady,   this, &MainWindow::onPacketReady);
    connect(m_engine, &SimulatorEngine::errorOccurred, this, &MainWindow::onSimError);
    connect(m_engine, &SimulatorEngine::finished,      this, &MainWindow::onSimFinished);

    if (isManual) {
        m_engine->setupManual(m_scale, m_transport, getManualFrame(), m_radioLoop->isChecked());
    } else {
        m_engine->setup(m_frames, m_scale, m_transport, m_radioLoop->isChecked());
    }

    m_lastSentTotal = 0;
    m_runTimer.start();
    m_progress->setValue(0);
    m_rowLabel->setText(isManual ? "Row: — (Manual)" :
                        QString("Row: 0 / %1").arg(m_frames.size()));
    m_sentLabel->setText("Sent: 0 packets");
    m_freqLabel->setText("Freq: — Hz");

    m_startBtn->setEnabled(false);
    m_stopBtn->setEnabled(true);
    statusBar()->showMessage(QString("Sending on %1 @ %2 baud — 125 Hz (%3)")
                                 .arg(m_portCombo->currentText()).arg(baud)
                                 .arg(isManual ? "Manual" : "CSV"));

    m_engine->start(QThread::TimeCriticalPriority);
}

void MainWindow::onStop() {
    if (m_engine) m_engine->requestStop();
}

void MainWindow::onStatsUpdated(int index, int total, qint64 sentTotal) {
    if (total > 0) {
        m_progress->setValue(static_cast<int>(100.0 * index / total));
        m_rowLabel->setText(QString("Row: %1 / %2").arg(index).arg(total));
    }
    qint64 bytes = m_transport->bytesSent();
    m_sentLabel->setText(QString("Sent: %1 pkts  (%2 bytes)").arg(sentTotal).arg(bytes));

    double elapsed = m_runTimer.elapsed() / 1000.0;
    if (elapsed > 0.5) {
        double hz = sentTotal / elapsed;
        m_freqLabel->setText(QString("Freq: %1 Hz").arg(hz, 0, 'f', 1));
    }
}

void MainWindow::onPacketReady(QByteArray packet, ImuFrame frame) {
    updateAllCharts(frame);

    QString hex;
    for (int i = 0; i < packet.size(); ++i) {
        if (i > 0 && i % 14 == 0) hex += "\n";
        hex += QString("%1 ").arg(static_cast<uint8_t>(packet[i]), 2, 16, QChar('0')).toUpper();
    }
    hex += QString("\n— Hdg=%.2f° Pitch=%.2f° Roll=%.2f°  "
                   "Gx=%.3f Gy=%.3f Gz=%.3f dps  "
                   "Ax=%.4f Ay=%.4f Az=%.4f g  T=%.1f°C  USW=%u\n")
               .arg(frame.yaw).arg(frame.pitch).arg(frame.roll)
               .arg(frame.gx).arg(frame.gy).arg(frame.gz)
               .arg(frame.ax).arg(frame.ay).arg(frame.az)
               .arg(frame.temp).arg(frame.usw);

    m_hexLog->append(hex);
    if (m_hexLog->document()->blockCount() > 500)
        m_hexLog->clear();
}

void MainWindow::onSimError(QString message) {
    statusBar()->showMessage("Error: " + message);
    QMessageBox::critical(this, "Simulator Error", message);
    onSimFinished();
}

void MainWindow::onSimFinished() {
    m_startBtn->setEnabled(true);
    m_stopBtn->setEnabled(false);
    m_transport->close();
    statusBar()->showMessage("Stopped.");
}

void MainWindow::showGuide(int tab) {
    m_guide->showTab(tab);
}
