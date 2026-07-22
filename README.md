# AIoT Hardware

Firmware cho thiết bị **EmotiCare** chạy trên ESP32. Thiết bị cung cấp giao diện TFT, kiểm tra cảm xúc, hoạt động hỗ trợ được đề xuất từ server, phát media, trò chuyện đồng hành, thống kê cảm xúc và cấu hình Wi-Fi/pairing ngay trên thiết bị.

> [!IMPORTANT]
> Repository hiện chứa một firmware PlatformIO tại [`Smart Device`](./Smart%20Device). Thay đổi chân GPIO trong [`pins_config.h`](./Smart%20Device/include/pins_config.h) trước khi nạp firmware vào phần cứng có wiring khác.

## Tính năng

- Giao diện EmotiCare trên màn hình **ST7789 240 × 280** và điều hướng bằng 5 nút vật lý.
- Wi-Fi provisioning qua captive portal, lưu cấu hình và device token trong ESP32 NVS.
- Pair thiết bị với AIoT Server qua `POST /api/devices/pair`.
- Đồng bộ emotion session và lấy activity recommendation từ API; có dữ liệu fallback khi offline.
- Danh sách activity có thể điều hướng và xem chi tiết.
- Danh sách/phát nhạc và podcast từ URL do server cung cấp.
- Mic test: I2S passthrough từ **INMP441** đến **MAX98357**.
- Companion chat, insight theo ngày/tuần/tháng, và trạng thái mạng trên thiết bị.

## Phần cứng

| Thành phần | Vai trò | Kết nối chính |
| --- | --- | --- |
| ESP32 DevKit (`mhetesp32devkit`) | Vi điều khiển | — |
| ST7789 240 × 280 | Màn hình TFT qua SPI | CS 15, RST 16, DC 17, MOSI 23, MISO 19, SCLK 18, BLK 4 |
| 5 nút nhấn active-low | Điều hướng | MODE 12, ACTION 13, START 14, NEXT 27, BACK 26 |
| INMP441 | Micro I2S | SCK 22, WS 21, SD 35 |
| MAX98357 + loa | Khuếch đại/loa I2S | BCLK 25, LRC 32, DIN 33, SD/MODE 5 |

Tất cả chân GPIO được khai báo tập trung tại [`Smart Device/include/pins_config.h`](./Smart%20Device/include/pins_config.h).

## Cấu trúc

```text
AIoT-Hardware/
├── README.md
└── Smart Device/
    ├── platformio.ini          # Môi trường PlatformIO và thư viện
    ├── include/                # Header, cấu hình GPIO và interface
    ├── src/
    │   ├── core/               # Vòng đời ứng dụng và state machine
    │   ├── hal/                # Display, button, network và audio
    │   ├── screens/            # Các màn hình TFT
    │   ├── services/           # HTTP client tới AIoT Server
    │   └── main.cpp            # Arduino setup() / loop()
    └── lib/                    # Thư viện PlatformIO cục bộ (nếu có)
```

## Yêu cầu

- [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/index.html)
- ESP32 DevKit kết nối USB
- AIoT Server đang chạy và có thể truy cập từ mạng Wi-Fi của thiết bị
- Pairing code hợp lệ do server cấp

Firmware dùng Arduino framework cho ESP32 cùng các thư viện:

- `TFT_eSPI`
- `ArduinoJson`
- `ESP32-audioI2S`

PlatformIO tự cài các dependency này theo [`platformio.ini`](./Smart%20Device/platformio.ini) ở lần build đầu tiên.

## Build, nạp firmware và theo dõi serial

Chạy các lệnh sau từ thư mục firmware:

```powershell
cd "Smart Device"
pio run
pio run -t upload
pio device monitor -b 115200
```

Nếu `pio` chưa có trong `PATH` trên Windows, dùng PlatformIO Core trong môi trường người dùng:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run
```

Environment hiện tại là `mhetesp32devkit`. Cổng upload được PlatformIO tự phát hiện; chỉ định thủ công khi cần:

```powershell
pio run -t upload --upload-port COM3
```

## Thiết lập lần đầu

1. Nạp firmware và cấp nguồn cho thiết bị.
2. Nếu chưa có Wi-Fi được lưu, kết nối điện thoại hoặc máy tính vào access point:
   - **SSID:** `EmotiCare-Setup`
   - **Mật khẩu:** `emotioncare`
3. Mở `http://192.168.4.1`.
4. Nhập Wi-Fi SSID/mật khẩu, base URL của AIoT Server (ví dụ `http://192.168.1.10:8000`) và pairing code.
5. Thiết bị kết nối Wi-Fi, gọi endpoint pair và lưu token vào NVS. Sau khi khởi động lại, thiết bị dùng cấu hình đã lưu.

> [!NOTE]
> Base URL phải truy cập được từ ESP32. `localhost` hoặc `127.0.0.1` trỏ tới chính thiết bị, không phải máy đang chạy server.

## Luồng sử dụng

1. Tại **Home**, chọn **Check-In**.
2. Hoàn tất kiểm tra cảm xúc để đồng bộ một emotion session với server.
3. Mở **Support** để nhận danh sách hoạt động. Dùng NEXT/BACK để chọn và ACTION/START để xem chi tiết.
4. Dùng **Discover** để duyệt và phát music/podcast được server đề xuất.
5. Vào **WiFi Setup** khi cần chuyển giữa Wi-Fi đã lưu và provisioning AP.

Khi offline, các luồng recommendation, chat và statistics sử dụng dữ liệu fallback để giao diện vẫn hoạt động. Các tính năng cần media stream hoặc API sẽ yêu cầu Wi-Fi và thiết bị đã pair.

## Kiến trúc

```text
Arduino entry point
        │
        ▼
DemoApp ── Navigation / AppState ── Screens
   │              │                   │
   ├── HAL ───────┼── Display, buttons, audio, Wi-Fi
   │              │
   └── Service ───┴── EdgeApiClient ── AIoT Server
```

- **HAL:** quản lý ST7789, nút nhấn, I2S audio và Wi-Fi provisioning.
- **Screens:** render giao diện cho Home, Check-In, Support, Discover, Music, Podcast, Chat, Insights, Mic Test và Wi-Fi Setup.
- **Service:** ưu tiên API thật, sau đó fallback mock data khi mất mạng hoặc API lỗi.
- **EdgeApiClient:** gửi JSON có header `X-Device-Token` đến AIoT Server.

## Phát triển

- Bật log HTTP chi tiết bằng cách thêm `-DEDGE_API_DEBUG=1` vào `build_flags` trong [`platformio.ini`](./Smart%20Device/platformio.ini).
- Không commit `.pio/`, firmware binary hoặc thông tin Wi-Fi/token. Các mục này đã được ignore.
- Không có firmware unit-test target hiện tại. Xác minh thay đổi bằng `pio run`, serial monitor và kiểm tra trực tiếp trên thiết bị.

## Giới hạn hiện tại

- Emotion detection hiện dùng stub trong service layer; model SER và pipeline audio thực tế chưa được tích hợp hoàn chỉnh.
- Một số trải nghiệm AI (chat, recommendation, statistics) có fallback cục bộ khi không gọi được server.
- Font GLCD tích hợp của TFT không hỗ trợ đầy đủ Unicode tiếng Việt; firmware chuyển chữ có dấu thành Latin không dấu để nội dung server vẫn đọc được.

## Tài liệu liên quan

- [AIoT Server](../AIoT-Server) — API, pairing code và recommendation service.
- [AIoT Testing Simulator](../AIoT-Testing-Simulator) — mô phỏng luồng UI/API trên trình duyệt.
