# R07 — Thế giới, mod và toàn bộ tạo nhân vật (giai đoạn 1, chỉ viết mã)

Tài liệu này ghi bằng chứng nguồn, quyết định giữ/bổ sung và các nhánh
chưa nối được của R07.1–R07.4 trong `REMAINING-CODE-PLAN.vi.md`. Trạng
thái: **đã viết, chờ kiểm thử** (xem `REMAINING-CODE-BACKLOG.json`):
chưa build, chưa chạy test, chưa tạo APK. Test ID sẽ chạy ở giai đoạn 2:
T01–T02, T17–T20, T28, T31, T35, T53, T57, T64.

Phương pháp: ba lượt rà soát độc lập chỉ đọc (worldfactory/mod,
newcharacter, chuỗi menu chính → vào game) đối chiếu với mã tại
`3c8e6f8c`, sau đó chỉ bổ sung chỗ chứng minh thiếu. Mọi adapter dùng mẫu
dùng chung R04 (uilist/popup/editor), hợp đồng R02 khi cần và intent qua
cửa R03; không adapter nào tự tạo bộ nhập riêng ở đợt này.

## 1. R07.1 — Thế giới và mod

### 1.1 Đã có và giữ nguyên (đã rà soát, chứng minh đủ)

- Chọn world: scope riêng `world-selection` (`worldfactory.cpp`) —
  node từng world kèm số save, activate chỉ đặt lựa chọn, `CONFIRM` mới
  vào world (tách duyệt khỏi xác nhận đúng quy ước R04.3).
- Tạo world cơ bản: scope `world-generation` (tên qua `prompt_world_name`
  / scope `STRING_INPUT`, các slider WORLDGEN kèm node tăng/giảm có lý do
  vô hiệu ở biên, các nút hoàn tất/đặt lại/ngẫu nhiên/nâng cao/mod).
- Chọn mod: scope `mod-selection` — tab nhóm, mod khả dụng/đang bật, thứ
  tự nạp trong `value`, chi tiết (tác giả, phụ thuộc, version, mô tả, lỗi
  phụ thuộc) ở node đang chọn, xung đột với danh sách đang nạp, đổi thứ
  tự bằng lệnh `MOVE_MOD_UP/DOWN` qua trung tâm lệnh.
- Xem mod của world hiện có: scope `active-world-mods`, chỉ đọc thật
  (handler chỉ trả intent focus nội bộ, không có đường sửa).
- Tùy chọn world nâng cao: scope `world-options` của R02/R04 qua điểm
  nối duy nhất `show_worldgen_tab_options`.
- Sao chép world (`Copy World Settings`): đi qua hai tab mod-selection +
  world-options rồi đặt tên mới; bản sao chỉ chép tên + `WORLD_OPTIONS` +
  thứ tự mod + cờ nén, **không chép save** (mã gốc `WORLD::COPY_WORLD`).

### 1.2 Bổ sung ở đợt này

1. **Xác nhận phá hủy nêu rõ hậu quả** (`main_menu.cpp`,
   `confirm_world_removal_for_reader`): khi reader bật, Delete World hỏi
   rõ tên world, số save sẽ mất, "không thể hoàn tác", mặc định No
   (kế thừa `query_yn`); Reset World hỏi tương tự và nói rõ giữ thiết
   lập + danh sách mod. Sau khi xóa/đặt lại xong có popup kết quả. Khi
   reader tắt, giữ nguyên câu hỏi gốc. Kèm theo: bật/tắt nén save nay
   thông báo kết quả (thành công/thất bại) khi reader bật — trước đây
   `set_compression_enabled` trả bool nhưng không ai đọc.
2. **Luồng sao chép kiểm tra tên** (`show_worldgen_advanced`): tên nhập
   vào nay qua `valid_worldname` như luồng cơ bản (trước đây đi thẳng
   tới `add_world`: tên trùng/không hợp lệ thất bại hoặc va chạm lặng
   lẽ). Không hợp lệ thì `valid_worldname` tự popup lý do và hỏi lại tên.
   Đây là thay đổi cho mọi người dùng, vì nó khôi phục cùng một kiểm tra
   an toàn mà luồng cơ bản đã có; không đổi luật chơi nào khác.
3. **Lỗi ghi tệp khi tạo world có tiếng nói** (`make_new_world`): sau
   `add_world` thất bại (chỉ còn khả năng lỗi ghi/thư mục, vì hủy bỏ trả
   về sớm hơn và xung đột mod đã popup riêng), reader nhận popup "không
   tạo được thế giới vì không ghi được tệp". Trước đây `WORLD::save` chỉ
   `debugmsg`.
