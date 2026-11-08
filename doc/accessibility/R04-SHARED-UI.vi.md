# R04 — Menu, form, nhập và trung tâm lệnh dùng chung (giai đoạn 1, chỉ viết mã)

Tài liệu này ghi các mẫu dùng chung mà mọi adapter từ R07 trở đi phải dùng
thay vì tự tạo bộ nhập/phím thô mới, cùng quyết định giữ/thay và bằng chứng
nguồn của R04.1–R04.4. Trạng thái: **đã viết, chờ kiểm thử** (xem
`REMAINING-CODE-BACKLOG.json`): chưa build, chưa chạy test, chưa tạo APK.
Test ID sẽ chạy ở giai đoạn 2: T13–T18, T31, T33, T38, T59–T60.

Ý tưởng "Tất cả lệnh" dựng từ dữ liệu thuần (không tài nguyên hình) và
cách tổ chức menu/đọc tin nhắn tham khảo GnollHack tham khảo
(`AccessibilityGameplayMenuController`, `AccessibilityMessageReader`);
mã ở đây viết mới trên hợp đồng snapshot C++/Java của CDDA, không chép mã C#.

## 1. Menu dùng chung — `uilist` (R04.1)

### 1.1 Đã có và giữ nguyên (đã rà soát, chứng minh đúng)

- **Ánh xạ sau lọc → giá trị logic là đúng và không sửa:** `update_reader`
  publish `node.id = "entry:" + chỉ số gốc trong entries`; vị trí sau lọc
  chỉ nằm ở `position/total`. Handler trong `create_reader_scope` khớp ID
  gốc, đặt `selected = chỉ số gốc`, trả `CONFIRM`; `query_once` lấy
  `ret = entries[selected].retval`. Không có chỗ nào trên đường reader
  dùng chỉ số sau lọc thay giá trị logic.
- Disabled + lý do: node mang `enabled`/`disabled_reason` (mặc định câu
  dịch được khi màn con không điền lý do); mục disabled vẫn duyệt/focus
  được để nghe lý do.
- Checkbox: `a11y_checked` publish trạng thái `checked` và role
  `checkbox`. **Việc lật chọn thuộc vòng lặp sở hữu màn** (tiền lệ: tạo
  nhân vật tự diễn giải `CONFIRM` rồi dựng lại trạng thái) — adapter dùng
  chung cố ý không đổi ngữ nghĩa `CONFIRM`, vì mọi màn đang dựa vào nó.
- Số lượng **không phải** tính năng của `uilist` (không trường, không
  popup số trong adapter): màn cần số lượng mở popup số sau khi nhận
  retval — xem mục 2.3.

### 1.2 Bổ sung ở đợt này (chỉ chỗ chứng minh thiếu)

Cả bốn bổ sung nằm trong `uilist::update_reader` (publish) và lambda của
`uilist::create_reader_scope` (handler), không đổi schema snapshot:

1. **Category/tab theo tên:** mỗi category một node `id="category:<key>"`,
   `role="tab"`, nhãn = tên hiển thị, `selected` theo tab hiện hành,
   `position/total` theo danh sách tab. Activate đặt thẳng
   `current_category` + `switch_to_category` rồi `filterlist()`, trả
   `TIMEOUT` (ở lại menu). Trước đây reader chỉ mò được bằng hai lệnh
   LEFT/RIGHT chung chung và không nghe được đang ở tab nào.
2. **Node filter:** `id="filter"`, `role="status"`, `value` = chuỗi lọc
   hiện tại (hoặc "No filter"), `detail` = "Showing X of Y entries."
   (đã lọc / tổng gốc). Activate mở popup lọc sẵn có (`UILIST.FILTER`).
   Khi đang có lọc, thêm node `id="filter:clear"` xóa lọc tại chỗ.
   `snapshot.summary` cũng nối thêm cùng thông tin (số đã lọc/tổng,
   category, chuỗi lọc) khi đang ở tập con.
3. **Trạng thái rỗng tường minh:** khi `fentries` rỗng, publish node
   `id="empty"` và `snapshot.focus = "empty"`. Trước đây focus thành
   `"entry:-1"` không khớp node nào và người dùng không phân biệt được
   menu rỗng / lọc quá hẹp / tab không có mục.
4. **Sửa bẫy lẫn hai không gian chỉ số:** `uilist::set_selected` (chỉ số
   logic) nay đồng bộ `fselected` bằng cách tra chỉ số gốc trong
   `fentries`; helper `set_uilist_selected` ở `newcharacter.cpp` bỏ lệnh
   `scrollby(abs(idx))` vốn diễn giải cùng một số theo không gian đã lọc.
   Đây là chỗ duy nhất tìm thấy dùng lẫn chỉ số đã lọc thay giá trị logic
   (ở đường chọn sẵn, không phải đường reader activate).

