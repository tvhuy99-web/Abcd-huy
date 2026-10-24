# Manifest phạm vi R01.1 — CDDA Android trợ năng

Ngày: 2026-10-23 (giờ Việt Nam). Mục kế hoạch: R01.1 trong
`REMAINING-CODE-PLAN.vi.md` bản 2.0, mục 4. Tài liệu liên quan:
`PLAN.vi.md` (yêu cầu gốc, mục 1.1/11/15.4), `COVERAGE.md`,
`JOURNEY-LEDGER.json` (sổ hành trình R01.3),
`REMAINING-CODE-BACKLOG.json` (96 đầu việc R01.4).

Trạng thái của tài liệu này: **bằng chứng nguồn của giai đoạn 1 (chỉ viết
mã/tài liệu)**. Lượt này không build, không chạy test, không giả lập, không
tạo APK. Mọi giá trị dưới đây đều đọc từ nguồn tại HEAD ghi ở mục 1; chỗ
nào nguồn không ghim được thì ghi rõ **chưa ghim**, không suy đoán.

## 1. Commit game và nội dung

| Mục | Giá trị đã ghim | Bằng chứng |
| --- | --- | --- |
| Nhánh làm việc | `recovery/android-reader-ad5ecf0` | git |
| HEAD khi lập manifest | `894345fa1a76c9710d88716be5034f77eecebd23` — "R00: reconcile ad5ecf0 baseline, engine swipe-movement flag, monotonic gesture trial" | `git log -1` |
| Nền gameplay/Android | `ad5ecf06d0db194ede3715fefab6857e9605eaa7` (cha trực tiếp của HEAD, cách đúng 1 commit) | `git log -1 ad5ecf0`, `git rev-list --count` |
| Nội dung game (`data/`, gồm mods) | Cùng commit HEAD — nội dung nằm trong cùng repo, không có kho nội dung riêng | cây nguồn tại HEAD |
| Phiên bản phát hành (versionName/versionCode) | **Chưa ghim trong repo.** `versionName` được đọc từ `src/version.h`, tệp này không có trong cây nguồn mà do task `generateVersionHeader` sinh lúc build; `versionCode` lấy từ biến môi trường `UPSTREAM_BUILD_NUMBER`, không có thì là 1. Vì vậy manifest này ghim phạm vi bằng **hash commit**, không bằng số phiên bản | `android/app/build.gradle` (`defaultConfig`, task `generateVersionHeader`); `git ls-files src/version.h` rỗng |
| Baseline đối chiếu CI | `8ef359c…` ghi ở đầu `STATUS.md` là baseline master gốc để đối chiếu, **không** phải cha của HEAD và không nằm trong phạm vi này | `doc/accessibility/STATUS.md` |

## 2. Flavor, ABI, API — cấu hình build trong repo

