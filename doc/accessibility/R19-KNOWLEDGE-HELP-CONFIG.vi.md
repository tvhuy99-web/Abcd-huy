# R19 — Kiến thức, trợ giúp và mọi cấu hình còn lại

Phạm vi đợt này: kiến thức nhân vật (kỹ năng/proficiency/traits), trợ giúp,
gán phím, và toàn bộ cấu hình còn lại (options/panels/color). Bảy tệp nguồn
đã sửa: `src/player_display.cpp`, `src/proficiency_ui.cpp`, `src/help.cpp`,
`src/input_context.cpp`, `src/options.cpp`, `src/panels.cpp`, `src/color.cpp`.
Không thêm tệp nguồn mới. Giai đoạn 1 chỉ viết mã — chưa build/test.

## R19.1 — Kiến thức nhân vật

**Xác minh bề mặt trước khi viết:** `src/skill_ui.cpp` không có màn hình —
chỉ là helper `get_HeaderSkills()`/`skip_skill_headers()` phục vụ tab SKILLS
của `Character::disp_info`. Không gắn adapter vào đó (tự chế màn là bịa bề
mặt); adapter kỹ năng nằm ở `player_display`.

- **`src/proficiency_ui.cpp` — adapter mới** (trước đây 0 điểm a11y) trong
  `prof_window::run()`: bảng R02 `proficiency:list` trên đúng tập đang hiển
  thị (`get_current_set()` — chỉ proficiency đã biết + đang học mà
  `display_proficiencies()` công bố): cột name/status/progress (đơn vị %,
  compare số)/time/category. Trạng thái vốn chỉ là màu hàng → ô status
  "Learned"/"In progress" bằng đúng từ vựng của chính màn. Tài liệu
  `proficiency:details` (`append_document`) cho mục đang chọn: Time to learn,
  Progress, Category, Pre-requisites, Leads to, Description — sao đúng
  trường/thứ tự của `draw_details()`. Node tab category (vô hiệu khi đang
  lọc, như gốc), filter qua đúng action `FILTER` của màn (popup gốc, gồm
  tiền tố `l:`/`i:`/`p:`) + node xóa lọc; tập rỗng có node `proficiency:empty`.
  Entry được tra lại theo `proficiency_id` trong tập hiện hành lúc intent;
  cả focus lẫn activate chỉ đồng bộ lựa chọn (màn gốc không có CONFIRM) —
  không học, không tốn lượt.
- **`src/player_display.cpp` — mở rộng adapter R10** (giữ nguyên phần R10):
  bảng `pd:skills` (cấp sau ghi đè CQB/enchantment đúng thứ tự
  `draw_skills_tab()`, tiến độ `knowledgeExperience()`, dodge, status bằng
  lời từ đúng vị từ tab kiểm: Locked/Cannot train/Practice below knowledge;
  detail sao nguyên công thức `draw_skills_info()` gồm nhánh rusty);
  node `pd:trait:<id>` cho tab TRAITS với mô tả từ
  `Character::mutation_desc()` — đúng nguồn `draw_traits_info()` của chính
  tab dùng, cộng ghi chú purifier nguyên văn (điểm mở R17 để lại đã đóng ở
  đây; màu `get_display_color()` của tab không có chú thích chữ và là khóa
  sắp xếp nên không dịch thành phân loại, theo tiền lệ R17); bảng
  `pd:proficiencies` tóm tắt (hồ sơ đầy đủ ở cửa sổ proficiency_ui).
- Phân biệt: danh sách kỹ năng = chỉ số năng lực (cấp + % luyện tập + rust);
  hồ sơ proficiency = điều kiện tiên quyết của hành động/công thức (biết/đang
  học, %, thời gian, pre-requisites/leads-to). Công thức đã học: bề mặt là
  `crafting_gui` đã phủ ở R12 — chỉ đối chiếu, không sửa.

## R19.2 — Trợ giúp (`src/help.cpp`)

**Kết luận rà `screen_reader` cũ (bằng chứng):** biến `screen_reader` đọc từ
option `SCREEN_READER_MODE`, mà mô tả option ghi rõ dành cho bộ đọc màn hình
NGOÀI quét chữ curses. Trong help nó chỉ đổi cách vẽ ở 4 điểm (tiền tố
"(read)" cho chủ đề đã xem, bỏ khung/trang trí, footer thành câu chữ). Nó
không công bố dữ kiện, không có mô hình điều hướng, không đưa nội dung thành
tài liệu — **không tương đương bộ đọc nội bộ, không tính là đã phủ**. Xử lý:
giữ nguyên toàn bộ, không tắt, adapter không gate theo option này; phần trùng
lặp được thay bằng dạng cấu trúc (trạng thái đã xem → ô Status; tên chủ đề
kề → summary).

