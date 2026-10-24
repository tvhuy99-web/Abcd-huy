# R02 — Hợp đồng cây, bảng và biểu mẫu (giai đoạn 1, chỉ viết mã)

Tài liệu này ghi schema, chính sách và bằng chứng nguồn của mục R02 trong
`REMAINING-CODE-PLAN.vi.md`. Trạng thái của R02.1–R02.4 là
**đã viết, chờ kiểm thử** (xem `REMAINING-CODE-BACKLOG.json`): chưa build,
chưa chạy test, chưa tạo APK trong đợt này. Test ID sẽ chạy ở giai đoạn 2:
T12–T18, T31–T34, T49, T51.

Ý tưởng tổ chức node theo cây (ID logic ổn định, quan hệ cha/con tách khỏi
trình bày phẳng) tham khảo cách `AccessibilityUiNode`/
`AccessibilityUiTreeBuilder` của GnollHack tham khảo tách node khỏi cây
hình ảnh; mã ở đây viết mới theo hợp đồng snapshot C++/Java sẵn có của
CDDA, không chép mã C#.

## 1. Schema xuyên hai phía

Nguồn sự thật: `src/a11y_semantics.h` (struct), `src/a11y_semantics.cpp`
(serialize + validate), phía nhận: `ReaderSnapshot.java` (parse + validate
gương) và `GameReaderController.java` (render + ý định). Mọi trường mới đều
nằm trong `snapshot_content_json`, nên thay đổi chúng làm tăng revision như
mọi nội dung khác.

### 1.1 Cây (mỗi node)

| Trường JSON | Mặc định khi vắng | Nghĩa |
| --- | --- | --- |
| `parentId` | `""` | ID node cha; rỗng = nút gốc/nút phẳng |
| `depth` | `-1` | `-1` = không thuộc cây; `0` = gốc; con = cha + 1 |
| `hasChildren` | `false` | Có con trong cây (kể cả khi đang thu gọn và con không được publish) |
| `expanded` | `false` | Con có đang được publish hay không |

Với node cây, `position`/`total` đếm **tập anh em cùng cha trong tập đang
publish**, không đếm toàn danh sách. Con của một nút đang thu gọn không bao
giờ được publish.

### 1.2 Bảng

- Snapshot khai báo bảng một lần: `tables: [{id, columns: [{id, header, unit}]}]`.
- Node hàng mang `tableId`, `rowIndex` (0-based, duy nhất trong bảng) và
  `cells: [{column, value, unit, compare}]`. `compare` là dạng giá trị có
  thể so sánh/sắp xếp bằng máy (ví dụ số thuần không dấu phân cách), để
  phía Java không phải bóc tách văn bản trình bày.
- Ô **không** mang hành động: chọn/kích hoạt luôn đi qua hành động của
  node hàng. Truy cập theo hàng = duyệt node hàng; theo thuộc tính = đọc
  cell theo `column` dưới header đã khai báo.

### 1.3 Biểu mẫu

Node trường có `fieldType` thuộc {`bool`, `int`, `float`, `choice`,
`text`} và:

| Trường JSON | Nghĩa |
| --- | --- |
| `appliedValue` | Giá trị đã áp dụng, dạng thô (khác `value` là văn bản hiển thị đã dịch) |
| `fieldMin`/`fieldMax`/`fieldStep` | Miền số; chỉ xuất hiện khi trường có miền (`has_field_range`) |
| `choices: [{value, label}]` | Lựa chọn cho `choice` và các trường có danh sách giá trị; `value` là giá trị thô ý định mang theo |
| `draftValue` | Chỉ xuất hiện khi có nháp chưa áp dụng; vắng = không có nháp |
| `fieldError` | Văn bản lỗi tại trường, do builder bản địa hoá |
| `dependsOn: [nodeId]` | Các trường đã publish mà giá trị áp dụng của chúng ràng buộc trường này |

Ý định riêng (action trong `actions` của node, qua mailbox/runtime sẵn có):

| Intent | Tác dụng | Không được làm |
| --- | --- | --- |
| `set_value` | Ghi nháp (số/text/bool) | Đổi giá trị đã áp dụng |
| `select_choice` | Ghi nháp bằng một `choices[].value` | Đổi giá trị đã áp dụng |
| `open_tree` | Mở cây con của trường/nút cây (ở mẫu Options = mở nhóm) | Gửi hành động gameplay |
| `expand` / `collapse` | Mở/thu nút cây | Gửi hành động gameplay |
| `apply` | Áp dụng nháp hiện có | Áp dụng khi không có nháp (no-op) |
| `cancel_draft` | Bỏ nháp, giữ giá trị đã áp dụng | Đổi giá trị đã áp dụng |

