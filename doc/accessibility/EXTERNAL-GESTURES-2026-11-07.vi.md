# Cử chỉ điều hướng các màn Android bên ngoài game

> **GHI CHÚ PHIÊN BẢN (13/11/2026):** Đây là hồ sơ chuyên đề theo thời điểm viết, không tự động là chỉ dẫn cho `master` sau PR #7. Khi cử chỉ, menu, quyền đọc bản đồ hoặc CI khác, ưu tiên [đặc tả hiện hành](CURRENT-REFERENCE.vi.md), [hướng dẫn](USER-GUIDE.vi.md) và [trạng thái](STATUS.md). **Không coi mã đã viết là nghiệm thu TalkBack trên máy thật.**

- Vuốt **hai ngón cùng xuống**: quay lại màn trước. Trên Cài đặt khởi động, nếu không có màn trước thì thoát về Android. Màn Cài đặt TTS đóng và hủy thay đổi chưa lưu. Các màn Trợ giúp trở về màn cài đặt trước đó.
- Để tránh nhận nhầm, phải có đúng hai ngón cùng đi xuống quá khoảng cách nhận cử chỉ, không di chuyển chủ yếu theo chiều ngang; cử chỉ hai ngón chạm (không vuốt) vẫn dừng phát giọng như trước.
- Vuốt **một ngón trái/phải**: di chuyển tiêu điểm giữa các mục.
- Khi tiêu điểm đang ở **thanh trượt** (tốc độ, cao độ, âm lượng, khoảng cách/thời gian vuốt), vuốt **một ngón lên** tăng một nấc, **một ngón xuống** giảm một nấc. TTS đọc lại tên/giá trị sau mỗi thay đổi; chỉ thay đổi trong giới hạn hợp lệ. Trên TTS, thao tác điều chỉnh vẫn cần bấm Lưu để áp dụng.
- Chạm hai lần kích hoạt nút đang chọn.
- **TalkBack hệ thống đang chạy**: cử chỉ do TalkBack kiểm soát. Ứng dụng nhường xử lý cho hệ thống để không tranh cử chỉ.
- Khi đang chép dữ liệu game, cử chỉ Quay lại không hủy giữa chừng, ứng dụng thông báo đang chuẩn bị.
- Cần kiểm thử trên máy thật với TalkBack bật và tắt, thao tác một ngón khi đang chọn thanh trượt, thao tác hai ngón và cài đặt đã lưu.
