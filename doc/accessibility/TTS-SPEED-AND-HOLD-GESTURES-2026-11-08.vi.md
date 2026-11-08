# Nâng tốc độ TTS và thêm cử chỉ chạm giữ

Áp dụng cho bản Android trợ năng của game.

## Tốc độ TTS

- Giữ nguyên vị trí **Tốc độ** trong màn hình **Cài đặt TTS**.
- Thanh trượt cũ từ 50% đến 300% được mở rộng thành **50% đến 1.000%**, theo bước 10%.
- Bộ nhớ cài đặt, khôi phục cài đặt và trình điều khiển Android TTS đều chấp nhận mức mới tối đa **10,0 lần**.
- Giữ các tùy chọn TTS và âm thanh đã thống nhất; không thêm hàng chú thích hoặc nút mới.
- **Giới hạn thực tế tùy bộ máy và giọng đọc đã cài**: Android có thể giới hạn tốc độ thực dù ứng dụng yêu cầu 1.000%. Nếu nghe thử không đạt, người dùng có thể giảm thanh trượt trước khi Lưu.

## 36 cử chỉ trong menu tổng hợp

Trong **Menu tổng hợp**, nhấn giữ một lệnh, bảng **Chọn cử chỉ** nay có **36** tùy chọn thay vì 33. Ba tùy chọn mới được nối vào **cuối danh sách**:

1. **Chạm và giữ một ngón tay** (`hold_1`).
2. **Chạm và giữ hai ngón tay** (`hold_2`).
3. **Chạm và giữ ba ngón tay** (`hold_3`).

Không đổi mã nhận diện hoặc thứ tự 33 cử chỉ cũ. Bộ gán vẫn lưu liên tục, tuân theo quy tắc **lần gán mới nhất ghi đè** và hỗ trợ **Khôi phục cử chỉ mặc định**.

Trong thế giới, một ngón tay nhấn giữ khi chưa gán vẫn có chức năng **Đọc chi tiết** như trước; nếu đã gán thì chạy lệnh mới. Trên các menu, nhấn giữ vẫn mở chức năng gán cử chỉ, không bị lệnh trò chơi chiếm. Hai và ba ngón tay chạm giữ được nhận diện sau thời gian giữ dài của Android, chỉ khi các ngón được đặt gần như đứng yên; chuyển ngón, nhấc ngón, vuốt, đổi màn hình hoặc sự kiện hủy sẽ dừng nhận diện. Sau khi một cử chỉ chạm giữ đã được kích hoạt, không phát lại cử chỉ chạm/đúp chạm hoặc vuốt khi nhấc tay.

**TalkBack** có thể chiếm cử chỉ nhiều ngón, vì vậy chức năng này áp dụng cho bộ đọc tích hợp trong lúc chơi và cần thử trên thiết bị thật.

## Kiểm thử

- Kiểm tra tĩnh: `test_custom_gesture_bindings.py`, `test_comprehensive_hub_restore.py`, `test_tts_settings.py`.
- Kiểm tra Android: `ReaderGestureBindingsTest`, `TtsSettingsPreferencesTest`, `GameMenuGestureDispatchTest`, bao gồm thời gian giữ, loại trừ vuốt và phân biệt menu/trò chơi.
- Biên dịch Android Java và build APK ARM64 trên nhánh tích hợp trước khi nhập vào `master`.
