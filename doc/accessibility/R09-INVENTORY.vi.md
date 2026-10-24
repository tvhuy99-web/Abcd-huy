# R09 — Hành trang, túi, kho hai phía và lớp mặc (giai đoạn 1, chỉ viết mã)

Trạng thái: **đã viết, chờ kiểm thử** (xem `REMAINING-CODE-BACKLOG.json`).
Chưa build, chưa chạy test, chưa tạo APK trong đợt này.
Test ID sẽ chạy ở giai đoạn 2: T18, T24, T31–T35, T41, T47, T50.

Nguyên tắc xuyên suốt của mục này: danh tính vật phẩm là `item_location`
(kho nâng cao, selector) hoặc iterator của `std::list` (lớp mặc), không
bao giờ là tên hay chỉ số hiển thị; mọi intent đều được handler tra lại
theo danh tính đó trong dữ liệu hiện tại tại lúc nhận (cổng R03); duyệt/
focus/toggle chọn là thao tác trong màn không tốn lượt, còn hành động
gameplay luôn quay về đúng đường action gốc của engine để engine tự
kiểm lại sức chứa, quyền sở hữu và điều kiện khi chốt.

## 1. R09.1 — Selector hành trang (`inventory_selector::reader_session`)

Phần đã có từ trước và **giữ nguyên** sau rà soát: binding
`(column, item_location, "item:<serial>")`; `focus` đặt highlight và trả
`TIMEOUT`; `activate` đặt `navigation_mode::ITEM` rồi trả chuỗi
activation của phiên (`CONFIRM`/`TOGGLE_ENTRY`); `value` có số đã
chọn/sẵn có; `enabled`/`disabled_reason` từ `entry->cache_denial(preset)`
— denial của preset dùng chính `can_contain` của pocket/holster nên lý
do từ chối động (không vừa, sai loại...) có nguồn thật, không bịa.

Nối thêm ở đợt này:

- **Form số lượng cho multi-select.** Các phiên `TOGGLE_ENTRY` mà owner
  là `inventory_multiselector` (multidrop, pickup, trade...) publish
  mỗi hàng theo hợp đồng form R02: `field_type="int"`, miền
  `0..get_available_count()`, bước 1, `applied_value` = `chosen_count`.
  `set_value` chỉ lưu nháp vào binding, `cancel_draft` bỏ nháp, `apply`
  mới chốt: tra lại entry theo location (đã biến mất → từ chối), kẹp
  trong miền sống, rồi đặt qua API công khai
  `inventory_multiselector::toggle_entry(entry, count)` — API này bọc
  `set_chosen_count` (protected, phiên ở lớp cơ sở không gọi trực tiếp
  được) và kích hoạt `on_toggle()` để các pane phụ thuộc (ví dụ trade)
  tính lại. Nháp vượt miền sống mới (số sẵn có co lại) bị bỏ thay vì
  publish ra ngoài miền khiến cả snapshot bị validate từ chối. Phiên
  `CONFIRM` không có form: số lượng của chúng thuộc popup số của luồng
  gọi. **Loại trừ có chủ ý:** `inventory_compare_selector` cũng dùng
  `TOGGLE_ENTRY` nhưng `chosen_count` của nó là cờ 0/1 của cặp so sánh
  do chính nó quản lý; đưa form số qua API multiselector sẽ phá cặp so
  sánh, nên nó giữ hành vi checkbox như cũ.
- **Phân biệt trùng tên:** `describe()` của location chỉ nêu vật chứa
  trực tiếp, nên detail có thêm dòng chuỗi vật chứa rút gọn dựng từ
  `parent_item()` (tối đa 4 tầng) — hai stack cùng tên ở hai túi khác
  nhau nghe khác nhau.