Cấu trúc thật: `help_data` là bản đồ phẳng các chủ đề theo `order` từ JSON
type "help"; không có cấp nhóm trên chủ đề, không tìm kiếm, không liên kết
trong chủ đề trong UI gốc. Adapter trong `help_window::show()`:

- Chế độ danh sách: bảng R02 `help:topics` (Topic/Status Read-Unread/Hotkey
  — hotkey đúng `short_description()` mã vẽ in), trường lọc `help:filter` là
  **lọc chỉ ở tầng trình bày của bộ đọc** (detail ghi rõ; UI gốc không có tìm
  kiếm), node `help:no_results` khi rỗng, summary "x of y help topics shown."
- Focus mọi node chỉ trả TIMEOUT quan sát thuần (cố ý không set
  `selected_option`); activate hàng mới set + trả "CONFIRM" để chính vòng
  chọn gốc mở chủ đề (kèm `mark_read`/`swap_translated_paragraphs` của nó).
- Chế độ chủ đề: `append_document` (`help:document`) ghép đúng các
  `translated_paragraphs` đã `parse_tags` mà `draw_category()` vẽ (bỏ đoạn
  `MM_SEPARATOR` vì chuỗi lưu của nó là tên màu của đường kẻ, không phải
  chữ) → ReaderDocumentReader (R04) đọc theo đoạn/câu/tìm/đọc tiếp được.
  Ba node `help:nav:prev/back/next` trả đúng "LEFT"/"QUIT"/"RIGHT" của vòng
  chủ đề gốc; quay lại dùng trạng thái của chính `help_window`, focus trở về
  hàng chủ đề vừa mở. Không tốn lượt, không đổi trạng thái game.

## R19.3 — Gán phím (`src/input_context.cpp`)

Đính chính tiền đề: màn gán phím thật nằm ở `input_context.cpp` (lớp
`keybindings_ui`, vòng `input_context::display_menu()`), không phải
`input.cpp` (chỉ là kho `input_manager`). Rà soát trước đó cho thấy fallback
lệnh chung đã công bố các lệnh menu nhưng **hàng hành động không được công
bố** — bộ đọc không chọn được hành động để gán/xóa/reset. Đã viết adapter
cục bộ trong `display_menu()` (screen_scope owner `&ctxt`, HELP_KEYBINDINGS):

- Bảng R02 `keybindings:actions` từ `kb_menu.filtered_registered_actions`:
  tên (`get_action_name()`), phím hiện tại (`get_desc()`), phạm vi ràng buộc
  (Unbound / Local to this screen / Global — suy từ cùng dữ kiện
  `draw_controls()` dùng), cờ Customized. Node `keybindings:mode` nói trạng
  thái thao tác đang chờ (show/remove/reset/add/add_global/execute/filter) +
  context đang sửa. Trường lọc là lọc trình bày của bộ đọc (màn hình không có
  setter cho ô lọc của nó — `cataimgui::window` chỉ có
  `get_filter()`/`clear_filter()`; giá trị lọc của màn hình được công bố và
  xóa được qua `clear_filter()`).
- Focus hàng = TIMEOUT quan sát thuần. Activate hàng khi mode ≠ show: đặt
  `kb_menu.highlight_row_index` rồi trả "SELECT" để nhánh dispatch có sẵn tự
  chạy `action_remove`/`action_reset`/`action_add` — không viết lại logic gán
  phím; lưu/hủy giữ ngữ nghĩa gốc (bản sao `old_action_contexts`, "Save
  changes?" khi thoát).
- **Công bố xung đột kèm context trước khi áp:** trong nhánh a11y của
  `action_add`, mỗi lựa chọn ký tự trong uilist chọn phím kèm mô tả xung đột
  tính bằng `get_conflicts()` **bổ sung bên đang giữ phím** (kiểm tra
  `inp_mngr.get_input_for_action(other, "default")` đối chiếu thuộc tính
  local của context hiện tại): "the local bindings for this screen (context
  %s)" / "the global default bindings" / cả hai. `resolve_conflicts()` của
  engine giữ nguyên làm chốt chặn cuối.
