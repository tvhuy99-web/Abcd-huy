# Báo cáo kết thúc giai đoạn 1 — viết mã CDDA Android trợ năng (R23.4)

Ngày: 2026-10-23. Mục kế hoạch: R23.4 trong `REMAINING-CODE-PLAN.vi.md` bản 2.0.
Nhánh: `recovery/android-reader-ad5ecf0` (local, chưa push).
Commit ứng viên (mã): `bc4f807c853e16909199578c4d68218ae50415b4` — ghim tại
`PHASE1-CANDIDATE-PIN.vi.md`.

**Đọc đúng phạm vi của báo cáo này:** giai đoạn 1 chỉ viết mã, chuẩn bị
fixture/bài kiểm thử và bằng chứng nguồn. Toàn bộ giai đoạn **không
build, không chạy test/runner, không giả lập, không tạo APK**. Báo cáo
này chứng minh hồ sơ viết mã đã đóng ở mức đối chiếu nguồn; **nó không
chứng minh game biên dịch được hay chơi được**. Mọi hành trình đều ở
trạng thái kiểm thử "chưa thử".

## 1. Kết quả theo sổ hành trình (chốt từ dữ kiện tệp ở R23.1)

115 hành trình tách từ 30 nhóm bề mặt của `COVERAGE.md`:
**110 đã viết, chờ kiểm thử / 5 chưa viết / 0 chưa kiểm kê.**

| Nhóm bề mặt | Số hành trình | Đã viết, chờ kiểm thử | Chưa viết |
| --- | ---: | ---: | ---: |
| Khoi dong Java | 7 | 7 | 0 |
| Menu chinh | 4 | 4 | 0 |
| The gioi/mod | 9 | 7 | 2 |
| Tao nhan vat | 5 | 5 | 0 |
| Menu chung | 5 | 5 | 0 |
| Popup/nhap | 5 | 5 | 0 |
| Ban do cuc bo | 5 | 5 | 0 |
| Di chuyen/hanh dong | 5 | 5 | 0 |
| Hanh trang | 5 | 5 | 0 |
| Kho nang cao | 2 | 2 | 0 |
| Y te/trang thai | 6 | 6 | 0 |
| Thong bao | 2 | 2 | 0 |
| Chien dau | 4 | 4 | 0 |
| Che tao/xay | 4 | 4 | 0 |
| Ban do lon | 5 | 5 | 0 |
| NPC/thuong mai | 2 | 2 | 0 |
| Nhiem vu/trai | 3 | 3 | 0 |
| Phe/nguoi theo | 1 | 1 | 0 |
| Phuong tien | 4 | 4 | 0 |
| Dieu khien dong co | 1 | 1 | 0 |
| Vung/tu dong hoa | 3 | 3 | 0 |
| Bionics/dot bien/phep | 4 | 4 | 0 |
| Ky nang/kien thuc | 2 | 2 | 0 |
| May tinh trong game | 1 | 1 | 0 |
| Nhat ky/diem/ket thuc | 4 | 4 | 0 |
| Cau hinh | 3 | 3 | 0 |
| Minigame thiet bi | 5 | 5 | 0 |
| Tro giup/phim | 2 | 2 | 0 |
| Luu/tai/sao luu | 4 | 4 | 0 |
| Cong cu phat trien | 3 | 0 | 3 |
| **Tổng** | **115** | **110** | **5** |

Số này đếm lại từ trường trạng thái của từng dòng trong
`JOURNEY-LEDGER.json` sau đối chiếu R23.1 (bản tóm tắt
`JOURNEY-LEDGER.vi.md` đã sinh lại từ cùng dữ kiện và khớp tuyệt đối
115 dòng). Đây là số hành trình có đường viết mã, **không phải phần
trăm hoàn thành** của game.

