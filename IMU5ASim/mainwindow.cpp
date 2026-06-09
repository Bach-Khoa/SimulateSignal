#include "mainwindow.h"
#include "datareader.h"

#include <QApplication>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
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

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChart>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLegend>

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("IMU 5A Simulator — 125 Hz RS422");
    setMinimumSize(900, 700);
    resize(1050, 760);

    m_transport = new SerialTransport(this);
    m_guide     = new GuideDialog(this);

    buildUi();
    onRefreshPorts();

    statusBar()->showMessage("Ready — load a CSV file and select a COM port.");
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
    // Menu
    auto* helpMenu = menuBar()->addMenu("&Help");
    auto* guideAct = helpMenu->addAction("&Guide (F1)");
    guideAct->setShortcut(Qt::Key_F1);
    connect(guideAct, &QAction::triggered, this, [this]{ showGuide(0); });

    // Central widget
    auto* central = new QWidget(this);
    setCentralWidget(central);

    auto* mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(6);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    // Top row: Input | Transport | Control
    auto* topRow = new QHBoxLayout;
    topRow->addWidget(buildInputGroup(),    3);
    topRow->addWidget(buildTransportGroup(), 2);
    topRow->addWidget(buildControlGroup(),  2);
    mainLayout->addLayout(topRow);

    // Bottom: charts + log
    mainLayout->addWidget(buildBottomTabs(), 1);
}

QWidget* MainWindow::buildInputGroup() {
    auto* box = new QGroupBox("Data Input");
    auto* lay = new QFormLayout(box);
    lay->setSpacing(6);

    // File row
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
    lay->addRow("File:", fileRow);

    m_framesLabel = new QLabel("Loaded: 0 frames");
    lay->addRow("", m_framesLabel);

    // Mode
    auto* modeRow = new QHBoxLayout;
    m_radioLoop = new QRadioButton("Loop");
    m_radioOnce = new QRadioButton("One-shot");
    m_radioLoop->setChecked(true);
    auto* modeGrp = new QButtonGroup(this);
    modeGrp->addButton(m_radioLoop);
    modeGrp->addButton(m_radioOnce);
    modeRow->addWidget(m_radioLoop);
    modeRow->addWidget(m_radioOnce);
    modeRow->addStretch();
    lay->addRow("Mode:", modeRow);

    return box;
}

QWidget* MainWindow::buildTransportGroup() {
    auto* box = new QGroupBox("Transport (RS422)");
    auto* lay = new QFormLayout(box);
    lay->setSpacing(5);

    // Port
    auto* portRow = new QHBoxLayout;
    m_portCombo = new QComboBox;
    m_portCombo->setMinimumWidth(90);
    auto* refreshBtn = new QPushButton("⟳");
    refreshBtn->setFixedWidth(30);
    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::onRefreshPorts);
    portRow->addWidget(m_portCombo, 1);
    portRow->addWidget(refreshBtn);
    lay->addRow("Port:", portRow);

    // Baud
    m_baudCombo = new QComboBox;
    for (int b : {921600, 460800, 230400, 115200, 57600})
        m_baudCombo->addItem(QString::number(b), b);
    m_baudCombo->setCurrentIndex(0);
    lay->addRow("Baud rate:", m_baudCombo);

    lay->addRow(new QLabel("<b>Scale Factors</b>"));

    auto makeScale = [](double val, const QString& suffix) {
        auto* sb = new QDoubleSpinBox;
        sb->setRange(1e-6, 1.0);
        sb->setDecimals(6);
        sb->setSingleStep(0.001);
        sb->setValue(val);
        sb->setSuffix(" " + suffix);
        return sb;
    };
    m_angleLsb = makeScale(0.01,   "deg/LSB");
    m_gyroLsb  = makeScale(0.001,  "dps/LSB");
    m_accelLsb = makeScale(0.001,  "g/LSB");
    m_tempLsb  = makeScale(0.01,   "°C/LSB");
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

    // Buttons
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

    // Progress
    m_progress = new QProgressBar;
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    m_progress->setTextVisible(true);
    lay->addWidget(m_progress);

    // Status labels
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

