# Menu tổng hợp, nhóm 15: Hệ thống, TTS và lưu game

Nhánh: `integration/reader-vietnamese-20261107`. Các chức năng dưới đây giữ luồng native và không thay đổi `master`.

## Thay đổi trợ năng

- **Ghi chú tự động:** lớp đọc cho hai tab Character/Global, tên từng loại ghi chú đã khám phá, ký hiệu và trạng thái, công tắc chung; chạm đôi toggles qua CONFIRM gốc; đổi tab đặt lại vị trí để không truy cập sai hàng. Thay đổi quy tắc vẫn phải qua hộp thoại Save changes? của engine khi thoát.
- **Điều kiện gián đoạn:** 17 hàng (16 điều kiện + Toggle all) với tên, mô tả, trạng thái; chọn qua CONFIRM gốc và phát kết quả bật/tắt. Chỉ duyệt không đổi lựa chọn.
- **Tải nhanh:** yêu cầu xác nhận khi trình đọc bật, trước thao tác tải/đánh dấu dirty; từ chối không kích hoạt tải.
- **Phóng to, thu nhỏ, bản đồ nhỏ:** dùng thông báo game/TTS để đọc mức zoom và trạng thái bản đồ nhỏ. Lệnh toàn màn hình không xuất hiện trên Android do engine không đăng ký.
- **Gỡ lỗi:** menu nêu rõ cách quay lại; trước mọi công cụ debug khi trình đọc bật đều có bước xác nhận riêng. Hủy không ảnh hưởng thành tựu hay trạng thái game. Các màn con dùng uilist đã có adapter chung; các công cụ debug đặc thù cần kiểm thử trên thiết bị, không khẳng định đã phủ toàn bộ.

## Kiểm thử

`python3 -m unittest discover -s tools/accessibility -p test_hub_system_accessibility.py -v`

Chạy CI Android, kiểm tra tương tác thực tế qua TalkBack/TTS: danh sách ghi chú rỗng, đổi tab khi số mục khác nhau, xác nhận lưu/hủy, bật/tắt 17 điều kiện, thử tải nhanh không lưu, mức zoom cuộn vòng, menu gỡ lỗi và quay lại. Không coi mã đã viết là đã nghiệm thu thiết bị.
