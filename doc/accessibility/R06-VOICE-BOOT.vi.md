# R06 — Giọng, khởi động và cài dữ liệu (giai đoạn 1, chỉ viết mã)

Trạng thái: **đã viết, chờ kiểm thử** cho R06.1, R06.2, R06.4;
R06.3 **mã đã viết và asset đã có thật**, với hai điểm mở ghi rõ ở
mục 3 (giọng asset là eSpeak NG, `CLIP_WELCOME` chưa nối luồng).
Chưa build, chưa chạy test, chưa giả lập, chưa tạo APK trong đợt
này. Test ID sẽ chạy ở giai đoạn 2: T01–T06, T08, T53–T57, T63
(theo `REMAINING-CODE-BACKLOG.json`).

Nền giữ lại, không viết lại: `AndroidSpeech` (timeout khởi tạo 8s,
timeout bắt đầu phát 6s, token/epoch của `SpeechSession` chống callback
trễ của engine đã thay), `ReaderSpeechQueue` (hàng đợi có giới hạn,
hết hạn, theo ngữ cảnh), `BootstrapSelfVoiceSurface`, luồng cài đặt
cũ của `SplashScreen` gồm luật giữ nội dung custom. Đợt này chỉ bổ
sung đúng phần kế hoạch R06 còn thiếu, bằng chứng theo từng mục dưới.

## 1. R06.1 — Bộ chọn engine / voice / ngôn ngữ theo nháp

- `AndroidSpeech` thêm kiểu dữ liệu chọn giọng: `EngineOption`,
  `VoiceOption` (giữ tên giọng ổn định + locale + cờ
  `networkRequired`), và `probeVoices(Context, engineName, listener)`:
  một instance TTS **tạm thời chỉ để dò**, luôn `shutdown()` trước khi
  gọi callback, không bao giờ phát tiếng — không vi phạm nguyên tắc
  một chủ sở hữu phát tiếng tại một thời điểm. Timeout dò 8 giây.
- `ReaderSetupActivity` thêm ba nút chu trình trong danh sách điều
  khiển tự đọc (cùng thứ tự focus với các điều khiển cũ):
  `a11y_voice_language` (ngôn ngữ đọc — chỉ chu trình qua các ngôn
  ngữ thật sự có trong danh sách giọng của engine đã dò),
  `a11y_voice_engine` (`Mặc định hệ thống` + từng engine đã cài),
  `a11y_voice_choice` (giọng của ngôn ngữ đang chọn, kèm dòng trạng
  thái `voiceDataStatus` ghi rõ giọng **hoạt động ngoại tuyến** hay
  **cần mạng** — chuỗi `a11yVoiceOfflineData` / `a11yVoiceNetworkData`
  / `a11yVoiceDefaultData`, Anh + Việt). Đổi engine thì dò lại giọng
  của engine đó; bản nháp không hợp lệ với kết quả dò (ngôn ngữ/giọng
  không còn) được tự sửa về lựa chọn hợp lệ đầu tiên, ưu tiên giọng
  ngoại tuyến.
- Nghe mẫu theo nháp, lưu theo đã thử: nút nghe mẫu đóng băng bộ
  nháp vào `testedEngineName/testedVoiceName/testedLanguageTag` rồi
  mới `speech.initialize(..., testedVoiceName)`. Đổi bất kỳ nháp nào
  cũng gọi `invalidateSampleForChangedSettings()` — tắt nút xác
  nhận. Chỉ nút "Tôi đã nghe mẫu" sau một mẫu hoàn tất mới ghi:
  `confirmHeardAndEnable(...)` như cũ **cộng**
  `ReaderPreferences.saveSpeechSelection(tested...)`. Lựa chọn chưa
  từng được nghe mẫu không thể lọt xuống bộ nhớ.
- Giọng mạng không bao giờ tự chấp nhận: mọi điểm khởi tạo vẫn suy
  `offlineOnly = !permitsNetworkVoice()` từ cùng một khóa đồng ý
  `network_voice_consent` hiện có (queue, Splash, Setup — xem mục 2);
  chọn giọng mạng mà chưa bật đồng ý thì mẫu rơi vào trạng thái
  `NETWORK_REQUIRED` như trước, không phát.
- Thanh tốc độ giữ nguyên 0,5–3,0 (SeekBar 25 nấc 0,1 trong Setup;
  `ReaderPreferences.getRate()` vẫn kẹp lưu trữ 0,2–3,0 như trước —
  không đổi để tránh phá hành vi đã có).