Đã rà soát và xác nhận tới được, **không nối thêm**: favorite là lệnh
`TOGGLE_FAVORITE` và filter là `INVENTORY_FILTER`/`RESET_FILTER` — cả
ba đăng ký trong constructor của selector nên `command_snapshot`
publish thành node lệnh, và vì `focus` của reader đặt highlight trước,
chúng tác động đúng mục đang đọc. Sort của selector là tự động theo
category/tên trong chính UI gốc (không có menu sort riêng để nối).
Compare đi qua hai lượt `inventory_compare_selector` cộng màn item
info sau `EXAMINE`. Nạp/tháo (reload/unload) từ hành trang đi qua menu
hành động vật phẩm dạng `uilist` — adapter menu dùng chung R04 — chứ
không qua selector này.

## 2. R09.2 — Cây vật chứa/pocket (cùng `reader_session`)

Cây dựng bằng hợp đồng cây R02 từ quan hệ thật của engine:
`location.has_parent()` / `parent_item()` cho ra vật chứa, đối chiếu
theo đẳng thức `item_location` trong các hàng đang publish của **mọi
cột đang hiển thị**. Hàng không có cha là gốc (depth 0); hàng có cha
thật nhưng cha không được publish (bị lọc/cột ẩn) giữ phẳng
(`depth=-1`, không bịa cha); độ sâu tính bằng thuật toán đi ngược chuỗi
cha có chống vòng, trần `a11y::max_tree_depth` (16). Con của vật chứa
đang thu gọn vốn vắng khỏi `get_entries` nên không bao giờ được
publish — adapter không tự đi bộ contents để lộ thêm. `has_children`
= có con đang publish hoặc `entry->contents_count > 0`; `expanded` =
có con đang publish. `position`/`total` tính theo nhóm anh em cùng cha
đúng hợp đồng; hàng phẳng giữ position/total phẳng.

Node pocket: mỗi pocket chứa đồ của một vật phẩm đang publish
(`item::get_container_pockets()`) sinh một node con
`pocket:<id vật phẩm>:<thứ tự>`, role `status`, actions chỉ `focus`
(đọc không tốn lượt; action khác bị từ chối ở handler). Detail chỉ
gồm dữ kiện được phép biết: tên/mô tả pocket, thể tích và khối lượng
**còn trống trên tối đa** (`remaining_volume()/volume_capacity()`,
`remaining_weight()/weight_capacity()`), giới hạn độ dài vật phẩm, và
các cờ hạn chế (watertight/airtight/rigid/holster) — đây là các lý do
tĩnh khiến một vật bị từ chối khi nhét vào. **Pocket đang niêm phong
(`sealed()`) chỉ công bố đúng một câu đã niêm phong: không đếm, không
liệt kê contents.** Pocket không niêm phong mới được ghi số stack bên
trong (`item_pocket::size()`); không đệ quy sinh node cho contents —
contents khi được mở đã là các hàng của chính selector. Pocket chỉ
treo cây khi vật phẩm chủ là node cây; dưới vật phẩm phẳng nó giữ
phẳng để không phá hợp đồng độ sâu. Việc cất/lấy đồ trong túi đi qua
các hàng và form số lượng ở mục 1, không qua node pocket.

Đường mã với đồ lồng hai tầng: balo (gốc, depth 0) → túi trong balo
(depth 1, `parent_id` = id balo, detail có chuỗi "túi > balo" rút gọn
theo chiều cha trực tiếp đi lên) → vật trong túi (depth 2); node pocket
của balo ở depth 1 và của túi ở depth 2, cùng nhóm anh em với các hàng
con của vật chủ khi tính position/total. Thu gọn túi thì vật bên trong
biến khỏi snapshot nhưng túi vẫn `has_children=true, expanded=false`.

## 3. R09.3 — Kho nâng cao (`advanced_inventory::reader_session`, mới)

Lớp phiên mới trong `src/advanced_inv.cpp` (khai báo lồng trong
`advanced_inv.h` đúng mẫu inventory), tạo trong `display()` ngay sau
`input_context` và trước vòng lặp (hủy ngược thứ tự, gồm cả đường
thoát sớm khi hết moves), gọi `update()` ngay trước
`ctxt.handle_input()`. Category scope: `advanced-inventory`.

