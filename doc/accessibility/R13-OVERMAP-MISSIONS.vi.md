# R13 — Bản đồ lớn, tuyến, ghi chú và nhiệm vụ (giai đoạn 1, chỉ viết mã)

Trạng thái: **đã viết, chờ kiểm thử** (xem `REMAINING-CODE-BACKLOG.json`).
Chưa build, chưa chạy test, chưa tạo APK trong đợt này.
Test ID sẽ chạy ở giai đoạn 2: T25, T28–T30, T45–T46.

Adapter overmap và nhiệm vụ đã có từ nền `ad5ecf0` và được giữ phần đúng:
con trỏ + 8 ô kề có đích focus riêng (focus chỉ dời con trỏ), node tuyến,
snapshot tìm kiếm có scope `overmap_search`, danh sách ghi chú qua `uilist`
dùng chung kèm callback mô tả, màn nhiệm vụ 4 tab với tài liệu chi tiết, và
lịch qua callback `uilist` dùng chung. Đợt này chỉ bổ sung chỗ chứng minh
thiếu bằng đọc mã, không viết lại các adapter đó.

## 0. Ranh giới độ chính xác (áp dụng cho cả mục)

| Thông tin | Độ chính xác công bố | Căn cứ |
| --- | --- | --- |
| Tọa độ con trỏ / ô đích tuyến / ô ghi chú | Chính xác | Bản đồ là bản ghi của chính nhân vật; UI gốc cũng in tọa độ (`display::overmap_position_text`) |
| Tên ô | Theo mức `seen` của engine: `full` / `details` / `outlines` / `vague`; `unseen` → chỉ nói "chưa khám phá", không đọc tên | `overmap_buffer.seen()` + `get_name(vision)`; nay kèm câu mức rõ như thanh bên của game (mục 1) |
| Đích nhiệm vụ / điểm quan tâm đang theo dõi | Chính xác, gồm cả việc công bố ô đích trên overmap | Game tự cấp vị trí này cho người chơi (dấu đích luôn vẽ trên bản đồ; `avatar::get_active_mission_target`) |
| Khoảng cách/hướng tới đích nhiệm vụ, vị trí NPC giao việc | Chỉ khi `you_know_where_you_are()`; thời gian còn lại chính xác khi có đồng hồ, gần đúng khi không | Nguyên văn điều kiện của UI gốc trong `mission_ui.cpp` |
| Kết quả tìm kiếm | Chỉ ô có `seen != unseen` và ghi chú do nhân vật tạo, tại tầng con trỏ | Vòng lọc trong `search()` — không lộ địa điểm chưa biết |

## 1. R13.1 — Overmap: tầng, legend, lọc, tìm kiếm

Đã có và giữ: `LEVEL_UP`/`LEVEL_DOWN` có mô tả + lý do khóa ở hai biên
tầng; con trỏ/ô kề/tuyến như trên; các lệnh chuyển đổi hiển thị đã publish.

Bổ sung trong `reader_snapshot` / `overmap_tile_description`
(`src/overmap_ui.cpp`):

1. **Mức rõ của ô:** mô tả ô (con trỏ, ô kề, kết quả tìm kiếm, đích tuyến
   — mọi nơi dùng `overmap_tile_description`) nay nối thêm đúng câu của
   thanh bên game khi mức nhìn là `vague`/`outlines`/`details`
   ("chỉ nhận ra chi tiết mơ hồ / đường nét / một số chi tiết"). Trước đây
   bộ đọc đọc tên ô ở mức thấp mà không nói tên đó kém chính xác.
2. **Legend bằng lời:** node tài liệu `overmap:legend` tóm tắt quy tắc đọc
   bản đồ: mức tên theo tri thức, tọa độ là bản ghi của nhân vật, ghi chú
   đọc kèm biểu tượng/màu bằng lời, bán kính nguy hiểm theo ô bản đồ, tuyến
   và ô đích mục tiêu được công bố thế nào, tìm kiếm chỉ thấy thứ đã biết.
3. **Lọc/hiển thị thành checkbox có trạng thái thật:** `TOGGLE_BLINKING`,
   `TOGGLE_OVERLAYS`, `TOGGLE_HORDES`, `TOGGLE_CITY_LABELS`,
   `TOGGLE_MAP_REVEALS`, `TOGGLE_FOREST_TRAILS` nay là `checkbox` với
   `checked` lấy từ `uistate` (trước đây không có trạng thái; ba lệnh đã có
   từ trước — fast travel, fast scroll, map notes — giữ nguyên). Kèm theo:
   - `TOGGLE_OVERMAP_WEATHER`: checkbox + **khóa kèm lý do** khi nhân vật
     đang ở trong nhà (đúng điều kiện engine chỉ đổi khi ở ngoài trời).
   - `TOGGLE_EXPLORED`: checkbox theo `overmap_buffer.is_explored(cursor)`
     và mô tả nói rõ lệnh đổi trạng thái đã khám phá của ô con trỏ.
   - `TOGGLE_LAND_USE_CODES`: **khóa kèm lý do ngoài debug mode** (handler
     gốc lặng lẽ không làm gì ngoài debug — trước đây bộ đọc vẫn cho bấm).
   - `look`, `zoom_in`, `zoom_out`, `SEARCH`, `MISSIONS` có mô tả tác động
     bằng lời (zoom/nháy chỉ đổi hiển thị hình, không đổi nội dung đọc).

