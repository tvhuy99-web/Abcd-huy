# R14 — NPC, giao dịch, đồng đội và trại (giai đoạn 1, chỉ viết mã)

Trạng thái: **đã viết, chờ kiểm thử** (xem `REMAINING-CODE-BACKLOG.json`
R14.1–R14.4): chưa build, chưa chạy test, chưa tạo APK. Đối chiếu bằng
đọc mã/grep/git diff tại HEAD `fdd8dd81` cộng thay đổi của đợt này.
Test ID sẽ chạy ở giai đoạn 2: T31–T34, T46–T48.

Bốn phần làm song song theo tệp riêng (hội thoại / giao dịch / đồng đội /
trại), coordinator rà soát toàn bộ diff, tự vá các điểm liên tệp mà từng
phần chuyển lên (lý do khóa của entry trại phía `faction_camp`, thông
điệp chọn ô overmap). Nguyên tắc xuyên suốt: dữ kiện công bố luôn lấy từ
chính dữ kiện UI gốc tại điểm dựng; không bịa ETA/tài nguyên/lý do mà
engine không cấp; tổng giao dịch là số của engine tại lúc chốt; duyệt
không bao giờ tự thực thi việc.

## 0. Đính chính tiền đề kiểm kê

`talk_function::start_camp` **định nghĩa tại `src/faction_camp.cpp:738`**
— `npctalk.cpp` chỉ đăng ký `WRAP( start_camp )` trong bảng đáp thoại.
Luồng thành lập trại vì vậy thuộc phạm vi R14.4 và đã được kiểm kê ở đó
(`popup` lý do + `uilist` dùng chung qua `base_camps::select_camp_option`,
không nhập chuỗi/query_yn riêng).

## 1. R14.1 — Hội thoại (`src/npctalk.cpp`, `src/dialogue_imgui.cpp`)

Adapter sẵn có từ ad5ecf0 (scope `dialogue`, node history + node response
theo chỉ số, `response_enabled` đối chiếu `response_condition_eval`) được
giữ nguyên khung; đợt này đóng bốn khoảng trống có bằng chứng:

1. **Người nói.** UI gốc vẽ `dialogue::speaker_name(d_img)` chữ lớn ở
   thanh bên (`dialogue_imgui.cpp` draw sidebar) nhưng snapshot chỉ có
   title "Dialogue" và summary là dòng thoại trơ. Nay title là
   "Dialogue: <tên người nói>" từ chính `speaker_name(d_img)` — bao phủ
   NPC thường (`disp_name`), hội thoại remote/máy tính (`remote_name`);
   khi hàm trả "" (không phải hội thoại) giữ title cũ.
2. **Lịch sử dài.** Tên người nói vốn là một `history_message` riêng đứng
   trước dòng thoại (`add_to_history(text, speaker_name, ...)`) nên đọc
   được ai nói câu nào. Đã kiểm chứng cả `validate_snapshot` (C++) lẫn
   `ReaderSnapshot` (Java) **không có trần độ dài** cho detail/summary —
   lịch sử dài không bị cắt thầm. Hai vá nhẹ: node history chuyển sang
   mẫu tài liệu `a11y::append_document` (giữ nguyên `actions={"focus"}`),
   lịch sử được đọc tươi mỗi lần publish thay vì chụp một lần trước vòng
   lặp; và `get_accessibility_history()` giữ dòng trống tách lượt của UI
   gốc thành `"\n\n"` để trình đọc tài liệu Java tách mỗi lượt thành một
   đoạn điều hướng được. Toàn văn nguyên văn, không dịch, không tóm tắt.
3. **Điều kiện và hậu quả của câu trả lời.** `create_option_line` đã đưa
   trial (`[PERSUADE 42%]`, `[Skill 2/5]`) và `show_reason`/
   `failure_explanation` của tác giả dữ liệu vào chính text của câu — các
   dữ kiện đó đã nằm trong `label`. UI gốc không công bố lý do điều kiện
   cụ thể nào hơn (chỉ tô xám), nên `disabled_reason` giữ câu chung —
   không bịa lý do. Lỗ hổng thật duy nhất: hậu quả hostile/helpless chỉ
   là màu đỏ/cam. Nay node mang `detail` cảnh báo trước khi chọn, lấy từ
   chính `responses[index].get_consequences(*this)` — cùng dữ kiện làm
   `opt_imgui` hỏi `query_yn` "You may be attacked!" sau khi chọn.
