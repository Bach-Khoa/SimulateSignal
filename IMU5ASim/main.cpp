#include <QApplication>
#include "mainwindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("IMU 5A Simulator");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("SimulateSignal");

    MainWindow w;
    w.show();
    return app.exec();
}