**Tìm kiếm — cả ba nhánh đã đối chiếu trong `search()`:**

| Nhánh | Hành vi engine | Đường bộ đọc |
| --- | --- | --- |
| Không có kết quả | Popup "No results found within %d tiles." rồi ở lại bản đồ, con trỏ nguyên vẹn | Popup có `screen_scope` (`src/popup.cpp`) — nghe được như mọi popup |
| Có kết quả | Duyệt vòng bằng NEXT_TAB/PREV_TAB; CONFIRM chốt ô kết quả; QUIT khôi phục con trỏ cũ | Snapshot `overmap_search` sẵn có: focus/activate trên node kết quả chỉ dời con trỏ quan sát, CONFIRM/QUIT có mô tả "không khởi hành" |
| Từ khóa trống/hủy ở ô nhập | Trở về bản đồ, không thông báo | Ô nhập là `string_input_popup` (scope dùng chung R04) |

## 2. R13.2 — Ghi chú, vùng nguy hiểm, điểm quan tâm

Đã có và giữ: form tạo/sửa ghi chú và điểm quan tâm đều là
`string_input_popup` với identifier `map_note` → trình nhập dùng chung R04
(scope `STRING_INPUT`, editor grapheme/IME), phần mô tả của form tự nó đã
giải thích cú pháp `GLYPH:`/`COLOR;` bằng chữ; xóa ghi chú hỏi `query_yn`
ở cả ba đường (lệnh trực tiếp, trong danh sách, sửa thành rỗng); bán kính
nguy hiểm qua `query_int` miền 0–20 (scope `NUMBER_INPUT`); danh sách ghi
chú làm mới sau sửa/xóa và snapshot overmap publish lại mỗi lượt nên focus
không bám vào ghi chú đã mất.

Bổ sung: **`note_symbol_description()`** — tương đương lời cho biểu tượng
và màu của ghi chú ("Note symbol: letter N. Note color: yellow."), suy từ
chính `get_note_display_info` + bảng màu ghi chú của engine. Dùng ở cả ba
bề mặt đọc: mô tả ô con trỏ (`overmap_tile_description`), `a11y_value` của
mỗi hàng trong danh sách ghi chú, và `detail` của `map_notes_callback`.
Trước đây bộ đọc bỏ qua hoàn toàn biểu tượng/màu — thứ mà người sáng mắt
thấy đầu tiên ở một ghi chú.

Điểm quan tâm: tạo từ overmap bằng cùng form nhập; quản lý ở tab riêng của
màn nhiệm vụ (mục 4).

## 3. R13.3 — Tuyến và di chuyển

Đã có và giữ: `try_travel_to_destination` là thẩm quyền khởi hành — popup
lý do khi không tìm được đường (gồm nhánh khác tầng z), `query_yn` xác
nhận riêng với đủ biến thể (quá cân, đang ở trong xe nhưng không lái, lái
xe, đi bộ) trước khi gán `ACT_TRAVELLING`/autodrive; chọn điểm
(`CHOOSE_DESTINATION`/`_DIRECT`) chỉ lập tuyến, chọn lại cùng tuyến mới mở
xác nhận đi; `GO_TO_DESTINATION` bị khóa kèm lý do khi chưa có tuyến;
dừng nguy hiểm và ngắt giữa đường là logic hoạt động/safe mode gốc
(R10.3, R11.1) — không viết lại ở đây.

Bổ sung:

1. **Lý do không lập được tuyến ở nhánh chọn điểm:** trước đây khi
   `get_overmap_path_to` trả rỗng, mã lặng lẽ thay tuyến hiện tại bằng
   tuyến rỗng — người chơi mất tuyến cũ mà không nghe lý do. Nay popup
   đúng dữ kiện engine: khác tầng z / ô đích chưa khám phá / không tìm
   được đường. Chọn ô của chính mình (ý định xóa tuyến) thì không popup.
2. **Node tuyến nói rõ vòng đời của tuyến:** chọn đích khác là thay
   tuyến, chọn ô của mình là xóa tuyến, focus vào node tuyến chỉ dời con
   trỏ tới đích. Hủy chuyến sau khi đã khởi hành = ngắt hoạt động đang
   chạy (R10.3), không phải một lệnh riêng của overmap.