| Mục | Giá trị | Bằng chứng |
| --- | --- | --- |
| AGP / namespace | AGP 8.7.3; namespace `com.cleverraven.cataclysmdda` | `android/build.gradle`, `android/app/build.gradle` |
| Flavor (dimension `version`) | `stable` (applicationId cơ sở `com.cleverraven.cataclysmdda`); `experimental` (suffix `.experimental`); `accessibility` (suffix `.accessibility` → `com.cleverraven.cataclysmdda.accessibility`, authority tài liệu riêng `com.cleverraven.cataclysmdda.accessibility.documents`, tên launcher "Cataclysm DDA - Thử trợ năng") | `android/app/build.gradle`: `productFlavors` |
| Flavor của bản trợ năng hiện tại | `accessibility` — theo `TESTING.vi.md` đây là bản **thử** trợ năng có vùng dữ liệu riêng, không dùng chung bản lưu với stable/experimental. Quyết định flavor phát hành chính thức cho bản trợ năng: **chưa ghi trong nguồn — chưa ghim** | `TESTING.vi.md` mục đầu; `android/app/build.gradle` |
| SDK (mặc định, ghi đè được bằng `gradle.properties`/`local.properties`/`-P`) | compileSdk 35, minSdk 24, targetSdk 35, NDK 28.1.13356709 | `android/gradle.properties`: `override_compileSdkVersion=35`, `override_minSdkVersion=24`, `override_targetSdkVersion=35`, `override_ndkVersion` |
| ABI | Splits theo ABI, `universalApk false`; mặc định `abi_arm_64=true`, `abi_arm_32=true`, `abi_x86_32=false`, `abi_x86_64=false` → APK mặc định cho `arm64-v8a` và `armeabi-v7a`. CI trợ năng build riêng ARM64 (`assembleAccessibilityDebug -Pabi_arm_32=false`) | `android/gradle.properties` (khóa `abi_*`); `android/app/build.gradle` (`splits`, `enabledAbis`, `abiFilters`); `.github/workflows/android-accessibility.yml` |
| Khóa ký của flavor trợ năng | `accessibility-test.keystore` là **khóa phát triển công khai** dùng cho bản thử để cài đè không phải gỡ app; không phải khóa phát hành | `TESTING.vi.md`; `android/app/build.gradle`: `signingConfigs.accessibilityTest` |

**Giới hạn bắt buộc phải đọc kèm:** các số SDK/ABI ở mục này chỉ là *cấu
hình build trong repo*. Chúng nói rằng app cài được về nguyên tắc từ
API 24 và nhắm API 35. Chúng **không** phải dải máy/Android đã chứng
nhận: trong repo chưa có danh sách máy đã nghiệm thu; bằng chứng hiện có
chỉ là CI ARM64 và emulator API 35, còn máy thật/âm thanh/TalkBack vẫn
chưa được nghiệm thu (theo `STATUS.md`). Dải máy chứng nhận là việc của
giai đoạn 2/3 và hiện **chưa ghim**.

## 3. Ngôn ngữ

| Lớp | Phạm vi đã ghim | Bằng chứng |
| --- | --- | --- |
| Ngôn ngữ game chào cho người chơi | English và Tiếng Việt. Luôn có `en`; `vi` xuất hiện khi biên dịch có `LOCALIZE` và có tệp dịch `vi`. Mặc định `USE_LANG = "vi"` | `src/options.cpp`: `options_manager::get_lang_options()`, `add("USE_LANG", …, "vi")` |
| Dữ liệu dịch | Catalog game duy nhất `lang/po/vi.po.gz` + overlay trợ năng `lang/po/accessibility/vi.po`; `.mo` sinh lúc build (`localize=true`, task `makeLocalization` chạy `make localization LANGUAGES=vi`) | `android/gradle.properties`, `android/app/build.gradle`, CI kiểm tra `assets/lang/mo/vi/LC_MESSAGES/cataclysm-dda.mo` |
| Giao diện Android (Java) | Hai bộ resource: `values/` (English, mặc định) và `values-vi/`, mỗi bộ có `strings.xml`, `a11y_reader.xml`, `a11y_editor.xml` theo cùng khóa; không có `values-*` nào khác | `android/app/src/main/res/` |
| Giọng đọc của bộ đọc | Khóa cứng `vi_VN` ở mọi điểm khởi tạo TTS (`ReaderSpeechQueue`, `ReaderSetupActivity`, `SplashScreen`), kể cả khi ngôn ngữ game là `en`. `AndroidSpeech` tự nó trung lập ngôn ngữ nhưng kiểm tra giọng thật khớp locale yêu cầu và **không fallback thầm** sang ngôn ngữ khác. Đây là phạm vi hiện tại đã ghim trong mã, không phải lỗi sót | các tệp Java trên; `AndroidSpeech.java` |
| Không thuộc phạm vi ngôn ngữ | Map locale trong `update_global_locale()` (ar/cs/de/fr/ja/ko/ru/zh_CN…) là mã ánh xạ locale của engine, **không** phải ngôn ngữ được chào trong bản này vì không có dữ liệu dịch tương ứng trong `lang/po/` | `src/options.cpp`, `lang/po/` |