**Không sửa (ghi rõ lý do):** `calc_data()` chỉ tự gán `retval = i` cho
mục enabled; mục disabled chưa có retval giữ `-1`. Đổi điểm này sẽ thay
nghĩa "không chọn gì" mà nhiều màn con đang dựa vào, và nó không liên
quan tới lọc — để nguyên, màn con dùng `allow_disabled` phải tự đặt
retval cho mục disabled của mình.

## 2. Form và trình nhập dùng chung (R04.2)

### 2.1 Mẫu form bắt buộc (từ R02.3, nay nối đủ tới editor)

- Trường publish theo hợp đồng R02: `fieldType`, `appliedValue`,
  `fieldMin/fieldMax/fieldStep` khi có miền, `choices`, `draftValue`,
  `fieldError`, `dependsOn`. Ý định: `set_value`/`select_choice` chỉ ghi
  nháp; `apply`/`cancel_draft` là hai nút riêng khi focus ở trường có
  nháp; duyệt không bao giờ áp dụng.
- Loại trường: `bool` = kích hoạt đảo nháp; `choice`/radio = kích hoạt
  chuyển nháp sang lựa chọn kế tiếp (nhãn lựa chọn đọc ở detail); số/text
  = mở `ReaderTextEditor`; "slider" trong CDDA là trường int/float có
  miền + bước, chỉnh bằng editor (nút tăng/giảm bước) hoặc các node
  previous/next-value của màn sở hữu — không có loại slider riêng trong
  hợp đồng và không bịa thêm.
- Đặt lại = `cancel_draft` (về giá trị đã áp dụng). Đặt lại về mặc định
  của nhà sản xuất thuộc màn sở hữu (ví dụ Options), không thuộc mẫu
  dùng chung này.

### 2.2 `ReaderTextEditor` — phần đã đúng từ trước và phần nối thêm

Đã rà soát và giữ nguyên: xóa/di chuyển/dấu thanh theo **grapheme**
(BreakIterator ICU vi_VN); EditText thật nên IME (gõ Telex/VNI, giọng
nói) và paste đi thẳng vào giá trị nộp; bàn phím một ngón là đường nội
bộ không cần IME/TalkBack để hoàn thành form.

Nối thêm ở đợt này:

- Constructor đầy đủ nhận `integer + rangeMin/rangeMax/rangeStep`.
  `valueDescription()` đọc kèm miền và bước. Nhóm điều khiển có hai mục
  **Tăng/Giảm một bước** (kẹp trong miền khi có miền).
- **Kiểm tra trước khi nộp** (`validationError()`), thuật toán khớp
  `check_field_value` của engine: không phải số / số nguyên có phần lẻ /
  ngoài miền / lệch bước (neo tại min, dung sai 1e-9) → đọc chuỗi lỗi
  riêng (`a11yEditorNotANumber`, `a11yEditorOutOfRange`,
  `a11yEditorStepMismatch`, Anh + Việt) và ở lại editor. Engine vẫn kiểm
  lại ở cổng R03 — đây là lớp phản hồi sớm, không thay thẩm quyền.
- `openFormEditor` truyền miền/bước từ chính node; `nodeText` (detail)
  đọc miền/bước của trường.
- IME: thêm `TYPE_TEXT_FLAG_NO_SUGGESTIONS` cho ô văn bản — IME không tự
  sửa ký tự người dùng gõ/dán. Chuỗi tìm kiếm: lưu **nguyên văn** (không
  trim, không bỏ dấu, không loại ký tự đặc biệt), chỉ hạ chữ thường
  locale vi ở khâu so khớp trong `visibleNodes()` — khóa lại hành vi đã
  đúng này, không "cải tiến" thêm.

### 2.3 Popup số cổ điển (`a11y_text.h` / `number_input_popup`)

`text_snapshot` nay cho node số mang `field_type` (`int`/`float` theo T)
và `fieldStep` khi popup có `step_size`; serialize phát `fieldStep` cả
khi không có miền (Java vốn đọc `fieldStep` độc lập với
`fieldMin/fieldMax`). `number_input_popup::query` truyền `step_size`
thật và tính nguyên của T. **Popup này không có min/max trong UI game**
nên snapshot không bịa miền; editor nhận bước để tăng/giảm và vẫn để
parser của popup quyết định hợp lệ cuối cùng. Hành trình
"nhập số và số lượng" trong sổ hành trình được cập nhật theo đúng phạm
vi này.

## 3. Trung tâm lệnh (R04.3)

### 3.1 Một builder duy nhất