- Lưu trữ: ba khóa mới `speech_engine`, `speech_voice`,
  `speech_language` (mặc định `vi-VN`) trong cùng tệp
  `internal_reader`; rỗng = engine/giọng mặc định hệ thống.
- Hạn chế ghi thật: câu mẫu vẫn là chuỗi mẫu tiếng Việt
  (`a11ySpeechSample`); chọn ngôn ngữ đọc khác vẫn nghe được giọng
  đó nhưng câu mẫu chưa bản địa hoá theo ngôn ngữ — phần chuỗi thuộc
  R21, không che ở đây.

## 2. R06.2 — Sức khỏe TTS, thử lại có giới hạn, giọng bị mất

- Timeout đã có giữ nguyên: khởi tạo 8 giây, bắt đầu phát 6 giây
  (`AndroidSpeech`), hạn hoàn thành từng đoạn ở queue (tới 90 giây
  theo độ dài). Thêm mới là **chính sách thử lại và rơi về mặc định**:
  - `AndroidSpeech` thêm trạng thái `MISSING_VOICE` và quá tải
    `initialize(..., String voiceName)`: giọng được chọn theo tên ổn
    định, đối chiếu lại trong `engine.getVoices()` lúc khởi tạo;
    không còn → `MISSING_VOICE`; là giọng mạng mà đang cấm mạng →
    `NETWORK_REQUIRED`; locale giọng khác ngôn ngữ yêu cầu →
    `MISSING_LANGUAGE`. Không có rơi thầm sang giọng khác ở lớp này.
  - `ReaderSpeechQueue.initialize(ReaderPreferences)` nay dùng lựa
    chọn đã xác nhận (engine/voice/ngôn ngữ) thay cho khóa cứng
    mặc định cũ. Khi `FAILED`: tự khởi tạo lại tối đa **2 lần**
    (+1 giây, +3 giây) rồi mới báo lỗi lên controller — binder chết
    hoặc engine trục trặc thoáng qua tự phục hồi, lỗi cứng thì dừng.
    Khi `MISSING_VOICE`: xóa lựa chọn cũ khỏi preferences, rơi về
    engine/giọng mặc định **đúng một lần**, báo qua
    `Listener.onVoiceRecovered()`; `GameReaderController` hiện thông
    báo `a11yReaderVoiceRecoveredDefault` (Anh + Việt) và cũng xử lý
    `MISSING_VOICE` trong nhánh phục hồi giọng hiện có.
  - Process chết: queue được controller tạo lại và `initialize` đọc
    lại preferences như trước; lựa chọn giọng đã dọn ở trên nên lần
    khởi động sau không lặp lại lỗi giọng đã mất.
- Chủ sở hữu backend: giữ đúng ba chủ sở hữu theo vòng đời đang có —
  `SplashScreen` (bootstrap, đóng ở `onDestroy`), `ReaderSetupActivity`
  (đóng ở `onDestroy`), `ReaderSpeechQueue` trong
  `GameReaderController` (đóng qua `close()` của controller). Không
  tạo TTS mỗi câu (queue giữ một `AndroidSpeech` duy nhất); lớp
  `AndroidSpeech` chỉ giữ `applicationContext`. Instance dò của
  `probeVoices` là ngoại lệ có chủ đích: không phát tiếng, tự hủy
  trước callback, ghi rõ trong javadoc của nó.
- Chuẩn bị cho giai đoạn 2: test host `AndroidSpeechTest` biên dịch
  `AndroidSpeech` với `-Xlint:all -Werror` chống bộ fake trong
  `tools/accessibility/fakes`. Bộ fake đã được mở rộng đúng bề mặt
  API mới (`Voice.getName`, `TextToSpeech.EngineInfo`,
  `getEngines`, `getVoices`, `getDefaultEngine`, `setVoice`) để bài
  đó còn biên dịch được; chữ ký công khai cũ
  (`initialize` 3 và 4 tham số, các trạng thái cũ) không đổi. Chưa
  chạy bài test này (giai đoạn 1).

## 3. R06.3 — Âm cứu hộ đóng gói

- Lớp mới `RescueAudioPlayer`: phát MP3 trong `res/raw` bằng
  `MediaPlayer`, một player cho mỗi màn hình sở hữu, thay clip khi
  phát clip mới, `stop()` ở `onPause`, `release()` ở `onDestroy`.
  Không hề chạm TTS.