4. **Kết quả nhánh.** Nhận/trả nhiệm vụ đã có kênh nghe được (dòng topic
   kế tiếp làm summary mới; `mission::wrap_up/fail` →
   `add_msg_if_player` → `announce_game_message`). Lỗ hổng thật: lời tạm
   biệt khi kết thúc hội thoại chỉ đi qua `npc::say()` — âm thanh thuần,
   không vào log, và snapshot hội thoại đã biến mất. Nay
   `dialogue_imgui.cpp` gọi thêm `a11y::announce_game_message(bye)`
   cạnh `say()` gốc với đúng câu đó.

**Chọn nhầm qua callback cũ — đã chứng minh an toàn, không vá:** cổng
R03 (`poll_input`) chặn token/revision/high-water và tra node lại trong
snapshot hiện hành; handler và vòng lặp CONFIRM cùng kiểm lại điều kiện
trên danh sách hiện hành; đổi topic tạo `screen_scope` mới (token
generation mới); `HELP_KEYBINDINGS` regenerate chỉ gán lại hotkey từ cùng
`responses`, không đổi ánh xạ chỉ số → câu.

## 2. R14.2 — Giao dịch (`src/trade_ui.*`, `src/npctrade.cpp`, hook trong `src/inventory_ui.cpp`)

Nền sẵn có và giữ: hai pane là `trade_selector` (kế thừa selector hành
trang R09); `trade_ui::build_accessibility_summary` đã công bố số đơn
vị, giá trị ước tính hai phía, Credit/Debt, ngân hàng khi CAPITALISM,
pane hiện tại — và được dựng lại mỗi vòng `execute()`.

1. **Giá từng món.** Cột "Unit price" của UI gốc có vào snapshot nhưng
   chỉ là số tiền trần không nhãn trong `detail`, không có cell theo
   hợp đồng bảng R02. Nay `trade_preset::unit_price()` bọc đúng
   `npc_trading::trading_price(_trader, _u, {loc, 1})` (cột hiển thị và
   `get_denial` dùng chung accessor này), và hook trade trong
   `inventory_selector::reader_session::update()` khai báo bảng
   `trade:items` (cột `item`/`unit_price`), mỗi hàng mang `cells` với
   `compare` = cent thô, dòng detail được gắn nhãn "Unit price: …".
2. **Số lượng và chia stack — đã có, không sửa.** Form số lượng R09 áp
   cho pane giao dịch (nháp → `apply` → `toggle_entry(entry, count)`);
   chọn một phần stack qua `get_available_count()`/`set_chosen_count`
   (cả charges lẫn đồ thường). Popup số của phím gốc là
   `string_input_popup` dùng chung (scope `STRING_INPUT`) — không phải
   đường riêng.
3. **Lệnh pane.** Cả 5 action (SWITCH_LISTS, AUTO_BALANCE,
   BANK_BALANCE, CONFIRM=TRADE_OK, QUIT=TRADE_CANCEL) đã là node lệnh
   qua `command_snapshot(_ctxt_trade)` và kích hoạt được qua
   `registered_intent`. Thiếu duy nhất mô tả tác động: `detail` trước
   đây chỉ là mô tả phím (CONFIRM dễ lẫn với chọn đồ). Hook trade nay
   đặt câu tác động trước mô tả phím cho cả 5 lệnh, ghi rõ chốt giao
   dịch là quyết định trao đổi riêng và game còn hỏi lại một lần.
4. **Tổng và giới hạn trước khi chốt.** Mọi đường đổi số lượng đều qua
   `toggle_entry/toggle_entries → on_toggle → recalc_values_cpane`;
   AUTO_BALANCE đi qua `toggle_entry`, BANK_BALANCE tự gọi recalc.
   `max_credit_extended` **không** bị đọc trước — nó chỉ xuất hiện ở
   popup từ chối của `_confirm_trade`, đúng như UI gốc.
