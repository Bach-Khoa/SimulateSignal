#include "guidedialog.h"
#include <QTabWidget>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>

// ---------------------------------------------------------------------------
// Guide content — HTML strings
// ---------------------------------------------------------------------------

static const char* G1_QUICKSTART = R"(
<h2>G1 — Quick Start (4 bước)</h2>
<ol>
  <li><b>Chọn file CSV:</b> Nhấn <b>[Browse]</b> → chọn file <code>.csv</code> chứa dữ liệu IMU.
      Nếu chưa có, nhấn <b>[Template]</b> để tạo file mẫu với dữ liệu sine-wave.<br>
      Sau khi load thành công sẽ hiện <i>"Loaded: N frames"</i>.</li><br>

  <li><b>Chọn COM port:</b> Chọn đúng port của USB-RS422 adapter trong combobox <b>Port</b>.
      Nhấn <b>[⟳]</b> nếu không thấy port. Baud rate mặc định <b>921600</b>.</li><br>

  <li><b>Nhấn [▶ Start]:</b> App bắt đầu gửi 125 gói tin/giây qua RS422.
      Thanh progress và các chỉ số sẽ cập nhật liên tục.</li><br>

  <li><b>Quan sát:</b> Chuyển tab <b>Orientation / Gyro / Accel</b> để xem biểu đồ realtime.
      Tab <b>Packet Log</b> hiển thị hex dump các packet gần nhất.</li>
</ol>
<p><b>Lưu ý:</b> Để dừng, nhấn <b>[■ Stop]</b>. Trong chế độ Loop, app sẽ lặp lại từ đầu khi hết data.</p>
)";

static const char* G2_CSV = R"(
<h2>G2 — Định dạng file CSV</h2>
<h3>Cấu trúc cột (bắt buộc theo thứ tự)</h3>
<table border="1" cellpadding="4" cellspacing="0">
  <tr><th>Cột</th><th>Tên</th><th>Đơn vị</th><th>Ví dụ</th></tr>
  <tr><td>1</td><td>Index</td><td>—</td><td>0, 1, 2...</td></tr>
  <tr><td>2</td><td>Roll</td><td>độ (deg)</td><td>10.50</td></tr>
  <tr><td>3</td><td>Pitch</td><td>độ (deg)</td><td>-3.20</td></tr>
  <tr><td>4</td><td>Yaw</td><td>độ (deg)</td><td>180.00</td></tr>
  <tr><td>5</td><td>Gx</td><td>dps</td><td>5.123</td></tr>
  <tr><td>6</td><td>Gy</td><td>dps</td><td>-2.456</td></tr>
  <tr><td>7</td><td>Gz</td><td>dps</td><td>0.001</td></tr>
  <tr><td>8</td><td>Ax</td><td>g</td><td>0.050</td></tr>
  <tr><td>9</td><td>Ay</td><td>g</td><td>-0.020</td></tr>
  <tr><td>10</td><td>Az</td><td>g</td><td>1.000</td></tr>
  <tr><td>11</td><td>Temp</td><td>°C</td><td>25.00</td></tr>
  <tr><td>12</td><td>USW</td><td>hex/dec</td><td>0x0000 hoặc 0</td></tr>
</table>
<h3>Cách export từ Excel</h3>
<ol>
  <li>Mở file Excel, đảm bảo dữ liệu từ cột A (Index) đến cột L (USW).</li>
  <li><b>File → Save As → CSV (Comma delimited) (*.csv)</b></li>
  <li>Lưu file, sau đó mở bằng nút Browse trong app.</li>
</ol>
<p><b>Lưu ý:</b> Hàng đầu tiên là header (tên cột), bắt đầu từ hàng 2 là dữ liệu.
Dấu thập phân dùng dấu chấm <code>.</code>, không dùng dấu phẩy.</p>
)";

