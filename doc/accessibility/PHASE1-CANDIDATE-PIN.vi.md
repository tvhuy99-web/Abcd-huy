# Hồ sơ ghim ứng viên cuối giai đoạn 1 (R23.3)

Ngày ghim: 2026-10-23. Mục kế hoạch: R23.3 trong `REMAINING-CODE-PLAN.vi.md` bản 2.0.

Tài liệu này ghim **ứng viên mã nguồn** của giai đoạn 1 và ghi kết quả kiểm
danh sách tệp/resource/build-list **bằng đọc nguồn**. Đây không phải hồ sơ
của một bản dựng: chưa build, chưa tạo APK, chưa tính checksum — các việc
đó thuộc quy trình `CANDIDATE-RECORD.vi.md` ở giai đoạn 2, áp dụng cho
từng bản dựng thật. **Không gọi đối chiếu nguồn trong tài liệu này là biên
dịch thành công.**

## 1. Commit ứng viên đã ghim

| Mục | Giá trị |
| --- | --- |
| Nhánh | `recovery/android-reader-ad5ecf0` (chỉ ở máy local, chưa push) |
| Commit ứng viên (mã) | `bc4f807c853e16909199578c4d68218ae50415b4` |
| Commit nền | `ad5ecf06d0db194ede3715fefab6857e9605eaa7` (đầu nhánh `feature/android-reader-code-phase` khi bắt đầu R00) |
| Số commit R00–R22 trên nền | 26 |
| Tổng thay đổi nền → ứng viên | 145 tệp, +32.677/−347 dòng (src 75 tệp, android 29, doc 32, tests 5, tools 3, lang 1) |
| Cây làm việc tại lúc ghim | Sạch (mọi thay đổi đã commit) |

Sau commit ứng viên, giai đoạn 1 chỉ còn các commit hồ sơ R23 (đối chiếu
sổ hành trình và tài liệu này cùng `PHASE1-CLOSING-REPORT.vi.md`); chúng
không đổi mã. Hash commit hồ sơ cuối của giai đoạn 1 được ghi trong
`PHASE1-CLOSING-REPORT.vi.md` mục 7.

## 2. Manifest nội dung — đối chiếu `SCOPE-MANIFEST-2026-10-23.vi.md` tại ứng viên

Mọi giá trị dưới đây đã kiểm lại từ nguồn tại commit ứng viên:

| Hạng mục | Giá trị tại ứng viên | Nguồn |
| --- | --- | --- |
| Flavor trợ năng | `accessibility` | `android/app/build.gradle` (productFlavors, :399–413) |
| Mã ứng dụng | `com.cleverraven.cataclysmdda.accessibility` (base + `applicationIdSuffix ".accessibility"`) | `build.gradle:342,411` |
| Authority tài liệu | `com.cleverraven.cataclysmdda.accessibility.documents` | `build.gradle:412` (manifestPlaceholders) |
| compile/target/min SDK | 35 / 35 / 24 | `build.gradle:340,356,339` |
| NDK | `28.1.13750724` | `build.gradle:341` |
| ABI mặc định | `arm64-v8a` + `armeabi-v7a` | `android/jni/Application.mk:1`, `build.gradle:466` |
| Ngôn ngữ game | en + vi; `USE_LANG=vi` mặc định | `android/app/build.gradle:481` |
| Giọng đọc ghim cứng | `vi_VN` | `ReaderPreferences.java` (`READER_LANGUAGE_TAG`) |
| Realtime | `TURN_DURATION` mặc định 0 | `src/options.cpp`, `data/json/game_balance.json` |
| Mod lõi thế giới mới | `dda` → `no_npc_food` → `personal_portal_storms` | `data/json/default_blacklist.json`, `src/worldfactory.cpp` |

Các mục **vẫn chưa ghim** (mục 8 của manifest — giữ nguyên trạng, không
điền giá trị đoán): danh sách mod của cấu hình phát hành; dải máy/Android
đã chứng nhận; phiên bản phát hành và khóa ký phát hành (khóa
`accessibility-test` là khóa thử công khai, cấm dùng làm khóa phát hành);
flavor phát hành chính thức. Cấu hình build ở bảng trên chỉ nói bản dựng
nhắm tới đâu, không phải dải máy đã chứng nhận.

## 3. Tệp nguồn mới của R00–R22 và kết quả kiểm danh sách build

Tệp nguồn/chương trình **mới thêm** so với nền (doc/hồ sơ không liệt kê):

