# Kết quả giai đoạn 2 — build, kiểm thử, sửa lỗi (bản đang cập nhật)

Nhánh: `recovery/android-reader-ad5ecf0`. Điểm bắt đầu: HEAD `9a651c63`
(ứng viên mã giai đoạn 1 ghim ở `bc4f807c`). Người dùng duyệt toàn quyền
chạy kiểm thử và sửa mọi vấn đề phát sinh (2026-10-23).

**Đọc đúng phạm vi:** tài liệu này chỉ ghi kết quả THẬT đã chạy trên máy
này. Bài chưa chạy ghi BLOCKED kèm lý do; không suy PASS từ đối chiếu
nguồn.

## 0. Môi trường build (đo thật trên máy)

- 2 nhân CPU; RAM tổng 7,7 GB, khả dụng thực tế chỉ ~0,3–0,6 GB (máy
  ảo nspawn trong KVM, phần lớn RAM do phía host dùng), **không swap**
  (tạo swapfile bị `swapon: Operation not permitted`). Đĩa trống ~97 GB.
- g++ 13.3, CMake 3.28, GNU make, python3. Đã cài thêm qua apt:
  `libncurses-dev` (headers curses vốn thiếu) và `gettext`
  (msgfmt/msgcat vốn thiếu).
- Cấu hình host: CMake ngoài cây nguồn tại `~/workspace/cdda-work/build-host`,
  `TILES=OFF, CURSES=ON, SOUND=OFF, TESTS=ON, LANGUAGES=vi`, liên kết
  gold (`-fuse-ld=gold`), biên dịch nối tiếp `-j1`. CDDA tự áp bộ cờ
  Debug của nó (`-Og -g2 -g1`, `-Werror` + bộ cảnh báo của repo) phía
  sau cờ `-O0 -g0` đã truyền — mức tối ưu hiệu dụng là `-Og`.
- Không có Android SDK/NDK/JDK lúc bắt đầu (mục 5).

## 1. Sửa lỗi cây nền: hỏng splice trong `src/handle_action.cpp` (TX-21)

Phát hiện gốc của giai đoạn 1 ("`game::cast_spell` không tồn tại") được
làm rõ tận gốc ở giai đoạn 2: tệp `src/handle_action.cpp` trong lịch sử
fork **đã từng được commit từ bản dump bị cắt cụt của công cụ** (các
commit `7f2f1793` và `a32b9e6e`, trước giai đoạn 1):

- Đầu tệp còn nguyên hai dòng rác `Warning: truncated output (original
  token count: ...)` / `Total output lines: ...` xếp chồng.
- Một dòng bị splice ở giữa `sleep()` đã thay 495 dòng mã: toàn bộ
  phần cuối `sleep()`, `loot()`, `wear()`, `takeoff()`, `read()`,
  `reach_attack()`, `fire()`, `open_movement_mode_menu()`, hàm tĩnh
  `cast_spell(bool)` mà dispatch `ACTION_CAST_SPELL`/`ACTION_RECAST_SPELL`
  gọi, và phần đầu của `Character::cast_spell` — tất cả bị mất, thân
  còn lại của `Character::cast_spell` bị dính vào giữa `sleep()`.
  Đây là lý do cả dispatch thi triển phép lẫn một loạt lệnh cơ bản
  (bắn, đọc, mặc, cởi, tấn công xa, loot) không có định nghĩa.

Đã khôi phục **nguyên văn** từ tổ tiên sạch cuối cùng của chính fork
(`41f9687d` — hai commit xen giữa ngoài hai commit dump không chạm
vùng này), gồm cả ngữ nghĩa recast đúng của fork: chọn phép qua
`known_magic::select_spell`, recast qua `known_magic::last_spell`
(có thông báo "Cast a spell first" khi chưa từng thi triển), và gán
`last_spell` sau mỗi lần chọn — trường mà cơ chế channeling đọc.
Commit sửa riêng: `93c595b7`. Kiểm chứng sau khôi phục: kiểm kê hàm
khớp bản sạch tuyệt đối, cân bằng ngoặc 0, không còn dấu cắt cụt nào
trong cây (quét toàn repo: chỉ tệp này bị). TX-21 chuyển trạng thái:
điểm đứt đã sửa ở mức nguồn; việc bài chạy đạt hay không chờ kết quả
build/test bên dưới.

## 2. Build host

Cấu hình CMake (mục 0) sinh được cây build đầy đủ ngay lần đầu. Quá
trình build nối tiếp `-j1` (giới hạn RAM) đi qua các lượt:

- Lượt đầu dừng ở 6%: `a11y_input.cpp` — cảnh báo thành lỗi
  `-Werror=range-loop-construct` (vòng lặp buộc tham chiếu `std::string`
  vào tạm từ danh sách khởi tạo). Đã sửa tại căn nguyên (commit
  `c733cbb6`).
- Một sự cố môi trường: `/tmp` là tmpfs 512 MB bị đầy bởi file tạm
  (zip SDK của đợt đánh giá Android + file .s của trình biên dịch),
  làm nhiều TU fail giả "No space left on device". Đã chuyển toàn bộ
  staging và `TMPDIR` của build sang đĩa workspace; các lỗi giả biến
  mất. Ghi nhận để các đợt sau tránh /tmp cho file lớn.
- **Máy ảo bị khởi động lại giữa chừng một lần**, phần lớn object đã
  biên dịch bị mất (chỉ còn 247 tệp .o) — build phải chạy lại từ gần
  đầu. Mã nguồn và các commit không mất.
- Các lượt `-k` tiếp theo bóc dần từng lớp lệch API của mã giai
  đoạn 1 so với cây hiện tại (mục 6, các commit từ `d306452a` đến
  `0ec27963`): `calendar::turnzero`, `bodypart_id` gọi như hàm,
  `itype::name`/`recipe::byproducts`/`vehicle_part::base` riêng tư,
  header chưa tự chứa (`a11y_world.h`, `a11y_requirements.h`),
  literal thiếu trường (`semantic_cell.compare`,
  `row_binding.draft`), `query_popup::message` thiếu đối số định
  dạng, khai báo reader để private (`mission_ui`, `scores_ui`),
  `faction::vitamin_stores` thiếu const, biến cục bộ `static_popup`
  che hàm `popup()`, `overmap_tile_description` dùng trước định
  nghĩa, `register_action` cần `to_translation`, và phía test:
  `tests/a11y_fixtures.cpp` lệch API (commit `096fa5d7`).
  Một phiên làm việc song song (cùng ủy quyền giai đoạn 2) phụ trách
  một phần các sửa này (`66c6e121`, `8ee839af`, `202470dd`,
  `2c752866`, `096fa5d7`); mọi sửa đều ở mức căn nguyên, không tắt
  tính năng, không stub, không nới validate.
- **Kết quả build cuối: `cata_test` liên kết thành công** (mục tiêu
  CMake `cata_test`, binary tại `repo/cata_test`, ~1,87 GB với thông
  tin debug theo bộ cờ của repo). Toàn bộ cây — mã nền + toàn bộ mã
  trợ năng giai đoạn 1 + các bài test — biên dịch và liên kết đạt.
  Quá trình bị kéo dài bởi các lần máy ảo khởi động lại làm mất
  object chưa kịp ghi đĩa; đây là yếu tố môi trường, không phải lỗi
  mã (xem mục 7).

## 3. Kiểm thử host (cata_test)

### 3.1 Bộ test trợ năng (tag `[a11y]`) — 21/21 ĐẠT

Chạy `./cata_test "[a11y]"` trên binary đã liên kết (log:
`~/workspace/cdda-work/test-results/a11y_run3.log`):

- Lần chạy đầu: 18/21 đạt. Ba ca fail cùng một gốc ở
  `tests/a11y_fixtures.cpp`: fixture ghi nhớ ô bằng API của avatar
  sau `clear_map_memory()`, nhưng thao tác clear làm mất hiệu lực
  mọi submap trí nhớ và `map_memory::set_tile_*` **âm thầm bỏ qua**
  ghi vào submap không hợp lệ — nên `has_memory_at()` đọc lại rỗng.
  Trong game thật, vùng quanh avatar được prepare khi map nạp
  (`prepare_map_memory_region`); fixture giờ làm đúng bước đó sau
  mỗi lần clear (kể cả lần clear thứ hai có chủ ý của builder vòng
  đời). Commit sửa: `b35c7a11`. Đây là lỗi **fixture kiểm thử**,
  không phải lỗi mã sản xuất — cổng tri thức của reader
  (`a11y_perception` dùng `has_memory_at`) hoạt động đúng ngữ
  nghĩa engine.
- Lần chạy chốt: **21/21 test case đạt, 146 assertions, rc=0**
  (a11y_body 3, a11y_context 7, a11y_knowledge 3, a11y_schema 6,
  a11y_unicode 2).

### 3.2 Toàn bộ cata_test