`a11y::command_snapshot(context, title)` là nơi duy nhất dựng danh sách
lệnh. Fallback trong `input_context::handle_input` đã bỏ bản sao nội
tuyến (vốn đã lệch: publish cả `SELECT`/`MOUSE_MOVE`, không mô tả,
handler cancel thiếu `TEXT.QUIT`) và gọi builder dùng chung, handler
dùng chung `a11y::registered_intent`. Khoảng 25 adapter riêng đang gọi
builder này được hưởng chung mọi cải tiến dưới đây.

### 3.2 Mỗi lệnh mang gì — chỉ dữ kiện có nguồn

| Thông tin | Nguồn | Cách trình bày |
| --- | --- | --- |
| Tên | `get_action_name()` | `label` |
| Mô tả/trợ giúp | `get_desc(action)` — mô tả **phím đã gán** (cùng văn bản trợ giúp keybinding của game, gồm trạng thái chưa gán phím) | `detail`, đọc qua detail() |
| Nhóm | **Không có dữ kiện nhóm chủ đề trong nguồn** (category của binding là tên màn/ngữ cảnh) | Bảng tường minh trong `a11y_input.cpp`: 6 nhóm (Di chuyển và nhìn; Tương tác và vật phẩm; Chiến đấu và sinh tồn; Nhân vật và thông tin; Chế tạo và xe; Chờ và hệ thống) chỉ gồm ID đã đối chiếu với `data/raw/keybindings*.json`, cộng nhóm cuối "Các lệnh khác của màn này" |
| Điều kiện/lý do vô hiệu | Đăng ký trong context hiện tại; adapter riêng điền `enabled/disabled_reason` | Lệnh không đăng ký thì vắng mặt (không publish thay mặt màn khác); lệnh bị vô hiệu do adapter điền đọc được lý do như mọi node |
| Chi phí | **Không tồn tại** trong keybindings/action/input_context | Không hiển thị con số nào; summary của trung tâm nói rõ điều này |
| Tham số | **Không có lược đồ** | Không hiển thị; summary nói rõ một số lệnh mở bước tiếp theo (hướng/mục tiêu/vật phẩm) ở màn kế tiếp, và bước đó là một snapshot riêng |

Nhóm phát thành node tiêu đề `id="group:<key>"`, `role="command"`,
`actions={"focus"}` — duyệt tới chỉ nghe tên nhóm; phía Java,
`activate()` trên node `group:` chỉ đọc tên nhóm, không gửi intent nào,
nên tiêu đề không bao giờ bị gọi như một lệnh. Thứ tự trong nhóm giữ
nguyên thứ tự đăng ký. **Mọi** lệnh đã đăng ký đều xuất hiện (kể cả chưa
phân loại — rơi vào nhóm cuối): thước đo đúng của trung tâm là độ tới
được bằng duyệt, không phải tỷ lệ đã phân loại.

### 3.3 Duyệt tách gọi, xác nhận ở đâu

- Duyệt = `focus`/`detail()`: không sinh thay đổi gameplay nào ở cả hai
  đầu (giữ nguyên cơ chế R03).
- Gọi = `activate()` — điểm gửi intent duy nhất, giữ nguyên.
- Không thêm hộp xác nhận chung ở reader: nguồn không có cờ "lệnh nguy
  hiểm", phần lớn lệnh chỉ mở màn con và game tự hỏi ở đúng chỗ cần hỏi.
  Lệnh có hậu quả phải được **adapter của màn đó** nói trước trong
  `detail` (tiền lệ: overmap ghi "…after confirmation" cho lệnh xóa
  note/khởi hành) — đây là quy ước bắt buộc cho các adapter R07+.

## 4. Tài liệu dài (R04.4)

Phía C++ không đổi: một node `document` mang toàn văn trong `detail`
(`a11y_document.h`). Lớp phiên mới **`ReaderDocumentReader`** (Java)
giữ vị trí đọc:

- Tách đoạn một lần cho mỗi tài liệu: theo dòng trống; đoạn dài hơn 700
  ký tự tách tiếp theo câu (BreakIterator câu vi_VN). Đơn vị vị trí là
  "đoạn gần nhất đã bắt đầu phát".
- Mười điều khiển tài liệu được nối vào **Reader controls** chỉ khi có
  phiên tài liệu đang mở, đặt sau các điều khiển gốc (và sau mục bỏ qua
  luyện tập khi có), trước hai mục thử ánh xạ luôn ở cuối: Đọc tiếp,
  Đoạn sau, Đoạn trước, Đọc lại câu cuối, Đánh vần đoạn hiện tại,
  Đánh dấu đoạn này, Tới dấu trang, Tìm trong tài liệu, Kết quả
  tiếp theo, Kết quả trước.
- Đánh vần: theo grapheme của đoạn hiện tại, giới hạn 120 grapheme và
  nói rõ khi bị cắt.