Ở R23.1, bốn dòng cũ được sửa theo bằng chứng nguồn tại ứng viên:
`chon-engine-va-voice` (bộ chọn engine/voice/ngôn ngữ đã có từ R06.1),
`smart-controller` (form đã có từ R15), `sao-chep-the-gioi` (định
nghĩa lại theo cơ chế thật duy nhất của game: "Copy World Settings" —
tạo thế giới mới sao cài đặt qua `WORLD::COPY_WORLD`, bản lưu không
được sao; đường tạo thế giới sau sao do adapter worldgen của R07 phủ),
và `doc-sach-va-hoc` (lật theo đúng chuẩn adapter dùng chung đã áp
cho các hành trình tương tự; hai thiếu sót trình bày ghi ở mục 2).
Một trường adapter sai của `trung-tam-lenh-day-du` được sửa, và
context `COMPUTER_CONTINUE` của R17 được bổ sung vào danh sách phân
loại đầu vào trong sổ. Backlog R00–R23: **96/96 đầu việc ở trạng thái
đã viết, chờ kiểm thử**.

## 2. Năm hành trình chưa viết — lý do đầy đủ

| Hành trình | Lý do |
| --- | --- |
| `doi-ten-the-gioi` | Đổi tên thế giới hiện có **không tồn tại trong UI thật** (rà ở R07, đối chiếu lại ở R23 trên `main_menu.cpp`/`worldfactory.cpp`: 6 mục con của thế giới không có Rename, toàn cây không có hàm đổi tên thế giới; lần xuất hiện duy nhất của đặt tên là cho thế giới đang tạo). Không có luồng để nối adapter. |
| `cai-thay-du-lieu-mod` | Cài/thay dữ liệu mod trong game **không tồn tại trong UI thật**: dữ liệu mod nạp từ tệp ngoài game. Lỗi dữ liệu mod khi chọn mod đã xử lý ở hành trình chọn mod (R07.1). |
| `debug-menu-va-console`, `editmap`, `wish-menu` | **Phạm vi riêng** theo R01.4: công cụ phát triển, không phải tính năng chơi. Nghĩa vụ còn lại là nhãn giới hạn + đường thoát khi chúng hiện ra; cả ba giữ trạng thái chưa viết trong phạm vi riêng đó, không tính vào khả năng chơi. |

Một hành trình được lật ở R23.1 sau khi bản đầy đủ của kiểm toán về
kịp: `doc-sach-va-hoc` **đã viết** theo đúng chuẩn adapter dùng chung
đã áp cho các hành trình tương tự (chọn sách trong hành trang → màn
yêu cầu đọc là `uilist` của `avatar::read` công bố kỹ năng có thể
huấn luyện từ mức nào tới mức nào, các mục đọc và lý do từ chối tường
minh → đọc như activity có tiến độ/ngắt của R10 → kết quả qua thông
điệp hoàn thành và màn kiến thức R19). Hai thiếu sót trình bày còn
lại, ghi trong sổ: chưa có trường cấu trúc yêu cầu của từng sách
ngay tại chi tiết hành trang trước khi Use (chúng xuất hiện ở màn
yêu cầu đọc sau khi chọn), và kết quả học chưa có node riêng cho
activity đọc.

## 3. Ràng buộc còn tồn tại (đầy đủ, không che)

### 3.1. Điểm đứt của cây nền: dispatch thi triển phép (`cast_spell`)

Đã kiểm lại trực tiếp tại commit ứng viên ở R23: `src/game.h` **không
có** khai báo `cast_spell`; toàn cây **không có** `game::cast_spell`;
`src/handle_action.cpp:2437-2444` gọi `cast_spell()` /
`cast_spell(true)` cho `ACTION_CAST_SPELL` / `ACTION_RECAST_SPELL` —
hàm không tồn tại đó; và `known_magic::select_spell` (định nghĩa ở
`src/magic.cpp:3694`) **không có caller nào**. Đây là lỗi có sẵn của
dòng fork, không do các đợt R gây ra. Hệ quả: chuỗi "phím Cast → menu
phép" chưa nối được ở cây nền; **thi triển phép đầu-cuối bị chặn, bài
T52 không chạy được trọn vẹn** cho tới khi giai đoạn 2 quyết định khôi
phục glue của engine. Mọi bề mặt bộ đọc của hành trình phép (menu
phép, spellbook, nhắm qua selector R11, AOE, kết quả) đã viết độc lập
với quyết định đó. Bài **TX-21** trong `TESTING.vi.md` phân loại riêng
điểm đứt này. Giai đoạn 1 không tự bịa glue engine.

### 3.2. Nguồn local thứ ba chưa có trong máy — R00.1 còn điều kiện mở

