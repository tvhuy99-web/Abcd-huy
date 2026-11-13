# Cử chỉ cố định và đọc đầy đủ thông tin nhân vật

**Đối chiếu lại 13/11/2026 trên `master@f0be189`.** Thay thế bản cũ nói chạm ba lần ba ngón còn trống và có thể gán. Xem [đặc tả hiện hành](CURRENT-REFERENCE.vi.md).

## Nguyên tắc

Các cử chỉ có chức năng gốc không thể bị ghi đè bởi cấu hình tùy chỉnh đã lưu. TalkBack và cử chỉ của Android luôn có quyền ưu tiên khi hệ thống giữ chúng.

| Cử chỉ | Hành vi có mã hiện tại | Gán lại? |
| --- | --- | --- |
| Vuốt một ngón lên/xuống/trái/phải | Di chuyển ở thế giới; duyệt menu | Không |
| Chạm đôi một ngón | Kích hoạt/xác nhận | Không |
| Chạm ba lần một ngón | Mở menu hành động | Không |
| Nhấn giữ một ngón | Hành động ngữ cảnh | Không |
| Vuốt hai ngón bốn hướng | Quét 16 ô ở thế giới; nghĩa riêng ở menu | Không |
| Chạm một lần hai ngón | Đọc chi tiết; ở combat đọc kẻ địch | Không |
| Chạm hai lần hai ngón | Đọc thông báo thế giới | Không |
| Chạm ba lần hai ngón | Mở menu tổng hợp | Không |
| Chạm một lần ba ngón | Đọc tọa độ | Không |
| Chạm hai lần ba ngón | Đọc đầy đủ nhân vật | Không |
| **Chạm ba lần ba ngón** | **Chuyển chế độ bản đồ/chiến đấu** | **Không** |
| Vuốt ba/bốn ngón, chạm bốn ngón, giữ hai/ba ngón, các chuỗi vuốt một ngón | Gán shortcut nếu không có chức năng mặc định | Có, tùy ngữ cảnh |

`ReaderGestureBindings.IDS` có **36 mã**. Hàm `isReserved()` hiện đánh dấu **11 mã**, còn **25 mã tùy chỉnh**. Một số cử chỉ khác do `ReaderGestureSurface` nhận trực tiếp không nằm trong danh mục gán nên không được cộng vào 36.

## Đọc thông tin nhân vật

Tại thế giới bình thường, chạm **nhanh hai lần bằng ba ngón**. Mã đọc node `world:character_detail` của snapshot hiện tại. Nội dung gồm tên, chỉ số nhân vật, nhu cầu sinh tồn, tình trạng bộ phận, hiệu ứng, chế độ an toàn và trang bị/trạng thái xe nếu game công bố. Đây là thao tác đọc, không tự tiêu lượt.

Không tự suy ra HP chính xác hoặc chi tiết bị ẩn nếu engine chỉ mô tả tình trạng bằng chữ. Nếu snapshot chưa sẵn sàng, có thể nghe thông báo đang chờ.

## Chế độ chiến đấu

Chạm **ba lần bằng ba ngón** để bật hoặc tắt chế độ chiến đấu, *không phải cử chỉ để gán*. Khi combat bật, một ngón dùng ra đòn hướng; hai ngón vuốt để đi; chạm một lần hai ngón đọc kẻ địch. Phải nghe trạng thái chế độ trước khi dùng.

## Tương thích cấu hình cũ

Những ánh xạ đã lưu cho mã cử chỉ cố định được bộ đọc gán bỏ qua và làm sạch. Các lựa chọn đổi chức năng vuốt ngang/dọc kiểu thử 45 giây không còn điều khiển map navigation trong mã mới. Vẫn có thể chỉnh độ dài/thời gian vuốt và thuận tay khi điều hướng.

## Bài cần thử trên thiết bị

Cử chỉ 3 ngón chạm hai và ba lần; chế độ bản đồ ↔ chiến đấu; gián đoạn màn hình; TalkBack bật; nâng cấp từ cài đặt cũ; mọi shortcut chưa gán; không có lệnh combat bị kích hoạt do người dùng chỉ muốn đọc nhân vật. Các bài nguồn chỉ xác nhận mã/khế ước, không thay cho thử đa ngón trên điện thoại.