| Tệp mới | Đợt | Cơ chế nạp | Kết quả kiểm |
| --- | --- | --- | --- |
| `src/a11y_outcome.h`, `src/a11y_outcome.cpp` | R08 | Makefile: `SOURCES := $(wildcard src/*.cpp)` (`Makefile:1034`); CMake desktop: `file(GLOB ...)` ở `src/CMakeLists.txt:16-18`; CMake JNI Android: `file(GLOB CATA_SOURCES .../src/*.cpp)` ở `android/app/jni/CMakeLists.txt`; MSVC: wildcard theo chữ cái đầu (mục 4) | Có mặt ở mọi đường nạp, gồm cả MSVC |
| `android/.../ReaderDocumentReader.java` | R04 | Gradle: `sourceSets.main.java.srcDirs = ['src/main/java']` (`build.gradle:524-528`) — nạp theo thư mục, không khai báo từng tệp | Tự vào build |
| `android/.../RescueAudioPlayer.java` | R06 | như trên | Tự vào build |
| `android/.../ReaderCuePlayer.java` | R21 | như trên | Tự vào build |
| `android/.../ReaderBackupArchive.java` | R20 | như trên | Tự vào build |
| `android/.../ReaderDiagnostics.java` | R21 | như trên | Tự vào build |
| `android/app/src/main/res/raw/a11y_rescue_*.mp3` (6 tệp) | R06 | AGP tự đóng gói mọi tệp trong `res/`; tham chiếu bằng `R.raw.*` trong `RescueAudioPlayer.java` | Tự vào build; nội dung là âm thật (eSpeak NG) — provenance ở `R06-RESCUE-AUDIO-ASSETS.vi.md` |
| `android/app/src/main/res/raw/a11y_cue_*.wav` (4 tệp) | R21 | như trên; tham chiếu trong `ReaderCuePlayer.java` | Tự vào build |
| `tests/a11y_fixtures.h`, `tests/a11y_fixtures.cpp`, `tests/a11y_schema_test.cpp`, `tests/a11y_unicode_test.cpp`, `tests/a11y_knowledge_test.cpp` | R22 | `tests/CMakeLists.txt:4-5` — `file(GLOB CATACLYSM_DDA_TEST_SOURCES ${CMAKE_SOURCE_DIR}/tests/*.cpp)` cho cả `cata_test` và `cata_test-tiles` | Tự vào build test host khi cấu hình lại CMake (glob chốt ở bước configure — giới hạn quen thuộc của glob, ghi rõ để giai đoạn 2 nhớ cấu hình lại sau khi thêm tệp) |
| `tools/accessibility/fixtures/FIXTURES.vi.md` | R22 | Tài liệu đặc tả fixture F-01..F-15 (không phải tệp dữ liệu nạp vào build) | Đủ 15 đặc tả; dữ liệu fixture phía host nằm trong `tests/a11y_fixtures.*` |

Không có activity/service/provider/receiver mới trong các tệp Java cộng
thêm, nên **không cần khai báo `AndroidManifest.xml` mới** (kiểm tại
`android/app/src/main/AndroidManifest.xml`; `ReaderSetupActivity` có từ
nền và đã khai ở :70; `SplashScreen` là activity launcher ở :48–60).

Overlay tiếng Việt `lang/po/accessibility/vi.po` (1.219 msgid tại ứng
viên) nạp bằng cơ chế overlay của chính repo: `lang/compile_mo.py:23-33`
tự suy đường dẫn overlay từ tệp `.po` chính và gộp bằng
`msgcat --use-first` trước `msgfmt`; mọi đường biên dịch đều đi qua
script đó (`lang/CMakeLists.txt:45-46`, `lang/Makefile:19,23`; phía
Android, `build.gradle:194-207` gọi `make localization LANGUAGES=vi`
hoặc `compile_mo.sh`). Không có danh sách đăng ký từng tệp, và đường
Makefile theo dõi overlay như phụ thuộc (`lang/Makefile:13,17,21`;
`Makefile:1273-1276`). **Caveat đã biết:** `lang/CMakeLists.txt:48`
khai `DEPENDS` chỉ gồm tệp `.po` chính và `compile_mo.py`, không gồm
tệp overlay, nên ở đường CMake thuần, sửa chỉ riêng overlay có thể
không kích hoạt dựng lại `.mo` — giai đoạn 2 phải kiểm chứng khi đóng
gói gettext (xem báo cáo đóng giai đoạn 1).

## 4. Danh sách nguồn MSVC — đã kiểm: nạp bằng wildcard, không thiếu tệp