Đối chiếu R00 đã hòa giải hai phía có trong máy (local cũ `f40dcdd` ↔
`ad5ecf0`, bảng 78 tệp trong
`RECOVERY-RECONCILIATION-2026-10-23.vi.md`). **Bản chưa commit phía thứ
ba của người dùng không có trong máy này**, nên phần hòa giải từng hunk
cho 8 tệp đã liệt kê trong tài liệu hòa giải (ReaderPreferences,
GameReaderController, ReaderGestureSurface, ReaderSetupActivity, hai
tệp `a11y_reader.xml`, `a11y_world.cpp/.h`) **chưa thực hiện được và
không được tuyên bố là đã hòa giải**. Khi người dùng cung cấp bản
local đó, R00.1 phải mở lại theo từng hunk cho 8 tệp trên. Liên quan:
quyết định hoán vị 8 hướng của cử chỉ (R05) đang hoãn chờ chính bản
local này xác nhận có yêu cầu hay không — không tự xây hệ preference
thứ hai.

### 3.3. Snake theo lượt — chờ nghiệm thu người khiếm thị

R18 giữ nhịp gốc của 4 minigame còn lại; riêng **Snake chạy theo lượt**
(mỗi lệnh hướng = một bước) thay cho vòng lặp thời gian thực — thay đổi
nhịp **đã công bố trong mã và tài liệu R18**, không giấu. Nghiệm thu
người khiếm thị ở giai đoạn 3 quyết định giữ chính sách theo lượt hay
mở lại R18.4; nếu bản theo lượt bị kết luận là không còn là Snake, mục
đó phải mở lại chứ không được coi là đã xong.

### 3.4. Âm cứu hộ — tệp thật, giọng máy, chờ thay giọng tự nhiên

6 tệp MP3 cứu hộ trong `res/raw/` là **âm thật** do eSpeak NG sinh
offline (không phải placeholder), có provenance và transcript trong
`R06-RESCUE-AUDIO-ASSETS.vi.md`, đặt tên để thay nguyên tệp khi có
giọng tự nhiên. Giới hạn giữ nguyên: giọng robot; **nghe rõ thuộc giai
đoạn 2/3** — không suy "có tệp" thành "nghe được".

### 3.5. Chưa ghim ở manifest phạm vi (mục 8 của SCOPE-MANIFEST)

Danh sách mod của cấu hình phát hành; dải máy/Android đã chứng nhận;
phiên bản phát hành và **khóa ký phát hành** (khóa `accessibility-test`
hiện dùng là khóa thử công khai — không được dùng phát hành); flavor
phát hành chính thức. Các mục này chốt ở giai đoạn đóng gói/phát hành,
không bịa ở giai đoạn 1.

### 3.6. Danh sách nguồn/build — đã kiểm đủ, không phải ràng buộc (đính chính)

> **Đính chính (2026-10-23, cùng đợt R23):** bản đầu của mục này ghi
> sai rằng danh sách MSVC thiếu `a11y_outcome.cpp/.h` và một số tệp
> nền. Kiểm chứng trực tiếp tại các tệp project cho kết quả ngược lại,
> ghi đúng ở đây và ở `PHASE1-CANDIDATE-PIN.vi.md` mục 4.

Các project MSVC trong `msvc-full-features/` nạp nguồn bằng wildcard
theo chữ cái đầu (`Cataclysm-libAL-vcpkg-static.vcxproj:175` —
`..\src\a*.cpp;…;..\src\l*.cpp`; `Cataclysm-libMZ-vcpkg-static.vcxproj:174,177`
— header `..\src\*.h` và phần `.cpp` còn lại), và trong repo không có
tệp `.vcxproj.filters` nào. Toàn bộ tệp nguồn mới của R00–R22 — gồm
`a11y_outcome.cpp/.h` — có mặt ở mọi hệ build (MSVC, CMake desktop,
CMake JNI Android, Makefile, Gradle) mà không cần sửa danh sách nào.
Điểm này **không còn là ràng buộc**; lưu ý duy nhất là glob/wildcard
của CMake chốt ở bước cấu hình.

### 3.7. Caveat đóng gói gettext ở đường CMake

Sửa chỉ riêng overlay `lang/po/accessibility/vi.po` có thể không kích
hoạt dựng lại `.mo` ở đường CMake (`lang/CMakeLists.txt` không khai
overlay là phụ thuộc của luật `.mo`; đường `lang/Makefile` có khai).
Giai đoạn 2 phải kiểm chứng khi đóng gói.