Snapshot: summary nêu pane nguồn/đích; hai node trạng thái pane
(nguồn/đích) gồm tên area, mô tả theo view (`desc[in_vehicle?1:0]`,
kèm "Vehicle storage."/"Ground."), tổng thể tích/khối lượng của cả ô
(`volume+volume_veh`, `weight+weight_veh`), `Items: n of max_size` khi
có giới hạn, và container đang xem (`pane.container`) nếu có; sau đó là
node cho từng `advanced_inv_listitem` của pane nguồn, binding theo
`item_location` đầu tiên của listitem (không dùng chỉ số hiển thị),
value = stacks + volume + weight của stack, detail =
`location.describe()` cộng cờ auto-pickup; cuối là các node lệnh từ
`command_snapshot` (MOVE_SINGLE_ITEM, MOVE_VARIABLE_ITEM,
MOVE_ITEM_STACK, MOVE_ALL_ITEMS, EXAMINE, FILTER, SORT, ITEMS_*,
TOGGLE_TAB, QUIT...).

Handler: `focus` trên vật phẩm tra lại listitem trong
`panes[src].items` hiện tại theo location đã binding (không thấy →
từ chối), đặt `panes[src].index`, trả `TIMEOUT`. `activate` trên vật
phẩm trả **`MOVE_VARIABLE_ITEM`**: adapter không tự chuyển đồ —
`process_action()` đưa action vào `action_move_item()`, nơi
`query_destination()` và `query_charges()` (popup số có scope reader
riêng, hủy được) hỏi số lượng và kiểm lại đích, sức chứa và quyền sở
hữu tại lúc chốt. `activate` trên node pane trả `TOGGLE_TAB` vì nhánh
`TOGGLE_TAB` trong `process_action()` đúng là đổi pane thao tác
(src = dest, dest tính lại là pane còn lại). Hủy giữa chừng dùng QUIT
và cancel của popup số như đường phím, không thêm cơ chế riêng.

## 4. R09.4 — Lớp mặc (`armor_layers_reader`, mới)

Cấu trúc phiên cục bộ trong `src/armor_layers.cpp`, bọc
`a11y::screen_scope(&ctxt, "armor-layers", ...)` trong
`outfit::sort_armor`, giữ tham chiếu tới các biến trạng thái thật của
màn (`tabindex`, `leftListIndex`, `selected`, `armor_cat`, `worn`) và
`update()` ngay trước `ctxt.handle_input()` sau `ui_manager::redraw()`.

Snapshot tự tính lại danh sách hiển thị đúng như `on_redraw` (tab
"All" → mọi vật đang mặc; tab phần cơ thể → chỉ vật `covers(bp)`), thứ
tự giữ nguyên thứ tự `worn` là thứ tự lớp mặc thật, gồm:

- Node tab cho từng phần cơ thể + "All" (role `tab`, `selected` theo
  tab hiện tại). `activate` đặt thẳng `tabindex` (không quay vòng như
  NEXT_TAB), reset chỉ số/chọn, trả `TIMEOUT`.
- Node từng vật đang mặc: label `tname()`, detail gồm lớp
  (`clothing_layer`), các phần cơ thể bao phủ, encumbrance của vật
  (`get_encumber(guy, bp)` ở tab phần; `get_avg_encumber(guy)` ở tab
  All), trạng thái đang được nhấc để sắp xếp và cờ HIDDEN. Danh tính
  binding là iterator của `std::list<item>` — iterator này không bị vô
  hiệu bởi `splice` (dời lớp) hay `worn.sort()` nên id bám đúng vật
  qua mọi sắp xếp lại; handler tra lại cả hai lớp (còn trong `worn`,
  còn trong tab hiện tại) trước khi tác động.
