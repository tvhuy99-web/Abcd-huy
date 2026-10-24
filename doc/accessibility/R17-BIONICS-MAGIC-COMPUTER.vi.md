# R17 — Bionics, đột biến, phép và máy tính trong game

Giai đoạn 1 (chỉ viết mã, không build/test). Trước đợt này cả bốn bề
mặt đều có 0 điểm `a11y::`: `bionics_ui.cpp`, `mutation_ui.cpp`,
`magic.cpp` (menu phép + spellbook), `computer_session.cpp`. Đợt này
viết adapter cho cả bốn, bám mẫu đã có (screen_scope gắn vào
input_context của chính màn, snapshot dựng lại mỗi vòng lặp, intent
trả action string về vòng lặp gốc — tiền lệ smart controller của R15;
với các menu `uilist` thì nạp dữ kiện vào entry để adapter dùng chung
của R04 công bố). Không thao tác nào của bộ đọc chạy ngoài logic gốc:
focus chỉ quan sát/dời con trỏ, kích hoạt luôn quay về nhánh CONFIRM
của vòng lặp gốc hoặc `ret` của uilist.

Nguyên tắc rủi ro xuyên suốt đợt này: chỉ công bố rủi ro/xác suất mà
chính engine in cho người chơi ở bề mặt tương ứng. Ba nơi đã kiểm
chứng: cột FAILURE CHANCE của preset chọn CBM và câu `query_yn` phần
trăm thất bại trong `can_install_bionics`/`can_uninstall_bionic`;
`colorized_fail_percent` của panel phép. Nơi engine không in gì (luồng
tự lắp CBM — mục 1.3) thì ghi rõ là không có, không bịa số để bù.

## 1. R17.1 — Bionics (`src/bionics_ui.cpp`)

### 1.1 Adapter màn bionics — `avatar::power_bionics()`

- Scope `a11y::screen_scope` category `bionics` gắn `&ctxt` (context
  `BIONICS`), handler + builder đặt trước vòng lặp; vòng lặp gọi
  `reader.update(...)` sau `ui_manager::redraw()`, trước
  `ctxt.handle_input()`.
- Logic định dạng điện/nhiên liệu vốn nằm inline trong
  `draw_bionics_titlebar()` được tách nguyên văn thành hai helper
  `bionic_fuel_string(avatar*)` / `bionic_power_string(avatar*)`;
  titlebar gọi lại hai helper này, snapshot dùng chung — một nguồn
  định dạng duy nhất, không sao chép bản thứ hai.
- Summary: `Bionic Power: <power>/<max>kJ` đúng format titlebar (đã
  lột thẻ màu), chuỗi nhiên liệu khả dụng (rỗng thì bỏ, đúng điều
  kiện của gốc), tab hiện tại + số lượng hai tab, chế độ bằng lời
  (Activating/Examining/Reassigning).
- Node tab `bionics:tab:active` / `bionics:tab:passive` (role `tab`,
  detail = số lượng): focus trả `TIMEOUT` và không đổi tab; activate
  trả `NEXT_TAB`/`PREV_TAB` đúng chiều của vòng lặp (đã ở tab đó thì
  `TIMEOUT`).
