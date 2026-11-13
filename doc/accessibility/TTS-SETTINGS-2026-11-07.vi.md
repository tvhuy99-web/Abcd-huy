# Cài đặt TTS Android: mô tả đã đồng bộ mã hiện tại

**Cập nhật đối chiếu `master@f0be189`, 13/11/2026.** Phiên bản cũ của trang này chứa các nút không còn xuất hiện trong `TtsSettingsActivity`. Đọc [hướng dẫn hiện hành](USER-GUIDE.vi.md) và [đặc tả](CURRENT-REFERENCE.vi.md).

## Thứ tự điều khiển thật

1. **Bật trình đọc TTS trong game**.
2. **Bộ máy đọc**.
3. **Ngôn ngữ**.
4. **Giọng đọc**.
5. **Tốc độ**, 0,5 đến 10,0 lần, bước 0,1.
6. **Độ cao**, 0,5 đến 2,0 lần.
7. **Âm lượng**, 0–100%.
8. **Nghe thử**.
9. **Hủy**.
10. **Lưu**.

**Không có** nút riêng “Cho phép giọng cần mạng”, “Khôi phục giọng mặc định” hoặc “Mở cài đặt giọng Android” ở trang này. Cài đặt cho phép giọng mạng đã lưu trước đây vẫn được dùng để chọn giọng. Việc đổi lựa chọn hủy trạng thái thử trước, nhưng `saveSettings()` hiện **không kiểm tra `previewPassed`**, tức nghe thử thành công **không phải điều kiện bắt buộc để Lưu**. Nếu bật TTS nhưng âm lượng đang bằng 0, mã báo lỗi và không lưu.

TTS trong game không điều khiển bật/tắt TalkBack hệ thống. Khi TalkBack giữ quyền khám phá bằng chạm, ưu tiên các điều khiển Android có nhãn. Bộ đọc nội bộ và bộ máy TTS phải được thử trên điện thoại riêng để xác nhận nghe rõ, độ trễ, nội dung Việt và fallback khi thiếu giọng.

## Liên quan

- **Cài đặt trợ năng**: hiển thị đồ họa thế giới, sonar, độ dài/thời gian vuốt, Quay lại.
- **Cài đặt âm thanh game**: nhạc và hiệu ứng, riêng với giọng TTS.
- **Nhật ký gỡ lỗi**: bật ghi, xuất/xóa, riêng với TTS.

Nguồn chính: `TtsSettingsActivity.java` và `ReaderPreferences.java`. Kiểm thử tĩnh: `tools/accessibility/test_tts_settings.py`; cần thêm kiểm thử Android thật.
