# R21 — Lời đọc, âm hiệu, tiếng Việt và chẩn đoán

Trạng thái: **đã viết, chờ kiểm thử** (giai đoạn 1 chỉ viết mã — không build,
không chạy test/runner, không giả lập/APK; mọi đối chiếu bằng đọc mã/grep).
Test sẽ chạy ở giai đoạn 2: T16, T22–T24, T30, T38, T54–T56, T60–T63.

Quy tắc cứng của mục: cue âm thanh/rung tuân quyền biết đúng như chữ (không
cue nào được lộ thông tin nhân vật chưa được biết) và luôn có tương đương
lời; không tuyên bố đạt ngưỡng độ trễ nào khi chưa đo — chỉ đo và ghi lại;
log chẩn đoán mặc định KHÔNG chứa save/dữ liệu cá nhân/nội dung câu đọc;
chuỗi mới phải vào đúng nguồn dịch để quy trình đóng gói nhặt được.

## R21.1 — Hàng lời: ưu tiên, gộp trùng, cooldown, hết hạn, đọc tiếp tài liệu

Nền giữ nguyên từ R04/R06: bốn mức ưu tiên trong `ReaderSpeechQueue`
(LONG_TEXT=0, INFORMATION=1, FOCUS=2, URGENT=3), focus debounce 120 ms,
hàng chờ tối đa `MAX_PENDING=8`, hết hạn mặc định focus 2 giây / còn lại
15 giây, và `clearForLifecycle()` của R20 (hủy retry khởi tạo TTS, xóa
request + ngữ cảnh + câu cuối khi pause/mất focus — Repeat của phiên trước
không phát sang phiên sau).

Bổ sung ở mục này (`ReaderSpeechQueue.java`):

- **Gộp câu trùng (coalescing phía lời):** engine đã gộp sẵn ở log (câu
  giống hệt + cùng loại trong 3 lượt thì tăng đếm; mẫu cooldown có thể ẩn
  câu) nên những gì tới hàng lời đã là "đáng nói". Hàng lời nay bỏ một
  request mới khi nội dung của nó (a) đang được đọc, (b) đang chờ ở mức
  ưu tiên bằng hoặc cao hơn, hoặc (c) — chỉ với mức dưới URGENT — đã bắt
  đầu đọc trong **cửa sổ 4 giây** (`DUPLICATE_WINDOW_MS`). URGENT không
  bao giờ bị cửa sổ chặn: cảnh báo lặp lại vẫn được đọc lại; nhưng URGENT
  vẫn không tự nhân bản trong hàng chờ/đang đọc (a, b). Mọi lần bỏ đều
  được đếm cho chẩn đoán (mục 4). Bảng `recentStarts` là heuristic có
  giới hạn (xóa khi quá 64 mục) và bị xóa trong `clearForLifecycle()` —
  cửa sổ không vươn qua ranh giới vòng đời.
- **TTL theo loại:** `say(key, text, priority, ttlMs)` mới; thông báo
  game thường (INFORMATION qua `announceGameMessage`) sống **8 giây**,
  khẩn (URGENT) giữ **15 giây** — một loạt thông báo theo lượt không còn
  kể chuyện của lượt cũ nhiều giây sau đó. Lời của màn hình giữ TTL cũ.
- **Đọc tiếp tài liệu không đọc lại từ đầu:** sửa hai lỗi thật trong
  `ReaderDocumentReader.open()`:
  1. Sau `clear()` (pause/đổi ngữ cảnh), đoạn văn bị rỗng nhưng khóa
     phiên còn giữ → mở lại cùng khóa bị coi là "đã mở" và báo hết tài
     liệu: cùng một tài liệu không đọc lại được sau một lần pause cho
     tới khi đổi ngữ cảnh. Nay `open()` tách lại khi khóa khác, HOẶC
     đoạn văn rỗng, HOẶC nội dung đổi dưới cùng khóa.
  2. Nội dung đổi dưới cùng khóa (ví dụ lọc lịch sử thông báo đổi) vẫn
     đọc bản cũ đã tách. Nay tách lại và khôi phục vị trí: vị trí đã lưu
     (hoặc vị trí hiện tại nếu cùng khóa) được kẹp vào văn bản mới; kết
     quả tìm kiếm bị xóa khi nội dung đổi, truy vấn được giữ cho cùng
     khóa. Cảnh báo ngắt giữa chừng vẫn qua `markInterrupted()` của R04 —
     vị trí không nhảy về đầu.

