# Hướng dẫn sử dụng thiết bị ESP32 EmotiCare

Tài liệu này tổng hợp cách sử dụng firmware EmotiCare trên ESP32 từ góc nhìn người dùng cuối: từ các nút bấm trên từng màn hình, cho đến quy trình thiết lập Wi‑Fi và pairing với server.

> Ghi chú: tài liệu này dựa trên README và mã nguồn hiện có trong repo. Khi có mâu thuẫn giữa README cũ và code hiện tại, ưu tiên theo code hiện tại.

---

## 1. Mục tiêu và phạm vi

Thiết bị chạy giao diện TFT và hỗ trợ các chức năng:
- Check-in cảm xúc
- Gợi ý hỗ trợ phù hợp với cảm xúc
- Khám phá nội dung âm nhạc/podcast
- Chat đồng hành
- Xem thống kê cảm xúc
- Kiểm tra mic
- Cấu hình Wi‑Fi và pairing

---

## 2. Cấu hình phần cứng cơ bản

### Board và màn hình
- Vi điều khiển: ESP32 DevKit
- Màn hình: ST7789 240 × 280
- Có 5 nút vật lý để điều hướng

### Bố trí chân GPIO
| Thành phần | Chân GPIO |
| --- | --- |
| MODE button | 12 |
| ACTION button | 13 |
| START button | 14 |
| NEXT button | 27 |
| BACK button | 26 |
| TFT CS | 15 |
| TFT RST | 16 |
| TFT DC | 17 |
| TFT MOSI | 23 |
| TFT MISO | 19 |
| TFT SCLK | 18 |
| TFT BLK | 4 |

Tất cả chân này được khai báo tập trung trong file cấu hình phần cứng của dự án.

---

## 3. Khởi động thiết bị

1. Nạp firmware lên ESP32 bằng PlatformIO.
2. Cấp nguồn cho thiết bị.
3. Đợi màn hình khởi động.
4. Nếu chưa có Wi‑Fi đã lưu, thiết bị sẽ tự mở chế độ thiết lập Wi‑Fi (Access Point).

### Màn hình Home
Sau khi khởi động, bạn sẽ thấy màn hình Home với:
- Tiêu đề: EmotiCare
- Trạng thái Wi‑Fi ở thanh trên
- Trạng thái cảm xúc gần nhất (nếu đã check-in)
- Danh sách các mục chính để chọn

---

## 4. Bảng nút bấm chung

Firmware dùng 5 nút vật lý, được gọi theo tên sau:

| Nút vật lý | Vai trò trong firmware |
| --- | --- |
| S1 (MODE) | Ngữ cảnh, thường dùng cho thao tác phụ / quay về / ghi âm |
| S2 (ACTION) | Xác nhận / chọn / thao tác chính |
| S3 (START) | Thao tác phụ, thường dùng để bắt đầu hoặc gửi |
| S4 (NEXT) | Chuyển xuống / next / cuộn tiếp |
| S5 (BACK) | Lên / quay lại / hủy |

> Mỗi màn hình có ý nghĩa nút khác nhau. Dưới đây là bản đồ nút theo từng trang cụ thể.

---

## 5. Hướng dẫn sử dụng từng màn hình

### 5.1 Màn hình Home

Màn hình này là trung tâm điều hướng.

#### Nội dung hiển thị
- Tiêu đề EmotiCare
- Trạng thái Wi‑Fi: Online / Unpaired / Setup AP / Offline
- Mood hiện tại: chưa check-in thì là None
- Menu gồm 6 mục:
  1. Check-In
  2. Discover
  3. Companion Chat
  4. Insights
  5. Test Mic
  6. WiFi Setup

#### Cách dùng nút
- S4 (NEXT): di chuyển xuống mục tiếp theo
- S5 (BACK): di chuyển lên mục trước đó
- S2/S3 (ACTION/START): chọn mục đang được đánh dấu
- S1 (MODE): không dùng ở Home

#### Mục đích sử dụng
- Chọn Check-In để bắt đầu quá trình cảm xúc
- Chọn Discover để mở danh sách âm nhạc/podcast
- Chọn Companion Chat để thử chat đồng hành
- Chọn Insights để xem thống kê cảm xúc
- Chọn Test Mic để kiểm tra mic và loa
- Chọn WiFi Setup để cấu hình mạng

---

### 5.2 Màn hình Check-In

Màn hình này dùng để bắt đầu quá trình nhận diện cảm xúc.

#### Trạng thái màn hình
- Giai đoạn 1: “Listening & analyzing...”
- Giai đoạn 2: hiển thị kết quả cảm xúc và độ tin cậy

