#include "datareader.h"
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <cmath>


QVector<ImuFrame> DataReader::readCsv(const QString& path, QString& errorOut) {
    QVector<ImuFrame> frames;
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errorOut = QString("Cannot open file: %1").arg(file.errorString());
        return frames;
    }

    QTextStream in(&file);
    in.setAutoDetectUnicode(true);
    int lineNum = 0;

    while (!in.atEnd()) {
        QString line = in.readLine();
        lineNum++;

        if (lineNum == 1) continue;  // skip header row
        if (line.trimmed().isEmpty()) continue;

        QStringList cols = line.split(',');
        // 12 columns: Index, Roll, Pitch, Yaw, Gx, Gy, Gz, Ax, Ay, Az, Temp, USW
        if (cols.size() < 12) {
            errorOut = QString("Line %1: expected 12 columns, got %2")
                           .arg(lineNum).arg(cols.size());
            return {};
        }

        ImuFrame f;
        // col[0] = Index  (ignored)
        // col[1] = Roll   (deg)
        // col[2] = Pitch  (deg)
        // col[3] = Yaw    (deg)
        // col[4] = Gx     (dps)
        // col[5] = Gy     (dps)
        // col[6] = Gz     (dps)
        // col[7] = Ax     (g)
        // col[8] = Ay     (g)
        // col[9] = Az     (g)
        // col[10]= Temp   (°C)
        // col[11]= USW    (decimal or 0x hex)
        f.roll  = cols[1].trimmed().toFloat();
        f.pitch = cols[2].trimmed().toFloat();
        f.yaw   = cols[3].trimmed().toFloat();
        f.gx    = cols[4].trimmed().toFloat();
        f.gy    = cols[5].trimmed().toFloat();
        f.gz    = cols[6].trimmed().toFloat();
        f.ax    = cols[7].trimmed().toFloat();
        f.ay    = cols[8].trimmed().toFloat();
        f.az    = cols[9].trimmed().toFloat();
        f.temp  = cols[10].trimmed().toFloat();

        QString uswStr = cols[11].trimmed();
        bool ok = false;
        if (uswStr.startsWith("0x") || uswStr.startsWith("0X"))
            f.usw = static_cast<uint16_t>(uswStr.toUInt(&ok, 16));
        else
            f.usw = static_cast<uint16_t>(uswStr.toUInt(&ok, 10));
        if (!ok) f.usw = 0;

        frames.append(f);
    }

    if (frames.isEmpty()) {
        errorOut = "No data rows found in file.";
    }
    return frames;
}

bool DataReader::writeTemplate(const QString& path, QString& errorOut) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        errorOut = QString("Cannot write template: %1").arg(file.errorString());
        return false;
    }

    QTextStream out(&file);
    // 12-column format: Index,Roll,Pitch,Yaw,Gx,Gy,Gz,Ax,Ay,Az,Temp,USW
    out << "Index,Roll(deg),Pitch(deg),Yaw(deg),Gx(dps),Gy(dps),Gz(dps),"
           "Ax(g),Ay(g),Az(g),Temp(C),USW\n";

    // 250 rows = 2 seconds at 125 Hz, sine-wave demo motion
    for (int i = 0; i < 250; ++i) {
        double t     = i * 0.008;
        double roll  = 15.0 * std::sin(2 * M_PI * 0.5 * t);
        double pitch =  8.0 * std::cos(2 * M_PI * 0.5 * t);
        double yaw   = i * 0.144;
        double gx    =  8.0  * std::cos(2 * M_PI * 1.0 * t);
        double gy    =  5.0  * std::sin(2 * M_PI * 1.0 * t);
        double gz    = 18.0  + 0.5  * std::sin(2 * M_PI * 0.2 * t);
        double ax    =  0.08 * std::sin(2 * M_PI * 2.0 * t);
        double ay    =  0.05 * std::cos(2 * M_PI * 2.0 * t);
        double az    =  1.0  + 0.01 * std::sin(2 * M_PI * 0.5 * t);
        double temp  = 25.0 + 0.5  * std::sin(2 * M_PI * 0.1 * t);

        out << QString("%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,0\n")
                   .arg(i)
                   .arg(roll,  0, 'f', 4)
                   .arg(pitch, 0, 'f', 4)
                   .arg(yaw,   0, 'f', 4)
                   .arg(gx,    0, 'f', 4)
                   .arg(gy,    0, 'f', 4)
                   .arg(gz,    0, 'f', 4)
                   .arg(ax,    0, 'f', 4)
                   .arg(ay,    0, 'f', 4)
                   .arg(az,    0, 'f', 4)
                   .arg(temp,  0, 'f', 4);
    }
    return true;
}
