#pragma once
#include <QByteArray>
#include "imuframe.h"
#include "scaleconfig.h"

class PacketBuilder5A {
public:
    // Builds the 42-byte 5A packet from physical IMU values.
    // Endianness: big-endian for all multi-byte fields.
    // Checksum: uint16 sum of bytes[2..39], stored at [40..41].
    static QByteArray build(const ImuFrame& f, const ScaleConfig& s);

private:
    static void writeI16BE(QByteArray& buf, int off, int16_t v);
    static void writeI32BE(QByteArray& buf, int off, int32_t v);
    static void writeU16BE(QByteArray& buf, int off, uint16_t v);
};
