#pragma once

struct ScaleConfig {
    double angle_lsb = 0.01;     // deg/LSB  → raw = deg  × 100   (0x07 Orientation)
    double gyro_lsb  = 0.00001;  // dps/LSB  → raw = dps  × 1e5   (0x21 Gyro HR)
    double accel_lsb = 0.00025;  // g/LSB    → raw = g    × 4000  (0x22 Accel, KA=4000 ±8g)
    double temp_lsb  = 0.1;      // °C/LSB   → raw = °C   × 10    (0x52 Temperature)
};
