# Điều hướng menu theo từng cấp cho trình đọc màn hình

> **GHI CHÚ PHIÊN BẢN (13/11/2026):** Đây là hồ sơ chuyên đề theo thời điểm viết, không tự động là chỉ dẫn cho `master` sau PR #7. Khi cử chỉ, menu, quyền đọc bản đồ hoặc CI khác, ưu tiên [đặc tả hiện hành](CURRENT-REFERENCE.vi.md), [hướng dẫn](USER-GUIDE.vi.md) và [trạng thái](STATUS.md). **Không coi mã đã viết là nghiệm thu TalkBack trên máy thật.**

**Phạm vi:** quy tắc chung trong trình đọc Android và bộ công bố menu native,
không phải bản vá chỉ dành riêng cho màn hình tạo nhân vật.

## Quy tắc

1. Mỗi vòng vuốt chỉ duyệt các mục thuộc **một cấp**. Trình đọc Android dùng
   `parentId` của cây dữ liệu để ẩn mọi anh em của menu cha khi đang ở menu con.
   Điều này áp dụng đồng nhất cho các nút TalkBack và danh sách dùng TTS.
2. Khi kích hoạt mục có menu con, native xác nhận thao tác mở và công bố
   các con; trình đọc chuyển tới **mục đầu** và đọc tên menu cùng vị trí.
3. Khi nhấn Quay lại, trình đọc trả về **đúng menu cha**, giữ tiêu điểm tại
   mục đã mở con, thay vì trở về đầu.
4. Khi có hộp thoại/màn hình native khác che lên, engine vẫn dùng phạm vi
   `screen_scope` của màn hình đang mở. Trình đọc lưu vị trí theo
   `session:frame` để phục hồi khi modal đóng, nếu mục đó vẫn còn tồn tại.
5. Tất cả lệnh thực thi đều qua kiểm tra action/revision/token ở native.
   Chuyển cấp menu không tự động kích hoạt hành động trong trò chơi.

## Menu chính

Ở bản gốc, 9 mục menu chính vẫn được vẽ trong game khi submenu mở, và
snapshot cũ gửi **cả 9 mục chính lẫn 5 chế độ** tới trình đọc. Hiện game
công bố riêng 9 mục gốc hoặc riêng danh sách con đang mở:

- Menu chính: 9 mục, gồm Trò chơi mới.
- Khi kích hoạt Trò chơi mới: 5 mục chọn nhân vật (3 mục khi chế độ
  chia sẻ bản đồ chặn hai lựa chọn Chơi ngay).
- Menu con không chứa lại 9 mục menu chính.
- Quay lại từ menu con: khôi phục menu chính và mục đã chọn.

Chính giao diện game có thể vẫn vẽ các mục cha trên nền, nhưng vùng
tương tác và vòng đọc của người dùng chỉ chứa cấp hiện hành.

## Phạm vi kiểm thử

Bộ kiểm tra cấu trúc `test_nested_menu_navigation.py` kiểm tra quy tắc
ở mã nguồn; CI Android biên dịch Java và C++ phải tiếp tục thành công.
Việc TalkBack đưa tiêu điểm đến nút mới và phát âm đúng trên từng điện thoại
phải được thử thực tế với APK mới. Các giao diện chưa khai báo cây bằng
`parentId` vẫn cần kiểm tra riêng; trình đọc không nên tự suy đoán một
quan hệ cha-con không có trong dữ liệu do game cung cấp.

## Chọn mod trong phần quản lý thế giới

Màn hình mod vốn gửi danh sách danh mục, mod có thể dùng, mod đang kích hoạt
và lệnh điều khiển vào chung một danh sách. Bản sửa cấp dữ liệu cha-con:
danh mục hiện tại chứa mod có thể thêm, một mục riêng chứa danh sách mod
đang hoạt động, và một mục riêng chứa các lệnh quản lý và thông tin bộ lọc.
Chuyển sang một danh mục khác sẽ tự vào mục con đầu tiên sau khi game xác nhận
đổi danh mục. Trình đọc sử dụng cùng quy tắc Back như các menu khác.

Phần Tùy chọn đã có cấu trúc cây `parent_id` của trang, nhóm tùy chọn,
trường và điều chỉnh giá trị. Trình đọc áp dụng cơ chế một cấp cho cấu trúc
đó mà không sửa những lệnh thao tác/tương tác native của game.