5. **Kết quả sau khi chốt — đã vá.** `npc_trading::trade()` vốn im lặng
   hoàn toàn. Nay sau khi áp dụng thật: `add_msg` công bố giá trị hai
   phía lấy thẳng từ `trade_result`, thanh toán/nhận tiền ngân hàng theo
   `delta_bank` kèm cash sau cùng, và số dư nợ sau cùng từ
   `op_of_u.owed` đã cập nhật (bỏ qua khi trao đổi tự do
   `will_exchange_items_freely`). Đường không chốt công bố
   "No trade took place. Nothing was exchanged." — đúng cho mọi đường
   gọi (Trade/Reward/Pay qua `pay_npc`/faction đều qua `trade()`; không
   có đường barter vòng ngoài `trade()`).
6. **Đồ không bán được.** `trade_preset::get_denial` chảy nguyên vẹn qua
   `cache_denial(preset)` → `node.enabled/disabled_reason` của reader
   R09 — đã xác minh bằng đường mã, không sửa.

**Tính lại tại lúc chốt:** tổng không do reader tính — `_balance` của
`trade_ui` được `recalc_values_cpane` tính lại từ `to_trade()` hiện
hành sau mỗi toggle, `_confirm_trade` phán quyết trên chính `_balance`
đó (chấp nhận/giới hạn tín dụng/không đủ túi), và cửa intent R03 bảo đảm
intent chốt chỉ chạy khi node còn thuộc snapshot hiện hành.

## 3. R14.3 — Đồng đội / giao việc (`src/mission_companion.cpp/.h`)

Adapter sẵn có (scope `companion_missions`, tab trại, entry theo chỉ số,
tài liệu `companion:details`) được giữ khung. Đối chiếu dữ kiện:
`entry.text` của engine đã chứa Time/Profit/Danger (việc bắt đầu),
roster từng người `[DONE]`/`[đã qua / mốc]`/`[X left]`,
`Positions: n/N`, `Time:` từ JSON (việc trại và trả người) — reader
publish nguyên qua `detail` + tài liệu chi tiết, không cần bịa ETA.
Mọi bước chọn người (`companion_choose`, `companion_choose_return`, và
đường trại qua `individual_mission`) đều đi qua **uilist dùng chung
R04** — không có UI chọn riêng chưa phủ.

Thay đổi của đợt này:

1. **Điểm giao việc trong summary.** Summary trước đây chỉ là
   `name_mission_tabs(...)` — không phân biệt được đang giao việc ở
   trại/điểm nào. Nay là "<tên trại>: <tab expansion>" qua
   `overmap_buffer.find_camp(omt_pos)` + `camp_name()` cho role trại,
   hoặc "<title>: <tên NPC>" nhờ tham số `point_name` mới (mặc định
   `""`, caller `companion_mission` truyền `p.get_name()`; hai caller
   trại 4-tham-số không phải sửa).
2. **Lỗ hổng chọn nhầm thật — đã vá.** Trước đây activate một entry chỉ
   chọn nó (TIMEOUT); việc thực thi nằm ở intent CONFIRM riêng không
   mang danh tính entry, nên xác nhận chạy theo `sel` hiện hành — cộng
   với việc revision theo nội dung không tăng khi đổi tab cho ra văn
   bản giống hệt nhưng khác `mission_id.dir`, một intent cũ có thể xác
   nhận nhầm việc khác. Nay: activate chỉ trả CONFIRM khi entry được
   activate chính là entry đang chọn (nguyên tử như uilist dùng chung);
   activate entry khác chỉ chọn; `reader_list_changed` ép revision sau
   mỗi lần dựng lại `cur_key_list` do đổi tab; handler tab chặn chỉ số
   khi danh sách rỗng.
