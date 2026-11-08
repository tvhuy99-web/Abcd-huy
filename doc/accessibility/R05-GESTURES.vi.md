# R05 — Cử chỉ, luyện tập và cứu hộ (giai đoạn 1, chỉ viết mã)

Tài liệu này ghi kết quả rà soát R05.1–R05.4 trên nền đã có từ R00.2/R00.3
(cờ engine cho vuốt di chuyển, trial dùng đồng hồ đơn điệu), R03 (cửa kiểm
quyền intent, suspend/resume, TalkBack) và R04 (điều khiển dùng chung).
Trạng thái: **đã viết, chờ kiểm thử** (xem `REMAINING-CODE-BACKLOG.json`):
chưa build, chưa chạy test, chưa tạo APK trong đợt này. Test ID sẽ chạy ở
giai đoạn 2: T07, T09–T12, T19, T50, T64.

Phần lớn R05 đã đạt từ các mục trước; đợt này **rà soát lại từng điểm bằng
đường mã**, chỉ sửa đúng các khe hở tìm thấy (mục 3 và mục 5), không viết
lại cơ chế cử chỉ. Tham chiếu ý tưởng từ gói GnollHack tham khảo: tách
recognizer / bản đồ cử chỉ / ngưỡng thành các phần riêng
(`AccessibilityGestureRecognizer`, `AccessibilityGestureMap`,
`AccessibilityGestureThresholds`, `AccessibilityGestureSettings`). CDDA đạt
cùng cách tách đó theo cấu trúc sẵn có: recognizer + phân vùng hướng nằm ở
`ReaderGestureSurface`, bản đồ hai trục + ngưỡng (khoảng cách dp, thời lượng
ms, tay thuận) nằm ở `ReaderPreferences`, hành động nằm ở
`GameReaderController`. Không chép mã C#.

## 1. R05.1 — Gán hai trục, trial 45 giây, cấu hình đã xác nhận là thứ duy nhất sống sót

**Kết luận: đã đạt tại R00.3, rà soát lại ở R05 — không sửa.** Bằng chứng
đường mã trong `ReaderPreferences.java` hiện tại:

- Gán hai trục còn đúng: `horizontal_swipe_action` / `vertical_swipe_action`
  là cấu hình đã xác nhận; ứng viên của trial ở hai khóa
  `trial_horizontal_swipe_action` / `trial_vertical_swipe_action` riêng.
  `beginGestureMapTrial` ép ít nhất một trục là `GESTURE_ACTION_NAVIGATE`
  (không bao giờ mất đường duyệt menu một ngón). Thời lượng mặc định giữ
  nguyên `GESTURE_MAP_TRIAL_MS = 45_000`.
- Deadline đơn điệu: `SystemClock.elapsedRealtime()`; không còn
  `System.currentTimeMillis()` và không còn khóa deadline/pending nào được
  ghi xuống đĩa (`gesture_map_trial_pending`, `gesture_map_trial_deadline`
  chỉ còn là tên khóa legacy để dọn).
- Pending + thời điểm bắt đầu chỉ ở biến tĩnh bộ nhớ tiến trình
  (`trialPendingInMemory`, `trialStartedElapsedMs`) — bắt buộc là static vì
  `ReaderSetupActivity` (bắt đầu trial) và `GameReaderController` (đếm giờ,
  xác nhận) là hai instance khác nhau.
