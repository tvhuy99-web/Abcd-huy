# Menu nhóm 6: Túi đồ và vật phẩm

> **GHI CHÚ PHIÊN BẢN (13/11/2026):** Đây là hồ sơ chuyên đề theo thời điểm viết, không tự động là chỉ dẫn cho `master` sau PR #7. Khi cử chỉ, menu, quyền đọc bản đồ hoặc CI khác, ưu tiên [đặc tả hiện hành](CURRENT-REFERENCE.vi.md), [hướng dẫn](USER-GUIDE.vi.md) và [trạng thái](STATUS.md). **Không coi mã đã viết là nghiệm thu TalkBack trên máy thật.**

Nhánh: `integration/reader-vietnamese-20261107`. Phạm vi: 14 lệnh
khác nhau, dùng lại nhặt đồ và danh sách xung quanh ở các nhóm trước.

## Thay đổi

- **Hành trang:** đọc vị trí cất, trọng lượng, thể tích, trạng thái
  đang cầm/đang mặc, món yêu thích và phím tắt; cây vật chứa vẫn giữ
  giới hạn quyền nhìn của phiên bản trợ năng hiện có.
- **Kho nâng cao:** đọc nguồn, đích, vật phẩm, dung lượng; mô tả rõ
  các lệnh chuyển một món, cả nhóm và đổi hai ngăn. Thông báo yêu cầu
  chuyển **chỉ sau khi** game chấp nhận lên lịch hành động, không gọi
  chuyển thành công khi đồ chưa thực sự chuyển.
- **Nhặt đồ và nhặt xung quanh:** dùng lại các cải tiến nhóm 5. Không
  tự động nhặt chỉ vì đang duyệt danh sách.
- **Thả tại chỗ hoặc ô kề:** đọc số lượng đã đánh dấu trong menu;
  khi xác nhận đọc lượng đồ và mô tả ô đích; hành động thả vẫn do game
  thực hiện, có thể tốn lượt hay bị từ chối.
- **So sánh:** thêm hai tài liệu TTS có đầy đủ thông số thực từ cùng
  `first_info`/`second_info` đang dùng ở hai cột giao diện gốc.
- **Đổi phím tắt:** đọc xác nhận khi đặt hoặc xóa thật.
- **Dùng đồ và menu hành động:** đọc vật đang cầm, mục được kích
  hoạt và các yêu cầu hiển thị của menu gốc; không khẳng định thành
  công trước khi game xử lý.
- **Cất vào vật chứa:** đọc vật chứa và đồ đã chọn; game kiểm tra
  sức chứa và có thông báo kết quả thành công/thất bại trong hoạt động
  `insert_item_activity_actor`.
- **Lấy đồ ra:** giữ lọc loại trừ chất lỏng và đồ đánh dấu yêu thích;
  đọc số nhóm đồ sắp lấy và mô tả nơi đặt xuống.
- **Danh sách vật phẩm xung quanh:** dùng lại menu quan sát trợ năng.

## Nguyên tắc

Duyệt, nghe, mở rộng túi, xem so sánh hoặc sửa nháp số lượng không
tiêu hao lượt và không chuyển đồ. Thao tác thật tiếp tục qua hành
động engine, có kiểm tra sức chứa, quyền sở hữu và điều kiện hiện hành.

## Kiểm thử

`python3 -m unittest discover -s tools/accessibility -p test_hub_inventory.py -v`

Cần kiểm tra trên thiết bị: nhiều vật cùng tên, cất đồ trong túi lồng,
thả chất lỏng, trộm cắp, chuyển giữa thùng xe và mặt đất, hai vật phẩm
có nhiều thông số, bộ đọc TalkBack và kết quả sau hoạt động kéo dài.
