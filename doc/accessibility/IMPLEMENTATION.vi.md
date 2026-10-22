# Bảng triển khai theo quy trình ba giai đoạn

Cập nhật 22/10/2026, giờ Việt Nam. Hiện ở **giai đoạn 1: viết mã**. Chưa bắt
đầu chạy kiểm thử cho các thay đổi ở nhánh `feature/android-reader-code-phase`.
Không có APK mới của nhánh này. Bản đã được người dùng xác nhận mở/đọc được
là APK thử giọng tại `cb3c082`, không phải mã mới ở đây.

## Quy trình đã chốt

1. Viết mã lần lượt cho toàn bộ phạm vi trong `PLAN.vi.md`. Duy trì bảng phần
   còn thiếu và các tình huống cần kiểm chứng; không đánh dấu hoàn tất chỉ vì
   có lớp/hàm, danh mục lệnh hoặc đường vào một menu.
2. Sau khi phần viết mã hoàn tất, biên dịch và chạy bộ kiểm thử đầy đủ: host,
   Android/API, giả lập, APK ARM64, máy thật và 64 hành trình/người khiếm thị.
   Kiểm thử người dùng không thể thay bằng test doubles hoặc build thành công.
3. Tổng hợp lỗi theo ảnh hưởng và nhóm tính năng, sửa, rồi chạy lại bài liên
   quan và hồi quy. Chỉ phát hành sau khi đủ bằng chứng theo tiêu chí kế hoạch.

Workflow trợ năng của nhánh mới chuyển sang `workflow_dispatch`; không chạy
build/test tự động trong đợt viết mã này. Không merge vào `master` và không
ghi PASS mới.

## Mã đã bổ sung trong đợt hiện tại, chưa kiểm thử

| Nhóm | Điểm triển khai | Trạng thái/giới hạn |
| --- | --- | --- |
| Ngữ nghĩa | `a11y_semantics`, `ReaderSnapshot` | Nút, vai trò, giá trị, trạng thái, vị trí, chi tiết, token và revision; chưa nghiệm thu schema/Unicode |
| Sở hữu và luồng | `a11y_runtime`, `a11y_android` | Scope, mailbox hữu hạn, lọc token/revision/request; Java không giữ con trỏ gameplay; chưa kiểm thử race/vòng đời |
| Android | `GameReaderController`, `CataclysmDDA` | Lớp trình bày native và gửi ý định; phối hợp touch exploration; chưa xác nhận focus/insets/Back trên máy |
| Lời đọc | `ReaderSpeechQueue` | Gộp focus, ưu tiên, hết hạn, đọc theo đoạn, dừng/đọc lại/watchdog; chưa kiểm thử TTS thật cho mã mới |
| Cử chỉ | `ReaderGestureSurface` | Duyệt/chọn/chi tiết/dừng, chặn hủy/nhiều ngón; chưa có cấu hình chơi nhanh tám hướng hoặc hướng dẫn luyện tập |
| Cấu hình | `ReaderPreferences`, `ReaderSetupActivity`, Splash | Bật bằng xác nhận người dùng đã nghe; tắt; đường vào màn thiết lập của flavor thử; bootstrap không TalkBack chưa đầy đủ |
| Nhập liệu | `ReaderTextEditor`, adapter popup cũ/ImGui/số | IME/TalkBack và bàn phím lựa chọn Việt, dấu thanh, biên ký tự ICU, nhận kết quả có token; chưa nghiệm thu giới hạn và callback tùy biến |
| Menu chính | `main_menu.cpp` | Tab, lựa chọn con, thế giới, tài liệu; chưa xác nhận tạo/tải save |
| Danh sách | `uilist.cpp` | Theo `fentries`, đúng `retval` qua đường xác nhận gốc, metadata đánh dấu/lý do vô hiệu; có helper cho vòng `query_once` ngoài |
| Hộp thoại | `popup.cpp` | Văn bản câu hỏi, chọn, hủy/tiếp tục qua action gốc; các dialog native khác vẫn cần tích hợp |
| Tạo nhân vật | `newcharacter.cpp` | Nối danh sách tab và lệnh; mô tả scenario/profession/trait/skill, trạng thái đánh dấu, giá trị chỉ số và một phần summary; chưa phủ mọi chi tiết hiển thị |
| Phiên chơi | `game.cpp`, `game_io.cpp` | Đổi định danh khi setup/load, bỏ snapshot/đầu vào cũ; chưa kiểm thử lỗi tải/quay về/Activity mới |
| Thông tin hợp lệ | `a11y_perception` | Nhánh CLEAR mới đọc dữ liệu sống; nhánh trí nhớ chỉ lấy terrain/furniture từ memory; bẫy/item/creature theo kiểm tra gốc; cảm nhận đặc biệt và nhiều tầng còn thiếu |
| Thế giới | `a11y_world`, `get_player_input` | Ô hiện tại, trạng thái, thông báo gần đây, bán kính quét và lệnh đã đăng ký; đọc ô không tự di chuyển; chưa có toàn bộ bộ thăm dò/bộ lọc/đường đi |
| Bản địa hóa | `lang/po/accessibility/vi.po`, helper compile | Bổ sung câu qua gettext, không thay từ tiếng Anh tại runtime; chưa chạy gettext/build mới |