4. **Chi tiết mod đầy đủ hơn** (`mod-selection` và `active-world-mods`):
   detail của mod đang chọn thêm **nhóm (category)** và **xung đột do
   chính mod khai báo** (`MOD_INFORMATION::conflicts`, trước đây không
   nguồn nào publish cho reader); màn xem mod của world hiện có thêm cả
   xung đột với danh sách đang nạp như màn chọn mod.
5. **Mod lỗi không còn bị từ chối lặng lẽ**: trong danh sách khả dụng,
   mod mà cây phụ thuộc báo không khả dụng (`mm_tree.is_available` false)
   nay publish `enabled=false` kèm `disabled_reason` là nguyên văn lỗi
   của cây phụ thuộc — thay cho `try_add` trả về im lặng và snapshot
   không đổi chút nào sau khi kích hoạt.

### 1.3 Nhánh không tồn tại trong UI thật (ghi rõ, không bịa)

- **Đổi tên world đã tồn tại: không có** trong toàn bộ nguồn đã rà
  (`worldfactory`, `main_menu`, `mod_manager*` không có hàm/mục nào).
  Đường duy nhất là sao chép sang tên mới rồi xóa bản cũ — hai thao tác
  nay đều có xác nhận/kết quả nghe được. Không tự chế tính năng đổi tên
  vì nó đụng định dạng thư mục save của engine.
- **Tùy chọn riêng của từng mod (mod options): không tồn tại** trong
  luồng tạo world; `MOD_INFORMATION` không có trường options. Tùy chọn
  duy nhất là `WORLD_OPTIONS` (đã có adapter).
- **Cài/thay dữ liệu mod**: không có luồng UI trong game (dữ liệu mod
  nạp từ tệp ngoài). Hành trình tương ứng trong sổ giữ trạng thái chưa
  viết với lý do này; lỗi *dữ liệu* mod khi chọn đã xử lý ở mục 1.2.5.
- **Danh sách từng save thuộc world**: chỉ có ở màn tải nhân vật
  (`load_character_tab`, uilist dùng chung, entry có tên + thời lượng
  chơi). Từ menu quản lý world không có đường mở danh sách này trong mã
  gốc, và không có thao tác xóa save đơn lẻ (xóa save thuộc R20).

## 2. R07.2 — Tạo nhân vật, đối chiếu từng tab

UI thật có đúng 7 tab (`character_creator_ui.h`): SCENARIO, PROFESSION,
BACKGROUND, STATS, TRAITS, SKILLS (đều là uilist, publish qua adapter
uilist dùng chung) và SUMMARY (scope `CHARACTER_CREATION` +
`append_character_reader_summary`). **Không tab nào cho chọn bionic hay
phép**: bionic/phép là thuộc tính kèm theo của nghề/nền tảng, áp dụng ở
`Character::initialize`. Mọi bổ sung dưới đây nằm trong
`update_uilist_entries` (metadata `a11y_value`/`a11y_disabled_reason`/
`desc` của entry) và hai điểm của màn tổng kết; không đổi chữ ký nào.

