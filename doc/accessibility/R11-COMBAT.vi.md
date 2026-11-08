# R11 — Chiến đấu, nhắm ô/tầng và vùng ảnh hưởng (giai đoạn 1, chỉ viết mã)

Trạng thái: **đã viết, chờ kiểm thử** (xem `REMAINING-CODE-BACKLOG.json`).
Chưa build, chưa chạy test, chưa tạo APK trong đợt này.
Test ID sẽ chạy ở giai đoạn 2: T20, T22–T24, T30, T39–T42, T52.

Tham chiếu ý tưởng từ gói GnollHack tham khảo (không chép mã):
`AccessibilityTargetingController` của nó tách chế độ nhắm khỏi kích
hoạt — con trỏ nhắm không bao giờ tự bắn. CDDA đạt cùng tính chất bằng
cấu trúc sẵn có của `target_ui` (chọn mục tiêu chỉ dời `dst`) cộng các
intent nội bộ của bộ đọc dưới đây.

Quy tắc bất biến của mục này:

- Engine là thẩm quyền duy nhất về khả năng tấn công. Focus/chọn điểm/
  dời con trỏ **không bao giờ** tự FIRE, không tốn lượt, không trừ đạn.
- Không đọc HP bí mật, không nêu tên kẻ ẩn; thực thể không thấy được
  không được tính vào bất kỳ tổng số hay cue AOE nào.
- Xác nhận bắn mục tiêu trung lập/đồng đội là bước riêng tường minh —
  chính là `query_yn` của engine ở nhánh FIRE, bộ đọc không thay và
  không bỏ qua nó.

## 1. R11.1 — Cận chiến, reach, võ thuật, safe mode

### 1.1 Những gì đã có từ R08.3 và giữ nguyên

`avatar_action::move` ghi outcome `attacked` (kèm tên mục tiêu người
chơi thấy) tại hai điểm `melee_attack` thật — quái và NPC địch. Kết
quả trúng/trượt do engine quyết và nằm trong log; kênh outcome không
tự phán hit/miss. Reach attack (vũ khí dài và tay không) đi qua
`target_handler::mode_reach`/`mode_unarmed_reach` → `target_ui` với
`TargetMode::Reach`, dùng chung toàn bộ adapter nhắm ở mục 2–4.
Rút lui không có action engine riêng: nó là bước đi thường, kết quả
qua outcome `moved`/`blocked` như mọi bước đi.

### 1.2 Safe mode chặn đòn — outcome tường minh (mới)

Nhánh trong `avatar_action::move`: safe mode đang bật và sinh vật ở ô
đích có thái độ NEUTRAL — engine chỉ `add_msg` cảnh báo rồi hủy bước.
Đợt này ghi thêm tại đúng nhánh đó:

- Kind mới `safe_mode_blocked` ở **cuối** enum `action_outcome_kind`
  (`src/a11y_outcome.h`, không đổi giá trị các kind cũ), câu mô tả
  "Safe mode blocked the attack on %s." trong `describe_action_outcome`
  — detail là `disp_name()` của sinh vật mà chính engine vừa in tên
  trong cảnh báo (ô kề, người chơi biết). Record tường minh thắng
  fallback của `move_outcome_guard` nhờ cơ chế sequence.
- Nơi tiêu thụ kind duy nhất là node `world:last_outcome` (kiểm
  `kind != none` rồi gọi `describe_…`) — đã đối chiếu grep toàn repo:
  không consumer nào switch thiếu kind mới, không serialize kind,
  không có consumer phía Java.

Trạng thái safe mode nay được công bố trong detail của node
`world:status` (`player_status`, `src/a11y_world.cpp`): "Safe mode:
On/Off/Stop" map đúng ba giá trị `safe_mode_type` qua biến toàn cục
`g` (repo không có `get_game()`; `g` là cách truy cập chuẩn, có guard
null), giá trị lạ bị bỏ qua không bịa chữ. Bật/tắt vẫn là các lệnh gốc
`TOGGLE_SAFEMODE`/`TOGGLE_AUTOSAFEMODE` qua trung tâm lệnh và quản lý
quy tắc ở `safemode_ui` thuộc R16.4 như sổ hành trình đã ghi.

