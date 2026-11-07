# Bật trình đọc mặc định khi mở game

Áp dụng cho bản Android flavor `accessibility` trên nhánh tích hợp.

- Đổi nhãn ứng dụng từ “Thử trợ năng” thành “Trình đọc”.
- Bỏ màn thiết lập/thử giọng bắt buộc ở lúc mở game. Game vào luồng khởi động hoặc cài dữ liệu như bình thường.
- Trình đọc nội bộ mặc định bật cả khi cập nhật từ trạng thái thử nghiệm cũ. Di chuyển tùy chọn `enabled` sang bật một lần bằng cờ `auto_reader_default_20261107`. Khi người dùng chủ động tắt lại, trạng thái tắt vẫn được tôn trọng.
- Giữ màn cài đặt giọng và khả năng nghe kiểm tra như lựa chọn **không bắt buộc** khi muốn đổi giọng hoặc sửa lỗi.
- TTS nội bộ không phải TalkBack của Android và không thể tự bật TalkBack. TTS tiếng Việt cần được cài trong hệ thống; nếu không có giọng phù hợp thì ứng dụng chưa bảo đảm phát lời được.
- Chưa nghiệm thu trên thiết bị thật. Cần kiểm chứng cài mới, cập nhật dữ liệu cũ, chủ động tắt rồi mở lại, không có giọng TTS Việt và chế độ TalkBack.

Kiểm thử Android bổ sung: `ReaderAutoStartTest` kiểm tra nâng cấp từ cờ tắt cũ và giữ tùy chọn tắt sau đó.