- **Trường hợp ứng viên sót sau process chết (điểm rà soát kỹ nhất đợt
  này): đã kín.** Constructor của `ReaderPreferences`: nếu tiến trình hiện
  tại không có trial sống (`!trialPendingInMemory`) mà trên đĩa còn bất kỳ
  khóa trial nào (hai khóa ứng viên mới ghi hoặc hai khóa legacy) thì xóa
  tất cả. Vì vậy ứng viên do `beginGestureMapTrial` ghi ngay trước khi
  process chết chỉ là dữ liệu trơ: lần khởi động sau, instance
  `ReaderPreferences` đầu tiên (Setup/Controller đều tạo ngay khi mở) dọn
  nó trước khi bất kỳ `getGestureAction` nào đọc, và `getGestureAction`
  vốn chỉ đọc ứng viên khi `isGestureMapTrialActive()` — vốn cần pending
  trong bộ nhớ. Restart / reboot / process chết đều trả cấu hình đã xác
  nhận. Ghi chú trung thực: `apply()` của Android cập nhật bản đồ trong bộ
  nhớ đồng bộ trước khi ghi đĩa, nên không có cửa sổ nào mà ứng viên trên
  đĩa tồn tại mà constructor không thấy.
- Hết hạn: `isGestureMapTrialActive()` tự hoàn trial khi quá 45 giây kể từ
  `startGestureMapTrialTimer()` (gọi ở `GameReaderController.onResume`);
  runnable đếm giờ `scheduleGestureMapTrialTimeout` hoàn trial, hủy cử chỉ
  đang dở và xóa trạng thái practice còn sót (R00.3).
- TalkBack: listener đổi trạng thái khám phá chạm trong
  `GameReaderController` hoàn trial đang chờ ngay khi TalkBack bật
  (`revertedTrial`), đọc thông báo hoàn ánh xạ; khi TalkBack đang bật,
  `ReaderSetupActivity.tryGestureMapping()` từ chối bắt đầu trial
  (`a11yGestureMappingTalkBack`) — không có thay đổi cấu hình nào diễn ra
  trong lúc người dùng không thử được bằng cử chỉ của bộ đọc.

## 2. R05.2 — Bài tập luyện cử chỉ trong ngữ cảnh độc lập

Bài luyện tám hướng (`GameReaderController.beginMovementPractice` /
`practiceMovementDirection`, bề mặt `ReaderGestureSurface`) — rà soát từng
kỹ năng kế hoạch yêu cầu:

| Kỹ năng | Đường mã | Kết luận |
| --- | --- | --- |
| Tám hướng | `ReaderGestureSurface.directionForSwipe` phân vùng 8 hướng (biên tan 22,5°); practice đếm bitmask `movementPracticeDirections`, đủ 8 mới hoàn thành và lưu `movement_practice_complete` | Đạt |
| Đọc từng hướng + tiến độ | Mỗi vuốt đọc nhãn hướng của node lệnh (`node.label`) + chuỗi `a11yReaderMovementPracticeProgress` "%1$s. %2$d of 8 directions practiced. No game action was sent." (Anh + Việt) | Đạt |
| Không gửi gameplay | `practiceMovementDirection` không gọi `send(...)` ở bất kỳ nhánh nào; `movementPracticeEnabled()` cố ý không đòi cờ engine vì bài luyện không phải intent gameplay (ghi chú trong mã từ R00.2) | Đạt |
| Thoát | Chạm hai lần (`activate()` → `endMovementPractice`) hoặc Back (`onBackPressed` → `endMovementPractice`); thoát đọc `a11yReaderMovementPracticeExited` và **không đổi cấu hình thật** (fast movement giữ nguyên, tiến độ trong phiên bị xóa) | Đạt |
| Học lại | Vào lại bài luyện đặt `movementPracticeDirections = 0` — đếm lại từ đầu. **Đợt này bổ sung:** chuỗi hướng dẫn `a11yReaderMovementPracticeInstructions` (Anh + Việt) nay nói rõ điều này ("starting practice again resets your progress…"), trước đây người dùng không được cho biết | Đã sửa ở R05 |
| Duyệt / chọn / đọc / hủy (kỹ năng cử chỉ chung) | Không nằm trong bài tám hướng mà ở cơ chế cử chỉ chung, được bài trial ánh xạ ở Setup + trợ giúp luyện: duyệt = vuốt theo ánh xạ trục (ít nhất một trục luôn là duyệt); chọn = chạm hai lần; đọc = nhấn giữ (`detail`) hoặc ánh xạ `READ_CURRENT`; hủy = Back mở Reader controls / mục "Cancel one game menu". Chuỗi trợ giúp `a11yReaderGestureInstructions` (Anh + Việt) mô tả đủ cả bốn | Đạt, ghi rõ phân bổ này để giai đoạn 2 kiểm đúng chỗ |