## Phần viết mã còn phải làm

Các hàng dưới đây vẫn là công việc bắt buộc của kế hoạch, không phải phần đã
hoàn tất nhờ menu chung hoặc danh mục action:

- G0/G1: hoàn thiện bootstrap khi TalkBack tắt, chọn engine/voice/tốc độ,
  thông báo phục hồi có âm thu sẵn, cài/nâng dữ liệu, lỗi dung lượng và phục hồi.
- G2: cử chỉ nhanh tám hướng, luyện tập, đổi ánh xạ/khôi phục, biểu mẫu/bảng/cây,
  các callback chỉ đọc phím thô, toàn bộ dialog native và adapter ngữ cảnh lồng.
- G3: hoàn thiện chi tiết/đánh giá mọi tab tạo nhân vật, chọn thế giới/mod,
  tên nhân vật, tạo mới và tải lại, cấu hình lượt/realtime có lựa chọn rõ ràng.
- G4: thăm dò nhiều tầng/bộ lọc/điểm đáng chú ý, cảm nhận đặc biệt và âm thanh
  bất định; quan sát theo hành động thực tế, nhặt/đồ chứa, cơ thể/y tế, ăn/uống,
  mặc/cởi, nghỉ/đợi/đọc, hủy hoạt động, trạng thái và cảnh báo tự động.
- G5: hành trang chọn nhiều/chuyển đồ, nhắm bắn và đường bắn, cận chiến,
  đạn/chế độ bắn, ném, chế tạo/công thức/tiến độ và xây dựng/chọn vị trí.
- G6: overmap/travel/note/mission; NPC/đối thoại/giao dịch/cộng đồng; phương tiện
  và sơ đồ bộ phận/lái/remote; vùng/tự động hóa; bionics/đột biến/phép thuật;
  các minigame, tài liệu, trợ giúp và những màn riêng trong `COVERAGE.md`.
- G7: ngủ/hoạt động dài, nguy hiểm và kết quả thật, chết/death-cam/điểm số,
  lỗi TTS/vòng đời/tiến trình, lưu/tải/xóa và sao lưu/khôi phục dữ liệu.
- G8: chẩn đoán/độ trễ/âm hiệu/rung, tài liệu người chơi và bảo trì nội dung;
  bộ kiểm thử/fixture, hoàn tất ma trận 64 bài, build/ký/bằng chứng phát hành.

Giai đoạn 1 chỉ hoàn tất khi từng hàng còn thiếu được triển khai cụ thể và
đối chiếu lại với toàn bộ ma trận tính năng của kế hoạch. Hiện chưa đạt mốc đó.