#### Cách dùng nút
- S2/S3 (ACTION/START): bắt đầu quét / xác nhận
- S5 (BACK): quay lại Home / hủy
- S1/S4: không dùng trong màn hình này

#### Luồng dùng thực tế
1. Vào Check-In từ Home.
2. Nhấn S2 hoặc S3 để bắt đầu quá trình phân tích.
3. Sau khi hoàn tất, thiết bị hiển thị cảm xúc và mức độ tin cậy.
4. Nhấn S2/S3 một lần nữa để chuyển sang Support và xem đề xuất hoạt động phù hợp.

---

### 5.3 Màn hình Support

Màn hình này hiển thị các hoạt động gợi ý cho cảm xúc vừa phát hiện.

#### Nội dung hiển thị
- Danh sách các hoạt động đề xuất từ server hoặc dữ liệu fallback
- Mỗi mục có thể được mở chi tiết

#### Cách dùng nút
- S2/S3 (ACTION/START): mở mục đang chọn
- S4 (NEXT): chuyển sang mục tiếp theo
- S5 (BACK): quay lên mục trước đó, hoặc quay lại màn hình trước nếu đang ở đầu danh sách
- S1 (MODE): quay về Home

#### Mẹo sử dụng
- Nếu danh sách không có mục nào, màn hình sẽ hiện “No activities available.”
- Khi đang xem chi tiết, S2/S3/S5 có thể dùng để đóng lại và quay về danh sách.

---

### 5.4 Màn hình Discover

Màn hình này là hub để chọn giữa âm nhạc và podcast.

#### Nội dung hiển thị
- Music
- Podcast

#### Cách dùng nút
- S4 (NEXT): chọn mục tiếp theo
- S5 (BACK): chọn mục trước đó hoặc quay lại Home
- S2/S3 (ACTION/START): vào mục đã chọn
- S1 (MODE): không dùng ở đây

---

### 5.5 Màn hình Music List

Sau khi chọn Music, bạn vào danh sách các bài hát.

#### Cách dùng nút
- S4 (NEXT): cuộn xuống bài tiếp theo
- S5 (BACK): cuộn lên, hoặc quay lại Discover nếu đang ở đầu danh sách
- S1 (MODE): quay lại Discover
- S3 (START): phát bài đã chọn
- S2 (ACTION): dừng phát

#### Lưu ý
- Trong firmware hiện tại, danh sách phát không phải là playlist động đầy đủ; mục đích là cung cấp trải nghiệm điều hướng và điều khiển media trên thiết bị.

---

### 5.6 Màn hình Podcast List

Tương tự Music List, nhưng dùng cho podcast.

#### Cách dùng nút
- S4 (NEXT): cuộn xuống episode tiếp theo
- S5 (BACK): cuộn lên, hoặc quay lại Discover nếu ở đầu danh sách
- S1 (MODE): quay lại Discover
- S3 (START): phát episode đã chọn
- S2 (ACTION): dừng phát

---

### 5.7 Màn hình Companion Chat

Màn hình này cho phép thử luồng chat đồng hành và ghi âm thoại.

#### Cách dùng nút
- S1 (MODE): bắt đầu ghi âm
- S3 (START): dừng và gửi ghi âm
- S5 (BACK): quay lại Home

#### Lưu ý
- Trong firmware hiện tại, luồng Companion Chat được thiết kế đơn giản hơn so với phiên bản đầy đủ.
- Nếu mic không sẵn sàng, thiết bị sẽ hiện thông báo tương ứng.

---

### 5.8 Màn hình Insights

Màn hình này dùng để xem thống kê cảm xúc theo khung thời gian.

#### Cách dùng nút
- S2/S4 (ACTION/NEXT): chuyển sang chu kỳ tiếp theo (Day → Week → Month)
- S3 (START): quay ngược chu kỳ trước đó
- S5 (BACK): quay lại Home

---

### 5.9 Màn hình Test Mic

Màn hình này dùng để kiểm tra mic và loa.

#### Cách dùng nút
- S5 (BACK): quay lại Home

#### Mục đích
- Kiểm tra khả năng thu và phát audio qua I2S.
- Nếu phần cứng kết nối đúng, bạn sẽ nghe được tín hiệu qua loa.

---

### 5.10 Màn hình WiFi Setup

Màn hình này dùng để kiểm tra và thay đổi cấu hình mạng.

#### Trạng thái hiển thị
- Online: đã kết nối Wi‑Fi và có thể dùng dịch vụ
- Unpaired: đã kết nối Wi‑Fi nhưng chưa pair với server
- Setup AP: đang ở chế độ hotspot cấu hình
- Offline: chưa có kết nối Wi‑Fi