### 1.3 Chọn style võ thuật — adapter dùng chung, không adapter riêng

`character_martial_arts::pick_style` (`src/character.cpp`) dựng
`uilist` (`MELEE_STYLE_PICKER`) nên được adapter uilist dùng chung R04
phục vụ trọn: từng style là một entry, mô tả style nằm sẵn trong
`node.detail` (từ `addentry_desc`), và `SHOW_DESCRIPTION` được
`create_main_input_context` đăng ký thành node lệnh — activate nó đi
qua `ma_style_callback::key()` mở chi tiết kỹ thuật như phím gốc.
Giữ nguyên quyết định này thay vì viết adapter style riêng.

**Khoảng trống ghi rõ:** cửa sổ chi tiết kỹ thuật
(`ma_details_ui_impl`, ImGui trong `src/martialarts.cpp`) không có
`screen_scope` nào — việc *kích hoạt* SHOW_DESCRIPTION đã được phục
vụ, còn *nội dung* cửa sổ đó có được đọc hay không chưa xác nhận
được ở đợt này. Mô tả từng style vẫn nghe được qua detail của entry
nên hành trình chọn style không bị chặn; cửa sổ details để mở cho
mục rà ImGui (R03/R19) — không tính là đã phủ.

### 1.4 Các nhánh move chưa có outcome tường minh riêng

Rà soát đầy đủ `avatar_action::move`: displace thất bại, `CANNOT_MOVE`
của người chơi, `check_safe_mode_allowed` đầu hàm, `move_effects`
chặn — tất cả đều rơi vào fallback của guard: `moved` nếu vị trí đổi,
`undetermined` nếu không. Không im lặng (log engine vẫn đọc), nhưng
chưa có kind đúng ngữ nghĩa cho từng nhánh. Đợt này chỉ vá nhánh
safe-mode (đúng phạm vi R11.1); các nhánh còn lại ghi nhận ở đây để
giai đoạn 2 quyết có cần kind riêng hay không, không tự mở rộng.

## 2. R11.2 — Nhắm ô trống, đổi tầng, dữ kiện điểm ngắm

Adapter sẵn có từ ad5ecf0 được giữ và mở rộng, không viết lại: danh
sách `target:<i>` (activate chỉ `set_cursor_pos`), node
`target:cursor`, cổng `pl_sees` cho sinh vật/xe dưới con trỏ.

Bổ sung trong `target_ui::reader_snapshot()` và `target_ui::run()`
(`src/ranged.cpp`):

- **Điều khiển con trỏ theo ô:** 8 node `target:move:<dx>:<dy>` và
  `target:level:up`/`target:level:down` (node tầng chỉ công bố khi
  `fov_3d_z_range > 0`, đúng điều kiện đăng ký `LEVEL_UP`/`LEVEL_DOWN`
  của `init_window_and_input`). Nhờ đó người dùng dời điểm ngắm tới
  bất kỳ ô trống/tầng nào — chọn ô trống chính là dời con trỏ tới đó,
  ngữ cảnh nhắm vẫn là của `target_ui`, tách khỏi con trỏ thăm dò của
  R08 như kế hoạch yêu cầu.
