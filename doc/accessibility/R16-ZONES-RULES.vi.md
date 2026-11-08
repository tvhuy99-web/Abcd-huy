# R16 — Vùng, tự động hóa và quy tắc

Giai đoạn 1 (chỉ viết mã, không build/test). Trước đợt này cả năm bề
mặt đều có 0 điểm `a11y::`: `zone_manager_ui.cpp`, `study_zone_ui.cpp`,
`safemode_ui.cpp`, `auto_pickup.cpp` và màn chọn vị trí
`game::look_around`. Đợt này viết adapter riêng cho cả năm, bám đúng
mẫu đã có (screen_scope gắn vào input_context của chính màn, snapshot
dựng lại mỗi vòng lặp, intent trả action string về vòng lặp gốc —
tiền lệ `smart_controller_ui` của R15 và `veh_interact::reader_session`
của R09/R15). Không thao tác nào của bộ đọc chạy ngoài vòng lặp gốc.

## 0. Kho lưu thật (kiểm chứng bằng mã) — không tạo kho song song

- **Vùng**: `zone_manager::zones` (singleton `zone_manager::get_manager()`).
  Màn quản lý sửa qua các `ref_zone_data` lấy từ chính vector đó; khi
  thoát, `zone_manager_ui::display_zone_manager` lưu tạm
  `save_zones("zmgr-temp")` lúc mở màn và chỉ giữ thay đổi khi người
  chơi trả lời Yes ở `query_yn` "Save changes?" (No → `load_zones`
  khôi phục). Adapter không giữ bản sao vùng nào; bindings chỉ giữ
  (tên, loại, start, end) để *tra lại* vùng sống tại lúc intent.
- **Study Zone**: `study_zone_options::npc_skill_preferences` trong
  `src/clzones.cpp`, truyền theo tham chiếu vào `study_zone_window`.
  Cửa sổ ImGui sửa trực tiếp trên map đó; adapter ghi qua đúng cùng
  map, cùng logic insert/erase của `draw_skill_row`.
- **Safe Mode**: `safemode::global_rules` / `character_rules` của
  singleton `get_safemode()`. `show()` sửa tại chỗ trên hai vector
  này; thoát có thay đổi thì "Save changes?" quyết định `save_global` /
  `save_character` hay khôi phục bản sao cũ giữ từ đầu hàm. Adapter
  apply một trường = một lần sửa tại chỗ như phím gốc, kèm
  `changes_made = true`.
- **Auto-pickup**: `auto_pickup::user_interface` làm việc trên
  `tab::new_rules` — bản làm việc mà chính màn gốc chỉ chốt vào
  `tab::rules` (tham chiếu tới `player_settings::global_rules` /
  `character_rules` hoặc `npc_settings::rules`) khi thoát + đồng ý
  "Save changes?", sau đó `player_settings::show` mới `save_global` /
  `save_character`. Adapter ghi đúng vào `new_rules`, không đường tắt.

"Đặt lại" (reset) của cả hai màn quy tắc trong UI gốc là đường thoát
không lưu (No ở "Save changes?" khôi phục toàn bộ); không có nút reset
riêng trong UI gốc nên bộ đọc không bịa thêm.

## 1. R16.1 — Tạo vùng (`zone-manager` + `look-around`)

### 1.1 Adapter quản lý vùng — `zone_manager_ui::display_zone_manager()` (`src/zone_manager_ui.cpp`)

- Scope `a11y::screen_scope` category `zone-manager` gắn `&ctxt`
  (context `ZONES_MANAGER`), tạo sau khi đăng ký action; builder chạy
  trong vòng lặp chính ngay trước `ctxt.handle_input()`.
- Bảng khai báo một lần `zones:list` theo hợp đồng R02.2, 8 cột:
  name / type / status / personal / vehicle / direction / distance
  (unit `tiles`) / displayed. Mỗi vùng một node hàng `zone:row:<i>`
  với bindings (tên, loại, start, end); handler tra lại trong vector
  `zones` hiện hành — không khớp thì từ chối (`std::nullopt`), không
  tác động lên vùng tình cờ nằm ở vị trí cũ.
- `detail` của hàng công bố đầy đủ những gì bản vẽ mã hóa bằng
  vị trí/màu/ký hiệu: tên loại (`get_name_from_type`), Enabled/Disabled
  (+ temporarily disabled), personal/vehicle, tâm vùng (hướng
  `direction_name_short(direction_from(...))` + khoảng cách
  `trig_dist` — cùng dữ kiện bản vẽ in ở cột phải), hai góc theo độ
  lệch từng trục so với người chơi, kích thước
  `(|dx|+1) × (|dy|+1)` ô và số tầng, trạng thái hiển thị trên bản đồ,
  và các cặp mô tả tùy chọn `get_options().get_descriptions()` — đây
  là nơi nguồn/đích/điều kiện của từng loại vùng (lọc loot, hạt giống,
  công trình, ngưỡng unload, tùy chọn study…) được đọc.