### 3.8. Trần snapshot và phân trang

Hợp đồng validate R02 giới hạn 4096 node và 16 cột/bảng mỗi snapshot.
Bảng Study Zone vượt 16 cột khi có hơn 15 NPC đồng đội; snapshot rất
lớn bị từ chối thay vì cắt thầm (đúng thiết kế an toàn). Phương án
phân trang chốt ở giai đoạn 2 cùng các bài liên quan.

### 3.9. Các điểm mở theo bề mặt (hành trình liên quan vẫn đã viết)

- **Marker `partial_con`** (R12): chưa công bố ngữ nghĩa trong snapshot
  thế giới; đường hiện có là menu `prompt_partial_construction` (%
  hoàn thành, Resume/Cancel/Ignore) đã phủ.
- **`src/veh_appliance.cpp`** (R15): tệp thiết bị dạng xe chưa có điểm
  trợ năng nào; menu của nó qua adapter dùng chung, header ImGui chưa
  được công bố — bề mặt kề, giai đoạn 2 rà.
- **Ô lọc gán phím** (R19): widget gốc không có setter cho ô lọc; bộ
  lọc của bộ đọc là lọc trình bày. Ghi thẳng vào ô lọc thật cần nới
  phạm vi sang `cata_imgui` — quyết định giai đoạn 2.
- **Chặn import backup khi phiên game đang sống** (R20): chặn cứng ở
  tầng thấp cần cờ C++/JNI; hiện chặn ở tầng Java theo trạng thái phiên
  đã biết — quyết định giai đoạn 2.
- **TODO/ghi chú trình bày còn lại** (đã phân loại đầy đủ ở R23.2,
  313 hit quét trên 104 tệp đã chạm): **không có TODO/stub/fallback
  "chưa hỗ trợ" nào chặn dùng** trên các hành trình bắt buộc. Các hit
  thuộc phạm vi trợ năng đều là ghi chú chất lượng trình bày (vài
  TODO trong `spell_reader_info` ở `src/magic.cpp`, gồm một chuỗi
  khởi tạo "FIXME" chỉ còn sót khi dữ liệu mod lỗi) hoặc cơ chế cố ý:
  fallback văn bản cảm nhận khi cue chứa placeholder động là biên
  thông tin có chủ đích (`src/a11y_perception.cpp`), và kênh trạng
  thái "unsupported" chỉ phát khi snapshot sai cấu trúc bị validate
  bác — không màn nào được thiết kế để rơi vào đó. Điểm chưa phân
  loại về đầu vào bằng 0 về thực chất tại ứng viên (rà lại toàn bộ
  diff R00–R22 ở R23.2; context `COMPUTER_CONTINUE` đã bổ sung vào
  danh sách literal trong sổ).

### 3.10. Chưa biên dịch, chưa đo hiệu năng

Mọi chữ ký/API ở các đợt đều đối chiếu theo tiền lệ cùng cây; biên
dịch thật có thể phát sinh lỗi phải sửa ở giai đoạn 2/3. Chi phí dựng
snapshot mỗi nhịp poll ở các màn rất lớn (bionics/mutations, xe lớn)
chưa đo; chẩn đoán và khung đo đã chuẩn bị ở R21.

## 4. Đối chiếu điều kiện "không qua R23" của kế hoạch

| Điều kiện của kế hoạch | Trạng thái tại đóng giai đoạn 1 |
| --- | --- |
| Còn nguồn chưa hòa giải | **Còn một phần, ghi công khai:** hai phía có trong máy đã hòa giải ở R00; phía local thứ ba chưa có trong máy nên chưa hòa giải được (mục 3.2). Giai đoạn 1 đóng với ràng buộc này tường minh, chờ người dùng cung cấp bản local để mở lại R00.1. |
| Tài sản cứu hộ chỉ placeholder | Đạt: 6 tệp âm thật có nội dung đọc (giọng máy eSpeak NG), provenance rõ (mục 3.4). |
| Minigame/xe/vùng không có đường chơi | Đạt ở mức viết mã: 5/5 minigame, 4/4 hành trình xe + điều khiển động cơ, 3/3 hành trình vùng đều đã viết, chờ kiểm thử. Snake có chính sách theo lượt đã công bố (mục 3.3). |
| Chưa phân loại hết phạm vi | Đạt: điểm đầu vào chưa phân loại = 0 trong phạm vi đã ghim, đã rà lại ở R23.2. |