QTabWidget* MainWindow::buildBottomTabs() {
    auto* tabs = new QTabWidget;

    // Orientation chart
    QChartView* oriView = makeChartView(m_oriChart,
        m_sRoll, m_sPitch, m_sYaw,
        {"Roll", "Pitch", "Yaw"},
        {Qt::red, QColor(0,180,0), Qt::blue});
    m_oriChart->setTitle("Orientation Angles (deg)");
    tabs->addTab(oriView, "Orientation");

    // Gyro chart
    QChartView* gyroView = makeChartView(m_gyroChart,
        m_sGx, m_sGy, m_sGz,
        {"Gx", "Gy", "Gz"},
        {Qt::red, QColor(0,180,0), Qt::blue});
    m_gyroChart->setTitle("Gyro HR (dps)");
    tabs->addTab(gyroView, "Gyro");

    // Accel chart
    QChartView* accelView = makeChartView(m_accelChart,
        m_sAx, m_sAy, m_sAz,
        {"Ax", "Ay", "Az"},
        {Qt::red, QColor(0,180,0), Qt::blue});
    m_accelChart->setTitle("Accelerometer (g)");
    tabs->addTab(accelView, "Accel");

    // Temp chart (single series — reuse s0 slot)
    QLineSeries *dummy1 = nullptr, *dummy2 = nullptr;
    QChartView* tempView = makeChartView(m_tempChart,
        m_sTemp, dummy1, dummy2,
        {"Temp"},
        {QColor(255, 120, 0)});
    m_tempChart->setTitle("Temperature (°C)");
    tabs->addTab(tempView, "Temp");

    // Packet log
    m_hexLog = new QTextEdit;
    m_hexLog->setReadOnly(true);
    m_hexLog->setFont(QFont("Courier New", 9));
    m_hexLog->setPlaceholderText("Packet hex dump will appear here...");
    auto* logContainer = new QWidget;
    auto* logLayout    = new QVBoxLayout(logContainer);
    auto* clearBtn     = new QPushButton("Clear log");
    connect(clearBtn, &QPushButton::clicked, m_hexLog, &QTextEdit::clear);
    logLayout->addWidget(m_hexLog);
    logLayout->addWidget(clearBtn);
    tabs->addTab(logContainer, "Packet Log");

    return tabs;
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
    appendChartPoint(m_sRoll,  f.roll);
    appendChartPoint(m_sPitch, f.pitch);
    appendChartPoint(m_sYaw,   f.yaw);

    appendChartPoint(m_sGx, f.gx);
    appendChartPoint(m_sGy, f.gy);
    appendChartPoint(m_sGz, f.gz);

    appendChartPoint(m_sAx, f.ax);
    appendChartPoint(m_sAy, f.ay);
    appendChartPoint(m_sAz, f.az);

    appendChartPoint(m_sTemp, f.temp);

    // Auto-scale each chart Y axis
    for (QChart* c : {m_oriChart, m_gyroChart, m_accelChart, m_tempChart}) {
        if (c) c->axes(Qt::Vertical).first()->setRange(
            qobject_cast<QValueAxis*>(c->axes(Qt::Vertical).first())->min(),
            qobject_cast<QValueAxis*>(c->axes(Qt::Vertical).first())->max());
    }
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
        QString("Template CSV saved to:\n%1\n\nEdit it in Excel, then load with Browse.").arg(path));
}

void MainWindow::onRefreshPorts() {
    m_portCombo->clear();
    for (const auto& info : QSerialPortInfo::availablePorts())
        m_portCombo->addItem(info.portName());
    if (m_portCombo->count() == 0)
        m_portCombo->addItem("(no ports found)");
}

