# Rà soát màn hình im lặng khi chơi nhanh và chuyển giao diện

Thực hiện trên nhánh `integration/reader-vietnamese-20261107`. Đây là rà soát **mã nguồn**, không phải kết quả thao tác thực tế trên toàn bộ màn hình của APK.

## Sự cố có thể tái hiện từ ảnh

- Người chơi chọn Trò chơi Mới → Chơi ngay! (Kịch bản Mặc định), tương ứng `sel2 == 3`.
- Màn ảnh đen có viền và mẹo màu xanh, phía dưới có nhãn trạng thái ví dụ “Đang khởi tạo. Công thức chế tạo”. Đây là `loading_ui::show()`, chưa chứng minh đã vào màn chơi.
- `game::setup()` hủy session trợ năng của menu chính khi bắt đầu tải, làm snapshot tạm thời không tồn tại.
- `GameReaderController.announceGameMessage()` trước đây chặn các thông báo khi `snapshot == null`. Màn ImGui tải không tạo `screen_scope`, nên bị im lặng.
- Sau khi trò chơi tạo snapshot `world:here`, ứng dụng trước đây chỉ đọc ô hiện tại, đã từng đọc lặp câu “đã vào màn chơi”.

## Thay đổi

1. Xác nhận bằng lời ngay khi bắt đầu chế độ chơi nhanh.
2. Đọc các giai đoạn tạo thế giới, dữ liệu và nội dung trên màn ImGui tải qua kênh `A11Y_LOADING:`. Đọc mẹo màu xanh **một lần**, không đọc lại liên tục.
3. Giới hạn thông báo trạng thái để tránh TTS bị ngập: nói lần đầu, sau đó tối đa theo sự thay đổi đã đủ 8 giây hoặc nhắc lại sau 30 giây có hoạt động tải.
4. Cho bộ đọc Java nhận trạng thái tải ngay cả khi `snapshot == null`, giữ bước tải gần nhất để đọc lại.
5. Nếu sau 45 giây không có cập nhật và cũng không có snapshot, nhắc rằng trò chơi có thể đang tải hoặc không phản hồi. **Không đoán là đã hoàn tất**.
6. Khi có snapshot bản đồ đầu tiên, im lặng ở ô thường; chỉ phát ngắn gọn địa hình quan trọng. Cử chỉ di chuyển nhân vật vẫn dùng cơ chế riêng của game.
7. Màn trình diễn ImGui dành cho nhà phát triển không có adapter trợ năng. Khi trình đọc bật, cung cấp mục quay lại bằng `uilist` có lời đọc thay vì chuyển vào trang demo im lặng.

## Đối chiếu các nhóm giao diện

| Nhóm | Cơ chế đọc quan sát được trong mã | Kết luận |
|---|---|---|
| Menu chính và chọn chế độ mới | `main_menu.cpp` `screen_scope` | Đã có snapshot |
| Tạo và cấu hình nhân vật | `newcharacter.cpp` `screen_scope`, `uilist` | Đã có adapter, cần thử thực tế từng nhánh |
| Danh sách, thông báo xác nhận | `uilist.cpp`, `popup.cpp` `screen_scope` | Đã có adapter |
| Thế giới, mod, cấu hình thế giới | `worldfactory.cpp` `screen_scope` | Đã có adapter |
| Tùy chọn, màu sắc, kiểu UI | `options.cpp`, `color.cpp`, `ui_style_picker.cpp` | Đã có adapter hoặc `uilist` |
| Túi đồ, chế tạo | `inventory_ui.cpp`, `crafting_gui.cpp` | Đã có adapter |
| Hội thoại, bản đồ lớn | `npctalk.cpp`, `overmap_ui.cpp` | Đã có adapter |
| Tương tác phương tiện | `veh_interact.cpp` | Đã có adapter |
| Màn tải/tạo thế giới | `loading_ui.cpp`, `game.cpp` | Đã bổ sung lời đọc trạng thái |
| Màn chơi thực tế | `handle_action.cpp`, `a11y_world.cpp` | Đã có snapshot bản đồ, bổ sung xác nhận khi vào |
| Màn lỗi C++/SDL | `debug.cpp`, `GameReaderController.java` | Có bảng lỗi trợ năng riêng, chưa kiểm thử trên máy thật |
| ImGui demo dành cho nhà phát triển | `imgui_demo.cpp` không có `screen_scope` | Chặn đường vào giao diện im lặng và cung cấp nút quay lại |

## Những giới hạn còn phải kiểm chứng

- Các màn hình của mod ngoài hoặc màn ImGui chưa được liệt kê có thể chưa cung cấp semantic snapshot.
- Adapter hiện diện trong mã **không bảo đảm** mọi trường dữ liệu/cử chỉ được đọc chính xác; cần thử từng quy trình.
- Khi native engine bị deadlock thật, thông báo watchdog chỉ báo **không có cập nhật**, không tự kết luận game đang tải bình thường hoặc sửa được deadlock.
- Các hộp thoại/cài đặt thuộc Android hệ thống thuộc quyền xử lý của TalkBack.
- Không tuyên bố sửa xong trải nghiệm trên máy thật trước khi APK mới được kiểm chứng.

## Thử nghiệm quan trọng trên điện thoại

- Đặt TTS tiếng Việt sẵn sàng, mở game mới lần đầu; xác nhận tên từng màn khởi động.
- Trò chơi Mới → Chơi ngay! (Kịch bản Mặc định). Phải nghe xác nhận bắt đầu tải, tiến độ có giới hạn, mẹo một lần, sau đó chỉ đọc những địa hình đáng chú ý hoặc khi người chơi kiểm tra chủ động.
- Khi đang tải, dùng cử chỉ đọc lại phải nghe giai đoạn gần nhất.
- Kiểm tra tương tự với tải nhân vật cũ, tạo nhân vật tùy chỉnh, bản đồ, NPC, túi đồ, chế tạo, tùy chọn và cài đặt TTS.
- Để yên >45 giây khi cố ý làm chậm tải; phải có cảnh báo chưa nhận được màn hình.