## 5. Thứ tự bắt đầu giai đoạn 2 (theo mục 6 của kế hoạch)

1. Build/kiểm tra host và Android (API/resource/native, gettext/đóng
   gói); lỗi chặn được ghi vào báo cáo để các bài phụ thuộc mang trạng
   thái BLOCKED. Việc kèm theo đã ghi ở hồ sơ ghim: khôi phục dispatch
   `cast_spell`, kiểm chứng caveat `.mo` ở đường CMake (danh sách build
   đã kiểm đủ, không cần bổ sung).
2. Bộ tự động ngữ nghĩa/input/Unicode/quyền biết bằng engine/dữ liệu
   thật; chạy cả reader bật/tắt và các biến thể cấu hình đã công bố.
3. Giả lập cho UI/vòng đời/modal (không dùng giả lập không âm thanh để
   kết luận đã nghe).
4. APK ARM64 đầy đủ, máy thật với voice offline/online theo opt-in,
   TalkBack bật/tắt theo vai trò công bố, loa/mono/tai nghe và các lỗi
   có chủ ý.
5. T01–T64 cùng bài bổ sung TX, phiên dài và nâng cấp/backup/khôi phục
   thật (gồm T63 theo quy trình ở `CANDIDATE-RECORD.vi.md`).
6. Người khiếm thị mới và đã quen thử chuỗi đầy đủ ở giai đoạn 3; chỉ
   công bố chơi độc lập khi đủ điều kiện của kế hoạch gốc.

## 6. Hồ sơ bàn giao của giai đoạn 1

- `PHASE1-CANDIDATE-PIN.vi.md` — ghim ứng viên, manifest, kiểm
  build-list, bài/fixture sẵn sàng.
- `JOURNEY-LEDGER.json` / `JOURNEY-LEDGER.vi.md` — sổ hành trình đã đối
  chiếu (trường `doi_chieu_R23` trong JSON).
- `REMAINING-CODE-BACKLOG.json` — 96/96 đầu việc đã viết, chờ kiểm thử.
- `SCOPE-MANIFEST-2026-10-23.vi.md`, `RECOVERY-RECONCILIATION-2026-10-23.vi.md`
  — phạm vi đã ghim và bảng hòa giải nguồn.
- Tài liệu từng đợt `R02`–`R22` (hợp đồng, dispatcher, UI dùng chung,
  cử chỉ, giọng/khởi động, world/nhân vật, thăm dò, hành trang, sức
  khỏe, chiến đấu, chế tạo/xây, overmap/nhiệm vụ, NPC/trại, xe,
  vùng/quy tắc, bionics/phép/máy tính, minigame, kiến thức/trợ giúp,
  lưu/vòng đời, lời/âm/Việt/chẩn đoán, chuẩn bị kiểm thử).
- `TESTING.vi.md` (T01–T64 + TX), `tools/accessibility/fixtures/FIXTURES.vi.md`
  (F-01..F-15), `USER-GUIDE.vi.md`, `BUG-REPORT-TEMPLATE.vi.md`,
  `CANDIDATE-RECORD.vi.md`.

## 7. Commit hồ sơ cuối của giai đoạn 1

Chuỗi commit hồ sơ R23 trên nhánh `recovery/android-reader-ad5ecf0`:
commit đối chiếu sổ `6376a64f`, commit đóng ban đầu `11fbe778` (thông
điệp bắt đầu bằng `R23.3/R23.4:`), và commit đính chính R23 chứa các
sửa về cơ chế nạp nguồn MSVC/build ở mục 3.6 của báo cáo này và mục
3–4 của hồ sơ ghim (thông điệp bắt đầu bằng `R23 errata:`) — **commit
đính chính đó là commit hồ sơ cuối của giai đoạn 1**, hash đầy đủ tra
bằng `git log` trên nhánh. Commit ứng viên mã vẫn là
`bc4f807c853e16909199578c4d68218ae50415b4` (mục đầu báo cáo này) —
mọi commit hồ sơ R23 sau ứng viên đều không thay đổi mã.