## R21.2 — Âm hiệu: cue/rung tùy chọn, audio focus, lỗi phát công bố bằng lời

**Kênh cue mới, nối xuyên suốt:**
`a11y::android::cue(name)` (`src/a11y_android.h/.cpp`, danh sách trắng đúng
4 tên nội bộ: `blocked`, `danger`, `pickup`, `activity_done` — mọi tên lạ
bị loại ở C++, không dữ kiện engine nào đi qua kênh này) → JNI
`CataclysmDDA.onAccessibilityCue(String)` → `GameReaderController.onGameCue`
(chặn khi controller đã đóng/không ở foreground/bộ đọc tắt; đếm
`cue_<tên>` cho chẩn đoán) → `ReaderCuePlayer.play(name)`.

`ReaderCuePlayer.java` (mới): SoundPool (tối đa 2 stream, thuộc tính
`USAGE_ASSISTANCE_ACCESSIBILITY`/`CONTENT_TYPE_SONIFICATION`) + Vibrator.
Mỗi cue có cooldown riêng để không dội bom khi sự kiện lặp nhanh. Bảng cue:

| Cue | Điểm kích (engine) | Âm thanh | Rung | Tương đương lời (luôn có) | Cooldown |
| --- | --- | --- | --- | --- | --- |
| `blocked` | `a11y::record_action_outcome` khi kind = blocked hoặc safe_mode_blocked | `a11y_cue_blocked.wav` — hai tiếng trầm 170 Hz, 0,19 s | 60–60–60 ms | Node `world:last_outcome` của snapshot ("Blocked by …"/lý do safe mode) — chính outcome vừa ghi | 700 ms |
| `danger` | `a11y::announce_game_message` khi urgent | `a11y_cue_danger.wav` — hai nốt đi lên 620→880 Hz, 0,22 s | 120–80–120 ms | Chính thông báo khẩn vừa được công bố ở mức URGENT | 2500 ms |
| `pickup` | `player_activity::do_turn` khi activity kết thúc và type = ACT_PICKUP, chỉ avatar | `a11y_cue_pickup.wav` — một tiếng 990 Hz, 0,08 s | 40 ms | Thông báo hoàn thành nhặt của engine trong log (finish path tự công bố) | 700 ms |
| `activity_done` | cùng điểm nghẽn trên, khi `moves_total >= 1000` (hoạt động dài, ~10 lượt) | `a11y_cue_done.wav` — hai nốt 784→1046 Hz, 0,24 s | 50–60–50 ms | Thông báo hoàn thành hoạt động của engine + node tiến độ activity (R10) về 100% | 1200 ms |
| `rejected` (chỉ Java) | biên nhận từ chối intent: form/editor giá trị không hợp lệ, node stale/disabled/unsupported | dùng lại `a11y_cue_blocked.wav` | 90 ms | Chính nhãn biên nhận vừa được nói ("…is no longer available." v.v.) | 700 ms |

**Quyền biết:** cả 5 cue đều kích tại đúng điểm engine đã tạo tín hiệu
lời tương đương cho người chơi (outcome đã phân loại chỉ từ điều nhân vật
nhận biết được — R08; thông báo urgent là thông báo engine gửi người
chơi; hoàn thành activity là của chính avatar). Không cue nào đọc dữ
liệu trực tiếp, nên không cue nào lộ thêm thông tin.

