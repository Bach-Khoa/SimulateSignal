#pragma once

struct ScaleConfig {
    double angle_lsb = 0.01;   // deg/LSB  → raw = round(deg  / 0.01)
    double gyro_lsb  = 0.001;  // dps/LSB  → raw = round(dps  / 0.001)
    double accel_lsb = 0.001;  // g/LSB    → raw = round(g    / 0.001)
    double temp_lsb  = 0.01;   // °C/LSB   → raw = round(°C   / 0.01)
};
