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

void PacketBuilder5A::writeI16LE(QByteArray& buf, int off, int16_t v) {
    buf[off]     = static_cast<char>(v & 0xFF);          // LSB
    buf[off + 1] = static_cast<char>((v >> 8) & 0xFF);   // MSB
}

void PacketBuilder5A::writeI32LE(QByteArray& buf, int off, int32_t v) {
    buf[off]     = static_cast<char>(v & 0xFF);
    buf[off + 1] = static_cast<char>((v >> 8)  & 0xFF);
    buf[off + 2] = static_cast<char>((v >> 16) & 0xFF);
    buf[off + 3] = static_cast<char>((v >> 24) & 0xFF);
}

void PacketBuilder5A::writeU16LE(QByteArray& buf, int off, uint16_t v) {
    buf[off]     = static_cast<char>(v & 0xFF);          // LSB
    buf[off + 1] = static_cast<char>((v >> 8) & 0xFF);   // MSB
}

QByteArray PacketBuilder5A::build(const ImuFrame& f, const ScaleConfig& s) {
    QByteArray pkt(42, '\0');

    // --- Fixed header (12 bytes) ---
    pkt[0]  = static_cast<char>(0xAA);
    pkt[1]  = static_cast<char>(0x55);
    pkt[2]  = static_cast<char>(0x01);   // message type = data
    pkt[3]  = static_cast<char>(0x95);   // cmd code ID = User_Def_Data
    pkt[4]  = static_cast<char>(0x28);   // payload length LSB = 40
    pkt[5]  = static_cast<char>(0x00);   // payload length MSB
    pkt[6]  = static_cast<char>(0x05);   // num data packages
    pkt[7]  = static_cast<char>(0x07);   // 0x07 Orientation angles
    pkt[8]  = static_cast<char>(0x21);   // 0x21 Gyro HR
    pkt[9]  = static_cast<char>(0x22);   // 0x22 Accelerometer
    pkt[10] = static_cast<char>(0x52);   // 0x52 Temperature
    pkt[11] = static_cast<char>(0x53);   // 0x53 USW

    // --- 0x07 Orientation angles [12..17], 6 bytes, LSB first ---
    // ICD order: Heading (word, unsigned), Pitch (sword), Roll (sword)
    // f.yaw → Heading (azimuth 0..360°, unsigned)
    writeU16LE(pkt, 12, static_cast<uint16_t>(std::round(f.yaw  / s.angle_lsb)));
    writeI16LE(pkt, 14, toI16(f.pitch, s.angle_lsb));
    writeI16LE(pkt, 16, toI16(f.roll,  s.angle_lsb));

    // --- 0x21 Gyro HR [18..29], 12 bytes, int32 × 1e5 dps, LSB first ---
    writeI32LE(pkt, 18, toI32(f.gx, s.gyro_lsb));
    writeI32LE(pkt, 22, toI32(f.gy, s.gyro_lsb));
    writeI32LE(pkt, 26, toI32(f.gz, s.gyro_lsb));

    // --- 0x22 Accelerometer [30..35], 6 bytes, int16 × KA g, LSB first ---
    writeI16LE(pkt, 30, toI16(f.ax, s.accel_lsb));
    writeI16LE(pkt, 32, toI16(f.ay, s.accel_lsb));
    writeI16LE(pkt, 34, toI16(f.az, s.accel_lsb));

    // --- 0x52 Temperature [36..37], int16 × 10 °C, LSB first ---
    writeI16LE(pkt, 36, toI16(f.temp, s.temp_lsb));

    // --- 0x53 USW [38..39], uint16, LSB first ---
    writeU16LE(pkt, 38, f.usw);

    // --- Checksum [40..41]: arithmetic sum of bytes[2..39], stored LSB first ---
    uint16_t chk = 0;
    for (int i = 2; i <= 39; ++i)
        chk += static_cast<uint8_t>(pkt[i]);
    writeU16LE(pkt, 40, chk);

    return pkt;
}
