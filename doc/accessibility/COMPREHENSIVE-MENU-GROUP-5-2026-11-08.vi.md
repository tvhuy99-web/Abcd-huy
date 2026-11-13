# Menu tổng hợp, nhóm 5: Tương tác với môi trường

> **GHI CHÚ PHIÊN BẢN (13/11/2026):** Đây là hồ sơ chuyên đề theo thời điểm viết, không tự động là chỉ dẫn cho `master` sau PR #7. Khi cử chỉ, menu, quyền đọc bản đồ hoặc CI khác, ưu tiên [đặc tả hiện hành](CURRENT-REFERENCE.vi.md), [hướng dẫn](USER-GUIDE.vi.md) và [trạng thái](STATUS.md). **Không coi mã đã viết là nghiệm thu TalkBack trên máy thật.**

Nhánh: `integration/reader-vietnamese-20261107`

- **Kiểm tra xung quanh**: thay chọn hướng bằng một danh sách tất cả ô **đang nhìn thấy và có đối tượng có thể kiểm tra** ở vị trí đứng và tám ô kề. Mỗi dòng đọc hướng và mô tả đối tượng; chỉ khi chọn mới thực hiện lệnh gốc, hủy không tốn lượt.
- **Tương tác**: liệt kê ô hiện tại và tám ô kề có hành động khả dụng; chọn ô rồi chọn hành động gốc nếu cần. Không tự dùng cầu thang ở ô bên cạnh.
- **Mở cửa, đóng cửa**: vẫn dùng kiểm tra và thông báo gốc của game; đối tượng được chọn trong danh sách được truyền tới luồng mở cửa.
- **Đập phá**: đọc kết quả phá hủy hoặc chưa phá xong khi thực sự đập; giữ nguyên cảnh báo tài sản, kiểm tra dụng cụ và xác nhận phá tiếp.
- **Nhặt đồ**: vẫn chọn đúng một ô. **Nhặt đồ xung quanh**: dùng ô hiện tại và tám ô kề, không mở rộng 60 ô. Danh sách đọc vị trí, trọng lượng, thể tích, số có và số đã đánh dấu; tổng số đánh dấu nằm trong thông tin màn hình. Duyệt không tự nhặt.
- **Kéo đồ**: đọc trạng thái đang kéo, số món và tự kéo khi thay đổi hoặc xem lệnh; không báo “đã kéo” nếu chỉ chọn trong menu.
- **Xẻ thịt/tháo dỡ**: thông báo số xác và các nhóm vật có thể xử lý. Thời gian được lấy từ tùy chọn gốc khi game cung cấp; cảnh báo thiếu dụng cụ và kết quả vẫn đi qua thông báo game.
- **Trò chuyện, nắm đồ, nhìn lén**: giữ nguyên menu, xác nhận và hành vi gốc.

Danh sách kiểm tra/tương tác tôn trọng lớp quan sát `a11y::observe_tile`: không đọc nội dung vật thể/địa hình chưa nhìn thấy, không tiết lộ đồ trong vật chứa kín.

Kiểm thử mã nguồn:
`python3 -m unittest discover -s tools/accessibility -p test_hub_interactions.py -v`

Cần kiểm thử trên điện thoại các tình huống thiếu ánh sáng, nhiều vật thể cùng ô, cửa xe, vật chứa đóng, vượt sức chứa, kéo nhiều món, cảnh báo trộm cắp và TalkBack.