- Node entry `bionics:entry:<chỉ số trong tab hiện tại>`: label =
  `build_bionic_powerdesc_string` nguyên văn (tên + chi phí
  act/deact/trigger + ON/OFF + incapacitated/shutdown off/hidden/
  fuel saving — tức đúng hàng danh sách hình vẽ, gồm cả các trạng
  thái vốn chỉ phân biệt bằng màu ở `get_bionic_text_color` vì chúng
  đã nằm trong chuỗi này); detail = đúng `draw_description`: mô tả +
  "Power usage: ..." + `list_occupied_bps` + "Installed weapon: ..."
  khi có. Danh sách rỗng: node `bionics:empty` role `status` mang
  đúng câu gốc ("No activatable bionics installed." / "No passive
  bionics installed.").
- Handler tra lại danh sách sống (`active`/`passive`, `tab_mode`,
  `cursor`) tại thời điểm intent; entry focus đặt cursor + kẹp
  `scroll_position` như logic UP/DOWN rồi trả `TIMEOUT`; entry
  activate đặt cursor rồi trả `CONFIRM`. Handler không bao giờ gọi
  `activate_bionic`/`deactivate_bionic` trực tiếp: vòng lặp gốc xử lý
  CONFIRM theo `menu_mode` — ACTIVATING đi trọn nhánh gốc (gồm menu
  vũ khí lồng `activate_action_menu`/`install_weapon_menu` — cả hai
  là uilist dùng chung), EXAMINING bỏ qua CONFIRM, REASSIGNING chọn
  bionic tại cursor rồi gọi `choose_key_from_user` dùng chung.
- Node lệnh từ `command_snapshot` nối sau node riêng:
  TOGGLE_EXAMINE / REASSIGN / TOGGLE_SAFE_FUEL / TOGGLE_SHUTDOWN /
  TOGGLE_SPRITE / SORT / NEXT_TAB / PREV_TAB / QUIT /
  HELP_KEYBINDINGS qua `registered_intent`. Các lệnh này tác động lên
  bionic tại cursor — nhất quán vì focus đã đặt cursor.

### 1.2 Lắp/tháo/phẫu thuật — rà soát bằng chứng (không cần adapter mới)

Mọi luồng đều đi qua bề mặt dùng chung đã có adapter (uilist R04,
popup/query_yn R03–R04, inventory selector R09):

- **Autodoc** (`iexamine::autodoc`, `src/iexamine.cpp`): menu là
  `uilist` (bản cyborg và bản thường; header cảnh báo "Operator
  missing … serious injuries or death" là `amenu.text` nên tới bộ
  đọc nguyên văn qua summary); chọn CBM qua
  `game_menus::inv::install_bionic`; tháo qua `uilist`.
- **Preset chọn CBM** (`bionic_install_preset`,
  `src/game_inventory.cpp`): có cột `FAILURE CHANCE` (=
  `100 - bionic_success_chance(...)`) cùng OPERATION DURATION /
  ANESTHETIC REQUIRED — các cell tới bộ đọc qua adapter inventory.
- **Cổng xác nhận của engine** (`src/bionics.cpp`):
  `can_install_bionics` hỏi `query_yn` "WARNING: %i percent chance
  of failure that may result in damage, pain, or a faulty
  installation! Continue anyway?" (100% thì câu xác nhận tương ứng);
  `can_uninstall_bionic` hỏi "WARNING: %i percent chance of SEVERE
  damage to all body parts! Continue anyway?". Cả hai qua adapter
  popup dùng chung.
- **NPC bác sĩ** (`bionic_install_common`/`bionic_remove_common`,
  `src/npctalk_funcs.cpp`): chọn qua cùng selector (preset surgeon
  có FAILURE CHANCE với skill 20) và cùng hai `query_yn` trên —
  installer là NPC nên không dính nhánh return sớm của tự lắp.
- **Ca mổ** chạy bằng `bionic_operation_activity_actor`
  (`src/activity_actor.cpp`) sau khi các cổng trên đã chạy; tiến độ
  qua kênh activity của R10, kết quả qua `add_msg` ở `finish` ("The
  operation is a success." / "…failure."). Rủi ro nằm ở bước tiền
  mổ, đúng như UI gốc.

### 1.3 Ngoại lệ ghi rõ: tự lắp CBM không có % rủi ro ở bất kỳ đâu

`install_bionic_actor::use` (`src/iuse_actor.cpp`) gọi
`can_install_bionics(..., autodoc=false)`; trong `src/bionics.cpp`
nhánh tự lắp (`!autodoc && installer.is_avatar()`) **return sớm** chỉ
kiểm tra thuốc mê + linh kiện, không bao giờ tới `query_yn` phần trăm
thất bại. Người sáng mắt cũng không thấy % nào ở luồng này — đây là
hành vi của UI gốc, bộ đọc không bịa số để bù. Các popup thiếu điều
kiện (thuốc mê/linh kiện) vẫn tới bộ đọc qua adapter popup.

## 2. R17.2 — Đột biến (`src/mutation_ui.cpp`)

### 2.1 Adapter màn mutations — `avatar::power_mutations()`

- Scope category `mutations` gắn `&ctxt` (context `MUTATIONS`);
  snapshot cập nhật mỗi vòng sau redraw, trước `handle_input()`.
- Summary: `Mutations: <chế độ>. <n> active, <m> passive.` — từ chế
  độ lấy đúng chữ titlebar (Activating/Examining/Reassigning/
  Hiding); ở Reassigning cộng thêm câu hướng dẫn duy nhất của
  titlebar không phải gợi ý phím ("Select a mutation to reassign or
  press [SPACE] to cancel.").
- Node tab `mutations:tab:active` / `mutations:tab:passive`: focus
  chỉ quan sát; activate tab còn lại trả `NEXT_TAB` (cả NEXT_TAB và
  PREV_TAB của vòng lặp đều là toggle + tự reset cursor/scroll).
- Node entry `mutations:entry:<chỉ số trong cột hiện tại>`: label
  sao nguyên văn hàng vẽ —
  - passive: `"<phím> <tên>"` + hậu tố `" H"` khi ẩn sprite;
  - active: phím + tên + cost/cooldown đúng cách ráp của `on_redraw`
    (đơn vị tài nguyên từ cờ của `mutation_branch`: kcal / thirst /
    sleepiness / stamina / mana, nối " &" — sao cả các quirk ráp
    chuỗi của gốc để hàng nói khớp hàng in) + `" - Active"` khi
    đang bật.
  Detail = `mutation_desc(trait)` (đúng cửa sổ examining) + ghi chú
  purifier khi `!purifiable()`; trạng thái vốn chỉ là màu/ký hiệu
  được nói thành lời: "Active"/"Not active" cho cột active, "Hidden"
  cho passive ẩn sprite. `checked` = `td.powered` ở cột active.
  Cột rỗng: node `mutations:empty` mang đúng chữ gốc "None".
- Handler tra lại `active`/`passive` sống + `cached_mutations` tại
  thời điểm intent (chỉ số phân giải thủ công, chống tràn; trait
  không còn thì từ chối). Focus đặt cursor/`examine_id`/
  `scroll_position` y hệt UP/DOWN và trả `TIMEOUT`; activate đặt
  cursor rồi trả `CONFIRM` — vòng lặp gốc dispatch theo
  `mutation_menu_mode`: activating (kiểm tra tài nguyên + popup
  thiếu của engine), examining, reassigning
  (`choose_key_from_user` dùng chung), hiding (lật `show_sprite`).
  Không gọi trực tiếp hàm kích hoạt trait từ handler.
- Node lệnh từ `command_snapshot` nối sau node riêng.

### 2.2 Phân vai với player_display — không trùng nguồn nói

`player_display` tab Traits (R10) là màn **hiển thị trạng thái**: in
tên trait + cùng `mutation_desc()` + cùng ghi chú purifier. Màn
mutations là màn **quản lý** (kích hoạt/tắt, gán phím, ẩn sprite,
chi phí/cooldown mà player_display không in). Hai màn dùng chung
một nguồn mô tả và cùng tập nền (loại `corrupted > 0` và
`!player_display`) nên không mâu thuẫn. Ghi nhận: R10 để ngỏ "chi
tiết traits/bionics thuộc R17/R19" ở tab của player_display — đợt
này không sửa player_display; điểm đối chiếu còn lại cho R19/R23.

### 2.3 Khía cạnh không có UI thật (bằng chứng, không bịa màn hình)

- Nhận đột biến mới: `Character::mutate` / `mutate_category` /
  `mutate_towards` (`src/mutation.cpp`) — chỉ được gọi từ sự kiện
  (suffer/phóng xạ, hiệu ứng phép, hội thoại NPC, hardcoded
  effects); kết quả chỉ ra log `add_msg_if_player`.
- Giấc mơ theo category: `get_category_dream` chỉ được gọi trong xử
  lý ngủ rồi in log.
- Vượt ngưỡng: `try_crossing_threshold`/`cross_threshold` không có
  caller UI; kích hoạt nội bộ từ `mutate_towards` + event bus + log.
- Ngoại lệ tương tác duy nhất của các luồng này là
  `Character::mutation_selector` — một `uilist`, đã có adapter dùng
  chung; tương tự `iuse::purify_smart` chọn trait qua uilist dùng
  chung. Không cần adapter riêng.

Ghi nhận thêm: UI gốc phân biệt trait gốc/trait nhận thêm chỉ bằng
sắc màu, không có chữ nào cho nó — bộ đọc không nói điểm này (không
có dữ kiện chữ để nói). Cột active không in dấu " H" dù mode hiding
vẫn lật được `show_sprite` của trait active — adapter tôn trọng
đúng dữ kiện UI in.

## 3. R17.3 — Phép (`src/magic.cpp`)

### 3.1 Menu phép — `known_magic::select_spell()`

Menu vốn là `uilist` (đã có adapter dùng chung) nhưng entry chỉ có
tên phép; toàn bộ dữ kiện giàu chỉ vẽ ở panel ImGui
`spellcasting_callback::display_spell_info`. Đợt này nạp vào entry:

- `entry.desc` = `spell_reader_info(sp, guy)` — bản chữ sao đúng
  panel, cùng thứ tự/cách diễn đạt: lớp phép, description (đã
  `parse_tags`), `enumerate_spell_data`, câu channeling, level hiệu
  dụng + max level (+ chỉnh tạm thời), fail (=
  `failed_caster_condition_message` khi điều kiện caster sai, ngược
  lại `colorized_fail_percent` đã lột thẻ màu — không bịa %), độ
  khó, exp/exp to next, chi phí năng lượng (kèm "(impeded)" và số
  hiện có như panel; thiếu thì câu "Not Enough ..."), thời gian thi
  triển, valid targets, range, AOE theo đúng nhánh effect của panel
  (cone/line/radius/variance), một dòng damage/healing/spawn/
  summon/polymorph, duration, components/tools đã fold.
- `entry.a11y_value`: "Favorite spell." khi là favorite + "Hotkey:
  <ký tự>." khi có invlet.
- `entry.a11y_disabled_reason` (khi `!can_cast`) =
  `spell_reader_disabled_reason` — sao đúng thứ tự kiểm tra của
  `spell::can_cast`, chỉ lấy lý do engine sở hữu: thông điệp
  `cannot_cast_message` của magic type, `failed_caster_condition_
  message`, câu thiếu năng lượng của chính đường cast
  (`Character::cast_spell`). Các nhánh can_cast không có thông điệp
  riêng (CANNOT_ATTACK, câm + verbal, thiếu components) để trống —
  adapter dùng chung dùng câu chung, không bịa.
- `spellcasting_callback::describe_accessibility_action` (mới):
  CAST_IGNORE thành checkbox mang đúng trạng thái `casting_ignore`
  (vốn chỉ là chữ màu "[I] ..." ở panel) + detail nêu tác động;
  SCROLL_FAVORITE thành checkbox theo phép đang chọn; CHOOSE_INVLET
  nêu tác động (prompt là `choose_key_from_user` dùng chung, không
  viết lại); SCROLL_UP/DOWN_SPELL_MENU nói thật là chỉ cuộn panel
  hình — cùng thông tin đã nằm trong detail của entry.

### 3.2 Học phép — spellbook

- Menu học của `learn_spell_actor::use` (`src/iuse_actor.cpp`) là
  uilist thường: trạng thái ("Level %u"/" (Max)", "Study to Learn",
  "Can't learn!") đã tới bộ đọc qua cột thứ hai của entry; engine
  không có chuỗi lý do riêng cho "can't learn" nên không đặt
  disabled_reason.
- Panel ImGui `spellbook_callback::draw_spellbook_info` là cửa sổ
  tự vẽ không adapter — đã đóng trong magic.cpp: `refresh()` nạp
  `entry.desc` = `spellbook_reader_info(spell_type)` (sao đúng
  panel: tên, lớp, mô tả, difficulty, max level, damage type, các
  hàng min/per-level/max cho damage/healing — gồm nhánh
  RANDOM_DAMAGE — range, AoE/variance, duration, cast cost, cast
  time) cho từng entry còn trống. Hai sai lệch có chủ đích, đã ghi
  trong comment mã: panel in Variance của short_range_teleport hai
  lần (bản chữ giữ một), và lỗi sẵn của panel làm hàng Duration mất
  số khi inc > 0 (bản chữ giữ giá trị panel định in).
- Sau chọn: menu thời lượng (uilist) → `study_spell_activity_actor`
  → kết quả qua `add_msg_if_player`; khung tiến độ là cơ chế
  activity chung của R10.

### 3.3 Đường thi triển và selector R11 (rà soát bằng chứng)

`Character::cast_spell` (`src/handle_action.cpp`) từ chối bằng
`add_msg` (điều kiện caster, thiếu năng lượng, choáng, Blood Magic)
rồi giao `spellcasting_activity_actor`; `finish` của actor gọi
`spell::select_target` → `target_handler::mode_spell` dựng
`target_ui` mode Spell — chính selector R11 (node `target:aoe` có
cổng `pl_sees`, node con trỏ, xác nhận bắn đồng minh/bản thân bằng
`query_yn` của engine; hủy nhắm có `query_yn` "Stop targeting? Time
spent will be lost."). Kết quả (fail theo `spell_fail`, message của
phép, lên level) qua log game. Focus trong menu phép không bao giờ
cast: uilist chỉ đặt `ret` cho entry enabled khi CONFIRM.

### 3.4 ⚠️ Lỗi có sẵn của cây nền: dispatch thi triển bị đứt

Tại HEAD, `src/handle_action.cpp` (ACTION_CAST_SPELL /
ACTION_RECAST_SPELL) gọi `cast_spell()` / `cast_spell(true)` trong
`game::do_regular_action`, nhưng **không tồn tại khai báo hay định
nghĩa `game::cast_spell` ở bất kỳ đâu trong cây, và
`known_magic::select_spell` không có caller nào** (đã grep toàn cây
nhiều lần, gồm cả lịch sử 557 commit của nhánh: không thấy định
nghĩa này xuất hiện). Đây là lỗi có sẵn của dòng fork, không do
R17 gây ra và không nằm trong phạm vi adapter; nhưng nó có nghĩa
chuỗi "phím Cast → menu phép" chưa nối được ở cây nền hiện tại —
cần cấp trên/giai đoạn 2 quyết định khôi phục glue của engine
trước khi T52 chạy được đầu-cuối. Mọi bề mặt bộ đọc của hành trình
phép (menu, spellbook, nhắm mục tiêu, kết quả) đã sẵn sàng độc lập
với quyết định đó.

## 4. R17.4 — Máy tính trong game (`src/computer_session.cpp/.h`)

### 4.1 Văn bản terminal thành tài liệu đọc được

Văn bản terminal tích lũy trong member `lines` qua
print_line/print_error/print_text — trước đợt này không tới bộ đọc
ở bất kỳ dạng nào. Helper mới `terminal_text()` dựng lại văn bản
từ chính `lines` (đúng thứ tự hiển thị, giữ indent và dòng trống,
lột thẻ màu) và được công bố qua node tài liệu `computer:text`
(`a11y::append_document`) ở cả hai loại query dưới đây. Mọi
print_text dài đã đối chiếu từng hàm (research, radio archive,
amigara log, bionic manifest, srcf messages...): tất cả đều kết
thúc ở một query_any/query_ynq/query_bool trước `reset_terminal()`
tiếp theo, nên văn bản luôn nằm trong tài liệu của query đó.

### 4.2 `query_ynq` — câu hỏi có tiếng nói

Tạo `a11y::screen_scope` category `computer-query` bound `&ctxt`
(context YESNOQUIT của chính hàm) trước vòng lặp: snapshot từ
`command_snapshot` (node lệnh YES/NO/QUIT — handler
`registered_intent` trả đúng action gốc, "cancel" phân giải về
QUIT), title = tên máy, summary = đúng câu hỏi đã format, kèm tài
liệu `computer:text` khi `lines` không rỗng. Vì scope bound `&ctxt`
nên fallback lệnh chung của `handle_input` không ghi đè. Ý định của
bộ đọc đến với raw event kiểu error — cả hai bộ lọc `allow_key`
(gồm nhánh FORCE_CAPITAL_YN) đều chấp nhận kiểu event đó, nên luật
chữ hoa cho phím vật lý không loại đáp án của bộ đọc. Giới hạn đã
ghi trong mã: snapshot cập nhật một lần trước vòng lặp (câu hỏi
không đổi trong vòng lặp; dòng in thêm trong lúc chờ không công bố
lại cho tới query kế tiếp — trong luồng hiện tại không có print
nào xảy ra trong vòng lặp đó).

### 4.3 `query_any` — chờ phím bất kỳ có đường cho bộ đọc

`inp_mngr.wait_for_any_key()` không đi qua `handle_input` (trên
Android nó tạo context WAIT_FOR_ANY_KEY nhưng không poll), nên ý
định của bộ đọc không bao giờ được phục vụ trong lúc terminal chờ.
Backend mới `query_any_impl(summary)`: khi bộ đọc tắt, giữ nguyên
đường gốc; khi bật, poll một `input_context` cục bộ
`COMPUTER_CONTINUE` đăng ký `ANY_INPUT` + `CONFIRM`, với scope
category `computer-continue` công bố summary (= đúng text vừa in,
hoặc toàn bộ văn bản terminal với dạng không tham số), tài liệu
`computer:text`, và node `computer:continue` mà activate trả
`ANY_INPUT`. Tương đương phím vật lý đã đối chiếu:
`handle_input` trả `ANY_INPUT` cho mọi phím không khớp action đã
đăng ký (cùng cơ chế bionics/popup dựa vào), Enter khớp CONFIRM
toàn cục, event chuột không action bị bỏ qua trước nhánh ANY_INPUT
— cả hai kết quả đều kết thúc vòng chờ như `wait_for_any_key`; chỉ
focus (trả TIMEOUT) giữ vòng chờ mở. Truy vấn không tự chạy lệnh
máy tính: đọc tài liệu/focus không kích hoạt option nào.

### 4.4 Menu chính — hậu quả trước khi chọn ở mức engine công bố

Entry của `computer_menu` trong `use()` nay có desc: khi option có
`security > 0` hoặc `security + comp.alerts > 0` (đúng điều kiện
kích hoạt trong `use()`), desc = "Password required." — đúng chuỗi
`use()` tự in tại điểm kích hoạt. Label giữ nguyên tên option gốc
(+ hậu tố "(UNAVAILABLE)" gốc). `can_activate()` là kiểm tra địa
hình thuần boolean, không có câu lý do nào trong engine/`computer.h`
nên cố ý không đặt `a11y_disabled_reason` — adapter dùng chung
dùng câu mặc định thay vì lý do bịa.

### 4.5 Bề mặt con

- `action_list_mutations`: menu nhóm `ssmenu` không có nguồn mô tả
  nhóm trong dữ kiện (`mutation_category` chỉ có tên/thông điệp)
  nên không bịa desc. Menu chọn mutation `wmenu` nay có
  `addentry_desc` dựng từ chính `mutation_branch`: `desc()` +
  "Changes to:" (replacements) + "Cancels:" — đúng khối in sau khi
  chọn. Phát hiện kèm theo: mọi entry của `wmenu` có `retval = -1`
  nên khối in mô tả sau chọn có vẻ không tới được trong mã hiện
  tại và vòng lặp thoát ngay lần chọn đầu — desc mới hiện là đường
  đọc mô tả duy nhất của menu này; không tự đổi retval (sẽ thay đổi
  luồng gốc), để giai đoạn 2 xác minh.
- Khoảng trống còn lại có bằng chứng: `action_srcf_seal()` in ba
  dòng ("Charges Detonated" / "Backup Generator Power Failing" /
  "Evacuate Immediately") rồi không qua query nào, gọi
  `activate_failure` và quay về `use()` — `reset_terminal()` xóa
  `lines`. Hai dòng đầu mất hẳn với bộ đọc; dòng thứ ba trùng
  `add_msg` "Evacuate Immediately!" nên vẫn nghe được qua hàng lời
  game. Không bịa cách sửa (thêm query sẽ đổi nhịp luồng gốc).

## 5. Hồ sơ và chỗ chưa chắc

- Backlog R17.1–R17.4 → `da-viet-cho-kiem-thu` (test T16–T18,
  T40–T42, T52; bổ sung TX-may-tinh, TX-phau-thuat).
- Sổ hành trình: 4 hành trình `bionics`, `dot-bien`,
  `phep-va-kha-nang`, `may-tinh-trong-game` → đã viết, chờ kiểm
  thử; tổng đếm lại từ dữ kiện: 94 đã viết / 21 chưa viết.
- Chưa biên dịch: chữ ký/helper đã đối chiếu theo tiền lệ cùng cây
  (`screen_scope`/`command_snapshot`/`registered_intent`,
  `addentry_desc`, `list_occupied_bps`, các hàm spell của panel) —
  xác nhận biên dịch thuộc giai đoạn 2. Rủi ro còn lại tập trung ở:
  (a) `game::cast_spell` bị đứt ở cây nền (mục 3.4) — chặn T52
  đầu-cuối cho tới khi glue của engine được khôi phục; (b) chi phí
  dựng snapshot mỗi vòng lặp ở màn bionics/mutations với danh sách
  rất dài chưa đo; (c) `spellbook_callback::refresh` giả định thứ
  tự entry khớp thứ tự `spells` (đúng cách menu duy nhất dùng
  callback này được dựng); (d) chuỗi mới chưa thêm bản dịch vào
  `vi.po` (phạm vi R21), mọi chuỗi đều qua `_()`.