- **Đường intent không thể FIRE:** handler đối chiếu `intent.node`
  với whitelist 10 cặp {node id, delta hằng} viết cứng; delta không
  bao giờ được parse từ giá trị intent. Khớp mới trả action nội bộ
  `A11Y_INTERNAL_MOVE_CURSOR` (đăng ký/khôi phục sau resize như
  `A11Y_INTERNAL_SELECT_TARGET`, và bị xóa khỏi snapshot công bố như
  nó). Nhánh vòng lặp của action này chỉ gọi
  `set_cursor_pos(dst + delta)` rồi xóa delta — không chạm
  `ExitCode::Fire`, không gọi `confirm_non_enemy_target()`/
  `set_last_target()`, không trừ moves. Node tầng bị handler từ chối
  cả khi snapshot không công bố. Khi hết đạn (`Status::OutOfAmmo`)
  các node bị disable kèm lý do, khớp việc `set_cursor_pos` từ chối.
  Rời chế độ nhắm bằng lệnh `QUIT` gốc: hủy, không bắn, không tốn
  lượt (hành vi sẵn có của vòng lặp).
- **Dữ kiện công khai ở điểm ngắm:** summary và `target:cursor` nay
  công bố chế độ nhắm (Fire/Throw/Blind throw/Turrets/Manual turret/
  Reach/Spell/Select only) và chênh tầng `dst.z() − src.z()`. Riêng
  mode Fire: **steadiness %** qua `calc_steadiness(*you, *relevant,
  dst, predicted_recoil)` — đúng con số panel gốc in qua
  `print_steadiness` cho điểm ngắm hiện tại — và **sight dispersion**
  qua `get_sight_dispersion()`. Che khuất/tầm hợp lệ vẫn do `status`
  của engine quyết (Good/BadTarget/OutOfRange/OutOfAmmo trong
  summary) — bộ đọc không tự tính LOS thay engine.

## 3. R11.3 — Chế độ bắn, đạn, reload/unload, ném, turret

- **Node `target:weapon`** (mode Fire): tên chế độ bắn hiện tại —
  `gun_mode::tname()` (gun_mode không có `name()`; `tname()` là tên
  dịch panel gốc in) — và số đạn còn qua `gun->ammo_remaining()`
  (đúng count panel hiển thị cho mode đó). Đổi chế độ/đạn là hai lệnh
  riêng `SWITCH_MODE`/`SWITCH_AMMO` trong cùng snapshot qua
  `command_snapshot`; khi `SWITCH_AMMO` không xử lý nội tuyến, vòng
  lặp thoát `ExitCode::Reload` sang UI nạp đạn gốc (selector R09) —
  giữ nguyên.
- **Reload/unload ngoài chế độ nhắm:** `ACTION_RELOAD_ITEM`/
  `RELOAD_WEAPON`/`RELOAD_WIELDED`/`UNLOAD` ở `handle_action.cpp`
  đi qua selector/menu dùng chung đã có adapter (R04/R09) — không
  cần và không viết đường riêng.
- **Ném và turret do nhân vật điều khiển:** `mode_throw` (gồm ném
  mù), `mode_turret_manual` đều dựng cùng `target_ui` nên hưởng toàn
  bộ adapter này (danh sách, con trỏ theo ô, node weapon khi áp
  dụng). Chọn turret hàng loạt qua điều khiển xe (`mode_turrets`)
  thuộc R15; phần nhắm/bắn thủ công ở đây đã phủ.
- **Xác nhận mục tiêu nhạy cảm:** ở nhánh `FIRE` và các shot đã ngắm,
  engine gọi `confirm_non_enemy_target()` — `query_yn` "Really
  attack %s?" cho NPC chưa chắc hostile, "Really attack yourself?"
  khi phép tự trúng — và `prompt_friendlies_in_lof()` khi có đồng
  minh mới vào đường đạn. Intent `activate` của bộ đọc trên node lệnh
  FIRE chỉ gửi đúng lệnh gốc đó qua `registered_intent`; hộp xác
  nhận là popup dùng chung R04 — duyệt/chọn điểm không bao giờ đi qua
  xác nhận này vì chúng không phải FIRE.
