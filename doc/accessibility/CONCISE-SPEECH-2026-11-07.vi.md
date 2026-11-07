# Tinh giản lời đọc theo danh sách người dùng duyệt

Nhánh: `integration/reader-vietnamese-20261107`.

## Nguyên tắc

- Quét bốn hướng: bỏ câu mở đầu “Quét phía … 16 ô, từ gần đến xa”; đọc ngay từ “Ô 1, …” đến “Ô 16, …”, giữ thứ tự và dữ liệu thực tế của từng ô.
- Cụm từ ngắn cho vật cản, bẫy, xe và tọa độ. Không xóa số ô, tên sinh vật, tên bẫy, đồ vật hoặc thông tin nguy hiểm.
- Bản dịch trợ năng rút gọn các câu đã duyệt ở trạng thái, menu thế giới, tạo nhân vật, ngắm bắn, chế tạo, giao dịch và hoạt động.
- Màn hình Android: thông báo cấu hình và kết quả thông thường ngắn gọn hơn. Tiến độ cài dữ liệu chỉ **phát lời** khi vượt các mốc 25%, 50%, 75%, 100%; không cập nhật vùng TalkBack liên tục từng phần trăm.
- Từ các thay đổi trước đã có: bước đi thông thường im lặng, màn hình menu đọc tên màn hình một lần rồi vuốt đọc tên mục và vị trí; không sửa lại các phần đã đáp ứng.
- Nội dung trợ giúp/chi tiết vẫn được đọc khi người chơi yêu cầu. Cảnh báo khẩn, dữ liệu không hợp lệ, lưu/khôi phục, xóa thế giới và xác nhận hành động nguy hiểm phải giữ nguyên nghĩa.
- Không suy đoán “đã hủy” nếu game chỉ cho biết không có giao dịch; dùng “Không giao dịch”.
- TalkBack của hệ điều hành có thể tự công bố nội dung riêng ngoài kênh TTS ứng dụng.

## Kiểm tra

`python3 -m unittest discover -s tools/accessibility -p test_concise_speech.py`

Cần kiểm chứng bằng APK trên điện thoại hoặc giả lập: giọng Việt ở cả 4 hướng quét; đọc đủ 16 ô đúng thứ tự; bẫy/quái/xe/đồ; cảnh báo nguy hiểm không bị lọc; tiến độ cài đặt không spam; đọc menu/chọn mục; hành động bị chặn; TalkBack bật và tắt. Mã nguồn và dịch chỉ là hợp đồng dự kiến, chưa thay thế xác nhận TTS thực.