#### Cách dùng nút
- S2 (ACTION): mở/đổi chế độ Wi‑Fi, hoặc thử kết nối lại
- S5 (BACK): quay lại màn hình trước

---

## 6. Cách thiết lập Wi‑Fi

### 6.1 Chế độ hoạt động mặc định

Khi thiết bị khởi động lần đầu và chưa có thông tin Wi‑Fi đã lưu, firmware sẽ tự kích hoạt chế độ Access Point để bạn cấu hình mạng.

#### Thông tin AP
- SSID: EmotiCare-Setup
- Mật khẩu: 12345678
- IP của AP: 192.168.4.1

> Lưu ý: README cũ có thể ghi mật khẩu khác, nhưng mã nguồn hiện tại đang dùng 12345678. Nếu bạn đang dùng bản firmware mới nhất, hãy ưu tiên theo code.

### 6.2 Thiết lập qua captive portal

1. Kết nối điện thoại hoặc máy tính vào Wi‑Fi tên EmotiCare-Setup.
2. Mở trình duyệt và truy cập địa chỉ: http://192.168.4.1
3. Bạn sẽ thấy form cấu hình gồm các trường:
   - SSID Wi‑Fi
   - Mật khẩu Wi‑Fi
   - Base URL của server (ví dụ http://192.168.1.10:8000)
   - Pairing code
4. Nhập thông tin và gửi form.

### 6.3 Quá trình tự động sau khi submit

Sau khi submit, thiết bị sẽ:
1. Kết nối tới mạng Wi‑Fi bạn nhập
2. Gọi endpoint pair tới server
3. Lưu token và device ID vào bộ nhớ NVS của ESP32
4. Khởi động lại để dùng cấu hình mới

### 6.4 Trạng thái Wi‑Fi trên màn hình
Sau khi cấu hình xong, bạn có thể quan sát trạng thái ở màn hình Home hoặc WiFi Setup:
- Online: đã kết nối mạng và đã pair thành công
- Unpaired: đã kết nối mạng nhưng chưa pair
- Setup AP: đang ở chế độ cấu hình
- Offline: chưa kết nối được mạng

### 6.5 Lưu ý quan trọng về server URL
- Base URL phải là IP hoặc hostname mà ESP32 có thể truy cập được.
- Không dùng localhost hoặc 127.0.0.1 vì chúng chỉ trỏ tới chính ESP32, không phải máy server.
- Ví dụ đúng: http://192.168.1.10:8000

---

## 7. Gỡ lỗi nhanh

### Thiết bị không vào được màn hình
- Kiểm tra cáp USB và nguồn cấp
- Xem Serial Monitor ở baud 115200
- Đảm bảo firmware đã được nạp đúng board target

### Không thấy hotspot EmotiCare-Setup
- Đảm bảo thiết bị đang ở chế độ provisioning
- Reset thiết bị nếu cần
- Kiểm tra log Serial để xem trạng thái Wi‑Fi

### Không kết nối được mạng đã nhập
- Kiểm tra mật khẩu Wi‑Fi
- Đảm bảo SSID đúng
- Kiểm tra tín hiệu Wi‑Fi và khoảng cách đến router

### Không pair được với server
- Kiểm tra Base URL có đúng và có thể truy cập từ ESP32
- Kiểm tra pairing code
- Kiểm tra server đang chạy và endpoint /api/devices/pair có sẵn

---

## 8. Lệnh build và upload nhanh

Từ thư mục firmware:

```powershell
cd "Smart Device"
pio run
pio run -t upload
pio device monitor -b 115200
```

Nếu cần upload tới cổng cụ thể:

```powershell
pio run -t upload --upload-port COM3
```

---

## 9. Tóm tắt ngắn để dùng ngay

- Home: dùng S4/S5 để chọn mục, S2/S3 để vào mục
- Check-In: dùng S2/S3 để quét cảm xúc
- Support: dùng S4/S5 để đổi mục, S2/S3 để mở chi tiết
- Discover: chọn Music hoặc Podcast rồi nhấn S2/S3
- Music/Podcast: S4/S5 cuộn, S3 phát, S2 dừng
- Companion Chat: S1 ghi âm, S3 gửi, S5 quay lại
- WiFi Setup: S2 để mở/đổi Wi‑Fi, S5 quay lại
- Cấu hình Wi‑Fi lần đầu: kết nối vào EmotiCare-Setup, mở http://192.168.4.1, nhập SSID/password/server URL/pairing code