- **Tra lại tại lúc chốt:** intent đi qua cổng dispatcher R03 (node
  phải tồn tại trong snapshot đang hiệu lực, action thuộc chính node,
  handler là lớp kiểm cuối). Trong vòng nhắm, `refresh_reader` dựng
  lại `targets` từ tập engine hiện tại trước mỗi lần publish và
  `dst_critter` được tính lại trong `set_cursor_pos` — mục tiêu đã
  biến mất/đổi chỗ không còn là cơ sở của phát bắn; `FIRE` bị cổng
  `status != Good` chặn.
- **Danh sách rỗng / rất dài:** rỗng — summary thêm câu "There are
  no targets." (trước đây chỉ có số 0). Dài — trần 256 giữ nguyên
  nhưng node `target:list-limit` nay công bố **cả hai số** ("Showing
  the first 256 of N targets…") và chỉ đường next/previous target
  của engine để duyệt hết danh sách đầy đủ: trần công bố rõ, không
  cắt thầm.

## 4. R11.4 — Vùng ảnh hưởng (AOE)

Node `target:aoe` (chỉ mode Spell, khi `spell_aoe` không rỗng) dựng
từ chính footprint engine đã tính trong `set_cursor_pos`:

- Số ô, hình dạng (`casting->shape()`: Blast/Cone/Line), chuỗi đo
  vùng `casting->aoe_string()` như panel gốc, hướng từ người cast
  tới tâm vùng (trung bình tọa độ các ô footprint) và khoảng tầng
  bao phủ (min/max z của footprint so với tầng người cast).
- Cảnh báo tự trúng qua `casting->in_aoe(src, dst, *you)` — cùng
  kiểm tra panel gốc dùng để in "IN RANGE".
- **Nguy cơ đồng minh đã biết:** duyệt từng ô footprint,
  `creature_at(tile, true)`; bỏ qua chính người cast (báo riêng qua
  in_aoe); **chỉ** khi `pl_sees(*cr)` mới xét thái độ — đếm và kể tên
  khi FRIENDLY hoặc NPC không HOSTILE (cùng công thức
  `list_friendlies_in_lof()`). Sinh vật không thấy được không đóng
  góp vào số đếm lẫn tên. Khi không có đồng minh đã biết, văn bản
  ghi rõ việc này chỉ tính sinh vật nhìn thấy và **không** đảm bảo
  vùng an toàn tuyệt đối.
- Sửa/hủy trước khi bắn: dời điểm ngắm bằng các node con trỏ mục 2
  (footprint tính lại theo điểm mới ở snapshot kế) hoặc hủy bằng
  `QUIT` — cả hai đều không phải FIRE.

Giới hạn trung thực: footprint chỉ tồn tại sẵn trong `target_ui` cho
phép. Vũ khí/đạn nổ dùng mode Fire không có tập ô AOE tương đương
trong UI gốc (đạn nổ xử lý lúc trúng), nên bộ đọc không bịa
footprint cho chúng — nguy cơ vùng của đạn nổ để giai đoạn 2 xem có
dữ kiện công khai nào khác không.

## 5. Chỗ chưa chắc

1. Chưa biên dịch: các API đã đối chiếu tại header/điểm dùng gốc
   (`gun_mode::tname`/`operator->`, `spell::aoe_string/in_aoe/shape`,
   `calc_steadiness`, `tripoint_rel_ms` trong whitelist,
   `g->safe_mode` theo tiền lệ `avatar_action.cpp`), nhưng tính biên
   dịch thật thuộc giai đoạn 2 (T39–T42, T52).
2. Steadiness công bố là steadiness **tại điểm ngắm theo predicted
   recoil hiện tại** của UI; đường vẽ gốc còn mô phỏng ngắm tới
   ngưỡng aim-mode cho dòng "Current". Nếu giai đoạn 2 muốn khớp
   tuyệt đối cả hai số, phải thêm mô phỏng đó vào adapter.
3. Cửa sổ ma-details (ImGui) chưa có screen_scope — mục 1.3.
4. Turret chọn qua điều khiển xe và cargo xe thuộc R15; ở đây chỉ
   phần nhắm/bắn thủ công của nhân vật.