- Summary nêu số vùng đang hiển thị/tổng số của phe, chế độ danh sách
  (mặc định màn chỉ liệt kê vùng trong tầm `MAX_VIEW_DISTANCE`;
  `show_all_zones` mới liệt kê tất cả — node lệnh `SHOW_ALL_ZONES`
  được đổi thành checkbox phản ánh đúng trạng thái này), và "There
  are unsaved changes." khi `stuff_changed`.
- Luồng tạo vùng giữ nguyên từng bước gốc: `ADD_ZONE` /
  `ADD_PERSONAL_ZONE` (lệnh gốc) → `mgr.query_type()` (uilist — adapter
  dùng chung R04, có mô tả từng loại) → `zone_options::query_at_creation()`
  của từng loại (uilist/popup dùng chung; riêng study zone xem mục 3)
  → `mgr.query_name()` (popup nhập — adapter `input_popup` R04) →
  chọn hai góc qua `look_around` (mục 1.2) → `mgr.add(...)` với đúng
  giới hạn của game. Ba đường vào — lệnh `zones` trong
  `handle_action.cpp`, `set_sort_points` của trại
  (`faction_camp.cpp`), và `vehicle_use.cpp` — đều gọi cùng
  `display_zone_manager()` nên được phủ chung; điểm mở của R14 về
  `zone_manager_ui` đóng tại đây.

### 1.2 Adapter chọn vị trí — `game::look_around()` (`src/game.cpp`)

look_around là màn chọn hai góc cho tạo vùng/sửa vị trí vùng, màn di
chuyển vùng (`is_moving_zone`), và cũng là màn nhìn quanh thường.

- Scope category `look-around` gắn `&ctxt` (context `LOOK`), tạo
  trước vòng `do`; handler chỉ trả `a11y::registered_intent` — di
  chuyển con trỏ là các action hướng gốc, focus không làm gì.
- Node `look:cursor`: vị trí con trỏ tương đối so với avatar (độ lệch
  từng trục + chênh tầng, cách diễn đạt của `relative_position_name`
  trong `a11y_world.cpp`, viết lại tại chỗ vì hàm đó không dùng chung)
  cộng mô tả ô theo đúng quyền biết của R08: cùng tầng dùng
  `a11y::observe_tile`, khác tầng chỉ `a11y::observe_memory_tile`
  (trí nhớ bản đồ, không đọc ô sống ở tầng chưa tới).
- Node `look:selection`: khi `select_zone && has_first_point`, công
  bố hình chữ nhật `start_point..lp` (hai góc tương đối + kích thước
  + số tầng) — tức phạm vi/kích thước được nghe *trước* khi xác nhận
  góc thứ hai, đúng yêu cầu R16.1. Khi `is_moving_zone`, công bố cặp
  `zone_start..zone_end` mà vòng lặp gốc tự tính (vị trí mới của vùng).
- Detail của `CONFIRM`/`QUIT` được ghi đè theo chế độ (đặt góc / kết
  thúc tại ô này / hủy không chọn ô).

## 2. R16.2 — Quản lý và chạy vùng (cùng adapter `zone-manager`)

- Duyệt không khởi chạy gì: focus hàng chỉ đặt `active_index`; nút
  lệnh duyệt qua `command_snapshot` như mọi màn.
- `activate` hàng đặt `active_index` rồi trả `CONFIRM` — mở menu sửa
  `uilist` gốc ("What do you want to change:": Edit name / Edit type /
  Edit options hoặc Edit filter / Edit position / Move position).
  Edit name/type/options chạy qua `zone_data::set_name/set_type` và
  `zone_options::query()` (uilist/popup dùng chung); Edit position
  chạy lại `query_position`/`query_personal_position`; Move position
  chạy `look_around(is_moving_zone)` — cả hai đều có adapter
  `look-around` ở mục 1.2.
- Xóa có xác nhận riêng (theo quy định của mục): handler chặn
  `activate` trên lệnh `REMOVE_ZONE` — lần đầu chỉ vũ trang (lưu
  identity của vùng đang chọn) và trả `TIMEOUT`; builder chỉ publish
  node `zone:remove:confirm` khi vùng vũ trang vẫn là vùng đang chọn.
  Kích hoạt node xác nhận mới trả `REMOVE_ZONE` về vòng lặp gốc. Mọi
  activate khác giải vũ trang; focus thì không.
