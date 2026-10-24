# R15 — Phương tiện, hàng hóa và lái xe

Giai đoạn 1 (chỉ viết mã, không build/test). Trước đợt này cả ba tệp
`veh_interact.cpp`, `vehicle_use.cpp`, `vehicle_display.cpp` có 0 điểm
`a11y::`. Đợt này viết mới adapter cho màn tương tác xe, mở rộng trạng
thái lái trong trạng thái thế giới, và viết adapter form cho smart
controller. Mọi thao tác thay đổi xe vẫn chạy qua logic gốc của
`veh_interact`; bộ đọc chỉ công bố và đồng bộ vị trí duyệt.

## 0. Ranh giới quyền biết (kiểm chứng bằng mã)

- Không có cơ chế che giấu dữ kiện cargo/nhiên liệu theo vị trí
  trong/ngoài xe: `game::examine` gọi `vehicle::interact_with` mà
  không kiểm tra `is_inside`, khóa hay chủ sở hữu cho phần *dữ kiện*;
  `display_stats`/`calc_overview` chạy vô điều kiện. Cổng duy nhất là
  `handle_potential_theft` bọc quanh các *hành động* thay đổi xe
  (INSTALL/REPAIR/MEND/REFILL/REMOVE/SIPHON/UNLOAD) — giữ nguyên, bộ
  đọc không đi vòng qua nó vì mọi xác nhận đều trả về vòng lặp gốc.
- Vì vậy adapter công bố đúng những gì các pane của `veh_interact` vẽ:
  không thêm dữ kiện, không bớt. Hai suy diễn duy nhất là *tương đương
  lời* của dữ kiện vốn chỉ có màu/ký hiệu, ghi rõ ở mục 1.

## 1. R15.1 — Sơ đồ xe (`veh_interact::reader_session`, scope `vehicle-interact`)

`reader_session` là lớp lồng trong `veh_interact` (tiền lệ
`advanced_inventory::reader_session` của R09), giữ `a11y::screen_scope`
gắn vào `main_context`, được tạo trong constructor và cập nhật sau mỗi
`ui_manager::redraw()` của từng vòng lặp. Chế độ chính (main) công bố:

- **Summary** dựng từ đúng các giá trị `display_stats` in: tên xe,
  `total_durability_text`, khối lượng, cargo đã dùng/tổng, tốc độ an
  toàn/tối đa (khi là xe mặt đất như điều kiện `is_ground` của pane),
  và phần trăm từng loại nhiên liệu tính bằng
  `fuel_left/fuel_capacity` trên `get_printable_fuel_types` — cùng các
  số mà thước đo `E…F` mã hóa (thước đo tự nó là đồ họa nên không đọc
  ký tự thước).
- **Cây ô → bộ phận**: các hàng của overview (vốn gom theo nhóm chức
  năng) được gom lại theo ô mount; nhãn ô là vị trí tương đối trong
  khung xe (gốc là part 0 — cùng gốc `coord_translate` dùng; quy ước
  khung ở `vehicle.h`: +x là phía trước, +y là bên phải). Detail của
  mỗi hàng gồm: tên nhóm chức năng của overview, **tình trạng bằng
  lời** và dữ kiện theo nhóm sao từ chính các callback `details` của
  `calc_overview`:
  - tình trạng: UI gốc chỉ có ký hiệu hư hỏng + màu. Tương đương lời
    dùng cùng phân số `damage_percent()` và cùng thang 5 mức/từ mà
    `count_durability()` áp cho toàn xe (like new / dented / battered
    / wrecked / destroyed), cộng `broken` khi `is_broken()`;
  - động cơ: nhiên liệu + đang dùng hay không + tên/mô tả từng fault
    (trước đây chỉ hiện ở cửa sổ thông báo khi chọn);
  - bình/kho nhiên liệu: nội dung + lượng (`item_measure_prefix` cho
    tank, lít cho fuel store), cờ frozen/rotten, rò rỉ;
  - pin: dung lượng + % sạc (cùng số header `3_BATTERY` in);
  - lò phản ứng/tháp pháo: loại đạn + số lượng; tháp pháo đa băng
    (firing requirements) có trạng thái từng ngăn qua
    `turret_query(...).multimag_display_state()` như callback
    multimag của overview;
  - ghế: tên người được phân công (`crew()`).
  Part không thuộc nhóm overview nào (khung, tấm giáp…) không có hàng
  overview trong UI gốc nên cũng không có trong cây; chúng xuất hiện
  đầy đủ ở danh sách ô con trỏ (dưới đây) — đúng phạm vi UI gốc.