- Asset: **6 clip thật**, transcript/nguồn/đo kiểm ở
  `R06-RESCUE-AUDIO-ASSETS.vi.md`. Nói thẳng: dự định dùng giọng
  Microsoft qua edge-tts **thất bại trong môi trường này** (lỗi SSL
  khi nối trực tiếp, treo qua proxy), nên asset hiện tại là giọng
  **eSpeak NG** (`vi`, sinh ngoại tuyến) — có tiếng nói thật, đo
  3,9–25,0 giây bằng ffprobe, nhưng nghe máy móc. Độ rõ phải nghiệm
  thu trên máy thật ở giai đoạn 2/3; thay giọng tự nhiên sau này chỉ
  là thay tệp cùng tên tài nguyên.
- Nối luồng:
  - `SplashScreen`: khi backend bootstrap báo `FAILED` /
    `MISSING_LANGUAGE` / `MISSING_VOICE` / `NETWORK_REQUIRED` ngoài
    lúc đang cài đặt, hiện **màn cứu hộ** (`showBootstrapRescueScreen`)
    gồm 5 nút thường đúng thứ tự clip `CLIP_TTS_FAILED` mô tả:
    nghe lại hướng dẫn, thử lại giọng đọc (đóng hẳn backend cũ, tạo
    mới bằng lựa chọn đã lưu), mở cài đặt giọng Android (chỉ bắn
    intent `TTS_SETTINGS`, lỗi thì rơi về `ACTION_SETTINGS`, lỗi
    nữa thì quay lại màn cứu hộ — app không điều khiển màn cài đặt),
    mở thiết lập bộ đọc, quay lại (khởi động lại Splash). Nếu cài
    đặt vừa chạy xong mà giọng vẫn hỏng, màn cứu hộ cũng hiện thay
    vì để người dùng kẹt ở màn cài đặt im lặng.
  - Lỗi cài đặt: màn lỗi bootstrap nay có nút "Nghe lại hướng dẫn"
    đứng đầu và tự phát clip khớp loại lỗi — `CLIP_INSTALL_FAILED`,
    riêng hết dung lượng là `CLIP_NO_SPACE` — thứ tự nút khớp
    transcript (nghe lại → thử cài lại → cài đặt → thoát).
  - `ReaderSetupActivity`: các trạng thái lỗi giọng phát
    `CLIP_SETUP_FAILED` một lần cho mỗi đợt lỗi (clip này cố ý không
    nêu thứ tự nút vì màn thiết lập có nhiều điều khiển), cộng nút
    thường `a11yRescueHelp` "Nghe hướng dẫn cứu hộ" trong danh sách
    điều khiển. Khi TalkBack đang bật, các đường này nhường cho
    TalkBack như mọi luồng self-voice khác (`usesSystemAccessibility`
    / `shouldSelfVoiceBootstrap` giữ nguyên).
- Điểm mở: hằng `CLIP_WELCOME` và asset chào đã có nhưng **chưa nối**
  vào luồng phát nào — phát chào vô điều kiện có nguy cơ chồng với
  self-voice; cần quyết định điểm phát (dự kiến cùng màn cứu hộ) ở
  vòng sau của R06.3 hoặc R21. Không ghi là đã dùng.

## 4. R06.4 — Cài / nâng dữ liệu bằng staging, journal, rollback

`InstallProgramTask` trong `SplashScreen` được viết lại phần lõi
(phần UI/tiến độ/cài đặt khởi động giữ nguyên):

1. **Khôi phục trước**: đọc journal `install_journal.txt` (bộ nhớ
   trong của app, ghi kèm `fsync`). `SWAPPING` dở → chuyển các cây
   đã đỗ trong `.reader_install_rollback` về lại vị trí sống;
   `STAGING`/`STAGED` dở → bỏ cây staging; `COMMITTED` dở → dọn
   rollback thừa. `installOrRun()` cũng kiểm journal: thấy
   `SWAPPING` thì không tin pref `installed` nữa, buộc chạy task để
   khôi phục trước.
2. **Đo và kiểm dung lượng**: stream toàn bộ assets một lượt để có
   tổng byte giải nén chính xác (cùng danh sách `asset_files.txt`
   hoặc đường duyệt APK như cũ). Dung lượng trống <
   tổng đo + 32 MB dự phòng → lỗi `ERR_NO_SPACE` **trước khi đụng
   vào dữ liệu đang sống**, hiện chuỗi `a11yBootstrapNoSpace` và
   clip `CLIP_NO_SPACE`; thử lại / thoát ở màn lỗi như cũ.
3. **Staging**: copy toàn bộ vào `.reader_install_staging` (cùng ổ
   với cây sống). Lỗi ở bước này không mất gì: cây sống nguyên vẹn.