void MainWindow::onStart() {
    if (m_frames.isEmpty()) {
        QMessageBox::warning(this, "No Data", "Please load a CSV file first.");
        return;
    }
    if (m_portCombo->currentText().startsWith("(")) {
        QMessageBox::warning(this, "No Port", "Please select a valid COM port.");
        return;
    }

    // Gather scale config from UI
    m_scale.angle_lsb = m_angleLsb->value();
    m_scale.gyro_lsb  = m_gyroLsb->value();
    m_scale.accel_lsb = m_accelLsb->value();
    m_scale.temp_lsb  = m_tempLsb->value();

    // Open serial port
    int baud = m_baudCombo->currentData().toInt();
    if (!m_transport->open(m_portCombo->currentText(), baud)) {
        QMessageBox::critical(this, "Port Error",
            QString("Cannot open %1:\n%2").arg(m_portCombo->currentText())
                                          .arg(m_transport->lastError()));
        return;
    }

    // Create and connect engine
    if (m_engine) { m_engine->requestStop(); m_engine->wait(2000); delete m_engine; }
    m_engine = new SimulatorEngine(this);

    connect(m_engine, &SimulatorEngine::statsUpdated, this, &MainWindow::onStatsUpdated);
    connect(m_engine, &SimulatorEngine::packetReady,  this, &MainWindow::onPacketReady);
    connect(m_engine, &SimulatorEngine::errorOccurred, this, &MainWindow::onSimError);
    connect(m_engine, &SimulatorEngine::finished,      this, &MainWindow::onSimFinished);

    m_engine->setup(m_frames, m_scale, m_transport, m_radioLoop->isChecked());

    m_lastSentTotal = 0;
    m_runTimer.start();
    m_progress->setValue(0);
    m_rowLabel->setText(QString("Row: 0 / %1").arg(m_frames.size()));
    m_sentLabel->setText("Sent: 0 packets");
    m_freqLabel->setText("Freq: — Hz");

    m_startBtn->setEnabled(false);
    m_stopBtn->setEnabled(true);
    statusBar()->showMessage(QString("Sending on %1 @ %2 baud — 125 Hz")
                                 .arg(m_portCombo->currentText()).arg(baud));

    m_engine->start(QThread::TimeCriticalPriority);
}

void MainWindow::onStop() {
    if (m_engine) m_engine->requestStop();
}

void MainWindow::onStatsUpdated(int index, int total, qint64 sentTotal) {
    // Progress
    if (total > 0) {
        m_progress->setValue(static_cast<int>(100.0 * index / total));
        m_rowLabel->setText(QString("Row: %1 / %2").arg(index).arg(total));
    }
    m_sentLabel->setText(QString("Sent: %1 packets").arg(sentTotal));

    // Frequency estimate
    double elapsed = m_runTimer.elapsed() / 1000.0;
    if (elapsed > 0.5) {
        double hz = sentTotal / elapsed;
        m_freqLabel->setText(QString("Freq: %1 Hz").arg(hz, 0, 'f', 1));
    }
}

void MainWindow::onPacketReady(QByteArray packet, ImuFrame frame) {
    // Update charts
    updateAllCharts(frame);

    // Hex log
    QString hex;
    for (int i = 0; i < packet.size(); ++i) {
        if (i > 0 && i % 12 == 0) hex += "\n";
        hex += QString("%1 ").arg(static_cast<uint8_t>(packet[i]), 2, 16, QChar('0')).toUpper();
    }
    hex += QString("\n— Roll=%.2f° Pitch=%.2f° Yaw=%.2f°  "
                   "Gx=%.3f Gy=%.3f Gz=%.3f dps  "
                   "Ax=%.3f Ay=%.3f Az=%.3f g  T=%.1f°C\n")
               .arg(frame.roll).arg(frame.pitch).arg(frame.yaw)
               .arg(frame.gx).arg(frame.gy).arg(frame.gz)
               .arg(frame.ax).arg(frame.ay).arg(frame.az)
               .arg(frame.temp);

    m_hexLog->append(hex);
    // Keep log from growing too large
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