Hai giới hạn giữ nguyên có ghi nhận:

- Bài luyện cần màn thế giới (`world:here`) vì nhãn hướng lấy từ node lệnh
  của snapshot thế giới — vào bài luyện ngoài màn thế giới nhận thông báo
  `a11yReaderMovementPracticeWorldRequired`. Đây là thiết kế đã ghi trong
  sổ hành trình ("vùng luyện trong game"); tách nhãn hướng khỏi snapshot
  sẽ tạo nguồn tên hướng thứ hai có nguy cơ lệch bản đồ. Để nguyên, ghi
  vào mục 6.
- Hai bài tập không sống cùng lúc: vào practice thì hoàn trial trước, trial
  hết hạn thì xóa practice (R00.3, rà soát lại — còn đúng).

## 3. R05.3 — Di chuyển nhanh qua quyền engine, hủy cử chỉ khi đổi chế độ, hướng bản đồ/isometric, khóa xe/remote

### 3.1 Quyền engine (đã nối ở R00.2/R03, rà soát lại — không sửa)

`ReaderGestureSurface.handleSwipe` rẽ nhánh theo thứ tự cứng: practice →
`worldMovementEnabled()` → vuốt theo ánh xạ menu. `worldMovementEnabled()`
đòi đồng thời: không controls / không practice / không editor, preference
`fast_movement` của người dùng, `snapshot.swipeMovementAllowed` (cờ engine
R00.2: chỉ đi bộ — không deathcam, không chết, không ở/trong/lái xe, không
còn hai khóa remote `remote_controlling` và `remote_controlling_vehicle`,
không activity/destination/ngủ), node `world:here`, và đủ 8 node lệnh
hướng. `moveDirection` kiểm lại `worldMovementEnabled()` lần nữa trước khi
gửi, và intent vẫn qua cửa kiểm quyền R03.

### 3.2 Hủy cử chỉ khi đổi chế độ — đã rà soát và bổ sung ở R05

Cơ chế nền đã có: chuỗi pointer thuộc về một "owner" là chuỗi ngữ cảnh
(`gestureContext()` = epoch trình bày + ngữ cảnh màn + revision + trạng
thái controls/editor); owner đổi thì `valid()` sai và vuốt không được phân
phát; `ACTION_CANCEL`/đa điểm chỉ hủy, không sinh intent. Các điểm đổi chế
độ đã gọi `root.cancelGesture()` từ trước: mở editor/form/search, mở
controls, vào practice, xác nhận/hoàn trial, hết hạn trial, mất focus cửa
sổ, tạm dừng (`onPause`), đổi trạng thái TalkBack, đóng controller.

**Khe hở tìm thấy và đã sửa ở R05** (`GameReaderController.java`): nhánh
đổi ngữ cảnh màn trong `receive()` (snapshot thuộc context mới) đặt lại
controls/practice/editor nhưng không hủy cử chỉ đang dở — trước đây chỉ
dựa vào owner-check của bề mặt. Nay nhánh này gọi `root.cancelGesture()`
tường minh. Cùng lý do, hai lối rời controls trong `performControl`
(Return — case 0, và Cancel — case 5) và nhánh đóng controls của
`onBackPressed` cũng gọi `root.cancelGesture()` trước khi render lại.

### 3.3 Định nghĩa hướng: bản đồ thường và isometric

Hướng cử chỉ được định nghĩa **theo tọa độ bản đồ của game**, giống nhau
trong mọi tileset: vuốt lên = `UP` = hướng bắc của bản đồ (dy < 0), và tên
đọc to là nhãn của chính node lệnh đó. Bằng chứng:

- `GameReaderController.actionForDirection` ánh xạ (dx, dy) → ID hành động
  bản đồ (`UP`, `RIGHTUP`, …); các node hướng do `a11y::command_snapshot`
  dựng từ `input_context` thế giới, cùng các hành động engine dùng cho
  phím; `a11y_world::relative_name` xác nhận dy < 0 là "North".
- Bài luyện đọc `node.label` của đúng node đó, nên hướng người dùng học
  khớp hướng được đọc trong game (kế hoạch gốc T19: "Hướng đọc khớp tọa
  độ game").
- Isometric trong CDDA là thuộc tính **trình bày của tileset**
  (`cata_tiles::is_isometric()`), chỉ đổi cách vẽ và phép đổi hướng cho
  đầu vào bằng mắt/chuột của người sáng (`get_movement_action_from_delta`
  với `iso_rotate`, dùng ở đường pointer). Đường bộ đọc không vẽ ô, không
  đi qua phép đổi đó (`src/a11y_world.cpp`, `a11y_perception.cpp`,
  `a11y_runtime.cpp` không tham chiếu tileset/isometric), nên **không có
  phép quay 45° nào áp lên cử chỉ**: bật tileset isometric không đổi
  nghĩa bất kỳ vuốt nào. Ghi rõ giới hạn bằng chứng: kết luận này là đối
  chiếu đường mã; tileset mặc định của bản Android phát hành do cấu hình
  đóng gói quyết định (R01/R06), và vì nghĩa cử chỉ không phụ thuộc
  tileset nên lựa chọn đó không mở lại mục này.

### 3.4 Cử chỉ đi bộ không dùng cho tăng ga xe / remote — bằng chứng đường mã

- Bề mặt cử chỉ chỉ có đúng các lối ra: `navigate`, `practiceMovementDirection`,
  `moveDirection`, `activate`, `detail`, `readCurrent`, `stopSpeech`.
  Không tồn tại lối "tăng ga" hay "điều khiển xe" nào trong
  `ReaderGestureSurface`.
- `moveDirection` là lối duy nhất biến vuốt thành intent di chuyển, bị
  chặn bởi `worldMovementEnabled()` (mục 3.1): đang lái/ở trong xe thì cờ
  engine tắt; đang remote (cả xe RC lẫn xe từ xa) thì cờ engine tắt do hai
  khóa remote. Khi cờ tắt, vuốt rơi xuống nhánh ánh xạ menu (duyệt/đọc),
  **không** rơi xuống bất kỳ lệnh xe nào.
- Lệnh xe (`control_vehicle` trong builder lệnh dùng chung R04) chỉ tới
  được bằng duyệt + kích hoạt có chủ ý trong trung tâm lệnh — một hành
  động menu được đọc tên trước, không phải cử chỉ đi bộ. Hai đường này
  không dùng chung lối vào.

## 4. R05.4 — Cứu hộ một ngón và quyết định hoán vị tám hướng

### 4.1 Mọi chức năng cứu hộ tới được bằng một ngón — bằng chứng đường mã

Điểm vào duy nhất: nút **Reader controls** là một nút Android thường trong
hàng công cụ của `GameReaderController` (chạm một ngón / TalkBack focus +
kích hoạt đều tới được; trong chế độ tự đọc, Back cũng mở controls qua
`onBackPressed` → `showControls`). Trong Reader controls, mọi mục là nút
có nhãn, duyệt bằng vuốt ánh xạ hoặc chạm trực tiếp:

| Chức năng cứu hộ | Mục trong Reader controls (case của `performControl`) |
| --- | --- |
| Trợ giúp cử chỉ | case 7 — đọc toàn văn `a11yReaderGestureInstructions` |
| Tôi đang ở đâu | case 1 — đọc tiêu đề + tóm tắt màn hiện tại |
| Đọc lại | case 3 — `speech.repeat()` (hoặc đọc lại mục hiện tại khi TalkBack) |
| Dừng đọc | case 4 — `stopSpeech()`; ngoài controls còn có nút Stop speech ở hàng công cụ và chạm hai ngón trên bề mặt |
| Hủy thao tác | case 5 — gửi intent `cancel` qua cửa R03, chỉ tác động màn sở hữu |
| Quay lại / thoát lớp phủ | case 0 — Return; Back cũng đóng controls |
| Khôi phục mặc định cử chỉ | case 6 (Reader setup) → `ReaderSetupActivity`: nút "Restore default gesture settings" là nút thường, nằm trong danh sách tự đọc của Setup (`button()` tự thêm vào `selfVoiceControls`) nên duyệt một ngón tới được; nó gọi `ReaderPreferences.restoreGestureDefaults()` — xóa ánh xạ trục, tay thuận, ngưỡng, ứng viên trial và trạng thái trial trong bộ nhớ, giữ nguyên `fast_movement` và `movement_practice_complete` (bất đối xứng đã ghi ở R00.3) |
| Thoát cấu hình lỗi | Trial sai tự hoàn sau 45 giây hoặc khi TalkBack bật (mục 1); practice thoát bằng Back/chạm hai lần và không đổi cấu hình; fast movement tắt được ở case 11 của controls. Không chức năng cứu hộ nào đòi vuốt chéo, vuốt hai ngón hay cử chỉ khó: chạm hai ngón duy nhất trong hệ là lối tắt dừng đọc, và dừng đọc luôn có nút thay thế |

### 4.2 Hoán vị tám hướng — quyết định: HOÃN, có truy vết

Truy vết yêu cầu đã rà soát ở đợt này:

- Kế hoạch còn lại (bản 2.0) chỉ nêu hoán vị ở dạng điều kiện: R05.4 "nếu
  giữ yêu cầu đổi tám hướng thì…" và câu đóng mục R05 "Hoán vị tám hướng
  là phần mở rộng cần truy vết yêu cầu, không phải lý do xóa cơ chế hai
  trục đang có."
- Kế hoạch gốc trong repo (`doc/accessibility/PLAN.vi.md`): cấu hình học
  dễ (mục tại dòng 230) và cấu hình chơi nhanh (dòng 232) chỉ yêu cầu vuốt
  tám hướng theo đúng hướng đã đặt tên và chuyển được giữa hai cấu hình;
  T19 yêu cầu "Hướng đọc khớp tọa độ game và không đổi theo xoay màn
  hình". Không có yêu cầu đổi/hoán vị hướng nào.
- `IMPLEMENTATION.vi.md`, `COVERAGE.md`, `STATUS.md` và sổ hành trình
  (`JOURNEY-LEDGER.json`, hành trình `luyen-tap-cu-chi` ghi chú sẵn:
  "Hoán vị tám hướng là mở rộng của R05.4, chưa viết") không chứa yêu cầu
  hoán vị nào khác.
- Gói GnollHack tham khảo cũng không có hoán vị hướng: grep
  Remap/Permut/Swap/Rotate/Mirror/Invert trong `Accessibility/Gestures/*`
  và `Accessibility/Settings/*` cho 0 kết quả (chỉ có tay thuận/ngưỡng/bản
  đồ hành động).

Kết luận: **không tồn tại yêu cầu hoán vị tám hướng truy vết được** ngoài
câu điều kiện trong chính kế hoạch còn lại. Theo đúng điều kiện của kế
hoạch, đợt này **không triển khai** hoán vị: không viết bảng hoán vị, không
viết vùng thử thứ hai, không viết hệ preference thứ hai, và giữ nguyên cơ
chế hai trục + tám hướng theo tên đang có. Khi nào có yêu cầu thật (người
dùng nêu hoặc tài liệu gốc bổ sung), cách triển khai đã được kế hoạch
khóa sẵn: bảng hoán vị hợp lệ (mỗi hướng đích đúng một lần) trên cùng cơ
chế preference hiện có, có vùng thử riêng — lúc đó mở lại R05.4, không
mở mục mới.

## 5. Thay đổi mã của đợt R05 (tóm tắt)

Chỉ hai tệp mã/nguồn lực cộng tài liệu và sổ sách:

- `GameReaderController.java`: `root.cancelGesture()` tường minh ở nhánh
  đổi ngữ cảnh của `receive()`, ở case 0 và case 5 của `performControl`,
  và ở nhánh đóng controls của `onBackPressed` (mục 3.2).
- `values/a11y_reader.xml` + `values-vi/a11y_reader.xml`: chuỗi hướng dẫn
  bài luyện nói rõ thoát bất cứ lúc nào và vào lại sẽ đếm lại từ đầu
  (mục 2).
- Tài liệu này; cập nhật `REMAINING-CODE-BACKLOG.json` (R05.1–R05.4) và
  ghi chú hành trình `luyen-tap-cu-chi` trong `JOURNEY-LEDGER.json`
  (sửa bằng chứng: bài luyện nằm ở `GameReaderController`, Setup là nơi
  bắt đầu trial ánh xạ — bản ghi cũ ghi nhầm vùng luyện ở Setup).
  Trạng thái của hành trình không đổi: đã viết, chờ kiểm thử.

Không đổi chữ ký công khai, không đổi schema snapshot, không đổi
preference nào. Các bài test nguồn hiện có
(`ReaderGestureSurfaceTest`, `ReaderSetupActivityTest`) không tham chiếu
chuỗi hướng dẫn hay các điểm hủy mới — tính nhất quán này kiểm bằng grep,
không phải bằng chạy test.

## 6. Chỗ chưa chắc / giới hạn đã biết

1. **Chưa biên dịch/chưa chạy** (đúng giai đoạn 1): các sửa đổi Java là
   thêm lời gọi tới phương thức đã tồn tại (`ReaderGestureSurface.cancelGesture`
   công khai, `root` là trường sẵn có ở mọi điểm sửa) và sửa chuỗi tài
   nguyên; tính biên dịch/chạy được chỉ xác nhận ở giai đoạn 2 (T07,
   T09–T12, T19, T50, T64).
2. Bài luyện tám hướng vẫn cần màn thế giới (mục 2): người dùng ở menu
   chính chưa nạp nhân vật chưa luyện được. Muốn bỏ giới hạn này phải có
   quyết định riêng về nguồn nhãn hướng ngoài snapshot; không tự ý làm
   ở R05.
3. Tiến độ bài luyện đọc số đếm ("X trên 8") và tên hướng vừa vuốt, không
   liệt kê các hướng còn thiếu. Đủ theo câu chữ R05.2 ("đọc từng hướng và
   tiến độ"); nếu nghiệm thu giai đoạn 3 thấy cần liệt kê hướng còn
   thiếu, đó là bổ sung nhỏ trong `practiceMovementDirection`, không đổi
   cơ chế.
4. Owner-check của bề mặt cử chỉ và `cancelGesture()` tường minh là hai
   lớp chặn cùng một rủi ro; lớp owner-check phụ thuộc `gestureContext()`
   đổi khi revision đổi — snapshot world đổi revision theo nhịp game nên
   trong thực tế cử chỉ đang dở qua một lần publish mới cũng bị hủy. Bài
   T11/T12 ở giai đoạn 2 nên khóa hành vi vuốt kéo qua lúc đổi màn.
5. Quyết định hoãn hoán vị (mục 4.2) dựa trên truy vết tài liệu hiện có
   trong repo và gói tham khảo; nếu yêu cầu thật nằm trong bản local chưa
   commit (phía thứ ba còn thiếu từ R00), R05.4 phải mở lại cùng điều
   kiện với R00.1.