4. **Kiểm chứng hoàn tất trên đĩa**: đếm lại cây staging —
   số tệp và tổng byte phải khớp cả danh sách assets, tổng byte đã
   đo ở bước 2 và tổng byte vòng copy tự đếm (`ERR_VERIFY` nếu lệch).
   Chỉ khi đó mới ghi journal `STAGED`.
5. **Hoán đổi**: journal `SWAPPING`; đỗ `data/gfx/lang` đang sống
   vào `.reader_install_rollback` bằng rename; chuyển cây staging
   vào vị trí sống bằng rename; chuyển nội dung custom của người
   dùng từ rollback về cây mới theo đúng luật preserve cũ (thư mục
   tên `font`, thư mục custom dưới `data/sound|mods|gfx` mà APK
   không có, tệp `user-default-mods.json`); journal `COMMITTED`.
6. **Chốt**: chỉ **sau** `COMMITTED` mới ghi pref
   `installed = version` (không bao giờ báo sẵn sàng sớm), rồi dọn
   rollback/staging và xóa journal.
- Lỗi được phân loại `no_space / copy / verify / swap` và mang theo
  chi tiết kỹ thuật trong thông điệp bootstrap như trước; đường thử
  lại là `installOrRun` (task mới tự khôi phục phần dở trước), đường
  thoát là nút thoát hiện có. Lỗi phía không-bootstrap vẫn là hộp
  thoại OK → kết thúc như cũ: ghi nhận là đường nghe được ở phía đó
  vẫn phụ thuộc TalkBack — không đổi ở đợt này.
- Đổi hành vi cần biết: bản cũ xóa cây sống **trước** rồi copy đè
  lên; bản mới không bao giờ xóa cây sống — đổi lại tốn thêm dung
  lượng tạm (một bản staging + một bản rollback trong lúc hoán đổi),
  đó là lý do có kiểm dung lượng ở bước 2 với số đo thật.

## 5. Đóng R06 — các trạng thái từ cài sạch tới menu

| Trạng thái | Đường tiếp / thoát nghe được |
| --- | --- |
| Cài sạch thành công | Tiến độ đọc theo mốc 10% → màn cài đặt khởi động (bootstrap hoặc hộp thoại) |
| Thiếu dung lượng | Lỗi `no_space` + chuỗi + clip riêng; thử lại / cài đặt / thoát; dữ liệu cũ nguyên vẹn |
| Lỗi copy / kiểm chứng / hoán đổi | Thông điệp phân loại + chi tiết; thử lại tự khôi phục staging/rollback trước |
| Bị kill giữa hoán đổi | Lần mở sau: journal `SWAPPING` → khôi phục rollback rồi cài lại; không vào game bằng dữ liệu dở |
| Thiếu giọng Việt / giọng đã lưu bị gỡ | Trạng thái rõ ràng ở Setup; queue rơi về giọng mặc định một lần + thông báo; clip cứu hộ ở Splash/Setup |
| TTS lỗi hẳn | Màn cứu hộ Splash (5 nút + clip) hoặc clip + nút thường ở Setup; mở được cài đặt hệ thống và quay lại |
| Engine chết thoáng qua khi đang chơi | Queue tự thử lại 2 lần có giới hạn trước khi báo lỗi |

## 6. Chỗ chưa chắc

- Chưa biên dịch: các API đã đối chiếu chữ ký trong repo và bề mặt
  fake (mục 2), nhưng tính biên dịch được — đặc biệt
  `TextToSpeech.getVoices()` trả `Set<Voice>`, `Locale.forLanguageTag`
  ở minSdk 24, và các quá tải mới — chỉ xác nhận ở giai đoạn 2.
- `AssetManager.open()` với các tên asset bị tước đuôi `.gz` trong
  `asset_files.txt` hoạt động thế nào là hành vi sẵn có của luồng
  copy cũ; vòng đo byte dùng đúng cùng tên đó nên tiêu chí hoàn tất
  tự nhất quán, nhưng nếu có asset không mở được thì cả đo và copy
  cùng lỗi `copy` — cần một bài giai đoạn 2 với gói APK thật.
- Độ rõ của giọng eSpeak NG và việc thay giọng tự nhiên: xem mục 3.
- Kiểm chứng hoán đổi khi rename khác ổ: staging/rollback/cây sống
  đều nằm dưới cùng `externalFilesDir` nên rename là cùng hệ thống
  tệp theo thiết kế; máy có cấu hình lưu trữ lạ cần kiểm ở giai
  đoạn 2 (T53).