Có bản dịch không đồng nghĩa đã chứng minh chất lượng/độ phủ/phát âm;
phần đó thuộc R21 và giai đoạn 2/3.

## 4. Mods và thứ tự nạp

### 4.1. Cơ chế (nguồn chứng minh được)

- Core duy nhất: `dda` (`"core": true`, trỏ tới `../../json`) —
  `data/mods/dda/modinfo.json`; grep `"core": true` trong `data/mods/`
  chỉ trúng tệp này.
- Danh sách mặc định của chính repo: `dev:default` với
  `dependencies: ["dda", "no_npc_food", "personal_portal_storms"]`, bị
  đánh dấu `obsolete: true` để ẩn khỏi danh sách chính —
  `data/mods/default.json`, truy cập qua `PATH_INFO::mods_dev_default()`.
- Khi chạy, `refresh_mod_list()` thử `user:default` trước (nằm ở
  `config/user-default-mods.json`, **ngoài repo**, do người dùng lưu lúc
  chạy); không có mới dùng `dev:default`. `default_mods` giữ đúng thứ tự
  của mảng `dependencies` đó — `src/mod_manager.cpp`:
  `refresh_mod_list()`, `set_default_mods(...)`.
- World mới lấy `active_mod_order = get_default_mods()` —
  `src/worldfactory.cpp` (`WORLD::WORLD()`). Thứ tự nạp **không** nằm
  trong world options: `WORLD` tách `WORLD_OPTIONS` và
  `active_mod_order`; thứ tự lưu riêng trong `<world>/mods.json` theo
  đúng thứ tự mảng — `src/worldfactory.h`, `src/mod_manager.cpp`
  (`save_mods_list()`/`load_mods_list()`, load giữ nguyên thứ tự tệp,
  chỉ khử trùng lặp).
- "Core luôn đứng đầu": đúng **khi đi qua UI** — `mod_ui::try_add()`
  chèn core vào đầu và thay core cũ ở vị trí 0, `set_usable_mods()` xếp
  core trước nhóm bổ sung. Nhưng `load_mods_list()` đọc `mods.json`
  **không sắp xếp lại**; thứ tự trong tệp được tin cậy nguyên trạng.
  Đây là một sự thật của engine mà các bài kiểm thử thứ tự nạp ở giai
  đoạn 2 phải tính đến, không phải lỗi của lớp trợ năng.

### 4.2. Điều đã ghim và điều chưa ghim

- **Đã ghim (mặc định của game tại HEAD):** world mới không có
  `user:default` sẽ nạp theo thứ tự `dda` → `no_npc_food` →
  `personal_portal_storms`.
- **Chưa ghim — danh sách mod của cấu hình phát hành.** Không tồn tại
  tệp cấu hình phát hành/đóng gói nào trong repo ghim danh sách mod cho
  bản phát hành trợ năng: workflow bundle Android chỉ build
  `bundleExperimentalRelease`, `release.yml` không chọn mod, và cấu
  hình `mods: magiclysm|xedra_evolved` trong
  `.github/workflows/matrix.yml` là ma trận test CI, không phải cấu
  hình phát hành.
- Hệ quả cho sổ hành trình: hành trình bắt buộc được kiểm kê theo
  **core `dda` + hai mod mặc định trên**. Các mod khác có trong repo
  (mục 4.3) **không** bị loại khỏi tầm nhìn — màn hình/hệ thống do chúng
  đưa vào vẫn phải đi qua adapter dùng chung và được phân loại ở sổ —
  nhưng hành trình riêng theo nội dung của từng mod ngoài mặc định chỉ
  trở thành bắt buộc khi danh sách phát hành được ghim có tên mod đó
  (quy tắc mở lại ở mục 8). Không được dùng việc chưa ghim để âm thầm
  bỏ hệ thống có trong core (phép/bionics của core và của mod mặc định
  vẫn thuộc phạm vi bắt buộc qua R17).

