#pragma once
#include <QMainWindow>
#include <QVector>
#include <QElapsedTimer>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChart>

#include "imuframe.h"
#include "scaleconfig.h"
#include "simulatorengine.h"
#include "serialtransport.h"
#include "guidedialog.h"

class QLabel;
class QLineEdit;
class QComboBox;
class QDoubleSpinBox;
class QSpinBox;
class QRadioButton;
class QPushButton;
class QProgressBar;
class QTabWidget;
class QTextEdit;
class QStackedWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onBrowse();
    void onTemplate();
    void onRefreshPorts();
    void onStart();
    void onStop();
    void onStatsUpdated(int index, int total, qint64 sentTotal);
    void onPacketReady(QByteArray packet, ImuFrame frame);
    void onSimError(QString message);
    void onSimFinished();
    void showGuide(int tab = 0);
    void onManualValueChanged();

private:
    // --- UI build helpers ---
    void buildUi();
    QWidget* buildInputGroup();
    QWidget* buildTransportGroup();
    QWidget* buildControlGroup();
    QWidget* buildBottomTabs();

    // --- Chart helpers ---
    QChartView* makeChartView(QChart*& chartOut,
                              QLineSeries*& s0, QLineSeries*& s1, QLineSeries*& s2,
                              const QStringList& names,
                              const QList<QColor>& colors);
    void styleSignalLane(QChart* c, const QString& yLabel, bool showXAxis);
    QWidget* buildSignalList();
    void appendChartPoint(QLineSeries* s, double y);
    void updateAllCharts(const ImuFrame& f);

    // --- Manual input helper ---
    ImuFrame getManualFrame() const;

    // --- State ---
    QVector<ImuFrame> m_frames;
    ScaleConfig       m_scale;
    SerialTransport*  m_transport = nullptr;
    SimulatorEngine*  m_engine    = nullptr;
    GuideDialog*      m_guide     = nullptr;
    QElapsedTimer     m_runTimer;
    qint64            m_lastSentTotal = 0;

    static constexpr int CHART_POINTS = 50;

    // --- CSV mode widgets ---
    QRadioButton*  m_radioCsv    = nullptr;
    QRadioButton*  m_radioManual = nullptr;
    QStackedWidget* m_inputStack = nullptr;
    QLineEdit*     m_fileEdit    = nullptr;
    QLabel*        m_framesLabel = nullptr;
    QRadioButton*  m_radioLoop   = nullptr;
    QRadioButton*  m_radioOnce   = nullptr;

    // --- Manual input spinboxes ---
    QDoubleSpinBox* m_spnHeading  = nullptr;
    QDoubleSpinBox* m_spnPitch    = nullptr;
    QDoubleSpinBox* m_spnRollM    = nullptr;
    QDoubleSpinBox* m_spnGx       = nullptr;
    QDoubleSpinBox* m_spnGy       = nullptr;
    QDoubleSpinBox* m_spnGz       = nullptr;
    QDoubleSpinBox* m_spnAx       = nullptr;
    QDoubleSpinBox* m_spnAy       = nullptr;
    QDoubleSpinBox* m_spnAz       = nullptr;
    QDoubleSpinBox* m_spnTemp     = nullptr;
    QSpinBox*       m_spnUsw      = nullptr;

    // --- Transport widgets ---
    QComboBox*     m_portCombo   = nullptr;
    QComboBox*     m_baudCombo   = nullptr;
    QDoubleSpinBox* m_angleLsb   = nullptr;
    QDoubleSpinBox* m_gyroLsb    = nullptr;
    QDoubleSpinBox* m_accelLsb   = nullptr;
    QDoubleSpinBox* m_tempLsb    = nullptr;

    // --- Control widgets ---
    QPushButton*   m_startBtn    = nullptr;
    QPushButton*   m_stopBtn     = nullptr;
    QProgressBar*  m_progress    = nullptr;
    QLabel*        m_rowLabel    = nullptr;
    QLabel*        m_sentLabel   = nullptr;
    QLabel*        m_freqLabel   = nullptr;
    QTextEdit*     m_hexLog      = nullptr;

    // Charts
    QChart*      m_oriChart   = nullptr;
    QChart*      m_gyroChart  = nullptr;
    QChart*      m_accelChart = nullptr;
    QChart*      m_tempChart  = nullptr;
    QLineSeries* m_sRoll  = nullptr, *m_sPitch = nullptr, *m_sYaw  = nullptr;
    QLineSeries* m_sGx    = nullptr, *m_sGy    = nullptr, *m_sGz   = nullptr;
    QLineSeries* m_sAx    = nullptr, *m_sAy    = nullptr, *m_sAz   = nullptr;
    QLineSeries* m_sTemp  = nullptr;
    int          m_chartX = 0;
};