3. **Lý do không đủ điều kiện.** UI gốc không có nguồn lý do nào ngoài
   `entry.text` (CONFIRM trên entry khóa im lặng). Entry return bị
   khóa nay có câu lý do tường minh ("No companion on this mission is
   ready to return yet.") prepend vào `disabled_reason` trước roster
   gốc. `companion_choose` nay đặt `a11y_disabled_reason` nêu đúng kỹ
   năng chưa đạt cho người bị khóa.
4. **Trả người/hoàn thành.** Kết quả đi qua `popup`/`query_yn`
   (`query_popup` có scope riêng), `add_msg` (`announce_game_message`)
   và popup vật phẩm/merch ghi rõ — không có lỗ hổng trong phạm vi tệp
   này; phía thực thi trại xem mục 4.4.

## 4. R14.4 — Trại phe (`src/faction_camp.cpp`, `src/overmap_ui.cpp`)

`faction_camp.cpp` trước đợt này có **0 tham chiếu a11y**. Kiểm kê toàn
bộ điểm UI tương tác trong tệp (bảng đầy đủ ở báo cáo kiểm kê đợt này):

| Loại UI | Phủ | Bằng chứng |
|---|---|---|
| `uilist` (~7 điểm) | dùng chung R04.1 | `uilist::query()` tự tạo scope đọc; node theo chỉ số gốc, detail = desc, disabled + lý do |
| `query_yn` (~15 điểm) | `query_popup` | gồm Trip Estimate (khoảng cách/số chuyến/thời gian/food kcal qua `camp_trip_description`) và cảnh báo kcal chế tạo |
| `query_int` (ưu tiên việc) | `string_input_popup` | scope `STRING_INPUT` R04.2 |
| `popup` (~70 điểm) | `query_popup` | lý do chặn khởi hành, kết quả chuỗi, lỗi |
| `add_msg` | `messages.cpp` | `announce_game_message` cho mọi tin nhắn |
| Inventory selector (give/get equipment, hạt giống) | R09 | `inventory_selector::reader_session` |
| `choose_components` (nâng cấp/công sự/ống muối/chế tạo) | uilist R12 | `select_item_component`/`select_tool_component` ghi cần/có + nguồn |
| Chọn ô overmap (`ui::omap::choose_point`) | adapter overmap R13 + vá đợt này | xem 4.3 |
| Hai màn `ui_adaptor` tự vẽ (`worker_assignment_ui`, `job_assignment_ui`) | **trước đây chưa phủ** → adapter riêng đợt này | xem 4.1 |
| `scan_pseudo_items`, `farm_action`, các hàm `*_description` | không có UI | hàm lặng/dựng dữ kiện — kết quả hiện qua entry mission và `*_return` |

### 4.1 Hai adapter riêng mới (mẫu `armor_layers`/`faction_ui`)

- **`worker_assignment_ui` → scope `faction-camp-workers`.** Trước đây
  vòng lặp chỉ nhận fallback lệnh chung của `input_context` — publish
  danh sách *lệnh*, không bao giờ publish danh sách follower. Nay
  snapshot có summary "Camp: <tên>. Followers: <n>." và node
  `camp-worker:<character_id>` cho từng follower: label = `disp_name()`,
  detail = trạng thái phân trại ("Already assigned to this camp." /
  "Assigned to another camp." / "Currently on a companion mission.");
  danh sách rỗng có node status mang đúng câu UI gốc. Handler tra lại
  follower theo `character_id` trong danh sách hiện tại, đồng bộ cả
  `selection` lẫn `cur_npc` (vòng lặp gốc hành động trên con trỏ cache
  này ở nhánh CONFIRM/INSPECT_NPC); focus trả `TIMEOUT`, activate trả
  `CONFIRM` qua vòng lặp gốc — **duyệt không gán ai**, việc gán vẫn do
  `talk_function::assign_camp` gốc thực hiện.
- **`job_assignment_ui` → scope `faction-camp-jobs`.** Node
  `camp-job-worker:<character_id>` cho từng cư dân `stationed_npcs`;
  detail của mỗi cư dân là nguyên bảng "verb : priority" mà pane gốc in
  cho người đang chọn (từ `guy->job.get_prioritised_vector()` /
  `get_priority_of_job`), hoặc "No current job." — publish cho mọi cư
  dân không lộ thêm dữ kiện nào vì cùng dữ kiện đó hiện ra khi chọn
  từng người và đều là cư dân của trại. Activate → CONFIRM gốc → menu
  ưu tiên (uilist đã phủ) → `query_int` (đã phủ). Duyệt/focus không đổi
  ưu tiên nào.

### 4.2 Dữ kiện quyết định bổ sung tại các menu dùng chung

- `player_eats_meal`: entry nay có desc "Costs %d kcal. Food stored:
  %d kcal." — chi phí bằng chính công thức của hàm (`ret*750-250`),
  trữ lượng đọc từ `food_supply().kcal()` trước menu.
- `choose_new_leader`: desc của mỗi cách chọn là chính văn bản giải
  thích của `query_yn` xác nhận tương ứng trong cùng hàm (trước đây chỉ
  nghe được sau khi đã chọn).
- `start_crafting` → `choose_crafter`: `a11y_disabled_reason` riêng
  (thiếu kỹ năng/thiếu proficiency/đang bận) thay câu chung, và `text`
  của menu nêu món chế + số mẻ. Cố ý **không** bật `allow_disabled`:
  hàm không kiểm lại `can_be_picked` sau menu, cho activate entry
  disabled sẽ thành đường vòng qua điều kiện.
- `recruit_return`: "Increase/Decrease Food" nay disabled đúng tại
  trần/sàn mà handler cũ áp lặng lẽ, kèm lý do; overview (chỉ số, food
  theo ngày, % cơ hội) vẫn là summary.
- Entry `Camp_Abandon` trong `get_available_missions`: thêm cảnh báo hệ
  quả tức thì/không hỏi lại (theo mẫu WARNING của Emergency Recall
  ngay bên dưới), vì `abandon_camp()` thực thi ngay khi chọn: gọi về
  companion đang làm việc, cư dân ngừng gác, trại bị xóa.
- `start_farm_op`: hết kết thúc lặng lẽ khi `plots_cnt == 0` — popup
  đúng theo tác vụ (plow/plant/harvest) bằng từ vựng ô đất của
  `farm_description`.

### 4.3 Lý do khóa của entry việc trại và thông điệp chọn ô (vá liên tệp của coordinator)

Bốn nhánh dựng text trong `get_available_missions_by_dir` để entry bị
khóa mà không một chữ lý do — phía reader không có dữ kiện gốc nào khác
để bổ sung:

- **Fortifications (cả hai mức trench):** khi đã có người làm, text vẫn
  là mô tả công sự thuần. Nay nối danh sách người đang làm kèm
  `[DONE]`/`[X left]` (helper `append_workers_in_progress`, cùng định
  dạng `update_time_left` dùng) vào text của bản start khi bị khóa.
- **Survey Expansion:** khi `possible_expansion_found == false`, nối
  câu "No suitable location for a new expansion was found around this
  camp." vào entry.
- **Plant bị khóa vì lạnh:** `warm_enough_to_plant(...)` được giữ lại
  thành biến; khi có ô mà không đủ ấm, nối chính câu lý do của engine
  (`can_plant.c_str()` — cùng câu `field_plant` bật popup) vào entry.
- **Crafting khi đủ 3 người đang chế:** trước đây entry **biến mất
  hẳn** khỏi danh sách. Nay publish entry khóa với cùng
  `DUMMY_RECIPE_REPLACED_IN_CAMP` mà `add_available_recipes` dùng, kèm
  lý do và danh sách người đang chế — entry khóa không bao giờ khởi
  chạy được (CONFIRM trên entry `possible=false` là no-op đã kiểm
  chứng ở R14.3).

Và khoảng trống ở màn overmap: tiêu chí chọn ô của `choose_point`
("Select a forest (or road/trail) from %d to %d tiles away."…) chỉ được
*vẽ* ở góc màn hình qua `g->overmap_data.message`; `reader_snapshot`
của overmap không publish trường này. Nay summary của snapshot overmap
nối thêm `overmap_data.message` khi khác rỗng — mọi điểm vào overmap
đều reset `overmap_data` trước và chỉ `choose_point` mới đặt message,
nên duyệt bản đồ thường không bị ảnh hưởng. Việc này phục vụ cả các
luồng trại (cut logs/clearcut/hide site/fortifications/combat patrol/
survey) lẫn mọi caller `choose_point` khác.

### 4.4 Kết quả chuỗi hậu cần

Đã rà từng hàm `*_return`/`finish_return`: mọi kết thúc đều qua kênh đã
có adapter — `finish_return` mở đầu bằng `popup("%s %s", tên, return_msg)`;
`upgrade_return` (va chạm xe → popup), hai hàm salt pipe (không thấy
pipe → popup lỗi), `gathering_return` (nguy hiểm → chuỗi popup của
`survive_random_encounter`, kể cả khi companion chết),
`combat_mission_return` (companion không về → popup + ghi note bản đồ),
`recruit_return` (chuỗi popup gặp/thuyết phục/kết quả),
`distribute_food` (popup kcal đã phân phối / lý do thất bại kèm số item
bị bỏ qua / thiếu zone), `abandon_camp` (`add_msg` qua log). Các
`return false` còn lại của `upgrade_return` là trạng thái nội bộ không
nhất quán (thiếu expansion/worker khớp parameters, lỗi dữ liệu recipe)
— entry return chỉ được dựng khi có worker đang làm việc đó, nên không
bổ sung popup để tránh báo sai trong luồng nâng cấp nhiều người hợp lệ.

Dữ kiện trại trong entry mission (không bổ sung con số nào ngoài nguồn
này): chi phí nâng cấp (nguyên liệu/dụng cụ/kỹ năng/thời gian) từ
`om_upgrade_description`; chi phí kcal của nâng cấp so với
`food_supply().kcal()` nối tại `get_available_missions_by_dir` và khóa
entry khi thiếu; trữ lượng lương thực ở entry Distribute Food; chi phí
chuyến đi ở Trip Estimate. Riêng loại trại/expansion khi thành lập:
recipe tạo trại **không có chi phí nguyên liệu trong engine**
(`start_camp` chỉ kiểm tra va chạm mapgen rồi `define_camp`) nên không
có chi phí nào để công bố — mô tả recipe group là toàn bộ dữ kiện quyết
định engine có tại đó.

## 5. Chỗ chưa chắc / còn mở (ghi rõ, không che)

1. **Chưa biên dịch** — chữ ký/API đã đối chiếu theo tiền lệ cùng cây
   (`screen_scope`/`command_snapshot` theo `mission_companion.cpp`/
   `armor_layers.cpp`; `addentry_desc`/`a11y_disabled_reason` theo
   `uilist.h` và `character_body.cpp`; `find_camp`/`camp_name` theo
   `overmapbuffer.h`/`basecamp.h`; `ret_val::c_str` theo
   `mission_companion.cpp` `field_plant`; hook trade theo hợp đồng R02
   và cột preset thật). Xác nhận biên dịch thuộc giai đoạn 2.
2. **`zone_manager_ui` chưa có adapter nào** (grep a11y = 0): bước *tạo*
   zone khi `set_sort_points` gọi `zone_manager_ui::display_zone_manager`
   (chọn vùng trên bản đồ) chưa đọc được. Đây là màn của hệ vùng — thuộc
   hành trình vùng của **R16**; luồng trại tới đó luôn đi qua
   popup/query_yn đã phủ trước và sau (phải tạo camp food zone + camp
   storage zone; `distribute_food` báo thất bại bằng popup khi thiếu
   zone). Ghi vào sổ hành trình trại để R16 đối chiếu, không tính là đã
   phủ ở R14.
3. **Thay đổi ngữ nghĩa activate ở companion missions** (activate entry
   đang chọn = xác nhận ngay, như uilist): cần bài kiểm thử giai đoạn
   2/3 xác nhận người dùng không bị "xác nhận sớm" khi chỉ định chạm
   vào entry đang focus để nghe lại — đường nghe lại an toàn là
   focus/detail.
4. **Chi phí `get_consequences` trong hội thoại:** được gọi thêm mỗi lần
   publish cho từng response để lấy cảnh báo hostile/helpless; nếu giai
   đoạn 2 đo thấy nặng ở topic rất nhiều response, cân nhắc chỉ tính cho
   response đang hiển thị. Chưa đo được ở giai đoạn 1.
5. **Quan sát hành vi engine, không sửa** (ngoài phạm vi publish thụ
   động): `start_menial_labor` bỏ qua kết quả `validate_sort_points()`
   (vẫn gán việc sort khi người chơi từ chối tạo zone); `start_crafting`
   hỏi `query_yn` chi phí kcal *trước* khi kiểm tra đủ kcal. Cả hai là
   logic gốc cho mọi người chơi.
6. `worker_assignment_ui`: vòng lặp gốc tự lập chỉ mục
   `followers[selection]` không kiểm tra biên khi danh sách co lại —
   lỗi gốc có từ trước; hai adapter mới không dựa vào giả định đó
   (node theo `character_id`, intent tra lại theo ID). Không sửa ở đợt
   này.
