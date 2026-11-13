> **Đính chính 13/11/2026 theo `master@f0be189`:** đã sửa điều kiện Nghe thử/Lưu. Trang dưới đây ghi hiện trạng ở commit ghim, không phải nghiệm thu thiết bị. Xem [mô tả TTS](TTS-SETTINGS-2026-11-07.vi.md) và [đặc tả](CURRENT-REFERENCE.vi.md).

# Giao diện cài đặt TTS và âm thanh

Áp dụng tại nhánh `integration/reader-vietnamese-20261107`.

## Cài đặt TTS

Thứ tự điều khiển từ trên xuống dưới:

1. Công tắc **Trình đọc TTS**.
2. Danh sách thả xuống **Bộ máy đọc**.
3. Danh sách thả xuống **Ngôn ngữ**.
4. Danh sách thả xuống **Giọng đọc**.
5. Thanh trượt **Tốc độ**.
6. Thanh trượt **Độ cao**.
7. Thanh trượt **Âm lượng**.
8. **Nghe thử**.
9. **Hủy**.
10. **Lưu**.

Không có phần giải thích, nút khôi phục, nút Android Settings hay công tắc giọng mạng trong màn hình TTS rút gọn. Quyền dùng giọng mạng đã được lưu trước đó vẫn được tôn trọng; **không tự chấp nhận** giọng mạng. Nghe thử trước khi Lưu là khuyến nghị để bảo đảm có âm thanh, nhưng mã `TtsSettingsActivity.saveSettings()` hiện **không yêu cầu `previewPassed`** trước khi lưu. Khi bật với âm lượng 0, mã từ chối Lưu. Khi tắt, không cần thử giọng.

Với TalkBack, danh sách chọn dùng bộ chọn gốc của Android. Với bộ đọc nội bộ, vuốt chọn điều khiển và chạm đôi trên danh sách để chuyển sang lựa chọn kế tiếp, sau đó nghe tên được chọn.

## Cài đặt âm thanh

Trong menu chính **Cài đặt** có thêm **Cài đặt âm thanh**. Chỉ có bốn điều khiển:

1. Bật/tắt **Nhạc nền**.
2. Thanh trượt **Âm lượng nhạc**.
3. Bật/tắt **Hiệu ứng âm thanh**.
4. Thanh trượt **Âm lượng hiệu ứng**.

Thay đổi được lưu ngay; nhấn Quay lại để thoát. Giá trị thanh trượt của từng nhóm được giữ trong cấu hình Android khi tắt nhóm đó, trong khi âm lượng thực gửi cho game là 0. Khi bật lại, game dùng mức đã giữ. Âm lượng dùng cùng thang giá trị 0 đến 128 như các tùy chọn gốc `MUSIC_VOLUME` và `SOUND_EFFECT_VOLUME`. Âm thanh môi trường vẫn tuân theo cơ chế `AMBIENT_SOUND_VOLUME` của game; trong bộ máy âm thanh SDL, âm lượng hiệu ứng cũng điều chỉnh âm thanh môi trường.

Android chỉ đặt yêu cầu vào hàng đợi; C++ trên luồng game áp dụng `SOUND_ENABLED`, `MUSIC_VOLUME`, `SOUND_EFFECT_VOLUME` và lưu `options.json`. Không gọi trực tiếp hệ thống game từ luồng UI.

## Kiểm thử

- `python3 -m unittest discover -s tools/accessibility -p test_tts_settings.py -v`
- `python3 -m unittest discover -s tools/accessibility -p test_sound_settings.py -v`
- `Android TTS settings validation` để xác nhận biên dịch Java/Android.
- Build APK ARM64, thử trên điện thoại: chọn giọng, phát mẫu, Hủy/Lưu, bật tắt âm thanh và khởi động lại. Xác nhận mức âm lượng và công tắc được lưu đúng; kiểm tra khi thiếu soundpack.

Chưa xác nhận trải nghiệm thực tế cho đến khi build APK và kiểm thử trên thiết bị.