## 2. Chính sách focus khi cây thay đổi

Cài ở `GameReaderController.resolveFocusFallback`, áp dụng khi node đang
focus biến mất khỏi snapshot mới của **cùng một màn** (context không đổi):

1. Rơi về **cha gần nhất còn tồn tại** (đi ngược chuỗi `parentId` trong
   snapshot cũ) — trường hợp cha bị thu gọn hoặc node bị xoá.
2. Nếu không còn tổ tiên nào: rơi về **node kề cùng cha cùng cấp** (chênh
   `position` nhỏ nhất trong snapshot mới).
3. Cuối cùng mới rơi về focus mặc định của snapshot / node đầu tiên.

Đổi context vẫn giữ chính sách cũ: focus mặc định của màn mới.

## 3. Validate và tương thích (R02.4)

- C++: `a11y::validate_snapshot` chạy trong `screen_scope::update` (thay
  kiểm tra trùng ID rời rạc trước đây) và `a11y::validate_intent_value`
  chạy trong `poll_input` trước khi handler nhận intent — cả hai ở game
  thread. Từ chối: ID rỗng/trùng; cha không tồn tại; con của nút đã thu
  gọn; độ sâu không khớp cha+1 hoặc quá 16; chu trình cha; nút khai
  `expanded && hasChildren` nhưng không publish con nào; bảng/cột chưa
  khai báo; hàng trùng `(tableId,rowIndex)`; loại trường lạ; miền
  min>max/step<0; lựa chọn trùng; nháp ngoài miền/sai bước/không thuộc
  lựa chọn; `dependsOn` trỏ vào chính nó hoặc node không tồn tại; quá
  giới hạn kích thước. Lý do từ chối là **mã ổn định nội bộ** (ví dụ
  `duplicate_node_id`, `tree_cycle`, `value_out_of_range`), không phải
  văn bản cho người dùng; phía Java nhận biên nhận `unsupported` (snapshot
  bị từ chối) hoặc `invalid_value` (giá trị intent bị từ chối, khớp
  request) và đọc văn bản đã bản địa hoá sẵn có.
- Java: `ReaderSnapshot` validate gương cùng các quy tắc trên khi parse;
  snapshot vi phạm bị coi là malformed như trước.
- Tương thích có chủ ý: snapshot cũ không có trường mới vẫn hợp lệ —
  `tables` vắng = không có bảng; `depth=-1`/thiếu = node phẳng; node
  không có `fieldType` = không phải trường; `draftValue` vắng = không có
  nháp. Đây là cùng mẫu với `swipeMovementAllowed` ở R00.
- Giới hạn kích thước: 4096 node, 16 bảng, 16 cột/bảng, 16 ô/hàng, 128
  lựa chọn/trường, độ sâu cây 16, giá trị intent tối đa 4096 ký tự.
  Builder có nhiều dữ liệu hơn phải phân trang; phía Java sẵn có phân
  trang render 30 mục. Vượt giới hạn = từ chối cả snapshot, không cắt
  thầm.

## 4. Ba mẫu tham chiếu

### 4.1 Form + cây — màn Options (`src/options.cpp`)

Được chọn vì đây là bề mặt duy nhất hiện có đồng thời cả hai cấu trúc
thật: trang → nhóm thu/mở được (`groups_state`) → option, và option có đủ
loại bool/int/int_map/float/string_select/string_input kèm miền và danh
sách giá trị trong `cOpt`. Cây pockets chưa tồn tại (thuộc R09) và công
thức lồng nhau của crafting chỉ có cờ `is_nested` mà không có danh tính
cha trong dữ liệu nguồn, nên không dùng làm mẫu cây.

- Cây: node trang/tab ở độ sâu 0; header nhóm là nút cây (intent
  `expand`/`collapse`/`open_tree` đổi `groups_state`; `activate` cũ vẫn
  toggle qua CONFIRM để giữ luồng gốc); hàng option treo dưới header nhóm
  của nó (hoặc dưới trang nếu không thuộc nhóm); hai nút
  previous/next-value treo dưới hàng option. `position`/`total` tính theo
  tập anh em; nhóm đã thu gọn vẫn khai `hasChildren` đúng số option của
  nhóm nhưng không publish con.