> **Đính chính (2026-10-23, cùng đợt R23):** bản đầu của mục này ghi
> sai rằng MSVC dùng danh sách tường minh và kết luận `a11y_outcome`
> cùng một số tệp nền bị thiếu. Kết luận đó dựa trên việc tên tệp
> không xuất hiện literal trong `.vcxproj` — nhưng các project MSVC
> không liệt kê từng tệp. Bản đầy đủ của kiểm toán danh sách build và
> kiểm chứng trực tiếp tại các tệp `.vcxproj` cho kết quả đúng dưới
> đây; mục này đã được viết lại theo bằng chứng đó.

Trong repo **không tồn tại** tệp `.vcxproj.filters` nào. Các project ở
`msvc-full-features/` nạp nguồn bằng **wildcard theo chữ cái đầu**:

- `Cataclysm-libAL-vcpkg-static.vcxproj:175`:
  `<ClCompile Include="..\src\a*.cpp;..\src\b*.cpp;…;..\src\l*.cpp" />`
  — mọi tệp `src/a*.cpp`, gồm toàn bộ `a11y_*.cpp` của nền và
  `a11y_outcome.cpp` của R08, đều được nạp.
- `Cataclysm-libMZ-vcpkg-static.vcxproj:177`: `..\src\*.cpp` trừ
  main/messages và trừ khối a–l (không trùng, không sót);
  `:174`: header `..\src\*.h` — gồm `a11y_outcome.h`.
- `Cataclysm-test-vcpkg-static.vcxproj:182,186`: `..\tests\*.h`,
  `..\tests\*.cpp` — gồm các tệp test mới của R22.
- Project exe chính chỉ liệt kê main/messages và tham chiếu hai thư
  viện trên qua ProjectReference.

Kết luận: **không có tệp nguồn mới nào của R00–R22 bị thiếu khỏi bất
kỳ hệ build nào** (MSVC, CMake desktop, CMake JNI Android, Makefile,
Gradle), và không cần sửa danh sách build nào cho các tệp này. Lưu ý
phương pháp duy nhất: glob/wildcard của CMake được đánh giá lúc cấu
hình — khi thêm tệp mới cần chạy lại cấu hình, không cần sửa danh
sách.

## 5. Bài kiểm và fixture sẵn sàng cho giai đoạn 2

- **Danh mục bài:** T01–T64 (nguồn gốc ở `PLAN.vi.md` mục 15.4) và các
  bài bổ sung TX-01..TX-32 cùng TX-debug-menu, TX-editmap, TX-wish —
  đầy đủ trong `TESTING.vi.md`, mỗi bài có fixture/cấu hình gán kèm.
  Chưa bài nào có kết quả PASS; viết bài không đồng nghĩa đã chạy.
- **Fixture:** F-01..F-15 đặc tả trong
  `tools/accessibility/fixtures/FIXTURES.vi.md` (kịch bản tái tạo được,
  không dùng save cá nhân); 40/115 hành trình trong
  `JOURNEY-LEDGER.json` có trường `fixtures` gán F-*; mã fixture/bài
  phía host ở `tests/a11y_*.cpp` (mục 3).
- **Kịch bản chạy thủ công:** `tools/accessibility/run_core_tests.sh`,
  `run_speech_tests.sh`, `run_android_smoke.sh` — giữ nguyên trạng thủ
  công, không bật CI ở lượt này.
- **Hồ sơ đi kèm cho giai đoạn 2:** `USER-GUIDE.vi.md` (hướng dẫn nghe),
  `BUG-REPORT-TEMPLATE.vi.md` (mẫu báo lỗi), `CANDIDATE-RECORD.vi.md`
  (quy trình lập hồ sơ từng bản dựng: ghim/checksum/ký/nâng cấp T63).

## 6. Những chỗ bắt buộc chứng minh bằng runtime ở giai đoạn 2

Theo mục 6 của kế hoạch, theo thứ tự: (1) build host + Android và sửa
lỗi biên dịch phát sinh (gồm khôi phục dispatch `game::cast_spell` —
điểm đứt của cây nền, bài TX-21; danh sách build đã kiểm đủ ở mục 3–4,
không cần bổ sung);
(2) bộ tự động ngữ nghĩa/input/Unicode/quyền biết bằng engine thật;
(3) giả lập cho UI/vòng đời/modal; (4) APK ARM64 trên máy thật với
giọng offline/online và TalkBack; (5) toàn bộ T01–T64 + TX, phiên dài,
nâng cấp/backup/khôi phục thật (T63); (6) nghiệm thu người khiếm thị ở
giai đoạn 3. Chi tiết ràng buộc: `PHASE1-CLOSING-REPORT.vi.md`.