## 4. R13.4 — Nhiệm vụ và lịch

Màn nhiệm vụ (`src/mission_ui.cpp`) — đã có và giữ: 4 tab
active/completed/failed/points of interest là node tab thật; focus/activate
trên hàng chỉ đổi lựa chọn đang xem; chi tiết gồm người giao, phe, vị trí,
mô tả đã parse (phần thưởng và điều kiện nằm trong mô tả sau khi
`parsed_description_for` thay token `<reward_count:...>`), hạn và thời
gian còn lại, mốc hoàn thành/thất bại ở hai tab kết quả.

Bổ sung:

1. **Mô tả tác động thật cho hai lệnh đổi trạng thái:** `CONFIRM` — nói rõ
   chỉ đặt mục tiêu theo dõi, không nhận/trả nhiệm vụ và không khởi hành;
   `DELETE_POINT_OF_INTEREST` — nói rõ game xóa ngay, **không** có hộp
   xác nhận (đây là hành vi gốc; việc nói trước là lớp bảo vệ của bộ đọc,
   không bịa thêm hộp xác nhận khác UI gốc).
2. **Liên kết vị trí nhiệm vụ với overmap:** khi con trỏ bản đồ đứng đúng
   ô đích của mục tiêu đang theo dõi (`get_active_mission_target`, gồm
   cả điểm quan tâm đang hoạt động), mô tả ô con trỏ công bố "ô đích của
   mục tiêu hiện tại" — tương đương bằng lời của dấu đích đỏ trên bản đồ
   hình. Độ chính xác theo mục 0: đích là thông tin game cấp.

Phạm vi đúng của màn nhiệm vụ: nhận và trả nhiệm vụ diễn ra trong hội
thoại NPC (R14), hoàn thành/thất bại do engine quyết và hiện ở tab kết
quả — màn này chỉ theo dõi/ưu tiên và xem kết quả. Không thêm nút
nhận/trả giả vào màn nhiệm vụ.

Lịch (`src/calendar_ui.cpp`): giữ nguyên adapter callback của `uilist`
dùng chung (mô tả tăng/giảm/đặt lại, lý do khóa trường vượt độ chính xác
cho phép). Rà soát ở R13 không thấy nhánh thiếu so với UI thật; hành
trình `lich-va-chinh-thoi-gian` giữ trạng thái đã viết.

## 5. Chỗ chưa chắc / giới hạn đã biết

1. **Chưa biên dịch/chưa chạy** (đúng giai đoạn 1): đã đối chiếu chữ ký
   bằng đọc mã (`get_note_string_from_color`/`get_note_color_names`
   trong `color.h`, `is_explored` trong `overmapbuffer.h`,
   `get_active_mission_target` trong `avatar.h`, `append_document`
   trong `a11y_document.h` — đã thêm include), tính biên dịch thật thuộc
   giai đoạn 2 (T45, T46).
2. Popup lý do không lập được tuyến dùng cùng cơ chế `popup()` của các
   nhánh overmap khác (có scope đọc); chưa đo được ở giai đoạn 1 việc
   popup này có chen đúng lúc vào hàng lời khi người dùng bấm chọn điểm
   liên tiếp hay không — bài T45 ở giai đoạn 2 cần một trường hợp chọn
   đích không tới được hai lần liên tiếp.
3. Trường hợp biên của popup mới: khi đang lái xe, `get_overmap_path_to`
   cũng trả rỗng nếu ô đích trùng ô của **xe** (khác ô nhân vật đứng);
   điều kiện bỏ qua popup hiện chỉ so ô nhân vật, nên chọn đúng ô xe khi
   đang lái có thể nhận popup "không tìm được đường" thay vì im lặng.
   Sai lệch này vô hại (vẫn không lập tuyến sai) và ghi lại để giai
   đoạn 2 quyết định có đáng tinh chỉnh hay không.
4. Tên màu ghi chú lấy từ `note_color.name` — cùng chuỗi UI gốc hiển thị
   trong trợ giúp của form sửa ghi chú; bản dịch tiếng Việt của các tên
   màu này thuộc lớp dịch R21, không xử lý riêng ở đây.
5. Độ chính xác vị trí NPC giao việc trong chi tiết nhiệm vụ là tọa độ
   chính xác theo `display::overmap_position_text` — bằng đúng UI gốc.
   Nếu giai đoạn 2/3 muốn siết theo quyền biết chặt hơn UI gốc thì đó là
   quyết định sản phẩm mới, không tự ý đổi ở đây (nguyên tắc ngang bằng
   thông tin với người sáng mắt, xem R08).
