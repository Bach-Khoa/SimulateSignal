#pragma once
#include <QVector>
#include <QString>
#include "imuframe.h"

class DataReader {
public:
    // Read CSV file → list of ImuFrame.
    // Expected columns (row 1 = header, skipped):
    //   Index, Roll(deg), Pitch(deg), Yaw(deg),
    //   Gx(dps), Gy(dps), Gz(dps),
    //   Ax(g), Ay(g), Az(g), Temp(C), USW
    static QVector<ImuFrame> readCsv(const QString& path, QString& errorOut);

    // Write a template CSV with sample data to path.
    static bool writeTemplate(const QString& path, QString& errorOut);
};
