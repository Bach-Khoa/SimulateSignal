#include "datareader.h"
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <cmath>

static bool parseBool(const QString& s) {
    QString t = s.trimmed().toLower();
    return (t == "true" || t == "1");
}

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
        // Need at least 49 columns (indices 0–48)
        if (cols.size() < 49) {
            errorOut = QString("Line %1: expected >=49 columns, got %2")
                           .arg(lineNum).arg(cols.size());
            return {};
        }

        ImuFrame f;
        // col[2]  = 5a_adc26   → temp (°C)
        // col[3]  = 5a_ml1     → roll (deg)
        // col[4]  = 5a_ml2     → pitch (deg)
        // col[5]  = 5a_ml3     → yaw (deg)
        // col[11] = 5a_duc1    → ax (g)
        // col[12] = 5a_duc2    → ay (g)
        // col[13] = 5a_duc3    → az (g)
        // col[15] = 5a_omega1  → gx (dps)
        // col[16] = 5a_omega2  → gy (dps)
        // col[17] = 5a_omega3  → gz (dps)
        // col[38..48]          → USW bits 0–10
        f.temp  = cols[2].trimmed().toFloat();
        f.roll  = cols[3].trimmed().toFloat();
        f.pitch = cols[4].trimmed().toFloat();
        f.yaw   = cols[5].trimmed().toFloat();
        f.ax    = cols[11].trimmed().toFloat();
        f.ay    = cols[12].trimmed().toFloat();
        f.az    = cols[13].trimmed().toFloat();
        f.gx    = cols[15].trimmed().toFloat();
        f.gy    = cols[16].trimmed().toFloat();
        f.gz    = cols[17].trimmed().toFloat();

        uint16_t usw = 0;
        for (int bit = 0; bit <= 10; ++bit)
            if (parseBool(cols[38 + bit]))
                usw |= static_cast<uint16_t>(1u << bit);
        f.usw = usw;

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
    out << "Index,Roll(deg),Pitch(deg),Yaw(deg),Gx(dps),Gy(dps),Gz(dps),"
           "Ax(g),Ay(g),Az(g),Temp(C),USW\n";

    // 125 rows = 1 second of data at 125 Hz, simple sine-wave demo
    for (int i = 0; i < 125; ++i) {
        double t = i * 0.008;  // 8ms step
        double roll  = 10.0 * std::sin(2 * M_PI * 0.5 * t);
        double pitch =  5.0 * std::cos(2 * M_PI * 0.5 * t);
        double yaw   = i * 0.1;
        double gx    = 5.0 * std::cos(2 * M_PI * 1.0 * t);
        double gy    = 3.0 * std::sin(2 * M_PI * 1.0 * t);
        double gz    = 1.0;
        double ax    = 0.05 * std::sin(2 * M_PI * 2.0 * t);
        double ay    = 0.03 * std::cos(2 * M_PI * 2.0 * t);
        double az    = 1.0;
        double temp  = 25.0 + 0.1 * std::sin(2 * M_PI * 0.1 * t);

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