- **Cảnh báo phím thiết yếu là chính sách phía bộ đọc** (engine không có
  khái niệm này — đã grep xác nhận; ghi rõ ngay trong văn bản cảnh báo
  "reader policy, not an engine rule"): tập thiết yếu dựng từ đăng ký thật
  lúc mở menu — {QUIT, SELECT, TEXT.CONFIRM, UILIST.UP, UILIST.DOWN} của
  HELP_KEYBINDINGS ∪ {CONFIRM, QUIT} của context cha đang sửa. Remove/reset
  một hàng thiết yếu phải activate hai lần (lần đầu chỉ công bố cảnh báo +
  node `keybindings:warning`), tránh tự khóa phím điều khiển menu mà không
  cảnh báo.
- **Khôi phục mặc định:** engine không có reset toàn cục/toàn-context (chỉ
  `action_reset` từng hành động). Node `keybindings:reset_all` là thao tác
  tổ hợp phía bộ đọc (ghi rõ trên detail): hai lần activate, vòng lặp chạy
  `action_reset()` của engine cho từng hành động đã đăng ký; prompt xung đột
  của engine vẫn xuất hiện theo từng hành động và "Save changes?" cuối menu
  bao phủ cả loạt.

## R19.4 — Cấu hình còn lại

### Bảng đối chiếu options/panels/color

| Bề mặt | Trạng thái | Ở đâu |
| --- | --- | --- |
| Options General (38) | Phủ ở R02; rà lại R19.4 đạt | `options.cpp` show() |
| Options Interface (66) | Phủ ở R02; `EDGE_SCROLL` (int_map) sửa ở R19.4 | nt |
| Options Graphics (65) | Phủ ở R02; option ẩn theo build khớp UI gốc qua `is_hidden()` | nt |
| Options World Defaults (3 hiện + 9 ẩn có chủ đích) | R02/R07; 9 option ẩn sửa tại slider tạo thế giới (R07), không thuộc màn này | nt |
| Options Debug (8) | Phủ ở R02; `PREVENT_OCCLUSION` (int_map) sửa ở R19.4 | nt |
| Options Android (54) | Phủ ở R02; 8 option `*_KEY` giới hạn 1 ký tự công bố + thực thi ở R19.4 | nt |
| Điều kiện tiên quyết option | R02: enabled/disabled_reason + dependsOn cùng trang | nt |
| "Requires restart" | Chỉ là chữ trong tooltip (21 chỗ), engine không có cờ máy — đã công bố qua detail từ R02; kiểm chứng R19.4 | nt |
| Reset mặc định option | Không tồn tại trong UI gốc; mặc định đã công bố trong detail, đặt lại qua draft/apply — không bịa nút | — |
| Panels (PANEL_MGMT) | Mới ở R19.4 (trước: 0 a11y) | `panels.cpp` show_adm() |
| Color (COLORS) | Mới ở R19.4 (trước: 0 a11y); UI thật, không phải debug | `color.cpp` show_gui() |

### Khoảng trống options đã đóng (`src/options.cpp`)

1. **Option dạng `int_map` bị công bố sai miền (lỗ hổng thật):** field khai
   "int" không miền, choices rỗng → draft nhận mọi số nguyên, rồi
   `cOpt::setValue` âm thầm đặt về mặc định khi ngoài tập (người dùng nghe
   "đã áp" nhưng giá trị thành mặc định). Hai option thật bị ảnh hưởng:
   `PREVENT_OCCLUSION` (tập {0,1,2}) và `EDGE_SCROLL` (tập {−1,100,30,10}).
   Nay field là "choice" với choices lấy trực tiếp từ `option.mIntValues` →
   validate R02 chặn ngoài tập trước khi áp.
2. **`string_input` không công bố/không thực thi giới hạn độ dài**
   (`DEF_CHAR_NAME` 30, 8 option `ANDROID_*_KEY` 1 ký tự, `ETERNAL_TIME_OF_DAY`
   8 ẩn): nay detail thêm "Maximum length: N characters.", draft quá dài bị
   gắn `field_error`, apply từ chối draft quá dài thay vì để `setValue` cắt
   âm thầm theo byte.
   Đã kiểm chứng không phải khoảng trống: option ẩn theo điều kiện biên dịch
   (adapter dùng đúng `is_hidden()`), option của mod/world qua `add_external`
   (không xuất hiện ở cả UI gốc), save/hủy cuối `show()` giữ nguyên.

