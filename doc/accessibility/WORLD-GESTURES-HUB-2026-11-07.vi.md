# Cử chỉ thế giới và menu tổng hợp: mô tả hiện hành

**Đính chính 13/11/2026, `master@f0be189`.** Tệp tháng 11 ban đầu ghi sai chức năng chạm hai ngón một lần, số mục menu và quyền gán đè cử chỉ; phiên bản này đã cập nhật. Đọc đầy đủ tại [đặc tả](CURRENT-REFERENCE.vi.md) và [hướng dẫn](USER-GUIDE.vi.md).

## Trên bản đồ thế giới

- Vuốt một ngón lên/xuống/trái/phải gửi bước đi Bắc/Nam/Tây/Đông khi game sẵn sàng. Có thể bị từ chối khi engine đang bận, ở xe hoặc trong cửa sổ khác.
- Vuốt hai ngón theo một trong bốn hướng đọc từng ô 1–16 bằng `scan_unrestricted_tile`, **có thể đọc xuyên tường và thấy dữ liệu bản đồ chưa được nhân vật biết**. Khác sonar và quan sát theo trí nhớ.
- Chạm một lần hai ngón là **Đọc chi tiết lựa chọn**, không phải mặc định đọc trạng thái nhân vật. Trong **chế độ chiến đấu**, cử chỉ này đọc danh sách kẻ địch.
- Chạm hai lần hai ngón đọc thông báo game; chạm ba lần hai ngón mở menu tổng hợp.
- Chạm một lần ba ngón đọc tọa độ; chạm **hai lần ba ngón** đọc đầy đủ thông tin nhân vật; chạm **ba lần ba ngón** bật/tắt chế độ chiến đấu.
- Chạm ba lần một ngón cũng mở Menu tổng hợp; nhấn giữ một ngón mở hành động ngữ cảnh.
- TalkBack hệ thống có thể giữ một số cử chỉ, khi đó dùng nút có nhãn và cách điều hướng TalkBack.

## Menu tổng hợp

Menu có **17 lựa chọn cấp đầu**, theo thứ tự: **Yêu thích**, 15 nhóm chức năng, **Tìm lệnh**. Không còn danh mục gốc 16 mục bắt đầu bằng Tìm lệnh. Chi tiết danh sách tên và thứ tự xem [hướng dẫn, mục 4](USER-GUIDE.vi.md).

Duyệt chỉ đọc, chạm đôi mới kích hoạt. Nhấn giữ một hàng để mở tùy chọn (thêm vào Yêu thích hoặc chọn shortcut khi phù hợp). Tìm lệnh và Tất cả lệnh chỉ hiện những lệnh được snapshot của engine công bố. Các mục đã lưu có thể không khả dụng ở ngữ cảnh mới.

## Gán cử chỉ

Có **36 mã** trong `ReaderGestureBindings.IDS`, **11 mã được giữ cố định**, còn **25 mã có thể gán**. Không thể gán đè vuốt một ngón bốn hướng, `hold_1`, các tap 2/3 ngón được giữ hoặc những hành vi trực tiếp của lớp Surface. Gán mới nhất đối với cử chỉ được phép sẽ thay gán cũ của nó.

**Không có khả năng thay “vuốt lên đi Bắc” bằng lệnh khác bằng bộ gán hiện hành.** Những đoạn hướng dẫn cũ nói ngược lại là thiết kế đã bị thay thế.

## Còn phải xác nhận

Tính đúng của nhận dạng đa ngón trên từng điện thoại, TalkBack bật/tắt, độ trễ sau một cú vuốt nhanh, mọi chuyển ngữ cảnh map/menu/combat và shortcut đã lưu từ APK trước. Có bài kiểm thử mã nhưng chưa có nghiệm thu toàn diện trên máy thật.