- Bookmark, vị trí theo tài liệu và **lịch sử tìm kiếm trong tài liệu**
  (tối đa 10, theo tài liệu) sống trong bộ nhớ phiên, khóa bằng
  `contextKey + nodeId`; đổi ngữ cảnh màn thì phiên đóng (vị trí đã lưu
  được giữ cho lần quay lại trong cùng phiên ứng dụng). Tìm trong tài
  liệu: chuỗi giữ nguyên văn, so khớp hạ chữ thường trên chính văn bản
  tài liệu, nhảy tới đoạn chứa kết quả và đọc vị trí kết quả.
- **Ngắt bởi cảnh báo:** `ReaderSpeechQueue` thêm `CompletionListener`
  (`onFinished`/`onInterrupted` theo key lời nói). Lời tài liệu ở mức
  `LONG_TEXT` vẫn bị cảnh báo/thao tác chen như thiết kế cũ — khác biệt
  là vị trí không mất: Đọc tiếp phát lại **đúng đoạn bị ngắt** kèm câu
  báo ngắt, không đọc lại toàn văn, và hàng đợi không tự phát lại gì
  sau cảnh báo. "Đọc lại câu cuối" đọc câu cuối của đoạn gần nhất đã
  phát, thay cho `repeat()` toàn văn trước đây.

## 5. Quyết định giữ/thay tóm tắt

| Phần | Quyết định | Lý do |
| --- | --- | --- |
| Đường activate của uilist | Giữ nguyên | Đã chứng minh dùng chỉ số gốc → retval đúng |
| Ngữ nghĩa CONFIRM cho checkbox | Giữ nguyên | Toggle thuộc vòng lặp sở hữu màn; đổi sẽ phá mọi màn dựa vào CONFIRM |
| `calc_data` retval cho mục disabled | Giữ nguyên | `-1` là nghĩa "không chọn" mà màn con dựa vào; không liên quan lọc |
| Builder lệnh nội tuyến trong `input_context` | **Thay** bằng builder dùng chung | Đã lệch khỏi bản dùng chung ở danh sách loại trừ, mô tả và cancel |
| Deadline/mẫu editor | Giữ khung, nối thêm miền/bước | Grapheme/IME/paste đã đúng; chỉ thiếu dữ kiện miền/bước tới editor |
| Đọc tài liệu toàn văn một lần | **Thay** bằng phiên theo đoạn phía Java | Không có vị trí đọc nào tồn tại trước đây; sửa phía Java là đủ, không đổi C++ |

## 6. Chỗ chưa chắc / giới hạn đã biết

1. **Chưa biên dịch/chưa chạy** (đúng giai đoạn 1): các chữ ký đã đối
   chiếu bằng đọc mã (`get_desc(action)` có đối mặc định;
   `std::is_integral<T>` cho hai bản thể `int`/`float` của
   `number_input_popup`; R.string trong mảng tĩnh `DOC_CONTROLS` là
   hằng số của module app; vị trí các điều khiển tài liệu trong
   `performControl` tính cùng công thức dựng nhãn ở `controlLabels()`),
   nhưng tính biên dịch được chỉ xác nhận ở giai đoạn 2 (T13–T18,
   T59–T60).
2. Nhóm lệnh là bảng tường minh khởi đầu: lệnh của mod hoặc lệnh ít
   dùng rơi vào "Các lệnh khác của màn này". Bổ sung bảng khi R01 mở
   lại đối soát theo nội dung; không được suy nhóm từ tiền tố ID hay
   tên đã dịch.
3. `detail` mặc định của lệnh là mô tả phím gán — trung thực nhưng
   không phải câu tác động. Adapter riêng ghi đè `detail` sau khi lấy
   snapshot sẽ mất mô tả phím (tiền lệ overmap đã chấp nhận đánh đổi
   này); nếu giai đoạn 2 thấy cần cả hai, nối ở adapter bằng cách cộng
   chuỗi, không đổi builder.
4. Vị trí đọc tài liệu là mức đoạn: bị ngắt giữa đoạn thì nghe lại cả
   đoạn đó (tối đa 700 ký tự), vì hàng đợi không báo tiến độ theo ký
   tự. Đây là thỏa hiệp đã ghi trong thiết kế, không phải lỗi.
5. Callback hoàn thành/ngắt của hàng đợi chạy trong lúc hàng đợi đang
   chuyển trạng thái: listener hiện tại chỉ ghi cờ vào
   `ReaderDocumentReader`. Listener tương lai không được phát lời nói
   từ trong callback.
6. Điều khiển tài liệu chỉ xuất hiện khi phiên tài liệu đang mở trong
   cùng ngữ cảnh màn; mở tài liệu rồi chuyển màn là phiên đóng theo
   thiết kế phiên (mục 4). Bài giai đoạn 2 cần một hành trình tài liệu
   dài bị cảnh báo chen giữa chừng (T16/T60) để khóa hành vi đọc tiếp.
