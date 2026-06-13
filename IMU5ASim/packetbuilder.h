#pragma once
#include <QByteArray>
#include "imuframe.h"
#include "scaleconfig.h"

class PacketBuilder5A {
public:
    // Builds the 42-byte KERNEL User_Def_Data packet from physical IMU values.
    // KERNEL ICD Rev 1.40: all multi-byte fields are LSB-first (little-endian).
    // Layout: AA 55 01 95 28 00 05 07 21 22 52 53
    //         [Heading×2][Pitch×2][Roll×2]      (0x07, uint16/int16 ×100 deg)
    //         [Gx×4][Gy×4][Gz×4]               (0x21, int32 ×1e5 dps)
    //         [Ax×2][Ay×2][Az×2]               (0x22, int16 ×4000 g)
    //         [Temp×2][USW×2][Checksum×2]       (sum bytes[2..39], LE)
    static QByteArray build(const ImuFrame& f, const ScaleConfig& s);

private:
    static void writeI16LE(QByteArray& buf, int off, int16_t v);
    static void writeI32LE(QByteArray& buf, int off, int32_t v);
    static void writeU16LE(QByteArray& buf, int off, uint16_t v);
};