**Mặc định TẮT, chọn tham gia rõ ràng:** hai khóa mới trong
`ReaderPreferences` (`sound_cues`, `haptic_cues`, đều mặc định false) và
hai hộp kiểm trong `ReaderSetupActivity` (mục "Cue âm thanh và rung",
lưu ngay như hộp kiểm tay trái của R05; bật lên thì phát mẫu
`activity_done` qua một player ngắn hạn để nghe/sờ đúng thứ vừa bật).
Tắt cả hai = mất đúng 0 thông tin, vì cue chỉ nhân đôi lời.

**Audio focus — ranh giới ghi rõ như R20:** lớp Java vẫn KHÔNG giữ
audio focus cho TTS; focus của luồng TTS thuộc tiến trình engine TTS
(grep toàn cây Java: không `requestAudioFocus` nào ngoài player này).
Cái mới ở mục này nằm trọn trong `ReaderCuePlayer`: mỗi lần phát xin
`AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK` (API ≥ 26 qua `AudioFocusRequest`,
dưới đó qua đường legacy với một listener dùng chung cho request/abandon
để khớp danh tính), nhả sau 900 ms. Bị từ chối focus (ví dụ đang cuộc
gọi) → bỏ qua tiếng (đếm `cue_focus_denied`), rung và lời không ảnh
hưởng. **Lỗi phát công bố bằng lời:** nạp mẫu lỗi / `play()` trả 0 /
không có Vibrator → listener báo một lần mỗi player qua chuỗi
`a11yCueUnavailable` ("Cue âm thanh hoặc rung không khả dụng trên thiết
bị này. Lời đọc vẫn tiếp tục bình thường.") và đếm `cue_unavailable`.
Bốn tệp WAV được tổng hợp ngoại tuyến (sóng sine, 22050 Hz mono 16-bit,
biên độ ≤ 0,5, có fade) trong `res/raw/` — thay được bằng tệp cùng tên.

`RescueAudioPlayer` (R06) không được dùng lại cho cue: nó cắt clip đang
phát và không có focus/ducking — chỉ mượn mẫu đóng gói tệp.

## R21.3 — Tiếng Việt: rà toàn bộ chuỗi R07–R20, thuật ngữ, fallback đánh vần

**Phương pháp rà (làm lại được):** trích mọi chuỗi `_()` một dòng và
nhiều dòng từ diff `ad5ecf0..HEAD` (875 msgid duy nhất theo trích xuất
mở rộng gồm cả chuỗi có `%`), đối chiếu overlay
`lang/po/accessibility/vi.po` và catalog chính `lang/po/vi.po.gz`
(112.946 msgid). Ngay trước mục này: 150 msgid đã có trong overlay, 240
đã có trong catalog chính (không cần thêm — gettext tra catalog gộp),
**485 thiếu cả hai**.

**Kết quả:** dịch và bổ sung đủ **485** msgid đó + **2** msgid mới của
R21.4 ("%d messages." và chuỗi "Showing the newest…") = **487 mục mới**;
overlay nay có **928 msgid** (không kể header), 0 trùng, 0 msgstr rỗng
(4 mục số nhiều tiền hữu giữ văn bản ở `msgstr[0]`, tiếng Việt
`nplurals=1`). Kiểm chứng bằng parser PO tự viết + đối chiếu specifier
`%` từng mục (msgfmt KHÔNG chạy ở giai đoạn 1 — để giai đoạn 2 xác nhận
cùng `compile_mo.py`). Chuỗi của R16 (vùng/chế độ an toàn/tự nhặt —
trước mục này thiếu ~95–119 msgid tùy phương pháp trích) nằm trọn trong
485. Hai `pgettext` duy nhất của diff đã có trong catalog chính, nên cả
487 mục mới đều là msgid thường, không ngữ cảnh.

**Đóng gói (đã xác minh bằng đọc mã, không chạy build):** đường Makefile
gốc nhặt overlay — `lang/Makefile` đặt `CATALOG_OVERLAYS` từ
`lang/po/accessibility/*.po` và mọi luật `.mo` phụ thuộc chúng, gọi
`compile_mo.py`, script này gộp bằng `msgcat --use-first` (**overlay
thắng khi trùng msgid** với catalog chính); `Makefile` gốc cũng liệt kê
overlay trong `MO_DEPS`. **Caveat CMake:** `lang/CMakeLists.txt` không
nhắc overlay và tiền quyet của luật `.mo` chỉ gồm tệp `.po` chính +
`compile_mo.py` — sửa chỉ riêng overlay có thể không kích hoạt dựng lại
`.mo` ở đường CMake; ghi nhận để giai đoạn 2 kiểm chứng, không sửa ở
giai đoạn 1 (ngoài phạm vi mục).

**Tên màu ghi chú (R13):** tên màu đến từ `color.cpp` qua `to_translation`
("brown", "light gray", …) — catalog chính ĐÃ dịch đủ cả 14 tên (nâu, xám
nhạt, xám đậm, đỏ nhạt, đỏ, xanh lá nhạt, xanh lá, xanh lam nhạt, xanh
lam, trắng, xanh lơ, xanh lơ nhạt, hồng, đỏ tươi), nên màu đọc lên đã là
tiếng Việt qua catalog chính. Bốn chuỗi khung của R13 ("Note symbol:
%1$s. Note color: %2$s.", "letter %c", "digit %c", "exclamation mark")
thiếu cả hai catalog — đã vào overlay ở mục này. Mã màu ngắn (`br`,
`lg`, …) là ký hiệu nội bộ, engine cố ý không dịch — giữ nguyên.

**Bảng thuật ngữ (thống nhất ở mục này; hai mục không nhất quán cũ đã sửa
trong overlay: "Vehicle" và "Spells: %s"):**

| English | Tiếng Việt | Ghi chú |
| --- | --- | --- |
| vehicle | xe | bỏ "phương tiện" trong overlay |
| vehicle part | bộ phận xe | |
| spell | phép | bỏ "phép thuật" trong mục "Spells: %s" |
| bionic | Bionic (giữ nguyên) | khớp catalog chính |
| mutation | đột biến | |
| trait | đặc tính | |
| zone | vùng | |
| safe mode | chế độ an toàn | |
| auto-pickup | tự động nhặt | |
| overmap | bản đồ lớn | |
| message log / history | nhật ký thông báo / lịch sử thông báo | |
| companion | đồng đội | |
| follower | người đi theo | |
| column / row | cột / hàng | |
| kcal, mana | giữ nguyên | đơn vị/quốc tế |
| robot / kitten | rô-bốt / mèo con | minigame R18 |

Số nhiều: tiếng Việt không biến đổi danh từ — các cặp msgid số ít/số
nhiều của tiếng Anh (ví dụ "%d column left"/"%d columns left") là hai
mục riêng, cùng một văn bản Việt tự nhiên. Đơn vị và hướng đi theo đúng
số engine công bố, không đổi hệ đo.

**Fallback khi không có dịch:** chuỗi chưa dịch vẫn được nói nguyên văn
tiếng Anh (hành vi gettext chuẩn — không bịa dịch tại chỗ). Bổ sung điều
khiển đọc **"Đánh vần mục hiện tại"** (`a11yReaderSpell`, control cơ số
13 trong `GameReaderController`): đọc mục đang focus qua
`ReaderDocumentReader.spellText` — tách grapheme tiếng Việt bằng ICU
BreakIterator, tối đa 120 grapheme kèm ghi chú cắt — để kiểm chứng
chính xác từng chữ của tên chưa có dịch hoặc tên ngoại ngữ.

Chuỗi Java của chính mục này (cue, chẩn đoán, đánh vần): 21 khóa mới,
thêm đối xứng vào cả `res/values/a11y_reader.xml` và
`res/values-vi/a11y_reader.xml` (152 → 173 khóa mỗi phía, tập khóa khớp
nhau — parity là bất biến đã kiểm).

## R21.4 — Chẩn đoán, lịch sử thông báo đầy đủ, chuẩn bị đo hiệu năng

**`ReaderDiagnostics.java` (mới)** — kho chẩn đoán tĩnh theo tiến trình
(Setup và game là hai Activity cùng tiến trình nên store tĩnh là nơi
chia sẻ trung thực duy nhất):

- *Sự kiện:* vòng đệm 200 mục, mỗi mục là `T+<giây>s <mã>` (đồng hồ đơn
  điệu từ lúc tiến trình sống), mã là literal nội bộ (`lifecycle_resume`,
  `speech_ready`, `receipt_stale`, …). Báo cáo lấy 30 mục mới nhất.
- *Bộ đếm:* map mã → số lần: `announce_info/urgent`, `receipt_<mã>`,
  `cue_<tên>`, `cue_unavailable`, `cue_focus_denied`.
- *Thống kê hàng lời:* `ReaderSpeechQueue.snapshotStats()` — đã đọc
  xong / bị ngắt / bỏ vì trùng / bỏ vì hết hạn / bị đẩy khỏi hàng đầy,
  cộng độ trễ bắt đầu đọc.
- **Hợp đồng nội dung: chỉ mã và số.** Không nội dung câu đọc, không
  nội dung thông báo, không tên save/world/nhân vật — báo cáo an toàn
  để đọc to và để gửi kèm báo lỗi. Vòng đời = vòng đời tiến trình;
  không ghi đĩa, không tồn tại qua process death.
- Phía C++ có sẵn `DebugLog` ghi `config/debug.log` (xoay `.prev` ở
  1 MiB), nhưng đó là log kỹ thuật của engine: nội dung không bị ràng
  buộc bởi hợp đồng không-dữ-liệu-cá-nhân ở trên, và `debugmsg` còn
  bật UI chặn chờ người dùng — nên chẩn đoán của mục này cố ý là kho
  mã/số phía Java, không đọc, không xuất và không ghi vào tệp log đó.

**Độ trễ đo tại chỗ:** tại sự kiện STARTED của `AndroidSpeech`, hàng
lời ghi thời gian từ lúc enqueue (`Request.created`) tới lúc engine
thật sự bắt đầu đọc: giữ lần gần nhất + chậm nhất. Đây là **số đo**,
không kèm ngưỡng nào và không kết luận đạt/chưa đạt — giai đoạn 2 dùng
chính điểm đo này trên máy thật.

**Báo cáo đọc được:** nút "Đọc báo cáo chẩn đoán" trong
`ReaderSetupActivity` (mục "Chẩn đoán"): chưa có phiên bộ đọc nào trong
tiến trình → nói rõ điều đó; có thì đọc tiêu đề + thời gian tiến trình,
dòng lời đọc, dòng độ trễ (kèm câu "Đây chỉ là số đo; không có ngưỡng
nào được tuyên bố."), hàng đợi hiện tại, số thông báo, số biên nhận bị
từ chối, số cue và sự cố cue, rồi 30 sự kiện gần nhất (mã sự kiện là
literal kỹ thuật tiếng Anh — như mã trạng thái C++ — phần còn lại của
báo cáo theo ngôn ngữ giao diện). Toàn bộ qua các chuỗi định dạng mới ở
cả hai tệp `a11y_reader.xml` (21 khóa mới của mục này, xem R21.3).

**Hành trình lịch sử thông báo đầy đủ** (`src/messages.cpp`,
`dialog::reader_update` — nối tiếp scope chống kẹt `MESSAGE_LOG` của
R20):

- Trước đây các thông báo nối bằng `\n` đơn → trình đọc tài liệu gộp cả
  lịch sử thành ~một đoạn, không điều hướng từng thông báo được. Nay
  mỗi thông báo là **một đoạn riêng** (nối `\n\n`), vẫn đúng thứ tự
  hiển thị và bộ lọc của dialog, vẫn kèm thời gian tương đối của dialog.
- **Giới hạn công bố 500 thông báo mới nhất** của dãy đã lọc (cuối dãy
  khi `log_from_top`, đầu dãy còn lại) — lịch sử engine vốn đã giới hạn
  bởi tùy chọn `MESSAGE_LIMIT` (tùy chọn mặc định 255, miền 1–10000;
  kiểm tra cắt chạy trước khi thêm nên trạng thái ổn định giữ 256
  thông báo) nên giới hạn này chỉ chạm khi người dùng nâng tùy chọn.
  Tóm tắt luôn nói số:
  "%d messages." khi không cắt, và khi cắt thì nói rõ đang hiển thị bao
  nhiêu trên tổng số + hướng dẫn dùng bộ lọc; có lọc thì tiền tố
  "Filter: …" giữ nguyên.
- Đọc tiếp/tìm kiếm: tài liệu mở qua cơ chế tài liệu chung — vị trí giữ
  qua redraw (snapshot giống hệt bị dedupe phía C++ nên không công bố
  lại), tìm bằng tìm-kiếm tài liệu, lọc bằng lệnh FILTER gốc của dialog
  (nội dung đổi dưới cùng khóa được xử đúng nhờ sửa `open()` ở R21.1).
  Node rỗng `messages:empty` giữ nguyên khi không có gì hiển thị.

**Đo hiệu năng & chế độ ít hình — chỉ chuẩn bị, không tối ưu:** điểm đo
đã có sẵn để giai đoạn 2 dùng: độ trễ enqueue→start (trên), các bộ đếm
rớt/hết hạn của hàng lời, và chi phí dựng snapshot vốn đã được các mục
trước ghi nhận chưa đo (xe rất lớn — R15; study zone/bảng kiểm tra vượt
nắp 4096 node — R02/R16: snapshot bị TỪ CHỐI chứ không cắt lặng, phân
trang là quyết định giai đoạn 2). Ứng viên "ít hình ảnh" là các tùy
chọn **chỉ-hiển-thị** sẵn có của engine (`src/options.cpp`, không đổi
luật chơi, không đổi tri thức nhân vật): `ANIMATIONS` cùng
`ANIMATION_RAIN/PROJECTILES/SCT/DELAY`, `USE_TILES` (tắt tiles → chữ),
`PIXEL_MINIMAP`, `FORCE_REDRAW`. Mục này KHÔNG đổi mặc định nào (quy
tắc R07.4: không âm thầm đổi tùy chọn); giai đoạn 2 đo trên máy thật
rồi mới quyết có đóng gói thành một preset hay không.

**Dụng cụ sẵn có (không mới ở mục này):** `tools/accessibility/` —
`inventory.py` (quét call-site theo heuristic, không phải bằng chứng bao phủ),
`run_speech_tests.sh`/`run_core_tests.sh` với fake TTS/Handler trong
`fakes/` (không giả lập Android/audio thật), `run_android_smoke.sh`.
Chẩn đoán của mục này là tầng trong-phiên cho người dùng cuối, bổ sung
cho các dụng cụ trên chứ không thay thế.

## Chưa chắc / để giai đoạn 2–3 xác nhận

- Chưa biên dịch: chữ ký API đã đối chiếu tiền lệ trong cây nguồn
  (`USAGE_ASSISTANCE_ACCESSIBILITY` tin là có từ API 21; hành vi
  SoundPool/audio focus trên máy thật, và việc `.mo` dựng lại ở đường
  CMake khi chỉ overlay đổi — thuộc giai đoạn 2).
- msgfmt/`compile_mo.py` chưa chạy trên overlay mở rộng (chỉ parser tự
  viết + đối chiếu specifier) — giai đoạn 2 xác nhận.
- Âm thanh cue là sóng sine tổng hợp: đủ phân biệt 4 sự kiện theo cao
  độ/nhịp, nhưng độ dễ nghe trên loa điện thoại thật thuộc nghiệm thu
  giai đoạn 3; tệp thay cùng tên được.
- Bộ đếm trùng của hàng lời đếm từ sau chốt kiểm tra cùng-khóa tiền hữu
  trong `say()` (chốt đó trả về sớm không đếm) — số đếm là chẩn đoán
  gần đúng theo thiết kế, không phải sổ kế toán.
- Cửa sổ trùng 4 giây và TTL thông báo 8 giây là lựa chọn chính sách
  ban đầu chưa có số đo người dùng thật; cả hai đều là hằng số một chỗ,
  chỉnh được sau kiểm thử giai đoạn 2/3.