- Node trạng thái encumbrance theo phần cơ thể (role `status`, chỉ
  `focus`): giá trị `Character::encumb(bp)` — số nguyên không đơn vị,
  đúng con số pane encumbrance gốc in ra (mã thật không có hàm
  `get_encumbrance(bp)` như dự đoán ban đầu).
- Node lệnh từ `command_snapshot` nối sau (MOVE_ARMOR, EQUIP_ARMOR,
  EQUIP_ARMOR_HERE, REMOVE_ARMOR, SORT_ARMOR, CHANGE_SIDE,
  TOGGLE_CLOTH, TOGGLE_FAVORITE, ASSIGN_INVLETS, QUIT...).

Handler: `focus` vật phẩm đồng bộ `leftListIndex` rồi trả `TIMEOUT`;
`activate` vật phẩm trả **`MOVE_ARMOR`** của vòng lặp gốc (bật/tắt
chế độ nhấc vật — thao tác chọn, không tốn lượt; việc dời lớp sau đó
qua UP/DOWN gốc). Phân biệt tốn lượt ghi ngay ở đầu adapter: duyệt/
focus/đổi tab/MOVE_ARMOR-chọn là thao tác trong màn; EQUIP/REMOVE
cũng trả về vòng lặp gốc nên đi qua activity và **cảnh báo gốc giữ
nguyên** ("Can't move your integrated armor!", "Can't sort this
under your integrated armor!", query_yn "Swap side for %s?", "Remove
selected armor?" — các popup này có scope reader riêng). Cầm/mặc/cởi/
dùng vật phẩm đi qua các lệnh EQUIP/REMOVE trên và menu hành động từ
hành trang (mục 1); chọn vật chứa đích khi cất đồ thuộc selector hành
trang/dòng cất của engine, không thuộc màn lớp mặc này.

## 5. Chỗ chưa chắc / giới hạn đã biết

1. **Chưa biên dịch/chưa chạy** (đúng giai đoạn 1): chữ ký đã đối
   chiếu bằng đọc mã (`get_container_pockets() const`,
   `item_pocket::size()/sealed()/remaining_*/volume_capacity()/
   weight_capacity()`, `toggle_entry(entry, count)` công khai,
   `Character::encumb`, `item::get_encumber/get_avg_encumber`,
   `spane.index/get_cur_item_ptr()`, `square.desc[2]` theo
   `in_vehicle()` như `redraw_pane`), nhưng tính biên dịch được chỉ
   xác nhận ở giai đoạn 2 (T18, T24, T31–T35, T41, T47, T50).
2. Tổng volume/weight ở node pane của kho nâng cao cộng cả phần đất
   và phần xe của ô (theo đúng cặp trường của `advanced_inv_area`),
   trong khi `print_items` hiển thị tách theo view đang xem. Nếu
   nghiệm thu muốn khớp view hiện tại, đổi cách cộng trong
   `pane_node()` là thay đổi cục bộ một chỗ.
3. Đổi tab bằng reader ở màn lớp mặc không reset `mid_pane.offset`
   như nhánh LEFT/RIGHT gốc (adapter không giữ tham chiếu tới
   `mid_pane`/`leftListOffset`); chỉ ảnh hưởng vị trí cuộn của pane
   thông tin hình ảnh, `on_redraw` tự kéo offset về hợp lệ nên không
   kẹt trạng thái.
4. Hàng mồ côi (cha có thật nhưng không publish) giữ position/total
   phẳng thay vì theo anh em — quy ước builder mà validate R02 chấp
   nhận, đã ghi trong comment của `reader_session::update()`.
5. Form số lượng cố ý không áp cho `inventory_compare_selector`
   (mục 1); nếu giai đoạn 2 muốn compare có form 0/1 riêng thì nối
   riêng, không nới form hiện tại.
6. Cây pockets phục vụ trình bày và duyệt; thao tác cất/lấy vẫn đi
   qua hàng selector và các luồng gốc của engine (gồm cả chọn vật
   chứa đích trong dòng cất của R10/game inventory) — R09 không tự
   viết đường chuyển đồ thứ hai.