- Bật/tắt, thứ tự, hiển thị: `ENABLE_ZONE` / `DISABLE_ZONE` /
  `TOGGLE_ZONE_DISPLAY` / `MOVE_ZONE_UP` / `MOVE_ZONE_DOWN` /
  `ENABLE_PERSONAL_ZONES` / `DISABLE_PERSONAL_ZONES` /
  `ENABLE_VEHICLE_ZONES` / `DISABLE_VEHICLE_ZONES` đi qua
  `registered_intent` tới đúng nhánh gốc; detail của
  ENABLE/DISABLE/TOGGLE nêu tên vùng đang chọn. Toàn bộ thay đổi vẫn
  chỉ được lưu qua "Save changes?" lúc thoát như bản gốc.
- **Thực thi tác vụ vùng**: UI gốc của màn quản lý không có lệnh chạy
  tác vụ. Tác vụ vùng (sắp xếp loot, farm, strip, unload…) chạy bằng
  `multi_zone_activity_actor` (`src/activity_actor.cpp`) như các
  activity thường — tiến độ, tạm dừng/hủy và kết quả đi qua cơ chế
  activity của R10, không phải qua màn này. Ghi rõ để không tính
  việc "đã đóng thực thi" từ adapter của màn quản lý.

## 3. R16.3 — Study Zone (`src/study_zone_ui.cpp`)

Cửa sổ `study_zone_window` là ImGui thuần (checkbox vẽ bằng chuột,
`ImGuiWindowFlags_NoNavInputs`); trước đợt này bộ đọc chỉ thấy danh
sách lệnh chung của context `STUDY_ZONE_UI`.

- Scope category `study-zone` gắn `&ctxt` (thành viên của cửa sổ),
  tạo trong `execute()` trước vòng lặp; cập nhật mỗi vòng trước
  `ctxt.handle_input()`.
- Bảng khai báo một lần `study:skills`: cột `skill` + một cột cho
  mỗi NPC (header là tên NPC). Node cha mỗi kỹ năng mang `table_id` /
  `row_index` / cells (ô NPC = "Allowed"/"Not allowed", compare
  "1"/"0") và `detail` "Allowed for X of N companions." — dữ liệu
  trước/sau đọc được ngay trên hàng mà không cần mở từng ô.
- Sửa qua cây theo R02.1: mỗi cặp (kỹ năng, NPC) là node con checkbox
  (depth 1, cha là node kỹ năng, luôn `expanded`) đồng thời là trường
  `bool` theo R02.3: `select_choice` chỉ ghi nháp (biến cục bộ
  `reader_drafts`), `apply` mới ghi, `cancel_draft` bỏ nháp,
  `activate` đổi ngay như click checkbox ImGui. Cả apply lẫn
  activate đều đi qua cùng một helper `set_skill_allowed` sao đúng
  logic của `draw_skill_row`: insert/erase skill, và erase luôn mục
  NPC khi set của họ thành rỗng; `preferences_changed = true` như
  đường ImGui. Trạng thái đọc (`skill_allowed`) bám đúng trạng thái
  vẽ: mục NPC không tồn tại = unchecked cho mọi kỹ năng (dù khi áp
  dụng, `get_skill_preferences` hiểu "không có mục" là đọc mọi kỹ
  năng — summary của snapshot nói rõ cả hai lớp nghĩa này).
- `study:check_all` / `study:clear_all`: chạy vòng lặp của
  `draw_footer` nhưng trên toàn bộ `all_skills`, vì bộ đọc không có
  tập lọc của ô lọc ImGui; `detail` ghi rõ phạm vi "every skill for
  every companion" thay vì im lặng đổi nghĩa. `study:done` trả
  `CONFIRM` như nút Done/`done_clicked` gốc.
- Trường hợp không có đồng đội: không khai báo bảng, chỉ publish
  node tài liệu "No companions." cùng nút Done và các lệnh.

## 4. R16.4 — Safe Mode và auto-pickup

### 4.1 Safe Mode — `safemode::show()` và `safemode::test_pattern()` (`src/safemode_ui.cpp`)

- Scope category `safe-mode-manager` gắn `&ctxt` (context
  `SAFEMODE`), tạo sau đăng ký action trong `show()`; cập nhật mỗi
  vòng lặp sau `ui_manager::redraw()`. Phủ cả đường gọi NPC
  (`show(custom_name, false)`).