- **Ô con trỏ và các part tại ô** (`vehicle:cursor`,
  `vehicle:here:*`): bản sao dữ kiện của `print_part_list` +
  `print_vparts_descs`: tên (không tiền tố ký hiệu), tình trạng, phụ
  chú nhiên liệu/pin, cargo (mục 3), trong/ngoài xe, nhãn ô, và mô tả
  part (`format_description`) kèm ghi chú degradation.
- **Hai node trạng thái** `vehicle:most-damaged` /
  `vehicle:needs-repair` sao đúng hai dòng cùng tên của pane stats.
- `vehicle:message` công bố `msg` của vòng lặp khi có (đây là kênh
  các `do_*` báo lý do không làm được, ví dụ "Cannot install any part
  here.").

Duyệt không thay đổi xe: focus/activate trên node ô/part chỉ gọi
`move_cursor` bằng đúng công thức `overview()` dùng để đồng bộ con
trỏ tới một part — không bắt đầu task nào, không di chuyển xe.

## 2. R15.2 — Lắp / sửa / tháo / chọn part

Cùng `reader_session`, bốn chế độ con khớp bốn vòng lặp con:

- **Install**: node cho từng ứng viên trong `tab_vparts` của tab hiện
  tại (summary nêu tên tab). Node đang chọn mang detail là nguyên văn
  `msg` mà `update_part_requirements` đã tính (yêu cầu kỹ năng/dụng
  cụ/vật tư, thời gian, lý do không lắp được, mô tả part) và `enabled`
  đúng cờ `can_install` mà vòng lặp dùng để gác CONFIRM
  (`update_part_requirements` được gọi lại khi dựng snapshot — hàm này
  chỉ đọc + ghi `msg`, idempotent trong vòng lặp). Intent được tra
  lại theo con trỏ `vpart_info` trong `tab_vparts` hiện hành trước
  khi đặt `install_info->pos`; activate trả `CONFIRM` để vòng lặp gốc
  tự kiểm tra lại và chạy các `query_yn` tình huống (mất khả năng bay,
  mất gấp gọn) rồi `assign_activity`.
- **Repair**: node theo `need_repair`; vị trí đồng bộ qua thành viên
  mới `repair_pos` (trước đây là biến cục bộ của `do_repair` — đổi
  thành viên để adapter đồng bộ được, hành vi không đổi). Node đang
  chọn mang detail là `msg` yêu cầu của chính vòng lặp (gồm yêu cầu
  nâng/jack qua `calc_lift_requirements`). Activate trả `CONFIRM`,
  vòng lặp kiểm tra lại `ok` như cũ.
- **Remove**: node theo `parts_here`; vị trí đồng bộ qua
  `remove_info->pos` (trước đây biến cục bộ `pos` của `do_remove`
  không dùng trường này — nay dùng, hành vi không đổi).
  `can_remove_part` được gọi cho từng part để công bố
  `enabled`/`disabled_reason` đúng lý do engine (part chưa tự do,
  phụ thuộc…); `msg` được lưu/khôi phục quanh lúc dựng để pane không
  đổi. Activate trả `CONFIRM`, vòng lặp kiểm tra lại lần nữa.
- **Select part** (dùng chung cho `do_mend`, `do_refill`, `do_siphon`,
  `do_assign_crew` và hàm tĩnh `veh_interact::select_part`): cây như
  chế độ chính nhưng hàng lấy từ `overview_opts` với `selectable` của
  vòng `overview()`; focus đặt `overview_pos` (vòng lặp tự đồng bộ
  con trỏ + thông báo của part), activate trả `CONFIRM` để callback
  gốc chạy. **Đây là nhánh bình chứa xe còn thiếu của R10**:
  `handle_liquid` → `handle_vehicle_target` → `select_part` (và nhánh
  lọc nước trong tank ở `vehicle_use.cpp`) nay có adapter; trường hợp
  chỉ có đúng một bình khớp thì engine tự chọn, không mở UI — giữ
  nguyên.
- Mọi intent part đều tra lại part theo chỉ số trong xe sống
  (`get_all_parts`) trước khi tác động; part đã mất thì intent không
  được xử lý và dispatcher R03 trả biên lai như các intent stale khác.
  Kết quả thật đến từ `vehicle_activity_actor` và log như cũ.

`do_relabel`/`do_rename` đi qua `string_input_popup` dùng chung (R04)
nên không cần adapter riêng; `do_unload` (nhiên liệu rắn) là activity
gốc thông báo qua log.

## 3. R15.3 — Cargo và nhiên liệu

- Trong màn `veh_interact` (UI gốc không có lệnh mở cargo, chỉ in
  tổng volume), mỗi part `VPFLAG_CARGO` có `reader_cargo_data`:
  volume đã dùng/tổng như `print_part_list` in, cộng danh sách món
  theo tên hiển thị — tối đa 8 món đầu và số món còn lại được đếm
  đúng (không cắt thầm), lấy từ `vehicle_stack` của chính part.
- Chuyển đồ thật giữa người ↔ đất ↔ xe dùng **pane xe của kho nâng
  cao R09**: `advanced_inv_area` không có location `AIM_VEHICLE`
  riêng — xe là cờ `in_vehicle` trên pane của 9 ô quanh + ô kéo
  (`AIM_DRAGGED`); `reader_session` của R09 đã công bố pane xe
  ("Vehicle storage.", tổng volume/weight gồm cả phần xe) và chuyển
  qua `MOVE_VARIABLE_ITEM` của engine. Không viết đường chuyển thứ
  hai.
- Nhiên liệu/điện: % tổng theo loại ở summary (mục 1), chi tiết từng
  bình/pin ở detail của part; đổ nhiên liệu qua chế độ select part
  (mục 2) + selector nguồn của engine (`inv_map_splice`); hút nhiên
  liệu (siphon) qua select part + `handle_liquid` gốc; nhiên liệu rắn
  qua `do_unload` gốc.

## 4. R15.4 — Lái, cruise, remote, smart controller

- **Vào/ra lái** giữ nguyên đường gốc: `game::control_vehicle` (cất
  vũ khí hai tay qua `query_yn`, `start_engines`, đặt
  `controlling_vehicle`) và menu tương tác xe `veh_menu` — vốn là
  `uilist` nên adapter dùng chung R04 phục vụ toàn bộ các mục: Examine
  vehicle, nổ/tắt máy, Stop driving, Let go of controls, Pull
  handbrake, Control individual engines, các chế độ tháp pháo,
  Smart controller settings, và Stop controlling khi remote.
- **Lái từng lượt** không cần adapter riêng: các action di chuyển mà
  bộ đọc gửi từ snapshot thế giới được `handle_action` định tuyến —
  `remote_controlling` (xe RC, cần radiocontrol/bionic remote đang
  bật) → `rcdrive`; có quyền điều khiển xe → `pldrive` (gồm cả xe
  thật qua `remoteveh()`). Nguồn này **không có trường cruise_on
  riêng**: `cruise_velocity` là tốc độ đích mà chính lệnh tăng/giảm
  tốc điều chỉnh trong `vehicle::pldrive`; trạng thái công bố là
  cruise text của HUD ("đích < hiện tại", đơn vị).
- **Trạng thái lái công bố một nguồn duy nhất**: khối xe trong
  `player_status()` (`src/a11y_world.cpp`, node `world:status`) được
  mở rộng — tên xe, mode `Driving`/`Remote control` (phân biệt theo
  `g->remoteveh()`), azimuth, cruise text, % nhiên liệu và % pin của
  sidebar (`display::vehicle_fuel/battery_percent_text_color`), và
  mức gắng sức động cơ bằng lời suy từ cùng giá trị `vehicle::strain`
  và cùng các ngưỡng màu của `display.cpp` (0 / 0,2 / 0,4). Không tạo
  nguồn trạng thái thứ hai ở nơi khác.
- **Vuốt đi bộ không bao giờ thành ga**, hai lớp độc lập: engine chỉ
  đặt `swipe_movement_allowed` khi đi bộ thật (R00 — chặn đang lái,
  đang ở trong xe, cả hai đường remote); phía Java khi
  `worldMovementEnabled()` là false thì vuốt rơi về các ánh xạ điều
  hướng của chính người dùng (navigate/detail/read — không ánh xạ nào
  gửi intent gameplay). Muốn lái phải kích hoạt tường minh lệnh
  hướng trong danh sách lệnh. Trạng thái mode nằm trong `world:status`
  nên người dùng nghe được mình đang ở mode nào trước khi ra lệnh.
- **Smart controller** (`src/smart_controller_ui.cpp`): màn tự vẽ
  duy nhất của hệ xe ngoài `veh_interact`. Adapter công bố ba trường
  form R02: `smart:enabled` (bool, có choices), `smart:battery_lo`
  (int, miền 5–90, bước 5), `smart:battery_hi` (int, miền 10–95,
  bước 5), hai ngưỡng `depends_on` lẫn nhau, cộng node mở hướng dẫn
  sử dụng (activate trả `CONFIRM` như phím gốc). Ngữ nghĩa ngưỡng lấy
  từ chính logic `vehicle_move.cpp`: lo là giới hạn cứng (cấm xả pin
  cho động cơ điện), hi là giới hạn mềm (sạc hướng tới hi, hạn chế
  xả). Sửa theo hợp đồng form: `set_value`/`select_choice` chỉ ghi
  nháp, `apply` mới ghi qua đúng các reference mà slider gốc ghi,
  với cùng phép coupling (nâng lo đẩy hi lên, hạ hi kéo lo xuống,
  luôn cách nhau ≥ 5); `cancel_draft` bỏ nháp. Ngoại lệ duy nhất:
  activate trên trường Enabled là chính CONFIRM của màn gốc — chuyển
  trạng thái ngay như phím thật. Khi áp dụng, caller gốc
  (`build_interact_menu`) tự đồng bộ `enabled` sang các part
  SMART_ENGINE_CONTROLLER như cũ.

## 5. Chuỗi và hồ sơ

- Chuỗi mới của bộ đọc có bản dịch Việt trong
  `lang/po/accessibility/vi.po` (chỉ chuỗi câu/nhãn mới; các từ đơn
  chung đã có ở catalog chính).
- Backlog R15.1–R15.4 → `da-viet-cho-kiem-thu` (test T32–T34, T44,
  T49–T50, T54). Sổ hành trình: 4 hành trình xe chuyển đã viết. Đếm
  lại từ dữ kiện tệp sau đợt này: 86 đã viết / 29 chưa viết. Lưu ý:
  đếm lại tại HEAD của R14 cho 82/33 — con số 76/39 trong báo cáo
  R14 không khớp tệp; chênh lệch có sẵn từ trước đợt này, để R23 đối
  chiếu sổ sách (đợt R15 chỉ lật đúng 4 hành trình xe, kiểm chứng
  được trong diff).

## 6. Chỗ chưa chắc

1. Chưa biên dịch — chữ ký đã đối chiếu tại điểm dùng gốc trong cùng
   cây (`turret_query`/`multimag_display_state` theo callback
   multimag, `item_measure_prefix` theo `tank_details`,
   `vpart_position::get_label/is_inside` theo `print_part_list`,
   `format_description` theo `do_repair`); xác nhận biên dịch thuộc
   giai đoạn 2.
2. `veh_app_interact` (thiết bị gia dụng dạng xe) không nằm trong
   nguồn R15 của kế hoạch: menu của nó là `uilist` (đã phủ bởi
   adapter dùng chung) nhưng phần header thông tin thiết bị vẽ bằng
   ImGui chưa được công bố — ghi nhận là bề mặt kề, để R23 đối
   chiếu, không tính là đã phủ ở R15.
3. Chi phí dựng snapshot: cây part dựng lại mỗi nhịp poll của các
   vòng lặp `veh_interact` (xe lớn hàng trăm part, detail cargo liệt
   kê món) — có khử trùng nội dung ở tầng runtime, nhưng chưa đo
   thực tế; nếu cần, giai đoạn 2 có thể cân nhắc giới hạn tần suất
   như đã làm với tiến độ hoạt động ở R10.
4. Node part ở chế độ chính không đánh dấu `selected` theo hàng
   overview (overview ở chế độ chính chỉ là hiển thị, không có hàng
   chọn); focus mặc định là node ô con trỏ hoặc thông báo của vòng
   lặp. Hàng đang chọn chỉ có nghĩa ở các chế độ con.
