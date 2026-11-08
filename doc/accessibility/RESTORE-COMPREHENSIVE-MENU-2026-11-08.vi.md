# Khôi phục menu tổng hợp và gán cử chỉ

Đối chiếu lịch sử commit `443d1b2` với nhánh tích hợp sau khi phần giao diện đọc cũ bị dọn. Giao diện menu tổng hợp đã bị loại bỏ, còn chức năng nhận dạng/lưu 36 cử chỉ vẫn tồn tại.

## Đã khôi phục

- Chạm **ba lần bằng hai ngón** trong thế giới để mở **Menu tổng hợp** gồm 16 nhóm, từ **Tìm lệnh** đến **Hệ thống, TTS và lưu game**.
- Lần mở đầu đọc: “Menu tổng hợp. Tìm lệnh, mục 1 trên 16”; vuốt tiếp chỉ đọc tên nhóm và thứ tự. Khi menu mở, vuốt một ngón chỉ điều hướng, **không làm nhân vật di chuyển**.
- Lệnh tìm kiếm hỗ trợ tiếng Việt có dấu hoặc không dấu; **Tất cả lệnh** hiển thị lệnh thực sự được bản mô tả từ engine cung cấp.
- Chạm hai lần để mở nhóm/thực hiện lệnh; **nhấn giữ một mục** mở danh sách 36 cử chỉ, kèm **khôi phục mặc định** và **hủy**.
- Một cử chỉ chỉ có một lệnh. Khi được gán cho lệnh mới, lệnh trước mất gán tùy chỉnh và quay về mặc định. Các cài đặt gán cũ được giữ, cả dạng `hub:group:`, `hub:search`, `action:`.
- Nút Quay lại đi từ bảng chọn cử chỉ về mục đã chọn, từ nhóm con về đúng vị trí ở menu gốc, sau đó đóng menu để trở lại trò chơi.
- Với lệnh thực hiện hành động, menu đóng trước khi gửi yêu cầu đến engine; engine kiểm tra lại lệnh, trạng thái và phiên giao diện. Không kích hoạt hành động khi chỉ duyệt.
- Màn cài đặt TTS được mở từ nhóm Hệ thống, không thay đổi thứ tự 10 điều khiển đã được duyệt. Mục cài đặt âm thanh tách riêng trong menu chính của game vẫn giữ nguyên.
- **Không khôi phục bảng điều khiển cũ luôn hiển thị**, nút thử trợ năng hay màn hình luyện tập.

## Phạm vi và hạn chế

Danh mục chức năng bên trong các nhóm phụ thuộc vào các lệnh **thực sự có trong snapshot của game**, không phải mọi chức năng mà bản thiết kế đề xuất đều có lệnh sẵn để thực hiện. Các mục chưa có dữ liệu chưa được giả lập thành nút không có tác dụng.

Chưa xác nhận vận hành trên điện thoại. Cần thử với TTS Việt và TalkBack, trong menu và ngoài thế giới, đặc biệt chạm ba lần bằng hai ngón và nhấn giữ khi màn hình cập nhật.

## Kiểm thử

`python3 -m unittest discover -s tools/accessibility -p 'test_*.py' -v`

`Android TTS settings validation` biên dịch Java Android; `Android accessibility baseline and candidate` chạy bộ kiểm thử và build APK ARM64.