- Summary: "Safe mode is currently on/off." theo `g->safe_mode`
  (đúng dữ kiện header gốc vẽ bằng màu), số quy tắc hai tab, và khi
  cả hai tab rỗng thì hai câu gốc "Safe mode manager is currently
  inactive." / "Default rules are used.  Add a rule to activate."
- Node tab `safemode:tab:global` / `safemode:tab:character` (chỉ
  activate mới chuyển tab; focus không chuyển). Tab Character bị
  khóa kèm `disabled_reason` "Please load a character first to use
  this page!" khi chưa có nhân vật — đúng điều kiện gốc.
- Bảng `safemode:rules` với đúng 6 cột của bản vẽ gốc: Rules /
  Attitude / Dist / B/W / Category / Mode. Cells bám đúng logic vẽ:
  attitude là "---" khi category là Sound; distance là "---" khi
  hostile-spotted mà whitelist, ngược lại là số (compare = số thuần).
  Trạng thái active vốn chỉ là màu (trắng/xám) được nói thành lời
  trong `value` của hàng: danh sách đen/trắng, category, attitude
  (khi áp dụng), distance kèm "(0 means max view distance)", mode,
  Active/Inactive.
- Sửa qua 7 trường R02 cho hàng đang chọn (`safemode:field:*`): rule
  (text — apply ghi qua `wildcard_trim_rule` như popup gốc; detail
  tóm tắt cú pháp wildcard theo category từ chính văn bản trợ giúp
  gốc), attitude (choice hostile/neutral/friendly/any, nhãn lấy từ
  `Creature::get_attitude_ui_data`; khóa khi Sound kèm lý do),
  proximity (int 0..`MAX_VIEW_DISTANCE`, step 1 — đúng miền của
  option `SAFEMODEPROXIMITY` mà popup gốc dùng để hợp thức hóa; khóa
  đúng điều kiện vẽ "---" kèm lý do), list (blacklist/whitelist),
  category (hostile/sound), movement mode (walking/driving/both),
  active (bool). Mỗi trường có nháp riêng kèm binding; apply chỉ ghi
  khi binding của nháp khớp binding của node, khớp hàng đang chọn và
  khớp chuỗi rule sống — không ghi nhầm vào quy tắc khác đã trượt vào
  cùng chỉ số. `depends_on` của attitude trỏ tới category, của
  proximity trỏ tới category + list, đúng quan hệ áp dụng của UI gốc.
- Thêm/sao chép/đổi thứ tự/bật/tắt/đổi tab bằng các lệnh gốc qua
  `registered_intent` (`ADD_RULE`, `ADD_DEFAULT_RULESET` — detail
  nêu hai quy tắc mặc định sẽ thêm, `COPY_RULE`, `MOVE_RULE_UP/DOWN`,
  `ENABLE_RULE`/`DISABLE_RULE`, `SWAP_RULE_GLOBAL_CHAR`,
  `NEXT_TAB`/`PREV_TAB`). Xóa theo cơ chế hai bước như vùng: node
  `safemode:remove:confirm`.
- Màn kiểm tra `safemode::test_pattern` có scope riêng
  `safe-mode-test`: summary là đúng chuỗi tiêu đề gốc "%d monster(s)
  match: %s", mỗi mtype khớp là một node tài liệu chỉ-focus, kèm
  note gốc "Lists monsters regardless of their attitude." — kết quả
  khớp nghe được từ chính dữ liệu UI gốc liệt kê, không suy diễn.

### 4.2 Auto-pickup — `auto_pickup::user_interface::show()` và `rule::test_pattern()` (`src/auto_pickup.cpp`)

- Scope category `auto-pickup-manager` gắn `&ctxt` (context
  `AUTO_PICKUP`) trong `user_interface::show()` — một adapter phủ cả
  màn người chơi (`player_settings::show`, 2 tab Global/Character)
  lẫn màn NPC (`npc_settings::show`, 1 tab).
- Summary công bố trạng thái option `AUTO_PICKUP` mà header gốc chỉ
  hiện bằng màu True/False; node lệnh `SWITCH_AUTO_PICKUP_OPTION`
  được đổi thành checkbox phản ánh option đó (activate vẫn đi qua
  action gốc — option tự đổi và lưu ngay như bản gốc).
- Node tab `autopickup:tab:<i>` cho từng tab; bảng `autopickup:rules`
  với cột Rules / Inc/Exc / Active — trạng thái active vốn chỉ là màu
  được nói trong `value` và cell `Active`/`Inactive`.
