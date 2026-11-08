# R20 — Lưu, phục hồi, vòng đời và kết thúc

Trạng thái: **đã viết, chờ kiểm thử** (giai đoạn 1 chỉ viết mã — không build,
không chạy test/runner, không giả lập/APK; mọi đối chiếu bằng đọc mã/grep).
Test sẽ chạy ở giai đoạn 2: T28, T53–T58, T63.

Quy tắc cứng của mục: lỗi lưu/tải không bao giờ tự ghi đè hay xóa dữ liệu
người chơi làm cách sửa mặc định; backup qua staging và chỉ thay thế sau khi
kiểm chứng; không lệnh/xác nhận nào của phiên cũ được phát muộn sau process
death; tri thức nhân vật cũ không rò sang nhân vật mới.

## R20.1 — Lưu/tải nhiều lớp, xóa save đơn, lỗi bản lưu (`src/main_menu.cpp`, `src/handle_action.cpp`, `src/game_io.cpp`)

- **Chi tiết save trong danh sách tải:** `load_character_tab` gắn `a11y_value`
  cho từng entry: tên world, thời lượng chơi từ tệp `.pt` của engine (không có
  thì công bố "Play time: not recorded."), và "Last saved" từ mtime của đúng
  tệp `.sav`/`.sav.zzip` qua helper `get_save_last_saved_text`
  (`main_menu.cpp:1326`). Ghi rõ nguồn: đây là mtime hệ thống tệp, đổi nếu thư
  mục save bị sao chép — không phải dữ kiện trong save.
- **Xóa từng save đơn (mới — trước đây không tồn tại ở đâu; R07 ghi nhận
  thuộc R20):** khi bộ đọc bật, chọn một save mở `query_popup` context
  `LOAD_DELETE_CANCEL`: LOAD là xác nhận tải riêng; DELETE phải qua thêm
  `query_yn` nêu tên nhân vật + world + "không thể hoàn tác". Xóa do
  `delete_character_save_files` (`main_menu.cpp:1370`): mọi tệp tiền tố
  `base_path()+"."` trong thư mục world (cùng tiền tố `move_save_to_graveyard`
  dùng) + tệp diary `base64(save_id+"_diary")+".json"`. Entry chỉ bị gỡ khỏi
  `world_saves`/danh sách khi tệp save tải được đã thật sự mất; xóa dở thì
  popup nói rõ và giữ entry (không tuyên bố đã xóa khi chưa xóa). Danh sách
  dựng lại sau mỗi lần xóa. Bộ đọc tắt → luồng gốc nguyên vẹn. Phạm vi khớp
  graveyard của engine: không xóa achievements (thư mục riêng) và memorial đã
  xuất.
- **Lỗi tải có đường phục hồi:** `main_menu::load_game` — cả nhánh `g->setup()`
  ném ngoại lệ lẫn `g->load()` trả false/ném (nay bọc try/catch) đều popup
  (khi reader bật) nêu tên save/world, khẳng định save **không bị thay đổi,
  ghi đè hay xóa**, và đường về menu chính/danh sách để chọn save khác.
  Nguyên nhân chỉ gộp (hỏng/thiếu/không tương thích) vì engine trả về
  false/debugmsg, không có mã lỗi phân biệt — không bịa phân loại chi tiết hơn.
- **Lưu thất bại không còn im lặng:** `ACTION_SAVE` khi `save()` trả false và
  người chơi không chọn thoát-không-lưu: popup "The game could not be saved.
  You are still in the game; nothing was quit." (`handle_action.cpp:2655`).
  `quicksave()` nay kiểm kết quả `save()`, thất bại popup "Quicksave failed.
  The game was not saved." (`game_io.cpp:915`); autosave đi qua cùng đường này.
- `src/savegame*.cpp` chỉ serialize, không có UI (grep `uilist|popup|query_|
  input_context` rỗng) — không gắn adapter, đúng phạm vi (không đổi định dạng
  save).

## R20.2 — Backup/export/import/restore (Android/Java)