static const char* G3_PORT = R"(
<h2>G3 — Cấu hình Port & Baud Rate</h2>
<h3>Tìm COM port của RS422 adapter</h3>
<ol>
  <li>Cắm USB-RS422 adapter vào máy tính.</li>
  <li>Mở <b>Device Manager</b> (Win+X → Device Manager).</li>
  <li>Tìm mục <b>Ports (COM & LPT)</b> → ghi nhớ số COM (ví dụ COM4).</li>
  <li>Nhấn <b>[⟳ Refresh]</b> trong app để cập nhật danh sách port.</li>
</ol>
<h3>Baud rate</h3>
<table border="1" cellpadding="4" cellspacing="0">
  <tr><th>Giá trị</th><th>Khi dùng</th></tr>
  <tr><td><b>921600</b> (mặc định)</td><td>RS422 tốc độ cao, khuyến nghị cho 5A</td></tr>
  <tr><td>460800</td><td>Nếu có lỗi ở 921600</td></tr>
  <tr><td>230400</td><td>Cáp dài, môi trường nhiễu</td></tr>
</table>
<h3>RS422 vs RS232</h3>
<p>RS422 dùng tín hiệu vi sai (differential) — ít nhiễu hơn RS232.
Khi dùng adapter USB-RS422, phần mềm vẫn dùng COM port bình thường,
chỉ khác phần cứng.</p>
<p><b>Lưu ý:</b> Đảm bảo driver của adapter đã được cài (thường là CH340, CP210x, hoặc FTDI).</p>
)";

static const char* G4_SCALE = R"(
<h2>G4 — Scale Factors (Hệ số tỷ lệ)</h2>
<p>Scale factor (LSB) xác định độ phân giải khi chuyển đổi từ giá trị vật lý sang raw integer trong packet.</p>
<h3>Công thức</h3>
<pre>  raw_value = round(physical_value / LSB)</pre>
<h3>Giá trị mặc định</h3>
<table border="1" cellpadding="4" cellspacing="0">
  <tr><th>Kênh</th><th>Mặc định</th><th>Ý nghĩa</th><th>Range</th></tr>
  <tr><td>Angle</td><td>0.01 deg/LSB</td><td>1 LSB = 0.01 độ</td><td>±327.67°</td></tr>
  <tr><td>Gyro</td><td>0.001 dps/LSB</td><td>1 LSB = 0.001 °/s</td><td>±2147483 °/s</td></tr>
  <tr><td>Accel</td><td>0.001 g/LSB</td><td>1 LSB = 0.001 g</td><td>±32.767 g</td></tr>
  <tr><td>Temp</td><td>0.01 °C/LSB</td><td>1 LSB = 0.01 °C</td><td>±327.67 °C</td></tr>
</table>
<h3>Khi nào thay đổi?</h3>
<p>Nếu khối điều khiển 5A kỳ vọng scale factor khác (xem datasheet IMU), hãy thay đổi
trong mục <b>Scale Factors</b> trên UI. Ví dụ: nếu Angle LSB = 0.001, thì roll = 45° → raw = 45000.</p>
)";

static const char* G5_CHART = R"(
<h2>G5 — Signal Preview (Biểu đồ realtime)</h2>
<p>Biểu đồ cập nhật ~10Hz, hiển thị cửa sổ 5 giây gần nhất.</p>
<h3>Các tab</h3>
<ul>
  <li><b>Orientation:</b> Roll (đỏ), Pitch (xanh lá), Yaw (xanh dương) — đơn vị độ</li>
  <li><b>Gyro:</b> Gx (đỏ), Gy (xanh lá), Gz (xanh dương) — đơn vị dps</li>
  <li><b>Accel:</b> Ax (đỏ), Ay (xanh lá), Az (xanh dương) — đơn vị g</li>
  <li><b>Temperature:</b> Temp (cam) — đơn vị °C</li>
</ul>
<h3>Đọc biểu đồ</h3>
<p>Trục X là thời gian (giây, đếm ngược về quá khứ). Trục Y là giá trị vật lý.
Biểu đồ tự scale theo giá trị thực tế của dữ liệu đang gửi.</p>
<p><b>Lưu ý:</b> Biểu đồ chỉ hiển thị khi app đang chạy. Khi dừng, biểu đồ giữ nguyên giá trị cuối.</p>
)";