Lượt chạy đầy đủ thứ nhất bị máy ảo khởi động lại giết ở phút ~2
(ghi rõ để phân biệt với crash dưới đây). Lượt thứ hai chạy được
~75 phút, qua phần lớn danh sách ca (thứ tự tệp a→s) rồi **dừng
bằng crash** trong ca `monster_speed_description` — sau 2 CHECK
fail, tiến trình gặp lỗi nghiêm trọng (stack trace trong log) nên
các ca sau nó chưa chạy ở lượt này. Tổng số assertion FAILED ghi
được trước crash: 46, thuộc 7 ca:

| Ca | Số assertion fail | Phân loại sau điều tra |
|---|---|---|
| `crafting_skill_gain` | 39 (lệch số lượt chế tạo, vd 209 vs 174) | Ô nhiễm trạng thái tiến trình dùng chung — chạy đơn lẻ **ĐẠT** |
| `proficiency_gain_short_crafts` | 1 (cùng họ lệch lượt) | như trên — đơn lẻ **ĐẠT** |
| `monster_speed_description` | 2 + crash kết thúc lượt chạy | như trên — đơn lẻ **ĐẠT**, không crash |
| `available_reloadables` | 1 | như trên — đơn lẻ **ĐẠT** |
| `ammunition` (iteminfo) | 1 (tên pin "test battery") | như trên — đơn lẻ **ĐẠT** |
| `memorials` | 1 (ô spawn còn quái từ ca trước) | như trên — đơn lẻ **ĐẠT** |
| `EOC_run_eocs` | 1 (kiểm tra i18n tiếng Nga) | **FAIL-nền của fork** — xem dưới |

Phương pháp phân loại: chạy lại từng ca **đơn lẻ trong tiến trình
mới** (log `iso_*.log` trong `~/workspace/cdda-work/test-results/`).
6/7 ca đạt ngay khi tách riêng ⇒ nguyên nhân là trạng thái dùng
chung của một tiến trình test dài (tracker sinh vật, trạng thái
nhân vật/dữ liệu phiên) — một đặc tính đã biết của bộ cata_test
dùng chung tiến trình, **không phải lỗi của mã trợ năng hay glue
đã khôi phục** (không ca nào trong các ca trên đi qua mã a11y;
toàn bộ 21 ca `[a11y]` đạt độc lập ở mục 3.1 và không có ca a11y
nào xuất hiện trong danh sách fail của lượt đầy đủ).

Riêng `EOC_run_eocs` fail **xác định** cả khi chạy đơn lẻ, và chỉ
fail đúng 1/43 assertion: kỳ vọng chuỗi Nga "батарейка" từ tệp
`data/mods/TEST_DATA/lang/mo/ru/.../TEST_DATA.mo`. Tệp nguồn
`lang/po/ru.po` của TEST_DATA **không tồn tại trong fork** — commit
của chính fork `f5904684` ("Chỉ giữ tiếng Anh ở giai đoạn đầu") đã
xóa các fixture ngôn ngữ này. Đây là **FAIL-nền có sẵn của fork**,
không liên quan trợ năng; giai đoạn 2 không bịa dữ liệu fixture để
xanh bài — ghi nhận để chủ kho quyết định riêng.

Lượt chạy phủ phần đuôi: danh sách ca theo `--list-tests` có
2.136 mục; lượt đầy đủ ở trên đã chạy tới mục 1.782
(`monster_speed_description`). Phần còn lại — 354 ca sau nó — được
chạy bằng 4 tệp spec Catch2 (`-f`), mỗi mảnh một tiến trình riêng:
**354/354 ca ĐẠT** (100 + 89 + 82 + 83 ca; tổng ~6,17 triệu
assertions; log `tail2_shard*.log`). Cộng với mục 3.1 và các lượt
chạy đơn lẻ ở bảng trên: **mọi ca trong danh sách đều đã chạy ít
nhất một lần**; ngoài 1 assertion FAIL-nền của fork (EOC i18n) và
cụm ô nhiễm tiến trình dùng chung (đều đạt khi tách riêng), không
có ca nào fail xác định.

### 3.3 Trạng thái hành trình (JOURNEY-LEDGER / TESTING)

**Không hành trình nào đổi trạng thái kiểm thử ở giai đoạn 2.**
Lý do trung thực: các bài T01–T64/TX trong `TESTING.vi.md` là bài
theo kịch bản (nhiều bài cần host-Java, giả lập hoặc máy thật
TalkBack); 21 ca Catch2 đã chạy là kiểm chứng đơn vị tầng host-C++
cho các hợp đồng nền (schema, ngữ cảnh, ranh giới tri thức, y
tế/thân thể, unicode) — là bằng chứng hỗ trợ ghi ở tài liệu này,
chưa phải "bài của hành trình đã chạy và đạt" theo đúng quy ước
của danh mục (mỗi bài có hồ sơ riêng, không gộp PASS). Toàn bộ 115
hành trình giữ `trang_thai_kiem_thu = chua-thu`; nghiệm thu thật
thuộc giai đoạn 3 trên máy thật.

