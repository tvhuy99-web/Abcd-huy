# Danh mục bề mặt cần nghiệm thu — bản đầu

Trạng thái mọi hàng: **chưa có nghiệm thu trợ năng**. Đây là bản kiểm kê đầu,
không phải danh sách đầy đủ hoặc mẫu số để công bố “100%”. Tên màn/context
được xác định bằng đọc nguồn tại baseline và công cụ `inventory.py`.

| Nhóm | Nguồn chính để rà luồng | Chi tiết bắt buộc |
| --- | --- | --- |
| Khởi động Java | `SplashScreen.java`, `NativeUI.java` | Cài/nâng dữ liệu, lỗi, cảnh báo, quay lại |
| Menu chính | `main_menu.cpp/.h` | Context MAIN_MENU trong constructor initializer, không nằm trong kết quả literal hiện tại |
| Thế giới/mod | `worldfactory.cpp`, `mod_manager_ui.cpp` | WORLDGEN_CONFIRM_DIALOG, MODMANAGER_DIALOG, chọn thế giới, xóa, phụ thuộc |
| Tạo nhân vật | `newcharacter.cpp` | Tất cả tab, mẫu, nhập tên, xác nhận |
| Menu chung | `uilist.cpp` | Filter/category/retval và nhánh native trả về sớm |
| Popup/nhập | `popup.cpp`, `input_popup.cpp`, `string_input_popup.cpp` | Hỏi, chọn nhiều, số, Unicode, IME, hủy |
| Bản đồ cục bộ | `game_ui_tile_info.cpp`, `surroundings_menu.cpp` | Tri thức theo từng trường, âm, memory, tầng |
| Di chuyển/hành động | `action.cpp`, `handle_action.cpp`, `avatar_action.cpp` | DEFAULTMODE, LOOK, realtime, kết quả thật |
| Hành trang | `inventory_ui.cpp`, `item_contents.cpp`, `pickup.cpp` | Nhiều lựa chọn, pockets, container, số lượng |
| Kho nâng cao | `advanced_inv.cpp`, `armor_layers.cpp` | Hai cột nguồn/đích, sức chứa, thứ tự mặc |
| Y tế/trạng thái | `medical_ui.cpp`, `player_display.cpp`, `morale.cpp` | Chi tiết bộ phận/hiệu ứng, giá trị không chỉ qua màu |
| Thông báo | `messages.cpp`, `output.cpp` | Cooldown/coalescing, lịch sử, văn bản dài |
| Chiến đấu | `ranged.cpp`, hệ thống hành động/võ thuật | Nhắm/chọn ô/tầng/vùng, bắn, hủy, nạp |
| Chế tạo/xây | `crafting_gui.cpp`, `construction.cpp` | Công thức, thay thế, batch, vị trí, thiếu điều kiện |
| Bản đồ lớn | `overmap_ui.cpp`, `calendar_ui.cpp` | Travel, notes, tầng, lịch và tọa độ |
| NPC/thương mại | `dialogue_imgui.cpp`, `npctalk.cpp`, `trade_ui.cpp` | Lịch sử người nói, câu trả lời, giá trị giao dịch |
| Nhiệm vụ/trại | `mission_ui.cpp`, `mission_companion.cpp`, `faction_camp.cpp`, `faction_ui.cpp` | Mục tiêu, thời hạn, giao việc và kết quả |
| Phương tiện | `veh_interact.cpp`, `vehicle_use.cpp`, `vehicle_display.cpp` | Bộ phận/vị trí, sửa/lắp, lái, cargo |
| Điều khiển động cơ | `smart_controller_ui.cpp` | SMART_ENGINE_CONTROLLER, điều chỉnh và hủy |
| Vùng/tự động hóa | `zone_manager_ui.cpp`, `study_zone_ui.cpp` | Hai góc/tầng, kích thước, STUDY_ZONE_UI |
| Bionics/đột biến/phép | `bionics_ui.cpp`, `mutation_ui.cpp`, `magic.cpp` | Chủ động/bị động, chi phí, xác nhận; bổ sung `bionics_ui.cpp` so với kế hoạch đầu |
| Kỹ năng/kiến thức | `skill_ui.cpp`, `proficiency_ui.cpp` | Biểu đồ, điều kiện, PROFICIENCY_WINDOW theo mã thật |
| Máy tính trong game | `computer_session.cpp` | YESNOQUIT và các nhánh chuyên biệt, không tự tiết lộ dữ liệu |
| Nhật ký/điểm/kết thúc | `diary_ui.cpp`, `scores_ui.cpp`, luồng kết thúc game | Chỉnh/xuất nhật ký, deathcam, nghỉ hưu, trở về menu |
| Cấu hình | `options.cpp`, `safemode_ui.cpp`, `auto_pickup.cpp`, `panels.cpp`, `color.cpp` | Kiểu giá trị, vùng kiểm thử quy tắc, reset/hủy, tất cả tùy chọn Android |
| Minigame thiết bị | `iuse_software_kitten.cpp`, `iuse_software_lightson.cpp`, `iuse_software_minesweeper.cpp`, `iuse_software_snake.cpp`, `iuse_software_sokoban.cpp` | Bàn chơi, thời gian/input riêng, luật và đường thoát; không mặc định bộ đọc bản đồ thế giới phủ được |
| Trợ giúp/phím | `help.cpp`, `input_context.cpp`, `output.cpp` | Tìm mọi lệnh, thay phím, phần không dùng input_context |
| Lưu/tải/sao lưu | Luồng game/main menu, `CDDADocumentsProvider.java` | Nhiều nhân vật/thế giới, lỗi, file picker và chuyển quyền đọc |
| Công cụ phát triển | `debug_menu.cpp`, `editmap.cpp`, `wish.cpp` | Phạm vi riêng; nếu hiển thị vẫn phải có mô tả giới hạn/đường thoát |

Mỗi hàng sẽ được tách thành bản ghi: đường tìm → dữ liệu → tham số → kích hoạt
→ hủy → kết quả → ranh giới thông tin → test ID → build → bằng chứng máy thật.
Các tên context hoặc số đếm ở đây không thay thế kiểm kê luồng runtime.