### Panels (`src/panels.cpp`, `panel_manager::show_adm()`)

Adapter mới: node cho đúng các panel renderable như UI gốc vẽ — role
checkbox, On/Off bằng lời (thông tin gốc chỉ truyền bằng màu chữ), detail
gồm thứ tự "Panel X of Y", width, height và mô tả widget gốc; node layout
(layout hiện tại có value "Current layout"). Intent: activate panel → trả
`TOGGLE_PANEL` cho vòng lặp gốc; activate layout → trả `MOVE_PANEL` đúng tổ
hợp gốc; move_up/move_down thực hiện đúng chuỗi swap của luồng gốc (kể cả
panel không renderable nằm giữa) và bị từ chối khi đang swapping bằng phím
cứng. UI gốc không có sửa width/height — adapter chỉ công bố bằng lời, không
bịa thao tác. Lưu qua đúng `QUIT → save()` gốc.

### Color (`src/color.cpp`, `color_manager::show_gui()`)

Kết luận bằng chứng: **có UI sửa màu thật trong cấu hình phát hành này** —
`show_gui()` (context "COLORS") vào được từ `ACTION_COLOR`
(`handle_action.cpp`) và Main Menu → Settings → Colors; không thuộc
debug/editor. Adapter đọc từ `name_color_map` — bản sao làm việc chính màn
hình vẽ/sửa: bảng R02 `colors:entries` (name/normal/invert), giá trị bằng
lời ("Normal: <tên màu thay thế hoặc default>. Invert: …" — tương đương lời
cho việc UI gốc in tên màu bằng chính màu đó), node con Normal/Invert (Remove
chỉ khi thật có custom). Intent trả `CONFIRM`/`REMOVE_CUSTOM`/
`LOAD_BASE_COLORS`/`LOAD_TEMPLATE` về vòng lặp gốc; chọn màu qua uilist
picker gốc (đã có adapter). Không ép vào form choice: miền thay thế ~154
màu có tên vượt giới hạn 128 choices của hợp đồng R02 — ghi rõ lý do tại mã.
Ghi chú "may require a restart" của màn nằm trong summary.

## Hồ sơ

Backlog R19.1–R19.4 → `da-viet-cho-kiem-thu` (test T13–T18, T38, T59, T62 +
TX-rieng-tung-trang, định danh bài riêng ở R22). Sổ hành trình: `ky-nang-va-proficiency`,
`panels-va-mau-sac`, `tro-giup-va-tim-lenh` chuyển chưa viết → đã viết;
`gan-phim-keybinding` cập nhật bằng chứng. Đếm lại từ dữ kiện sau R19:
**102 đã viết / 13 chưa viết** trên 115 hành trình (đếm lại sau R16 là 90/25;
R17 lật 4 hành trình và R18 lật 5 → 99/16 ở đầu đợt R19; R19 lật thêm 3 →
102/13; đối chiếu toàn bộ lần cuối thuộc R23).

## Chỗ chưa chắc

1. Chưa biên dịch (giai đoạn 1) — chữ ký/API đối chiếu tại nguồn: skill.h /
   proficiency.h / `Character::{mutation_desc, purifiable,
   display_proficiencies}`, `option.mIntValues` qua `friend class
   options_manager` (options.h:82), `get_input_for_action`/
   `get_action_attributes`, tiền lệ screen_scope trong help/bionics/diary.
   Xác nhận biên dịch thuộc giai đoạn 2.
2. Nhánh rusty của `draw_skills_info()` có mẫu số `knowledgeExperience()` có
   thể bằng 0 trong trường hợp hẹp — adapter chép nguyên biểu thức gốc
   (hành vi trùng màn gốc) thay vì tự sửa; để giai đoạn 2 kiểm tra biên.
3. Bộ lọc gán phím là lọc trình bày của bộ đọc vì `cataimgui::window` không
   có setter cho ô lọc của màn hình; nếu muốn ghi thẳng vào ô lọc thật cần
   nới phạm vi sang cata_imgui — ghi lại cho R23 quyết định.
4. Thước độ dài string_input dùng `utf8_width()` (khớp popup gốc với ký tự
   thường; ký tự rộng/thẻ màu có thể lệch) — builder và handler dùng cùng một
   thước nên nhất quán nội bộ.
5. Số node snapshot color (~154 màu × tối đa 5 node + lệnh) ước tính dưới
   trần 4096 của validate R02; số thật chốt ở giai đoạn 2.