static const char* G6_HEXLOG = R"(
<h2>G6 — Packet Hex Monitor</h2>
<p>Tab <b>Packet Log</b> hiển thị hex dump của packet gần nhất (cập nhật ~5Hz).</p>
<h3>Đọc hex dump</h3>
<pre>
AA 55 01 95 00 28 05 07 21 22 52 53   ← Header cố định (12 bytes)
11 94 02 1C 03 E8                     ← Roll, Pitch, Yaw (int16 × 3)
00 00 27 10 FF FF D8 F0 00 00 03 E8   ← Gx, Gy, Gz (int32 × 3)
00 32 FF CE 03 E8                     ← Ax, Ay, Az (int16 × 3)
09 C4                                 ← Temp
00 00                                 ← USW
XX XX                                 ← Checksum (sum bytes[2..39])
</pre>
<h3>Verify checksum thủ công</h3>
<ol>
  <li>Lấy tất cả bytes từ vị trí 3 (byte 0x01) đến byte thứ 40 (trước checksum).</li>
  <li>Cộng tổng tất cả bytes (unsigned).</li>
  <li>Kết quả uint16 phải bằng 2 bytes cuối của packet.</li>
</ol>
)";

static const char* G7_TIMING = R"(
<h2>G7 — Timing & Jitter</h2>
<p>App dùng <b>busy-wait spin loop</b> (QueryPerformanceCounter trên Windows) để đạt
timing chính xác 125Hz. Đây là kỹ thuật giống với TelemetryCheck C# gốc.</p>
<h3>Chỉ số jitter</h3>
<table border="1" cellpadding="4" cellspacing="0">
  <tr><th>Jitter</th><th>Đánh giá</th></tr>
  <tr><td>&lt; 0.3 ms</td><td>Xuất sắc ✓</td></tr>
  <tr><td>0.3 – 0.8 ms</td><td>Tốt ✓</td></tr>
  <tr><td>0.8 – 2.0 ms</td><td>Chấp nhận được</td></tr>
  <tr><td>&gt; 2.0 ms</td><td>Cần tối ưu ✗</td></tr>
</table>
<h3>Tối ưu khi jitter cao</h3>
<ul>
  <li>Đóng các ứng dụng nặng đang chạy nền (antivirus scan, Windows Update...).</li>
  <li>Cắm USB-RS422 trực tiếp vào cổng USB của bo mạch chủ, không qua hub.</li>
  <li>Trong Task Manager → Details → tìm <code>IMU5ASim.exe</code> → Set Priority → <b>High</b>.</li>
  <li>Tắt Hyper-V nếu đang bật (gây timer interrupt jitter).</li>
</ul>
<p><b>Lưu ý:</b> Busy-wait tiêu thụ 1 core CPU liên tục. Đây là đánh đổi cần thiết để đạt 125Hz chính xác.</p>
)";

// ---------------------------------------------------------------------------

GuideDialog::GuideDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("IMU 5A Simulator — Guide");
    setMinimumSize(700, 500);
    resize(750, 580);

    auto* tabs = new QTabWidget(this);

    auto addTab = [&](const char* html, const QString& title) {
        auto* browser = new QTextBrowser;
        browser->setOpenExternalLinks(true);
        browser->setHtml(QString::fromUtf8(html));
        tabs->addTab(browser, title);
    };

    addTab(G1_QUICKSTART, "G1: Quick Start");
    addTab(G2_CSV,        "G2: CSV Format");
    addTab(G3_PORT,       "G3: Port Setup");
    addTab(G4_SCALE,      "G4: Scale Factors");
    addTab(G5_CHART,      "G5: Signal Preview");
    addTab(G6_HEXLOG,     "G6: Hex Monitor");
    addTab(G7_TIMING,     "G7: Timing");

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::accept);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(tabs);
    layout->addWidget(buttons);
}

void GuideDialog::showTab(int index) {
    if (auto* tabs = findChild<QTabWidget*>())
        tabs->setCurrentIndex(index);
    show();
    raise();
    activateWindow();
}
