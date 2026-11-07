# Trang Cài đặt TTS cho Android

Trong menu chính: **Cài đặt > Cài đặt TTS**. Trong game có thể mở trang
này bằng nút **Cài đặt TTS** trên bảng điều khiển trình đọc. Phần
**Cài đặt trình đọc** cũ vẫn giữ các tính năng cử chỉ, cứu hộ,
sao lưu và phục hồi, nên không được xóa tùy tiện.

## Các mục theo đúng thứ tự

1. **Bật trình đọc TTS trong game** (ô bật/tắt, ở trên cùng).
2. **Bộ máy đọc** (các bộ máy TTS đã cài trong Android).
3. **Ngôn ngữ** (chọn trong các ngôn ngữ bộ máy đọc cung cấp).
4. **Giọng đọc** (giọng tương ứng với ngôn ngữ).
5. **Tốc độ** (0,5 đến 3,0 lần).
6. **Độ cao** (0,5 đến 2,0 lần).
7. **Âm lượng** (0 đến 100 phần trăm, theo từng câu nói).
8. **Cho phép giọng cần mạng** (mặc định không đồng ý).
9. **Khôi phục giọng mặc định**, **Mở cài đặt giọng đọc Android**,
   **Hủy, không lưu**.
10. **Nghe thử**, cuối cùng **Lưu**.

Bộ đọc của game tự bật sau cài mới, trừ khi bạn chủ động tắt rồi lưu.
TTS này **không bật/tắt được TalkBack** của Android.

Các lựa chọn nằm trong bản nháp. Khi nhấn **Nghe thử**, Android dùng
trực tiếp bộ máy, giọng, ngôn ngữ, tốc độ, độ cao và âm lượng vừa chọn
để nói câu mẫu. Nếu đã bật TTS, phải nghe thử thành công trước khi lưu
để giảm nguy cơ chọn phải giọng không phát tiếng; khi **tắt** TTS
vẫn cho phép lưu mà không cần thử. Mọi lựa chọn chỉ được ghi vào
SharedPreferences khi nhấn Lưu, rồi game tải lại chúng khi trở về.

Giọng cần mạng phải được chủ động cho phép. Một số bộ máy Android có
thể không hỗ trợ đầy đủ thông số độ cao hoặc âm lượng; thực tế còn tùy
máy và bộ máy đọc. Có nút mở cài đặt Android để cài giọng Việt khi cần.

## Kiểm thử cần thiết

- Chạy `python3 -m unittest discover -s tools/accessibility
  -p test_tts_settings.py`.
- Trên giả lập API 35: chạy `TtsSettingsPreferencesTest` và các bài
  Android instrumentation hiện có.
- Trên thiết bị thật: bật/tắt TTS và mở lại, đổi bộ máy/giọng,
  thử tốc độ/độ cao/âm lượng, không có giọng Việt, mất internet,
  bật TalkBack, trở về game sau Lưu và trường hợp nhấn Hủy.
- Chỉ sau khi build APK và xác minh thực tế mới coi mục này đã nghiệm thu.