- Ba trường R02 cho hàng đang chọn (`autopickup:field:*`): rule
  (text; nháp rỗng bị từ chối bằng `field_error` "A rule cannot be
  empty. Use * to match everything." và giữ nguyên nháp — bám đúng
  quy tắc gốc "rỗng = hủy sửa"; nháp hợp lệ ghi qua
  `wildcard_trim_rule`; detail tóm tắt cú pháp wildcard + tiền tố
  vật liệu `m:`/`M:` từ văn bản mô tả gốc), exclude (choice
  include/exclude — tương ứng cột phải của bản gốc), active (bool).
  Apply đặt `bStuffChanged = true` như mọi sửa đổi bằng phím.
- Xóa hai bước qua node `autopickup:remove:confirm`; thêm/sao
  chép/đổi thứ tự/bật/tắt/đổi tab qua lệnh gốc. Vì adapter ghi trên
  `new_rules`, việc chốt vào kho thật vẫn do "Save changes?" lúc
  thoát quyết định (mục 0).
- Màn kiểm tra `rule::test_pattern` có scope riêng
  `auto-pickup-test`: summary là chuỗi gốc "%d item(s) match: %s",
  node tài liệu cho từng itype khớp, note gốc "Won't display content
  or suffix matches".

## 5. Chỗ chưa chắc / điểm mở (chuyển giai đoạn 2)

1. Chưa biên dịch — toàn bộ chữ ký/API đã đối chiếu bằng đọc mã tại
   cây hiện tại (tiền lệ `screen_scope`/`command_snapshot`/
   `registered_intent`, `a11y::observe_tile`/`observe_memory_tile`,
   `Creature::get_attitude_ui_data`, `wildcard_trim_rule`,
   `MAX_VIEW_DISTANCE`); tính biên dịch thật thuộc giai đoạn 2.
2. Giới hạn validate của R02 (16 cột/bảng, 16 ô/hàng, 4096
   node/snapshot): bảng `study:skills` vượt giới hạn cột khi có hơn
   15 đồng đội, và snapshot study zone có thể vượt 4096 node khi
   (số kỹ năng × số NPC) lớn; hai màn test cũng có thể vượt 4096
   node khi một mẫu khớp rất nhiều mtype/itype. Theo đúng hợp đồng
   R02, snapshot vượt giới hạn bị từ chối thay vì cắt thầm — cần
   chốt phương án phân trang ở giai đoạn 2 (cùng điểm mở đã ghi ở
   R02 cho các bề mặt lớn).
3. Con trỏ `look_around`: cách diễn đạt vị trí tương đối không nói
   tường minh "cùng tầng" khi dz = 0 (giữ đúng cách diễn đạt của
   R08); nếu nghiệm thu giai đoạn 3 cần câu "same level" tường minh
   thì bổ sung sau.
4. Nháp của các trường phụ thuộc (attitude/proximity của Safe Mode)
   vẫn được giữ và publish lại nếu điều kiện áp dụng quay lại sau
   khi category/list đổi bằng apply khác; apply trong lúc điều kiện
   không áp dụng bị từ chối. Hành vi có chủ ý, chưa kiểm thử.
5. Vũ trang xóa vẫn còn nếu người dùng xóa bằng phím gốc ngay sau
   khi vũ trang bằng bộ đọc; binding (tab + chỉ số + chuỗi) khiến
   node xác nhận chỉ hiện lại khi một quy tắc trùng cả ba yếu tố ở
   hàng đang chọn — trường hợp biên, chưa kiểm thử.
6. Chuỗi mới của R16 chưa thêm bản dịch vào
   `lang/po/accessibility/vi.po` — dịch Việt hoàn chỉnh là phạm vi
   R21; các chuỗi đều qua `_()` nên trích xuất được.
7. Số liệu sổ hành trình: `tong_quan` trong `JOURNEY-LEDGER.json`
   trước đợt này ghi 76/39 nhưng đếm lại từ dữ kiện từng hành trình
   cho 86/29 (lệch đã được R15 ghi nhận); đợt này đếm lại và sửa
   `tong_quan` theo dữ kiện: sau R16 là 90 đã viết / 25 chưa viết.
   Bảng tổng quan theo nhóm trong `JOURNEY-LEDGER.vi.md` còn vài
   hàng cũ lệch tương tự ở các nhóm ngoài R16 (chỉ hai nhóm R16 chạm
   tới — Vùng/tự động hóa, Cấu hình — đã sửa); bản .vi.md cần một
   đợt đối chiếu toàn bộ ở R23, dữ kiện từng hành trình trong
   `JOURNEY-LEDGER.json` là nguồn đúng.
