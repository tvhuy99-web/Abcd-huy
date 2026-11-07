# Bộ đọc gọn khi di chuyển tiêu điểm

Mục tiêu: khi vuốt tới một mục, bộ đọc **chỉ đọc tên mục và vị trí**, không nhắc lại tên trò chơi, tiêu đề màn hình hay trạng thái "Đang chọn".

- Ví dụ tại menu chính: `Trò chơi Mới, Mục 2 trên 9`.
- Vuốt tới mục kế tiếp: `Tải game, Mục 3 trên 9`.
- Áp dụng chung tại `GameReaderController.readCurrent()` cho các snapshot trong game, bao gồm menu, túi đồ, tùy chọn, tương tác và danh sách, đồng thời rút gọn lời đọc lựa chọn ở bàn phím nhập liệu.
- Vẫn đọc giá trị, trạng thái checkbox, lý do mục bị vô hiệu, thông tin bản nháp và lỗi biểu mẫu khi cần; không được loại bỏ cảnh báo hay thông điệp khẩn cấp.
- Nút "Tôi đang ở đâu?" vẫn đọc tên màn hình và tóm tắt khi người chơi **chủ động yêu cầu**. Chức năng "Đọc chi tiết" vẫn cho biết trạng thái "Đang chọn" và mô tả chi tiết.
- Với TalkBack bật, Android có thể tự đọc thuộc tính được chọn của nút theo cơ chế hệ thống; thay đổi này kiểm soát lời đọc do ứng dụng tạo ra, không kiểm soát TalkBack.
- Kiểm thử nguồn: `python3 -m unittest discover -s tools/accessibility -p test_concise_focus.py`.
- Cần kiểm tra thêm trên điện thoại/giả lập: menu chính lần đầu, vuốt qua các mục, tạo nhân vật, túi đồ, bảng tùy chọn, checkbox, mục vô hiệu, cảnh báo, nút "Tôi đang ở đâu?" và màn nhập liệu; xác nhận TTS Việt thực sự phát ra câu mong đợi.

Các chức năng sao lưu/khôi phục và cảnh báo an toàn không bị rút gọn.