| Tab | Đã có (giữ) | Bổ sung ở đợt này |
| --- | --- | --- |
| SCENARIO | mô tả; khóa kèm lý do từ `can_pick()` | **chi phí điểm** vào `a11y_value` (trước đây chỉ "Current choice"); khóa **CITY_START khi thành phố bị tắt** thành disabled kèm lý do — `can_pick()` không kiểm tra điều này và confirm từng bỏ lựa chọn trong im lặng |
| PROFESSION | chi phí điểm; khóa kèm lý do `can_pick()`; mô tả | nối **bionic và phép** của nghề vào `desc` (pane chi tiết có, reader ở tab này chỉ nhận `desc`) |
| BACKGROUND | checkbox đã chọn; chi phí điểm; mô tả | nối **bionic và phép** của nền tảng vào `desc` như nghề. Xung đột trait của nền tảng giữ cơ chế confirm-time `hobby_conflict_check` (tự popup lý do) — không pre-publish vì hàm kiểm tra đó là hàm confirm có thông điệp riêng |
| STATS | giá trị hiện tại | `a11y_value` thành **giá trị + mô tả mức** (cùng hàm `stat_level_description` của header) + **miền MIN–MAX**; `desc` = **điểm thuộc tính còn lại** theo `multi_pool` |
| TRAITS | điểm trait; checkbox; xung đột/scenario cấm/nghề cấm đều disabled kèm lý do; bionic chặn ghi ở `desc` | bionic chặn trait nay cũng **disabled kèm lý do** như ba trường hợp cấm còn lại (khi trait chưa được lấy); trait **bị khóa** (scenario/nghề/nền tảng) có ghi chú "không thể gỡ bỏ" trong `desc` |
| SKILLS | mô tả; nhãn chứa cấp trong `txt` | tab này **không có `a11y_value` nào** — cấp độ chỉ nằm trong `txt`, bị làm rỗng khi `SCREEN_READER_MODE` bật. Nay `a11y_value` = cấp tự mua, tách phần cộng thêm từ nghề/nền tảng theo đúng công thức của `get_skill_entry_text`; `desc` thêm **điểm kỹ năng còn lại** |
| SUMMARY | 13 document: tổng quan, thuộc tính, điểm còn lại 3 pool, traits, proficiencies, bionics, phép, nhiệm vụ, kỹ năng, điểm bắt đầu, mô tả scenario/nghề, rating; value cho các lệnh đổi tên/giới tính/outfit/chiều cao/tuổi/máu/ngày/thành phố/vị trí | proficiencies thêm phần **đã biết** (`known_profs()`) mà pane UI có; nhãn nguồn xe: **"(from scenario)"/"(from profession)"** như `draw_starting_vehicle` |

Điểm bắt đầu, làm rõ theo mã thật: thành phố chỉ chọn được khi option
`SELECT_STARTING_CITY` bật (hành động không đăng ký ngoài điều kiện đó);
vị trí mà scenario không cho phép thì **vắng mặt** khỏi danh sách chọn
(loại trừ, không phải disabled kèm lý do — giữ nguyên cơ chế gốc); xe
bắt đầu không chọn được, suy ra từ scenario/nghề.

## 3. R07.3 — Tên và mẫu nhân vật

- Tên nhân vật: nút Name ở thanh trên mọi tab mở `string_input_popup_imgui`
  (scope `STRING_INPUT` riêng, intent `set_text` qua cửa R03, trần
  `NAME_CHARACTER_LIMIT`). Tên ngẫu nhiên (`RANDOMIZE_CHAR_NAME`) không
  khóa sửa lại: mở editor và ghi đè bình thường. **Không có rào cản ký
  tự tiếng Việt** trong đường nhập tên (hàm lọc ký tự chỉ áp cho tên
  world và tên tệp); giới hạn thật duy nhất là trần 50 **byte**, ký tự
  có dấu nhiều byte chạm trần sớm hơn 50 ký tự — đây là giới hạn engine,
  ghi lại để bài kiểm thử T17/T64 lưu ý, không đổi ở R07.
- Lưu mẫu trong creator: lệnh `SAVE_TEMPLATE` mọi tab →
  `query_for_template_name` (cùng scope `STRING_INPUT`, có lọc ký tự tên
  tệp) → `avatar::save_template`. Mẫu "Last Character" tự lưu sau khi
  tạo xong không hủy (mã gốc).
- Tải/xóa mẫu ở menu chính (không nằm trong creator): uilist chọn mẫu
  (adapter dùng chung) → `query_popup` LOAD/DELETE/CANCEL (scope Dialog)
  → xóa có `query_yn` xác nhận và popup khi thất bại. Bổ sung ở đợt này:
  **popup kết quả khi xóa thành công** (trước đây im lặng) và **popup
  lỗi tải mẫu** trong `avatar::create` khi reader bật — `load_template`
  chỉ `debugmsg` khi tệp hỏng/mất rồi `create` trả false về menu không
  một lời giải thích.
- Hủy tạo: `QUIT`/`PREV_TAB` ở tab đầu đều có `query_yn` "Return to main
  menu?" qua scope Dialog. Không có bản nháp giao dịch riêng: creator
  thao tác thẳng lên avatar toàn cục và caller (`new_character_tab`) có
  `on_out_of_scope cleanup` reset avatar + world active khi hủy, nên
  hủy không để lại nhân vật nửa vời. Điểm âm **không bị chặn** ở bước
  hoàn tất (mã gốc không gọi `can_afford` ở đâu cả) — reader thấy số âm
  ở document điểm tại SUMMARY; không tự thêm chặn vì đó là luật chơi.