### 4.3. Mod có trong repo tại HEAD (kiểm kê thư mục, không phải danh sách phát hành)

47 thư mục dưới `data/mods/` cộng `default.json` và `folder_info.txt`:
Aftershock, Backrooms, BombasticPerks, CrazyCataclysm, Defense_Mode,
DinoMod, Generic_Guns, Isolation-Protocol, Limb_WIP, MA, MMA,
Magiclysm, Megacity, Megafauna, MindOverMatter, MindOverMatterNoKnacks,
My_Sweet_Cataclysm, Mythos-Creatures, No_Hope, No_NPC_Food,
Only_Wildlife, Perk_melee, Personal_Portal_Storms, Rummaging, Sky_Island,
Sorcerer, Standard_Combat_Tests, TEST_DATA, Tamable_Wildlife,
TropiCataclysm, XedraWood, Xedra_Evolved, aftershock_exoplanet,
alt_map_key, cbm_slots, classic_zombies, dda, dda_tutorial,
deadly_bites, desert_region, extra_mut_scen, hunvre, innawood,
package_bionic_professions, railroads, rural_biome, translate-dialogue.

Lưu ý kiểm kê: thư mục `Aftershock/` không có `modinfo.json` (chỉ có
`profession_groups.json`) nên không nạp được như một mod bằng cơ chế ở
mục 4.1; 46 thư mục còn lại có `modinfo.json`. `TEST_DATA` và
`Standard_Combat_Tests` là nội dung kiểm thử của game gốc, không phải
nội dung phát hành cho người chơi.

## 5. Chế độ lượt / realtime

- Option `TURN_DURATION` ("Realtime turn progression"): số thực
  0.0–10.0 giây, bước 0.05, **mặc định 0.0 = tắt** → cấu hình chuẩn của
  phạm vi này là **chơi theo lượt thuần**. Bằng chứng:
  `src/options.cpp` (`add("TURN_DURATION", "general", …, 0.0, 10.0, 0.0, 0.05)`).
- Engine: khi `TURN_DURATION ≤ 0.005` không sinh lượt theo thời gian;
  lớn hơn thì lượt sinh theo thời gian thực đã trôi —
  `src/handle_action.cpp` (`user_turn::moves_elapsed()`).
- Rào chắn của lớp trợ năng: nếu bộ đọc bật và `TURN_DURATION > 0.005`,
  các đường vào game/tải save/tutorial hiện xác nhận YES/NO/QUIT với
  con trỏ mặc định ở QUIT; chọn NO đặt `TURN_DURATION = 0.0f` và lưu
  thiết lập — `src/main_menu.cpp`:
  `confirm_realtime_progression_for_accessibility()`, gọi ở các điểm
  vào game/tải/tutorial. Snapshot hoạt động cũng mang cảnh báo realtime
  — `src/a11y_world.cpp`.
- Hành trình realtime (người chơi chủ động giữ realtime) vẫn thuộc phạm
  vi: cảnh báo thời gian tiếp tục trôi khi ngủ/đợi/hoạt động dài nằm ở
  R08/R10 và bài T08; manifest này không biến realtime thành "không hỗ
  trợ".

## 6. Cách đọc

Hai đường đọc **song song, không phải chọn một**, đã ghim trong mã:

1. **Bộ đọc nội bộ (đường chính khi tắt touch exploration):**
   `GameReaderController` → `ReaderSpeechQueue` → `AndroidSpeech`
   (TTS của Android, giọng `vi_VN`). Bộ đọc mặc định **tắt**: chỉ bật
   khi `enabled && heard_confirmation` cùng đúng — người dùng phải nghe
   hết câu thử tiếng Việt và xác nhận trong `ReaderSetupActivity`;
   phía C++ `a11y::enabled()` đọc cờ `reader_enabled` —
   `ReaderPreferences.java`, `src/a11y_runtime.cpp`, `src/a11y_android.cpp`.
