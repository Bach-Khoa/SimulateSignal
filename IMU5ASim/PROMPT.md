# PROMPT — IMU 5A Simulator (Qt C++)

## Mục tiêu
Viết app Qt C++ mô phỏng tín hiệu IMU-kernel gửi vào khối điều khiển 5A qua RS422,
tần số 125 Hz, input từ file CSV (export từ Excel), có GUI đầy đủ và guide cho người dùng.

---

## Frame Analysis — Bản tin 5A (42 bytes, RS422)

| Offset | Size | Field               | Value/Note                              |
|--------|------|---------------------|-----------------------------------------|
| 0      | 1    | Header1             | 0xAA (fixed)                            |
| 1      | 1    | Header2             | 0x55 (fixed)                            |
| 2      | 1    | Message type        | 0x01 (fixed)                            |
| 3      | 1    | Cmd code ID         | 0x95 (fixed)                            |
| 4–5    | 2    | Length              | 0x0028 = 40 (big-endian, từ byte[2] đến byte[41]) |
| 6      | 1    | Num packages        | 0x05 (fixed)                            |
| 7–11   | 5    | Data list           | 0x07 0x21 0x22 0x52 0x53 (fixed)        |
| 12–13  | 2    | Roll                | int16 big-endian                        |
| 14–15  | 2    | Pitch               | int16 big-endian                        |
| 16–17  | 2    | Yaw                 | int16 big-endian                        |
| 18–21  | 4    | Gyro X (HR)         | int32 big-endian                        |
| 22–25  | 4    | Gyro Y (HR)         | int32 big-endian                        |
| 26–29  | 4    | Gyro Z (HR)         | int32 big-endian                        |
| 30–31  | 2    | Accel X             | int16 big-endian                        |
| 32–33  | 2    | Accel Y             | int16 big-endian                        |
| 34–35  | 2    | Accel Z             | int16 big-endian                        |
| 36–37  | 2    | Temperature         | int16 big-endian                        |
| 38–39  | 2    | USW                 | uint16 big-endian                       |
| 40–41  | 2    | Checksum            | uint16 BE, sum của bytes[2..39]         |
| **Total** | **42** | | |

**Checksum rule:** `chksum = sum(packet[2..39])` lưu dưới dạng uint16 big-endian tại [40..41].

---

## Scale Factors (mặc định, user có thể chỉnh trong UI)

| Field              | Default LSB     | Công thức raw → phys         |
|--------------------|-----------------|------------------------------|
| Orientation angles | 0.01 deg/LSB    | `raw = round(deg / 0.01)`    |
| Gyro HR            | 0.001 dps/LSB   | `raw = round(dps / 0.001)`   |
| Accelerometer      | 0.001 g/LSB     | `raw = round(g / 0.001)`     |
| Temperature        | 0.01 °C/LSB     | `raw = round(°C / 0.01)`     |

---

## CSV Input Format

```
Index,Roll(deg),Pitch(deg),Yaw(deg),Gx(dps),Gy(dps),Gz(dps),Ax(g),Ay(g),Az(g),Temp(C),USW
0,0.00,0.00,0.00,0.000,0.000,0.000,0.000,0.000,1.000,25.00,0
1,0.10,0.05,0.20,...
```
- Row 1: header (bắt buộc, tên cột tự do)
- Cột theo thứ tự cố định: Index, Roll, Pitch, Yaw, Gx, Gy, Gz, Ax, Ay, Az, Temp, USW
- USW: thập phân hoặc hex (0x...)
- Export từ Excel: File → Save As → CSV (Comma delimited)

---

## Transport

- Giao thức: RS422 qua USB-RS422 adapter
- Thư viện: `QSerialPort`
- Baud rate mặc định: 921600
- Data bits: 8, Stop bits: 1, Parity: None, Flow: None
- Configurable: port name, baud rate

---

## Timing — 125 Hz

- Period: 8 ms
- Windows: `QueryPerformanceCounter` busy-wait (giống TelemetryCheck C#)
- Linux: `clock_gettime(CLOCK_MONOTONIC)` busy-wait
- Thread: `QThread` với priority `TimeCriticalPriority`
- Mục tiêu jitter < 0.5 ms

---

## Kiến trúc module

```
IMU5ASim/
├── imuframe.h          — struct ImuFrame (physical values)
├── scaleconfig.h       — struct ScaleConfig (LSB values)
├── packetbuilder.h/cpp — build 42-byte packet
├── serialtransport.h/cpp — QSerialPort wrapper
├── datareader.h/cpp    — CSV → vector<ImuFrame>
├── simulatorengine.h/cpp — QThread 125Hz loop
├── guidedialog.h/cpp   — tabbed help dialog
├── mainwindow.h/cpp    — main UI
├── main.cpp
└── IMU5ASim.pro
```

---

## Guides cần có (tự thiết kế)

| ID  | Tên guide              | Nội dung                                                    |
|-----|------------------------|-------------------------------------------------------------|
| G1  | Quick Start            | 4 bước: chọn file → chọn port → nhấn Start → quan sát      |
| G2  | CSV Template           | Format cột, đơn vị, cách export từ Excel, download template |
| G3  | Port & Baud Setup      | Cách tìm COM port, driver RS422, baudrate phù hợp           |
| G4  | Scale Factors          | Giải thích LSB, cách tính raw value, khi nào cần thay đổi   |
| G5  | Signal Preview         | Đọc biểu đồ, rolling window 5s, màu sắc các kênh           |
| G6  | Packet Hex Monitor     | Cách đọc hex dump, verify checksum thủ công                 |
| G7  | Timing & Jitter        | Jitter < 0.5ms là ổn, cách tối ưu khi CPU load cao         |

---

## Quyết định kỹ thuật

1. **Endianness**: Big-endian cho tất cả multi-byte field (phù hợp industrial IMU protocol)
2. **Excel → CSV**: Không dùng QXlsx để tránh dependency ngoài — user export CSV từ Excel
3. **Chart**: Qt Charts (built-in), 3 series/chart, rolling 5s window, cập nhật 10Hz
4. **Timing**: Busy-wait spin loop trong dedicated QThread — không dùng QTimer (jitter > 1ms)
5. **Thread safety**: Engine emit signal → QueuedConnection → main thread xử lý UI
6. **Loop mode**: Khi hết data, quay vòng lại frame đầu (nếu chọn Loop)
7. **Error injection**: Không implement trong v1 (có thể thêm sau)