## 4. R07.4 — Chuỗi vào game

Đã xác minh toàn chuỗi bằng ngăn xếp lời gọi thật, không điểm đứt ở điểm
nối tạo nhân vật → thế giới:

`opening_screen` (scope `MAIN_MENU`) → `new_character_tab` → `pick_world`
(scope `world-selection`; 0–1 world thì tự chọn theo mã gốc) →
`g->setup()` → `pc.create()` (creator) → xác nhận hoàn tất là
`query_yn` bên trong creator → `g->start_game()` → lượt đầu
`do_turn` → `handle_action` tạo scope world `DEFAULTMODE` với snapshot
`a11y::world_snapshot`, focus `world:here`. Chọn vị trí/city nằm trong
creator (không có màn chọn điểm spawn trên overmap sau tạo). Lỗi tìm vị
trí bắt đầu đi qua popup/`query_yn` "Try again?" — các scope dùng chung,
không đứt. Đi/mở cửa/nhặt/ăn/uống dùng adapter thế giới/hành trang đã
có; phần chuyên sâu là R08/R09. Lưu/thoát/tải lại dùng
`ACTION_SAVE/QUICKSAVE/QUICKLOAD` + các scope menu/world-selection đã có
(phần xóa save và lỗi save thuộc hành trình R20).

Realtime: rào `confirm_realtime_progression_for_accessibility` có đúng
3 callsite — tutorial, `new_character_tab`, `load_game` — phủ mọi đường
vào game qua menu, kể cả quickload và CLI `--world` (đều qua
`load_game`). Hàm API `game::load(const std::string&)` không qua rào
nhưng không có caller trong luồng menu đã rà. Không callsite nào tự đổi
option: chọn No mới lưu `TURN_DURATION = 0` theo ý người chơi, đúng mã
gốc của rào.

## 5. Chỗ chưa chắc / giới hạn đã biết

1. **Chưa biên dịch/chưa chạy** (đúng giai đoạn 1): các chữ ký đã đối
   chiếu bằng đọc mã (`scenario::point_cost()`, `cities_enabled()` tĩnh
   trong cùng tệp, `char_creation::stat_level_description`,
   `multi_pool` cùng tệp, `mm_tree` là thành viên công khai của `mod_ui`
   và đã thêm `#include "dependency_tree.h"` vào `worldfactory.cpp` vì
   gọi hàm của cây phụ thuộc cần kiểu đầy đủ, `WORLD::world_saves`,
   `set_compression_enabled` trả bool), nhưng tính biên dịch được chỉ
   xác nhận ở giai đoạn 2 (T01–T02, T17–T20, T57, T64).
2. Giá trị của các lệnh thanh trên (CHANGE_NAME, CHOOSE_CITY...) chỉ
   gắn ở tab SUMMARY vì `append_character_reader_summary` chỉ chạy ở
   đó; `command_snapshot` dùng chung không có kênh value. Người dùng ở
   tab khác vẫn gọi được lệnh và nghe kết quả ở SUMMARY/tài liệu tổng
   kết. Muốn có value ở mọi tab phải đổi builder lệnh — quyết định
   riêng, không làm ở R07.
3. Màn chờ "Please wait…" trong `g->setup()`/`g->start_game()` là
   `static_popup` không publish snapshot: trong lúc tải, reader không có
   gì mới để đọc cho tới scope world đầu tiên. Đây là khoảng chờ tạm
   thời của vòng đời khởi động; xử lý nếu cần thuộc R20 (vòng đời),
   không bịa scope mới ở R07.
4. `character:points` có thể hiện số âm vì engine cho phép hoàn tất khi
   âm điểm (mục 3). Tài liệu này ghi nhận hành vi gốc, không coi là lỗi
   adapter.
5. Mod bị disabled vì lỗi cây phụ thuộc: nếu ở giai đoạn 2 thấy một mod
   khả dụng thật bị cây báo lỗi oan (ví dụ phụ thuộc là mod core đặc
   biệt), chỗ sửa đúng là dữ kiện cây phụ thuộc/`try_add`, không phải
   bỏ lý do vô hiệu — bài T63 cần một mod lỗi mẫu để khóa hành vi.
6. Chuỗi mới đã thêm bản dịch Việt vào `lang/po/accessibility/vi.po`
   (22 mục). Chuỗi thuộc UI gốc có sẵn (ví dụ "Point cost: %d") dùng lại
   mục dịch đã có, không thêm trùng.
