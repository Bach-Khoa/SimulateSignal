#include "packetbuilder.h"
#include <cmath>
#include <cstdint>

static inline int16_t toI16(double phys, double lsb) {
    double raw = std::round(phys / lsb);
    if (raw >  32767.0) raw =  32767.0;
    if (raw < -32768.0) raw = -32768.0;
    return static_cast<int16_t>(raw);
}

static inline int32_t toI32(double phys, double lsb) {
    double raw = std::round(phys / lsb);
    if (raw >  2147483647.0) raw =  2147483647.0;
    if (raw < -2147483648.0) raw = -2147483648.0;
    return static_cast<int32_t>(raw);
}

void PacketBuilder5A::writeI16BE(QByteArray& buf, int off, int16_t v) {
    buf[off]     = static_cast<char>((v >> 8) & 0xFF);
    buf[off + 1] = static_cast<char>(v & 0xFF);
}

void PacketBuilder5A::writeI32BE(QByteArray& buf, int off, int32_t v) {
    buf[off]     = static_cast<char>((v >> 24) & 0xFF);
    buf[off + 1] = static_cast<char>((v >> 16) & 0xFF);
    buf[off + 2] = static_cast<char>((v >> 8)  & 0xFF);
    buf[off + 3] = static_cast<char>(v & 0xFF);
}

void PacketBuilder5A::writeU16BE(QByteArray& buf, int off, uint16_t v) {
    buf[off]     = static_cast<char>((v >> 8) & 0xFF);
    buf[off + 1] = static_cast<char>(v & 0xFF);
}

QByteArray PacketBuilder5A::build(const ImuFrame& f, const ScaleConfig& s) {
    QByteArray pkt(42, '\0');

    // Fixed header
    pkt[0] = static_cast<char>(0xAA);
    pkt[1] = static_cast<char>(0x55);
    pkt[2] = static_cast<char>(0x01);  // message type
    pkt[3] = static_cast<char>(0x95);  // cmd code ID
    writeU16BE(pkt, 4, 0x0028);        // length = 40
    pkt[6] = static_cast<char>(0x05);  // num packages
    pkt[7] = static_cast<char>(0x07);
    pkt[8] = static_cast<char>(0x21);
    pkt[9] = static_cast<char>(0x22);
    pkt[10] = static_cast<char>(0x52);
    pkt[11] = static_cast<char>(0x53);

    // Orientation angles [12..17]
    writeI16BE(pkt, 12, toI16(f.roll,  s.angle_lsb));
    writeI16BE(pkt, 14, toI16(f.pitch, s.angle_lsb));
    writeI16BE(pkt, 16, toI16(f.yaw,   s.angle_lsb));

    // Gyro HR [18..29]
    writeI32BE(pkt, 18, toI32(f.gx, s.gyro_lsb));
    writeI32BE(pkt, 22, toI32(f.gy, s.gyro_lsb));
    writeI32BE(pkt, 26, toI32(f.gz, s.gyro_lsb));

    // Accelerometer [30..35]
    writeI16BE(pkt, 30, toI16(f.ax, s.accel_lsb));
    writeI16BE(pkt, 32, toI16(f.ay, s.accel_lsb));
    writeI16BE(pkt, 34, toI16(f.az, s.accel_lsb));

    // Temperature [36..37]
    writeI16BE(pkt, 36, toI16(f.temp, s.temp_lsb));

    // USW [38..39]
    writeU16BE(pkt, 38, f.usw);

    // Checksum: sum of bytes[2..39]
    uint16_t chk = 0;
    for (int i = 2; i <= 39; ++i)
        chk += static_cast<uint8_t>(pkt[i]);
    writeU16BE(pkt, 40, chk);

    return pkt;
}