- Form: mỗi hàng option là một trường. `cOpt` thêm ba accessor chỉ đọc
  `getNumericMin/Max/Step` (options.h) để publish miền int/float. Nháp giữ
  trong state của chính màn Options (`reader_drafts`), chỉ intent `apply`
  mới gọi `cOpt::setValue`; rời màn là mất nháp. `fieldError` được builder
  điền khi một nháp trở nên không hợp lệ sau khi miền thay đổi (lớp phòng
  thủ thứ hai sau validate ở runtime). `dependsOn` nối tới node của option
  tiên quyết khi nó cùng được publish trên trang hiện tại; tiên quyết ở
  trang khác vẫn chỉ thể hiện bằng `enabled`/`disabledReason` như trước.
- Luồng gốc giữ nguyên: previous/next-value vẫn áp dụng ngay như trước;
  hợp đồng nháp là đường mới cộng thêm, không thay luồng cũ.

### 4.2 Bảng — màn Medical (`src/medical_ui.cpp`)

Bảng `medical:limbs`: cột `part` (bộ phận), `condition` (tình trạng),
`flags` (BLEEDING/BITTEN/SEPTIC/WOUNDED). Giá trị condition dùng nguyên
`a11y::limb_health_description` — chính sách tri thức y tế hiện hành (mô
tả thô theo cảm nhận/vạch, không lộ HP chính xác). Hàng vẫn chọn bằng
focus/activate của chính node chi như trước (`reader_intent` không đổi);
cell chỉ trình bày.

### 4.3 Điều khiển phía Java (`GameReaderController`)

- Đọc node: nói trạng thái mở/thu gọn và cấp cây (ở detail), đọc cell dưới
  header cột, luôn nói nháp chưa áp dụng và lỗi trường, đọc danh sách lựa
  chọn ở detail.
- Kích hoạt nút cây có con: gửi `expand`/`collapse`, không gửi `activate`.
- Trường `choice`: mỗi lần kích hoạt chuyển nháp sang lựa chọn kế tiếp;
  trường `bool`: kích hoạt đảo nháp; trường số/text: mở `ReaderTextEditor`
  sẵn có để nhập nháp (`set_value`). Khi focus ở trường có nháp, danh sách
  hiện thêm hai nút "Áp dụng giá trị nháp" / "Bỏ giá trị nháp" gửi
  `apply` / `cancel_draft`. Biên nhận `invalid_value` của intent form được
  khớp bằng request riêng (`formRequest`) và đọc bằng chuỗi lỗi editor
  sẵn có.

## 5. Chỗ chưa chắc / giới hạn đã biết

1. **Chưa biên dịch/chưa chạy** (đúng giai đoạn 1): chữ ký và kiểu đã đối
   chiếu bằng đọc mã (aggregate init của `semantic_table`/`semantic_cell`,
   `cOpt::setValue(const std::string&)` xử lý mọi loại, ternary
   `ACTIVE_WORLD_OPTIONS : OPTIONS` theo đúng mẫu sẵn có trong
   `options.cpp`), nhưng tính biên dịch được chỉ xác nhận ở giai đoạn 2.
2. Giới hạn 4096 node: chưa đo số node lớn nhất của các snapshot hiện hữu
   (ví dụ hành trang rất lớn hoặc quét thế giới bán kính lớn). Nếu một màn
   thật vượt ngưỡng, nó sẽ bị từ chối thành `unsupported` thay vì cắt
   thầm — bài kiểm thử giai đoạn 2 phải gồm một màn lớn để chốt ngưỡng.
3. Option int có `format` chứa ký tự trang trí (ví dụ `%`): `appliedValue`
   giữ nguyên dạng `cOpt::getValue()` (có thể là `100%`), trong khi nháp
   phải nhập số thuần; `cOpt::setValue` tự lột `%` ở đường áp dụng gốc
   nhưng validate nháp của hợp đồng không chấp nhận `%`. Đây là khác biệt
   có chủ ý để giữ miền số sạch; nếu giai đoạn 2 thấy gây vướng, xử lý ở
   builder Options (lột định dạng khi publish) chứ không nới validate.
4. `dependsOn` mới nối được tiên quyết cùng trang (xem 4.1); ràng buộc
   giữa các trường ở các trang/màn khác nhau thuộc các mục adapter sau
   (R04/R16/R19), hợp đồng đã có trường dữ liệu nhưng chưa có builder nào
   publish qua trang.
5. `open_tree` hiện mới có consumer thật là nhóm Options (= mở nhóm).
   Trường có giá trị dạng cây (chọn vật chứa, chọn ô xe...) sẽ dùng cùng
   intent khi các adapter R09/R15 được viết; không khai thêm intent khác.