2. **TalkBack / touch exploration (đường phối hợp và cứu hộ):** khi
   touch exploration đang bật, cùng controller chuyển thông báo sang
   `announceForAccessibility(...)` để TalkBack đọc, không phát TTS nội
   bộ. TalkBack cũng là đường phục hồi khi TTS lỗi/thiếu giọng Việt và
   là đường bootstrap trước khi bộ đọc được bật —
   `GameReaderController.java` (`usesTouchExploration()`,
   `announceGameMessage()`, `onSpeechHealth()`), `ReaderGestureSurface.java`.

Đường tắt bộ đọc và phối hợp với bộ đọc hệ thống phải giữ đúng lựa chọn
sản phẩm này ở mọi mục sau (quy tắc ở mục 3 của kế hoạch).

## 7. Ranh giới phạm vi

- **Trong phạm vi bắt buộc:** mọi hệ thống gameplay dành cho người
  chơi trong cấu hình ở các mục trên — gồm cả xe, vùng/tự động hóa,
  trại, minigame, bionics/đột biến/phép, máy tính trong game (chi tiết
  ở `JOURNEY-LEDGER.json`). Không mục nào trong số này được giảm khỏi
  phạm vi bằng manifest này.
- **Phạm vi riêng (R01.4):** công cụ phát triển/debug — `debug_menu`,
  `editmap`, `wish`. Chúng không phải tính năng chơi và không quảng bá
  là chơi được, nhưng mọi đường vẫn hiển thị cho người dùng phải có
  nhãn giới hạn và đường thoát trong sổ hành trình (nhóm
  `cong-cu-phat-trien`).
- **Ngoài phạm vi:** nội dung của mod chưa được ghim vào cấu hình
  phát hành (mục 4.2) ở mức hành trình riêng theo nội dung; backend
  không phải Android; bản dịch ngoài en/vi.

## 8. Danh sách chưa ghim (phải chốt ở các mục sau, không bịa ở đây)

| # | Chưa ghim | Hệ quả / nơi chốt |
| --- | --- | --- |
| 1 | Danh sách mod của cấu hình phát hành | Mặc định runtime `dev:default` là mốc tạm; chốt khi đóng gói phát hành (R22/R23). Nội dung/mod thay đổi → mở lại đối soát R01 |
| 2 | Dải máy/Android đã chứng nhận | Chỉ có cấu hình build API 24–35 và bằng chứng CI/emulator; chốt ở giai đoạn 2/3 bằng máy thật (R22/R23) |
| 3 | Phiên bản phát hành và khóa ký phát hành | versionName sinh lúc build; khóa `accessibility-test` là khóa thử công khai, cấm dùng làm khóa phát hành (R22) |
| 4 | Flavor phát hành chính thức cho bản trợ năng | Hiện chỉ có flavor thử `accessibility` được mô tả trong `TESTING.vi.md` |
| 5 | Asset âm cứu hộ thật (nguồn, transcript, quyền dùng) | Việc của R06; manifest này không coi tệp âm nào hiện có là đã đủ |
| 6 | `user:default` của người dùng | Nằm ngoài repo lúc chạy; mọi kết luận về thứ tự nạp phải nêu đang nói về `dev:default` hay `user:default` |

## 9. Quy tắc mở lại manifest

Mở lại tài liệu này và các mục bị ảnh hưởng trong
`JOURNEY-LEDGER.json`/`REMAINING-CODE-BACKLOG.json` khi: HEAD thay đổi
ngoài tiến trình R00–R23 đã ghi; danh sách mod phát hành được ghim
hoặc đổi; thêm ngôn ngữ/flavor/ABI; đổi mặc định `TURN_DURATION` hoặc
chính sách phối hợp TalkBack; hoặc một màn hình/hệ thống mới xuất hiện
từ nội dung đã ghim mà sổ chưa có.
