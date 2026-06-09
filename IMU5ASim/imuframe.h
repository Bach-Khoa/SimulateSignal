#pragma once
#include <cstdint>

struct ImuFrame {
    float roll   = 0.f;   // degrees
    float pitch  = 0.f;   // degrees
    float yaw    = 0.f;   // degrees
    float gx     = 0.f;   // dps
    float gy     = 0.f;   // dps
    float gz     = 0.f;   // dps
    float ax     = 0.f;   // g
    float ay     = 0.f;   // g
    float az     = 0.f;   // g
    float temp   = 25.f;  // Celsius
    uint16_t usw = 0x0000;
};
