# Phân biệt tạo mới và tải bản lưu, khảo sát tốc độ tải Android

Phân tích theo mã nguồn nhánh `integration/reader-vietnamese-20261107`, không
phải phép đo hiệu năng trên điện thoại người chơi.

## Vì sao cả hai đều hiện "Đang khởi tạo"

- `main_menu::new_character_tab` với `sel2=3` hoặc `4` gọi
  `game::setup()`, `avatar::create()` rồi **`game::start_game()`**.
- `main_menu::load_game` nhận save có thật, gọi
  `game::setup(true)` rồi **`game::load(savegame)`**.
- Cả hai chạy `load_core_data()` và `load_world_modfiles()`.
  Hai việc này đọc lại công thức, vật phẩm, mod, v.v. Vì thế
  màn hiện cùng giai đoạn **không chứng minh** save bị làm lại.
- Chỉ nhánh tạo mới dùng `start_game`. Nhánh tải cũ giải mã file
  `<save>/<tên-thế-giới>/<mã-tên-nhân-vật>.sav` và nạp map memory,
  journal, NPC, món đồ, thời gian đã chơi.

## Cải tiến trợ năng và chống nhầm

1. Menu "Chơi ngay" nói rõ **tạo nhân vật mới**.
2. Nếu đã có bất kỳ nhân vật lưu nào, chọn "Chơi ngay" yêu cầu
   xác nhận **tạo mới**, mặc định có thể từ chối và trở về menu.
3. Khi tải nhân vật cũ, game phát lời riêng "Đang tải nhân vật đã lưu";
   khi thực sự hoàn tất `g->load`, thông báo ngắn "Đã tải bản lưu: TÊN".
   Không được nói thành công trước khi `g->load` trả về true.
4. Tự đo thời gian thực `core_ms`, `mods_ms`,
   `character_and_world_ms`, các phần của `game::load` và
   `save_total`, ghi dưới tiền tố `A11Y_LOAD_TIMING` vào debug log.
   **Không gán một thời gian chờ ước lượng** và không hứa rằng
   chỉ sửa lời đọc sẽ làm game chạy nhanh hơn.

## Lưu giữ dữ liệu

- Trên Android, mã dùng thư mục ứng dụng ngoài
  `Android/data/com.cleverraven.cataclysmdda.accessibility/files/`.
- Nhân vật nằm ở `save/`, còn `data/`, `gfx/` và `lang/` được
  cập nhật tách biệt trong quá trình cài game.
- Không xóa dữ liệu ứng dụng, không xóa thế giới và không tạo bản sao
  save cùng tên để thử tốc độ tải.
- Nếu nhân vật cũ **không có tên trong Tải trò chơi**, cần xem log,
  danh sách file và tên gói ứng dụng trước khi kết luận dữ liệu mất.
- Nếu nhân vật cũ có tên mà 10 phút sau vẫn ở màn tải, xem log
  `A11Y_LOAD_TIMING` để biết chậm ở core, mod hay đọc save, rồi
  tối ưu đúng phần đó. Không tự động xóa cache/save vì có thể mất dữ liệu.

## Xác minh Android

1. Tạo nhân vật mới, đi vài ô, lưu bằng chức năng lưu game gốc.
2. Thoát đúng cách, mở lại ứng dụng, vào **Tải trò chơi**, chọn
   tên thế giới, chọn tên nhân vật, xác nhận **Tải**.
3. Kiểm tra vị trí, đồ đang mang, thời gian chơi, và trạng thái.
   **Chỉ nhìn cùng màn "Đang khởi tạo" chưa chứng tỏ tải sai.**
4. Trở về menu mới, bấm Chơi ngay khi đã có save: phải nghe
   cảnh báo tạo nhân vật mới trước khi bắt đầu.
5. Trích nhật ký các dòng `A11Y_LOAD_TIMING` và đối chiếu tổng thời
   gian giữa lần tạo mới và lần tải lại, kiểm tra trên thiết bị thật.