## 4. Gettext / overlay tiếng Việt — PASS (sau 1 sửa)

- `msgfmt --check` trên `lang/po/accessibility/vi.po`: đạt (chỉ cảnh
  báo thiếu vài trường header, không phải lỗi).
- Chạy `lang/compile_mo.py lang/po/vi.po.gz` (đúng đường build dùng):
  **lúc đầu FAIL** — `msgcat: msgid 'Blueprint' is used without plural
  and with plural`. Căn nguyên: catalog chính có "Blueprint" kèm
  `msgid_plural "copies of Blueprint"` còn overlay thì không; msgcat
  từ chối gộp khi hai phía khác hình thức số nhiều. Quét toàn bộ
  overlay (928 mục, 51 mục trùng catalog chính) cho thấy đây là xung
  đột duy nhất. Đã sửa overlay dùng đúng hình thức số nhiều của
  catalog chính (commit `3cc9c6d2`); chạy lại compile_mo.py thành
  công: .mo gộp 117.151 mục, tra cứu "Blueprint" qua overlay cho
  "Bản thiết kế" (cả dạng số nhiều). Bước lang `test_mo` trong build
  CMake cũng đã qua với bản sửa này.

## 5. Phía Java / Android — BLOCKED-môi-trường (quyền mạng loopback)

Đánh giá bởi một phiên làm việc riêng, có bằng chứng thực nghiệm:

- Đã staged đầy đủ trên đĩa workspace: Temurin JDK 17.0.21,
  Gradle 8.9, Android cmdline-tools, `platforms;android-35`,
  `build-tools;35.0.0` (cài thủ công bằng curl+unzip vì sdkmanager
  không tải được gói qua egress proxy của máy này — lỗi
  `HttpURLConnection.doTunneling`).
- **NDK chưa tải**: lưu ý repo ghim `override_ndkVersion=28.1.13356709`
  trong `android/gradle.properties` (khác số 28.1.13750724 trong brief
  ban đầu) — khi cần, tải đúng bản repo ghim.