**Kiểm kê `CDDADocumentsProvider`:** gốc là `externalFilesDir.getParentFile()`
(`Android/data/<pkg>/`, gồm `files/` và `cache/`); root có
`FLAG_SUPPORTS_CREATE`; có đường ghi thật (`openDocument` qua
`ParcelFileDescriptor.parseMode`, `createDocument`, `deleteDocument` đệ quy);
chống path traversal bằng canonical path. Nhưng không có Activity/luồng đọc
được nào nối vào nó (journey ghi "0 Reader/a11y"). Provider được giữ nguyên
không sửa — đổi mô hình quyền của provider exported là rủi ro riêng ngoài
phạm vi; luồng trợ năng được xây bằng Storage Access Framework, độc lập với
provider.

**Đường dẫn dữ liệu thật:** `src/main.cpp:659-672` —
`GetAndroidExternalStoragePath()` (= `getExternalFilesDir(null)`) là cả base
path lẫn user dir, nên `save/`, `config/`, `memorial/`, `achievements/`,
`templates/` đều nằm dưới gốc external files đó.

**Lớp mới `ReaderBackupArchive.java`** (thuần Java, chủ sở hữu truyền live
root + tệp journal, giữ khả năng host-test như mẫu `AndroidSpeech`):

- `exportTo(OutputStream)`: chỉ đọc; zip gồm manifest
  `cdda-reader-backup.txt` (marker, format=1, số tệp/byte chính xác, danh sách
  cây) + các cây `save, config, memorial, achievements, templates`. Không có
  tệp save/config nào → lỗi `empty` nói được. Tệp đổi kích thước giữa lúc chép
  (game ghi save chen vào) → lỗi `verify`, không trả archive hỏng.
