# R12 — Chế tạo, sửa chữa, xây và tháo

Giai đoạn 1: chỉ viết mã, không build/test/APK. Mọi số dòng đối chiếu tại
HEAD trước R12 (`20d576a9`) cộng thay đổi của chính mục này. Trạng thái các
đầu việc: **đã viết, chờ kiểm thử** (bài T21, T24, T31–T32, T37, T43–T44).

Nguyên tắc xuyên suốt: đủ nguyên liệu tính trên **tổng tài nguyên dùng
chung**; không tung preview ngẫu nhiên mới cho bộ đọc; điều kiện ô xây chỉ
lấy từ kiểm tra hợp lệ của engine và quyền nhìn thấy; khởi công/hủy/hoàn
thành luôn qua logic gốc (activity và message log của engine).

## 1. R12.1 — Công thức và tổng tài nguyên dùng chung

Đã có từ nền (giữ nguyên, đã rà): hàng công thức có `Can craft/Cannot
craft`, `Favorite`, vai `treeitem` cho nhóm lồng (nested là thư mục phân
loại công thức — repo không có khái niệm sub-recipe; thành phần luôn là
`item_comp`), trường mẻ `craft:batch` 1–50 (áp số không bắt đầu chế tạo),
tài liệu chi tiết/mô tả/yêu cầu từng nhóm/thuộc tính/steps/tài nguyên nhân
vật; filter, favorite, tab, đổi crafter tới được qua node lệnh đã đăng ký
(`FILTER`, `TOGGLE_FAVORITE`, `CHOOSE_CRAFTER`... của `command_snapshot`).

Vấn đề của bản nền: `append_requirements` gọi `alternative.has()` cho từng
nhóm **độc lập trên toàn bộ kho**, nên một vật duy nhất có thể làm hai nhóm
cùng đọc "Available" dù tổng cần vượt tổng có.

Đã bổ sung trong `src/a11y_requirements.h` (`append_requirements`, tham số
tùy chọn `deduped`):

- Tài liệu `<prefix>:total` "Shared total": phán quyết toàn tập bằng đúng
  API engine dùng tính craftability — `deduped_requirement_data::
  can_make_with_inventory` khi caller truyền bản deduped (caller công
  thức chính trong `crafting_gui.cpp` nay truyền
  `&rec.deduped_requirements()`), ngược lại
  `requirement_data::can_make_with_inventory` trên cùng một inventory.
  Một vật dùng chung chỉ được tính một lần trong phán quyết này.
- Dòng tổng theo loại cho các nhóm **đơn lựa chọn** (nhóm bắt buộc dùng
  đúng loại đó): cần = tổng `|count| × mẻ` qua mọi nhóm như vậy; có =
  `charges_of`/`amount_of` với cùng filter thành phần. Loại nằm trong nhóm
  có lựa chọn không bị gán tổng bịa — phán quyết toàn tập ở trên bao phủ.
- Áp dụng cho cả ba caller: công thức chính, từng step, và yêu cầu công
  trình (`construction.cpp`). **Không** cộng gộp root + steps thành một
  tổng: các step chạy tuần tự, kho thay đổi sau mỗi step (tiêu hao và sản
  phẩm trung gian), nên tổng đúng là theo từng tập yêu cầu.

Đã bổ sung trong `crafting_gui.cpp` (tài liệu `craft:conditions`): crafter
đang chọn và kết quả cổng bắt đầu thật `can_start_craft` — đủ điều kiện /
không chế được / mẻ tạo quá nhiều vật / quá tối (kiểm ánh sáng của engine)
— công bố **trước** khi kích hoạt, thay vì chỉ thành popup sau CONFIRM.
Các kiểm muộn hơn trong `Character::craft` (morale, đang lái xe) vẫn do
engine chặn lúc bắt đầu và phát qua message log như người sáng mắt thấy
popup/thông báo.

## 2. R12.2 — Chọn thành phần, xác nhận, kết quả thật

Chuỗi thực thi (đã rà, không cần adapter riêng): menu chế tạo đóng →
`craft_command::execute`:

1. chọn bộ alternative deduped (`select_requirements`, uilist "Use which
   selection of components?") khi có nhiều bộ khả thi;
2. từng nhóm thành phần qua `select_item_component` — uilist "Use which
   component?" với entry ghi rõ **cần/có** và nguồn (trên người / gần đây /
   cả hai) — và dụng cụ qua `select_tool_component` (uilist "Use which
   tool?"); mọi uilist đi qua scope đọc dùng chung R04, hủy ở bất kỳ bước
   nào là hủy thật, chưa tiêu hao;
3. vật chứa cho kết quả lỏng: engine tự gom container đủ điều kiện, chỉ hỏi
   `query_yn` khi không đủ chỗ chứa (popup dùng chung) — không có selector
   vật chứa riêng trong UI gốc;
4. xác nhận tiêu hao/bắt đầu là các query tình huống của chính engine
   trong `craft_command::execute`: charges không đủ để hoàn thành
   ("Start crafting anyway?"), dùng thành phần thối, dùng thành phần
   favorite, planning modal cho step cần chú ý. Không có `query_yn` "bắt
   đầu chế?" chung trong UI gốc — bộ đọc giữ đúng hành vi đó, không thêm
   bước xác nhận khác người sáng mắt;
5. gián đoạn/tiếp tục/hủy và tiến độ qua cơ chế activity R10; sản phẩm và
   kết quả thật công bố khi activity hoàn thành và qua message log.

Byproduct: bản nền chỉ hiện byproduct trong `craft:details` khi UI hình
ảnh đã điền cache preview — cache trống thì công thức đọc như không có
byproduct. Đã bổ sung tài liệu `craft:byproducts` trong `crafting_gui.cpp`
từ **dữ liệu công thức**: phần legacy dùng đúng số học của
`recipe::get_byproducts` (× `charges_default` nếu đếm theo charges, × mẻ);
phần item-group chỉ ghi "sẽ sinh khi chế tạo hoàn thành" — **không tung
lại nhóm tại lúc đọc**, kết quả thật của nhóm đến từ lần chế tạo thật.

## 3. R12.3 — Sửa đồ và tháo rã

Kết luận đối chiếu: **không có modal riêng nào thiếu adapter** — toàn chuỗi
đi qua adapter dùng chung đã có, nên mục này đóng bằng bằng chứng, không
viết adapter trùng:

- Sửa đồ (`repair_item_actor`): dùng tool → activity `ACT_REPAIR_ITEM` →
  chọn mục tiêu bằng `game_menus::inv::repair` (selector R09; preset có các
  cột MATERIALS AVAILABLE / SUCCESS CHANCE / DAMAGE CHANCE / DIFFICULTY và
  denial, tất cả chảy vào `detail` của hàng selector) → menu lặp
  `repeat_menu` (uilist R04; `text` của menu chính là title chứa charges,
  vật tư hiện có, skill, % thành công, % gây hỏng — tức rủi ro đã biết
  được đọc trong summary) → mỗi lần thử do `repair()` của engine quyết
  định, vật tư chỉ tiêu thụ khi thành công, kết quả (thành công / hỏng /
  phá hủy / hết charges) qua message log.
- Nhánh kề cận đã rà cùng: `gun_repair` (selector qua `inv_internal`) và
  `mend` (uilist chọn fault có mô tả Repairs/Time/Skills) — đều qua adapter
  dùng chung.
- Tháo rã: `ACTION_DISASSEMBLE` / menu vật phẩm / iuse / butcher →
  selector `inv::disassemble` (preset YIELD + TIME, `get_denial` từ
  `can_disassemble` — gồm lý do thiếu dụng cụ/chất lượng) → các xác nhận
  của engine bằng `query_yn` dùng chung: đồ favorite, đồ đang mặc, đồ của
  người khác ("may anger the people who own it" — xác nhận phía sở hữu),
  và khi `interactive` + tùy chọn `QUERY_DISASSEMBLE`: danh sách yield
  trước khi tháo → số lượng qua `number_input_popup` dùng chung → activity
  `ACT_DISASSEMBLE`; kết quả thu hồi từng thành phần do
  `complete_disassemble` của engine tung và công bố qua log ("You recover /
  You fail to recover ..."), vật thu được rơi tại vị trí vật gốc.

Hành trình `sua-chua-va-thao-ra` trong sổ được nâng từ "chưa viết" lên
"đã viết, chờ kiểm thử" với adapter ghi rõ là dùng chung.

## 4. R12.4 — Xây dựng

Đã có từ nền (giữ): node nhóm có trạng thái buildable/blueprint, tài liệu
chi tiết, tài liệu từng giai đoạn với trạng thái kỹ năng-vật liệu và vị
trí, yêu cầu từng giai đoạn.

Đã bổ sung trong `src/construction.cpp`:

- Tài liệu `construction:placement`: các **hướng kề hợp lệ** cho nhóm đang
  chọn, lấy từ chính `con_preview` mà engine tiền tính cho preview hình
  ảnh (một ô chỉ có mặt khi `can_construct` và `player_can_build` cùng
  đúng) — phát biểu tính hợp lệ của engine, không suy từ dữ kiện ô ẩn.
  Ở chế độ blueprint, tài liệu ghi rõ chọn công trình chỉ ghi lại cho
  vùng, không đặt gì (đúng `CONFIRM` của blueprint: trả `construction_id`,
  không qua `place_construction`).
- Tài liệu giai đoạn có thêm: thời gian hoàn thành
  (`time_duration::from_moves(con->time)`), từng kỹ năng yêu cầu kèm mức
  nhân vật hiện có, và `pre_note`.

Chọn ô thật đi qua scope `adjacent-direction` (R08): mỗi hướng đọc mô tả
ô và — chỉ khi ô **nhìn thấy được** — hint "valid / not valid for the
selected construction" của chính `place_construction`. Engine không sinh
lý do chi tiết cho ô không hợp lệ (`can_construct` chỉ trả bool;
`explain_failure` mặc định chỉ phát "You cannot build there!") nên bộ đọc
không bịa lý do cụ thể hơn. Xác nhận phe phái trong xây dựng không phải
"xây cho phe": cổng duy nhất là cảnh báo lãnh thổ
`warn_player_maybe_anger_local_faction` trong đường CONFIRM — `query_yn`
của engine, từ chối thì không đặt và ở lại menu, đồng ý thì engine tự trừ
điểm phe. Khởi công tạo `partial_con` + activity `ACT_BUILD`, tiến độ/hủy
qua R10, hoàn thành do `complete_construction` của engine (kèm byproduct
và thông báo "finished construction").

Điểm còn mở (ghi rõ, không giấu): marker `partial_con` (công trình đang
dở trên bản đồ) chưa có công bố ngữ nghĩa riêng trong snapshot thế giới;
đường tiếp cận hiện có là menu `prompt_partial_construction` khi tương
tác với ô dở (uilist dùng chung: % hoàn thành, Resume / Cancel trả vật
liệu / Ignore). Việc công bố marker trong snapshot thế giới thuộc phạm vi
màn thế giới (R08/R23 đối chiếu lại).

## 5. Chỗ chưa chắc

- Chưa biên dịch: chữ ký đã đối chiếu theo tiền lệ trong repo
  (`deduped::can_make_with_inventory` tại `requirements.h:514`;
  `charges_of/amount_of` tại `temp_crafting_inventory.h:88-94` và tiền lệ
  `requirements.cpp:879-881`; `direction_from` cho tọa độ typed tại
  `coordinates.h:966`, `direction_name` qua `map.h → line.h`;
  `entry.first->name.translated()`/`charges_default()` theo
  `recipe.cpp:1325-1331`). Tính biên dịch được xác nhận ở giai đoạn 2.
- Tài liệu `construction:placement` dùng tên nhóm cho mọi ô hợp lệ; khi
  nhiều biến thể của nhóm cùng hợp lệ ở các ô khác nhau, tên biến thể
  riêng từng ô chưa được phân biệt (preview của engine lưu một
  construction cho mỗi ô; tên nhóm là mức UI gốc hiển thị).