- Task `:app:compileAccessibilityDebugJavaWithJavac` **chưa từng chạy
  tới cấu hình dự án**: Gradle (kể cả `--no-daemon`, vì vẫn fork
  single-use daemon) giao tiếp client↔daemon qua TCP loopback, và
  máy này chặn loại kết nối đó. Bằng chứng gốc: chương trình Java
  thuần tự viết (server/client loopback) nhận nguyên văn thông điệp
  chặn của runtime — quyền `other_tcp = Deny` trong
  Settings → Permissions → Direct network protocols. `gradle help`
  trong thư mục rỗng cũng fail y hệt ("Could not receive a message
  from the daemon"), nên đây là chặn môi trường, **không phải lỗi mã
  Java**; chưa thể kết luận gì về chất lượng mã Java từ lần thử này.
- Điều kiện mở khóa duy nhất: đổi `other_tcp` từ Deny sang Ask.
  Mọi thứ đã staged sẵn, chạy lại ngay được. Các chặn dự kiến tiếp
  theo (suy luận từ `android/app/build.gradle`, CHƯA kiểm chứng bằng
  chạy thật): bản sao probe thiếu `src/version.h`, symlink assets,
  và các task preBuild (`fetchSdl3Aars`, `makeLocalization`,
  `generateVersionHeader`); cờ `-PEXCLUDE_NATIVE_LIBS` có thể giúp
  tách riêng bài biên dịch Java khỏi CMake/NDK.

## 6. Lỗi đã sửa trong giai đoạn 2

| # | Lỗi | Commit |
| --- | --- | --- |
| 1 | Hỏng splice dump cắt cụt trong `src/handle_action.cpp` (mất 9 hàm, gồm glue thi triển phép) | `93c595b7` |
| 2 | `-Werror=range-loop-construct` trong `a11y_input.cpp` | `c733cbb6` |
| 3 | Overlay gettext "Blueprint" lệch hình thức số nhiều với catalog chính — msgcat chặn toàn bộ build tiếng Việt | `3cc9c6d2` |
| 4 | Cụm lệch API ở `a11y_world.{h,cpp}`: thiếu include `coordinates.h`/`a11y_runtime.h`/`point.h`, `calendar::turnzero`→`turn_zero`, khóa cảnh báo chi dùng `part.id.id().str()` | `d306452a` |
| 5 | `itype::name` riêng tư → dùng `nname()`; `recipe::byproducts` riêng tư → thêm accessor chỉ-đọc `legacy_byproducts()`/`has_byproduct_group()` (không lăn item group) | `9cc4482e` |
| 6 | Literal `semantic_cell` 3 trường trong `color.cpp` → bổ sung trường `compare` rỗng | `1183ee0d` |
| 7 | `faction::vitamin_stores` thiếu const; `::popup` bị che bởi biến cục bộ `static_popup`; `row_binding` thiếu trường `draft`; `magic.cpp` thiếu include `a11y_semantics.h` | `e053f723` |
| 8 | `query_popup::message` không đối số định dạng trong glue menu chính | `b9bbad52` |
| 9 | `mission_ui_impl` để reader ở private; nối chuỗi `const char*` trong `mission_companion.cpp` | `b3cd5324` |
| 10 | `a11y_requirements.h` thiếu include `temp_crafting_inventory.h` | `88b9e84a` |
| 11 | (phiên song song) glue phép: `get_folded_tools_list` thừa đối số lọc cũ | `66c6e121` |
| 12 | (phiên song song) `reader_target::data` thiếu khởi tạo mặc định trong `options.cpp` | `8ee839af` |
| 13 | (phiên song song) `register_action` cần `to_translation` trong `ranged.cpp` | `202470dd` |
| 14 | (phiên song song) `scores_ui`: reader public + dùng `get_memorial()` toàn cục thay `game::memorial()` private | `2c752866` |
| 15 | Glue xe đọc `vehicle_part::base` private → dùng `get_base()` | `0ec27963` |
| 16 | `overmap_tile_description` dùng trước định nghĩa (thiếu khai báo trước); `note_color.name` là translation | `59cfa4e7` |
| 17 | (phiên song song) `tests/a11y_fixtures.cpp` lệch API (memorized_tile, số part, Catch2) | `096fa5d7` |
| 18 | Fixture a11y không prepare vùng trí nhớ bản đồ sau clear → 3 ca test knowledge fail | `b35c7a11` |

## 7. Điểm còn mở

1. **Android/Java: BLOCKED-môi-trường** (mục 5). Mở khóa duy nhất:
   đổi quyền `other_tcp` Deny → Ask trong Muse Settings →
   Permissions → Direct network protocols, rồi chạy lại lệnh gradle
   đã ghi. Sau khi mở khóa vẫn cần NDK đúng phiên bản
   `28.1.13356709` (theo `android/gradle.properties` của repo) và dự
   kiến còn các bước preBuild của repo (sinh `src/version.h`, symlink
   assets, `makeLocalization`) — chưa kiểm chứng.
2. **FAIL-nền của fork**: `EOC_run_eocs` thiếu fixture
   `TEST_DATA/lang/po/ru.po` (fork đã xóa ở commit `f5904684`).
   Ngoài phạm vi giai đoạn 2; chủ kho quyết định có khôi phục
   fixture ngôn ngữ hay không.
3. **Bộ cata_test dùng chung tiến trình**: 6 ca nền fail do ô nhiễm
   trạng thái khi chạy chung một tiến trình dài nhưng đạt khi chạy
   đơn lẻ (mục 3.2). Muốn có một lượt "toàn bộ xanh" cần chiến
   lược shard/seed của CI thượng nguồn — không thuộc phạm vi sửa
   mã trợ năng.
4. **Các điểm mở từ giai đoạn 1 vẫn còn** (không đổi ở giai đoạn
   này): bản local chưa commit của người dùng chưa có trong máy
   (R00.1 mở lại có điều kiện cho 8 tệp + hoán vị cử chỉ 8 hướng);
   chính sách Snake theo lượt khi bộ đọc bật (chờ nghiệm thu giai
   đoạn 3); âm cứu hộ eSpeak NG còn máy móc; các quyết định phát
   hành chưa ghim (danh sách mod, dải máy, khóa ký — khóa
   `accessibility-test` là khóa thử công khai); snapshot >4096 node
   chưa phân trang; import backup chưa chặn cứng được phiên game
   đang sống (cần cờ C++/JNI); đường CMake có thể không dựng lại
   `.mo` khi chỉ sửa overlay dịch.
5. **Chưa push**: toàn bộ commit giai đoạn 2 nằm trên nhánh local
   `recovery/android-reader-ad5ecf0`; kênh GitHub do điều phối cha
   xử lý riêng theo ủy quyền của người dùng.