- `stageImport(InputStream)`: chép vào `.reader_backup_staging` (cùng
  filesystem với cây sống) và kiểm chứng **trước khi đụng dữ liệu sống**:
  ZipFile mở được; manifest khớp số tệp/byte chính xác; ≥1 tệp save/config;
  mọi entry nằm strict dưới cây backup (chặn `..`, `\`, đường tuyệt đối, entry
  trùng); tổng giải nén + 32 MB dự phòng ≤ dung lượng trống (`no_space`);
  giải nén với kiểm canonical path, đếm lại byte theo entry và tổng. Sai bất
  cứ điểm nào → xóa staging, dữ liệu cũ nguyên vẹn.
- `restoreStagedImport()`: đo lại staging trên đĩa (không tin summary suông),
  journal `BACKUP_SWAPPING` (fsync, ở `getFilesDir()` như mẫu R06), đỗ cây
  sống vào `.reader_backup_rollback` bằng rename, đưa cây staged vào, journal
  `BACKUP_COMMITTED`, dọn rollback/staging/journal. Swap lỗi → rollback ngay;
  rollback không xong → journal ở lại SWAPPING và `recoverInterruptedSwap()`
  khôi phục ở lần mở sau. Chỉ cây có trong backup mới bị thay; cây không có
  trong backup giữ nguyên.
- **Chống chế manifest:** danh sách cây trong manifest/summary được đối chiếu
  với đúng tập cây thật sự mang tệp trong archive và đều thuộc cây backup
  (kiểm ở cả `verifyAndExtract` lẫn trước swap). Import không bao giờ thay
  được `data/gfx/lang`.

**Nối UI trong `ReaderSetupActivity`** (mẫu sẵn có: nút thường +
`showStatus/showStatusText` qua TTS/TalkBack, việc nền bằng thread +
`runOnUiThread` có kiểm `isFinishing()/isDestroyed()` và sequence như R03):
mục mới trước nút Return gồm heading + 5 nút (`a11y_backup_export`,
`a11y_backup_import`, `a11y_backup_open_picker`, `a11y_backup_confirm_restore`,
`a11y_backup_cancel_restore`).

- **Hướng dẫn trước picker, hai bước tường minh:** bấm Export/Import lần đầu
  chỉ *nói toàn bộ hướng dẫn* (picker sắp mở, hệ thống tiếp quản màn hình và
  lời đọc của app dừng, chọn trùng tên ở đích thì tệp đích bị thay, quay về
  đây sẽ có thông báo kết quả) và bật nút "Tiếp tục"; picker
  (`ACTION_CREATE_DOCUMENT`/`ACTION_OPEN_DOCUMENT`, `application/zip`) chỉ mở
  ở lần kích hoạt thứ hai, khi lời hướng dẫn đã phát trong lúc app còn sở hữu
  speech. Hủy picker → thông báo `a11yBackupPickerCancelled` tại màn hình này
  — đó là đường quay về được công bố.
- **Tên trùng/ghi đè:** restore không mặc định đè — sau khi staging kiểm
  chứng xong, trạng thái chỉ là "đã sẵn sàng" + thông báo nói rõ phạm vi thay
  thế; khôi phục chỉ chạy từ nút Confirm restore riêng, cùng instance; có nút
  Cancel hủy staging, giữ dữ liệu cũ. Trùng tên phía export do picker hệ
  thống làm trung gian và đã được nêu trong hướng dẫn trước khi mở.
- Mọi lỗi đều nói được và đều nói "chưa có gì thay đổi": 23 chuỗi `a11yBackup*`
  Anh + Việt đối xứng trong `values/a11y_reader.xml` và
  `values-vi/a11y_reader.xml` (`no_space/invalid/verify/empty/swap/io` ánh xạ
  từng chuỗi riêng); `getExternalFilesDir(null) == null` → chuỗi
  `a11yBackupStorageUnavailable`.

**Giới hạn kiến trúc (ghi rõ, không bịa cầu nối):** không có JNI/cầu nối nào
để yêu cầu engine C++ flush save hoặc đóng băng ghi đĩa trong lúc
import/export. Giảm thiểu trong khả năng Java: hướng dẫn import dặn khôi phục
từ màn khởi động *trước* phiên chơi và cảnh báo game đang chạy có thể lưu đè
về sau; export kiểm chứng byte trực tiếp nên save bị ghi chen giữa chừng trở
thành lỗi nói được thay vì archive hỏng; thông báo restore xong yêu cầu khởi
động lại game. Chặn cứng import khi phiên game đang sống cần cờ trạng thái từ
phía C++/JNI — điểm mở ghi cho giai đoạn 2 quyết định, không tự chế.

## R20.3 — Vòng đời/epoch phía Java (kiểm chứng + một lỗ hổng đã khép)

Đã đủ theo bằng chứng, không sửa:

- `GameReaderController`: `closed` latch chặn
  `send/onResume/onWindowFocusChanged/onBackPressed`; `presentationEpoch` tăng
  ở resume/pause/touch-exploration và `receive()` loại callback theo epoch;
  `onPause` xóa snapshot/editor/`formRequest`/`editorRequest`/gesture timeout;
  `close()` gọi `handler.removeCallbacksAndMessages(null)`.
- `ReaderPreferences`: trial cử chỉ chỉ ở bộ nhớ tĩnh + dọn khóa legacy khi
  khởi tạo — không sống qua process death (tiếp nối R00.3).
- `ReaderSetupActivity`: không đọc gì từ `savedInstanceState`; các view xác
  nhận có `setSaveEnabled(false)`; nút "đã nghe mẫu" luôn khởi tạo disabled —
  xác nhận giọng không sống qua recreate. `SplashScreen` chỉ khôi phục boolean
  `reader_setup_opened` (cờ điều hướng, không phải xác nhận/lệnh).
- Xoay màn hình: manifest có `configChanges` gồm
  `orientation|screenSize|...` cho Splash và game nên game không recreate khi
  xoay; Setup recreate rơi về trạng thái an toàn như trên.
- Picker backup: toàn bộ trạng thái (`pendingBackupAction`,
  `stagedBackupReady`, sequence) chỉ ở bộ nhớ instance, không ghi
  savedInstanceState/preferences; instance Setup mới gọi
  `recoverInterruptedSwap()` rồi `cancelStagedImport()` — staged import của
  instance đã chết không thể được khôi phục "hộ".

**Lỗ hổng thật đã sửa:** `ReaderSpeechQueue.stop()` giữ
`lastText/lastKey`/`contextKey`, nên sau pause/mất focus, nút Repeat ở phiên
foreground sau có thể phát lại lời của phiên cũ (đặc biệt khi snapshot mới có
cùng context key — `setContext` chỉ dọn khi context *đổi*); retry khởi tạo
TTS có giới hạn cũng không bị hủy khi pause, engine có thể sống lại ở nền. Đã
thêm `ReaderSpeechQueue.clearForLifecycle()` (hủy init retry + clear requests
+ hủy ngữ cảnh/last utterance) và nối vào `GameReaderController.onPause()` và
nhánh mất focus của `onWindowFocusChanged()` thay cho `speech.stop()` ở đúng
hai ranh giới đó. Stop tường minh trong cùng phiên vẫn giữ Repeat như thiết
kế.

**Giới hạn ghi rõ:** grep toàn cây Java cho `AudioFocus|requestAudioFocus`
không có — lớp Java không tự giữ/thả audio focus; việc dừng khi khóa máy/
cuộc gọi đi qua `onPause`/`onWindowFocusChanged` của Activity (controller
dừng lời; Splash/Setup `onPause` cancel speech và `rescuePlayer.stop()`),
còn focus của luồng TTS thuộc tiến trình engine TTS. Không bịa thêm cơ chế
focus mới ở đợt này.

**Nâng cấp APK giữ save/config — đã đủ theo bằng chứng, không sửa:** mọi dữ
liệu người chơi nằm dưới external files dir (không phải cache, không phải thư
mục staging cài đặt chạm tới): swap cài đặt R06 trong `SplashScreen` chỉ di
chuyển `DATA_TOPS = {"data","gfx","lang"}` và `restorePreservedCustom` đưa nội
dung custom về; `save/`, `config/`, `memorial/`… nằm ngoài ba cây đó nên cập
nhật APK không đụng vào; journal cài đặt ở `getFilesDir()` sống qua cập nhật;
manifest `allowBackup="true"`. Rủi ro thật duy nhất là *gỡ cài đặt* (không
phải nâng cấp) xóa cả external app dir — hành vi nền tảng, và luồng export SAF
ở R20.2 là đường giảm thiểu nay đã có.

## R20.4 — Kết thúc và hậu game

**Đã đủ theo bằng chứng — giữ nguyên, không viết lại:**

- **Deathcam + chấp nhận tử vong:** world snapshot có "Death camera center" +
  `swipe_movement_allowed = false` khi có `camera_offset` (`a11y_world.cpp`);
  QUIT trong context deathcam tên "Accept your fate" (`handle_action.cpp:280`),
  handler `QUIT_WATCH → QUIT_DIED` (`handle_action.cpp:2916`).
- **Lời cuối + màn kết thúc:** scope `end_screen` (`end_screen.cpp:72`) —
  document chi tiết, node `end:last_words` set_text ghi thẳng `p_impl.text`
  (giá trị gửi kèm sự kiện `game_avatar_death`), Continue qua TEXT.CONFIRM.
  Không có hủy nhưng có bước tiếp hợp lệ.
- **Nhật ký/điểm:** `diary::death_entry` hỏi `query_yn` (adapter Dialog) →
  scope diary đã có + tự export memorial; scope `scores` (`scores_ui.cpp:129`,
  5 tab) giữ nguyên.
- **Xử lý thế giới khi còn nhân vật:** popup gốc "World retained. Characters
  remaining…" + nhánh query dùng uilist dùng chung/`query_yn` — đã qua được bộ
  đọc. **Trở về menu:** `cleanup_at_end()` trả true → `main.cpp:877-883` quay
  lại `opening_screen()`.

**Khoảng trống thật đã đóng:**

- `scrollable_text` (`src/output.cpp:494`) trước đây có 0 điểm a11y nhưng là
  màn hiển thị epilogue NPC (`game.cpp:3052`) và faction (`game.cpp:3067`)
  sau chết → kẹt hoàn toàn. Nay có scope `SCROLLABLE_TEXT`: document toàn văn
  + command_snapshot (CONFIRM/QUIT/cuộn) qua `registered_intent`; focus chỉ
  quan sát.
- `Messages::dialog` (`src/messages.cpp:889,948`) trước đây cũng không có
  scope, mà `death_screen()` gọi `display_messages()` trước nhật ký/điểm. Nay
  có scope `MESSAGE_LOG` + `dialog::reader_update`: document gồm đúng các
  thông báo dialog đang hiển thị (thứ tự + bộ lọc của chính dialog, kèm thời
  gian tương đối) + command nodes (FILTER/RESET_FILTER/QUIT…); lọc vẫn qua
  popup gốc. Lưu ý phạm vi: đây là mức chống kẹt cho chuỗi hậu game; hành
  trình lịch sử thông báo đầy đủ (thứ tự đọc, không đọc kép) vẫn thuộc R21 —
  sổ đã ghi chú tương ứng.
- Sau `move_save_to_graveyard()` + `write_memorial_file()`:
  `announce_game_message` cho biết memorial đã ghi và save đã chuyển vào
  graveyard (`src/do_turn.cpp:154`) — trước đây hoàn toàn tự động và im lặng.
- Xóa/reset world sau kết thúc (tự động theo `WORLD_END` hoặc chọn từ menu
  query): popup kết quả nêu tên world (chụp tên trước khi xóa), cùng chuỗi R07
  dùng ở menu (`src/do_turn.cpp:204-217`).

**Nghỉ hưu — không có luồng riêng, không bịa màn hình:** grep
main_menu/game/handle_action không tồn tại luồng retirement riêng. Tương
đương thật là lệnh `SUICIDE` tên hiển thị **"Abandon character"**
(keybindings.json), đăng ký vô điều kiện ở DEFAULTMODE (`game.cpp:2678`) nên
có node lệnh trong world snapshot; handler hỏi hai `query_yn` ("Abandon this
character?" → "This will kill your character. Continue?") rồi vào cùng chuỗi
hậu game ở trên.

**Rò tri thức khi load/đổi nhân vật — kiểm chứng đã đủ, không bổ sung:**
`game::setup()` gọi `a11y::replace_game_session()` (`game.cpp:691`) và mọi
đường vào game (load/tạo mới/mẫu/tutorial) đều qua `setup()` trước
`start_game()`/`load()`; `game::load(save_t)` gọi thêm lần nữa
(`game_io.cpp:325`). Hàm này reset status monitor R10, action outcomes R08,
vô hiệu session context, xóa mailbox + snapshot. Rà
`a11y_perception/world/outcome`: không còn cache tĩnh theo nhân vật nào khác;
con trỏ explore là biến cục bộ gắn `lifecycle_epoch`
(`handle_action.cpp:297-310`).

## Dịch

16 msgid mới phía C++ vào `lang/po/accessibility/vi.po` (2 chuỗi world
deleted/reset và "Filter: %s" đã có sẵn, không thêm trùng); 23 chuỗi
`a11yBackup*` Anh/Việt đối xứng phía Java (xem R20.2).

## Chỗ chưa chắc (giai đoạn 2 chốt)

1. Chưa biên dịch — chữ ký/API đã đối chiếu tại nguồn: phía C++
   (`get_files_from_path` bản cata_path ở `filesystem.cpp:439`,
   `base64_encode`, `zzip_suffix`, idiom `file_time_type` của `tgz_archiver`,
   `localtime_s/r`); phía Java (ZipFile/ZipOutputStream chuẩn,
   `ContentResolver.openOutputStream` bắt `IOException`). Xác nhận biên dịch
   thuộc giai đoạn 2.
2. Popup lỗi tải gộp nguyên nhân vì engine không cấp mã lỗi phân biệt.
3. Document `MESSAGE_LOG` chứa toàn bộ lịch sử đang hiển thị — lịch sử rất
   dài sẽ là document lớn; cần bài T38/T58 ở giai đoạn 2 để chốt kích
   thước/nhịp đọc.
4. Chặn cứng import backup khi phiên game đang sống cần cờ C++/JNI — điểm mở
   của R20.2, giai đoạn 2 quyết định.
