# Hòa giải nguồn R00 — đối chiếu ba phía f40dcdd / ad5ecf0 / local

Ngày: 2026-10-23 (giờ Việt Nam). Mục kế hoạch: R00 trong
`REMAINING-CODE-PLAN.vi.md` bản 2.0, mục 4.

Trạng thái của tài liệu này: **bằng chứng nguồn của giai đoạn 1 (chỉ viết
mã)**. Lượt làm việc này không build, không chạy test, không giả lập,
không tạo APK. Mọi câu "đã nối/đã sửa" dưới đây chỉ có nghĩa là mã đã được
viết và đọc đối chiếu hai phía; chưa có nghĩa là đã biên dịch hoặc chạy
được. Kiểm chứng thuộc giai đoạn 2 (bài T07, T09–T12, T19, T28, T50,
T53–T55, T62 theo kế hoạch).

## 1. Các phía nguồn và giới hạn đối chiếu

| Phía | Mốc | Tình trạng trong máy này |
| --- | --- | --- |
| Nguồn cũ | `f40dcdd05b59bd50976fbd9843a714544fda0418` | Có trong lịch sử clone nông; là tổ tiên của ad5ecf0, cách 37 commit |
| Nguồn phục hồi (nền) | `ad5ecf06d0db194ede3715fefab6857e9605eaa7` | Có; đầu nhánh `feature/android-reader-code-phase`, gốc của nhánh làm việc `recovery/android-reader-ad5ecf0` |
| Thay đổi local chưa commit của người dùng | bản sao trước phục hồi ở workspace cũ | **KHÔNG có trong máy này** — không worktree, không patch, không manifest |

Hệ quả trung thực của giới hạn trên: R00.1 ở lượt này đối chiếu được hai
phía có trong git (`f40dcdd` ↔ `ad5ecf0`) ở mức từng tệp, cộng hai delta
local đã được kế hoạch ghi lại ở mục 2 của nó (deadline thử ánh xạ và
`allowSwipeMovement`). Phần "từng hunk" đối với phía local thứ ba **chưa
đối chiếu được** và được ghi rõ như vậy ở mục 5; không suy đoán nội dung
bản local từ hai commit trong git. Khi có bản sao local, phải mở lại
R00.1 cho các tệp ở mục 5 trước khi coi R00.1 là đóng hoàn toàn.

Quyết định nền cho mọi tệp: **giữ `ad5ecf0` làm nền**, không áp toàn bộ
patch cũ/local lên nguồn mới — đúng quy định của kế hoạch. Chỉ hai delta
đã biết được ghép có chọn lọc, và cả hai được ghép bằng cách viết lại an
toàn hơn bản local đã mô tả (mục 3 và 4), không chép nguyên trạng.

Nguồn lệnh đã dùng để trích danh sách:

```bash
git diff --stat        f40dcdd05b59bd50976fbd9843a714544fda0418..ad5ecf06d0db194ede3715fefab6857e9605eaa7
git diff --name-status f40dcdd05b59bd50976fbd9843a714544fda0418..ad5ecf06d0db194ede3715fefab6857e9605eaa7
git diff --numstat     f40dcdd05b59bd50976fbd9843a714544fda0418..ad5ecf06d0db194ede3715fefab6857e9605eaa7
git grep -n "allowSwipeMovement\|worldMovementEnabled\|world:here" <từng commit> -- src/ android/
```

Tổng diff: 78 tệp, 7.559 dòng thêm, 810 dòng xóa; 7 tệp mới (A), 71 tệp
sửa (M), 0 tệp xóa/đổi tên. Phân nhóm: Java Android chính 8, Android test
3, Android resource 5, C++ lõi a11y 12, C++ test 1, C++ điểm nối
gameplay/UI 45, tài liệu 3, dịch 1.

## 2. Bảng quyết định cho 78 tệp

Cột Delta: `A` = liên quan delta deadline thử ánh xạ; `B` = liên quan
delta cờ vuốt di chuyển; `—` = không liên quan trực tiếp. Quyết định cho
mọi hàng là **giữ nền ad5ecf0**; các hàng có delta ghi thêm việc đã làm
trong cùng commit R00 này (mục 3, 4) hoặc việc phải mở lại khi có bản
local.

| # | TT | Tệp | + / − | Nhóm | Delta | Quyết định / ghi chú nguồn |
|---|---|---|---:|---|---|---|
| 1 | A | `android/app/src/androidTest/java/com/cleverraven/cataclysmdda/BootstrapSelfVoiceSurfaceTest.java` | 75 / 0 | Android test | — | Giữ ad5ecf0 |
| 2 | A | `android/app/src/androidTest/java/com/cleverraven/cataclysmdda/ReaderGestureSurfaceTest.java` | 57 / 0 | Android test | B gián tiếp (test bề mặt cử chỉ) | Giữ ad5ecf0 |
| 3 | M | `android/app/src/androidTest/java/com/cleverraven/cataclysmdda/ReaderSetupActivityTest.java` | 85 / 2 | Android test | A (caller test của trial, dòng 156–168) | Giữ ad5ecf0; logic test vẫn khớp ngữ nghĩa mới (begin → active → confirm/revert trong cùng tiến trình), kiểm chứng ở giai đoạn 2 |
| 4 | A | `android/app/src/main/java/com/cleverraven/cataclysmdda/BootstrapSelfVoiceSurface.java` | 267 / 0 | Java Android | — | Giữ ad5ecf0 |
| 5 | M | `android/app/src/main/java/com/cleverraven/cataclysmdda/CataclysmDDA.java` | 7 / 0 | Java Android | — | Giữ ad5ecf0 |
| 6 | M | `android/app/src/main/java/com/cleverraven/cataclysmdda/GameReaderController.java` | 429 / 5 | Java Android | **A + B (tệp chính của cả hai)** | Giữ ad5ecf0; đã sửa `worldMovementEnabled()` tại R00.2 (mục 3) và điểm cạnh tranh trial/practice tại R00.3 (mục 4.1); các chữ ký trial giữ nguyên nên các caller khác không phải sửa cho delta A |
| 7 | M | `android/app/src/main/java/com/cleverraven/cataclysmdda/ReaderGestureSurface.java` | 96 / 13 | Java Android | B (interface `worldMovementEnabled()` dòng 21, rẽ nhánh dòng 184) | Giữ ad5ecf0; chữ ký interface không đổi |
| 8 | M | `android/app/src/main/java/com/cleverraven/cataclysmdda/ReaderPreferences.java` | 155 / 0 | Java Android | **A (tệp chính)**; đồng thời giữ `fast_movement` cho B | Giữ ad5ecf0; đã viết lại cơ chế deadline tại R00.3 (mục 4) |
| 9 | M | `android/app/src/main/java/com/cleverraven/cataclysmdda/ReaderSetupActivity.java` | 599 / 11 | Java Android | A (caller `beginGestureMapTrial` dòng 412) | Giữ ad5ecf0; chữ ký `beginGestureMapTrial` không đổi nên caller không phải sửa |
| 10 | M | `android/app/src/main/java/com/cleverraven/cataclysmdda/ReaderSpeechQueue.java` | 27 / 3 | Java Android | — | Giữ ad5ecf0 |
| 11 | M | `android/app/src/main/java/com/cleverraven/cataclysmdda/SplashScreen.java` | 380 / 19 | Java Android | — | Giữ ad5ecf0 |
| 12 | M | `android/app/src/main/res/values-vi/a11y_reader.xml` | 51 / 1 | Resource VI | A/B (chuỗi thử 45 giây, fast movement) | Giữ ad5ecf0; ngữ nghĩa chuỗi không đổi (vẫn thử 45 giây, hết hạn thì hoàn) nên không sửa chuỗi ở R00 |
| 13 | M | `android/app/src/main/res/values-vi/strings.xml` | 5 / 2 | Resource VI | — | Giữ ad5ecf0 |
| 14 | M | `android/app/src/main/res/values/a11y_ids.xml` | 9 / 0 | Resource | — | Giữ ad5ecf0 |
| 15 | M | `android/app/src/main/res/values/a11y_reader.xml` | 51 / 1 | Resource EN | A/B (như hàng 12) | Giữ ad5ecf0; không sửa chuỗi ở R00 |
| 16 | M | `android/app/src/main/res/values/strings.xml` | 5 / 2 | Resource EN | — | Giữ ad5ecf0 |
| 17 | M | `doc/accessibility/COVERAGE.md` | 6 / 5 | Tài liệu | — | Giữ ad5ecf0 |
| 18 | M | `doc/accessibility/IMPLEMENTATION.vi.md` | 235 / 22 | Tài liệu | — | Giữ ad5ecf0 |
| 19 | M | `doc/accessibility/STATUS.md` | 137 / 1 | Tài liệu | — | Giữ ad5ecf0 |
| 20 | M | `lang/po/accessibility/vi.po` | nhị phân | Dịch | — | Giữ ad5ecf0 |
| 21 | M | `src/a11y_android.cpp` | 55 / 0 | C++ lõi a11y | — | Giữ ad5ecf0 |
| 22 | M | `src/a11y_android.h` | 1 / 0 | C++ lõi a11y | — | Giữ ad5ecf0 |
| 23 | A | `src/a11y_body.h` | 13 / 0 | C++ lõi a11y | — | Giữ ad5ecf0 |
| 24 | A | `src/a11y_document.h` | 28 / 0 | C++ lõi a11y | — | Giữ ad5ecf0 |
| 25 | M | `src/a11y_input.cpp` | 3 / 1 | C++ lõi a11y | — | Giữ ad5ecf0 |
| 26 | M | `src/a11y_perception.cpp` | 88 / 1 | C++ lõi a11y | — | Giữ ad5ecf0 |
| 27 | M | `src/a11y_perception.h` | 4 / 0 | C++ lõi a11y | — | Giữ ad5ecf0 |
| 28 | A | `src/a11y_requirements.h` | 43 / 0 | C++ lõi a11y | — | Giữ ad5ecf0 |
| 29 | M | `src/a11y_runtime.cpp` | 28 / 6 | C++ lõi a11y | B gián tiếp (đường publish snapshot) | Giữ ad5ecf0; không cần sửa: cờ mới đi qua `snapshot_json` sẵn có |
| 30 | M | `src/a11y_runtime.h` | 5 / 0 | C++ lõi a11y | — | Giữ ad5ecf0 |
| 31 | M | `src/a11y_world.cpp` | 415 / 7 | C++ lõi a11y | **B (producer của snapshot world)** | Giữ ad5ecf0; đã thêm tính cờ engine tại R00.2 (mục 3) |
| 32 | M | `src/a11y_world.h` | 10 / 1 | C++ lõi a11y | B (khai báo đi kèm) | Giữ ad5ecf0; chữ ký `world_snapshot` không đổi |
| 33 | M | `src/action.cpp` | 40 / 3 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 34 | M | `src/action.h` | 5 / 2 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 35 | M | `src/bionics_ui.cpp` | 3 / 2 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 36 | M | `src/calendar_ui.cpp` | 68 / 0 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 37 | M | `src/construction.cpp` | 88 / 0 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 38 | M | `src/crafting_gui.cpp` | 196 / 0 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 39 | M | `src/crafting_gui_helpers.cpp` | 7 / 2 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 40 | M | `src/crafting_gui_helpers.h` | 2 / 1 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 41 | M | `src/dialogue_imgui.cpp` | 18 / 0 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 42 | M | `src/dialogue_imgui.h` | 1 / 0 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 43 | M | `src/diary_ui.cpp` | 172 / 8 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 44 | M | `src/do_turn.cpp` | 51 / 3 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 45 | M | `src/do_turn.h` | 1 / 0 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 46 | M | `src/end_screen.cpp` | 41 / 1 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 47 | M | `src/end_screen.h` | 2 / 0 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 48 | M | `src/faction_ui.cpp` | 484 / 35 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 49 | M | `src/faction_ui.h` | 4 / 0 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 50 | M | `src/game_inventory.cpp` | 5 / 2 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 51 | M | `src/handle_action.cpp` | 85 / 512 | C++ gameplay/UI | B gián tiếp (điểm phân phát hành động di chuyển: `rcdrive`/`pldrive`/đi bộ) | Giữ ad5ecf0 — tệp duy nhất giảm ròng lớn; **không** khôi phục bản f40. Nguồn đối chiếu cho điều kiện remote ở mục 3 chính là tệp này |
| 52 | M | `src/input_context.cpp` | 37 / 7 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 53 | M | `src/input_context.h` | 2 / 0 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 54 | M | `src/inventory_ui.cpp` | 161 / 2 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 55 | M | `src/inventory_ui.h` | 6 / 0 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 56 | M | `src/magic.cpp` | 12 / 4 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 57 | M | `src/main_menu.cpp` | 60 / 0 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 58 | M | `src/medical_ui.cpp` | 156 / 4 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 59 | M | `src/messages.cpp` | 28 / 11 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 60 | M | `src/mission_companion.cpp` | 116 / 0 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 61 | M | `src/mission_ui.cpp` | 323 / 3 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 62 | M | `src/mutation_ui.cpp` | 33 / 66 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 63 | M | `src/newcharacter.cpp` | 265 / 15 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 64 | M | `src/npctalk.cpp` | 79 / 1 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 65 | M | `src/options.cpp` | 216 / 2 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 66 | M | `src/output.cpp` | 25 / 2 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 67 | M | `src/output.h` | 4 / 0 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 68 | M | `src/overmap_ui.cpp` | 328 / 0 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 69 | M | `src/ranged.cpp` | 189 / 0 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 70 | M | `src/scores_ui.cpp` | 219 / 4 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 71 | M | `src/trade_ui.cpp` | 40 / 0 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 72 | M | `src/trade_ui.h` | 4 / 0 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 73 | M | `src/ui_iteminfo.cpp` | 16 / 0 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 74 | M | `src/uilist.cpp` | 11 / 1 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 75 | M | `src/uilist.h` | 5 / 0 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 76 | M | `src/wish.cpp` | 94 / 11 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 77 | M | `src/worldfactory.cpp` | 431 / 3 | C++ gameplay/UI | — | Giữ ad5ecf0 |
| 78 | A | `tests/a11y_body_test.cpp` | 57 / 0 | C++ test | — | Giữ ad5ecf0 |

Ngoài 78 tệp trên, commit R00 này còn chạm các tệp **không** nằm trong
diff f40..ad5 vì chúng chứa hợp đồng snapshot dùng chung:
`src/a11y_semantics.h` và `src/a11y_semantics.cpp` (không đổi trong
f40..ad5) — lý do và phạm vi ở mục 3. Đây là thay đổi mới của R00.2,
không phải hòa giải một hunk cũ.

## 3. R00.2 — Cờ cho phép vuốt di chuyển xuyên C++ → JSON → Java

### 3.1 Sự thật nguồn trước khi sửa

- Tên `allowSwipeMovement` **không tồn tại** ở cả `f40dcdd` lẫn
  `ad5ecf0` (`git grep` = 0 kết quả, cả `src/` và `android/`). Nó chỉ có
  trong bản local chưa commit — phía thứ ba không có trong máy.
- Ở `f40dcdd` chưa có `worldMovementEnabled()`; ở `ad5ecf0`,
  `GameReaderController.worldMovementEnabled()` chỉ kiểm
  `!controls && !movementPractice && editor == null &&
  preferences.isFastMovementEnabled() && snapshot.find("world:here") != null`.
- Phía C++ **không có cờ nào** trong snapshot ở cả hai commit. Node
  `world:here` được `world_snapshot()` (`src/a11y_world.cpp`) tạo cả khi
  đang deathcam (chỉ đổi nhãn thành "Death camera center", ID giữ nguyên)
  — vì vậy điều kiện Java cũ cho phép vuốt "đi bộ" ngay trong deathcam,
  và cũng không phân biệt lái xe/remote/hoạt động.

### 3.2 Cách nối đã làm (thay cho việc chép `allowSwipeMovement` của local)

Ý tưởng của local (một cờ cho phép vuốt do engine quyết định) được giữ,
nhưng điều kiện được mở rộng theo đúng yêu cầu R00.2 và đặt ở game
thread, nơi trạng thái engine là thật:

1. `src/a11y_semantics.h`: `semantic_snapshot` thêm trường
   `bool swipe_movement_allowed = false`. Mặc định false cho mọi
   snapshot; chỉ snapshot world mới có thể bật.
2. `src/a11y_semantics.cpp`: `snapshot_content_json()` serialize thêm
   `"swipeMovementAllowed":true|false` ở cấp snapshot. Vì cờ nằm trong
   content JSON, thay đổi của nó tự làm tăng revision qua cơ chế so
   content sẵn có của `a11y_runtime.cpp` — không cần đổi chữ ký, JNI hay
   build-list nào.
3. `src/a11y_world.cpp` (`world_snapshot`): cờ chỉ true khi đồng thời —
   không có `camera_offset` (không deathcam); `!viewer.is_dead_state()`;
   `!viewer.in_vehicle`; `!viewer.controlling_vehicle`;
   `!viewer.is_driving()`; không còn khóa `remote_controlling` (điều
   khiển xe RC — đường `rcdrive` trong `src/handle_action.cpp`); không
   còn khóa `remote_controlling_vehicle` (đường `game::remoteveh()`
   trong `src/game.cpp`); `!viewer.activity`; `!viewer.has_destination()`
   và `!viewer.has_destination_activity()` (không đang di chuyển tự
   động); `!viewer.has_effect(effect_sleep)`. Mọi trường hợp không chắc
   chắn đều rơi về false. Hai kiểm tra khóa remote cố ý rộng hơn vị từ
   dispatch thật của engine (vốn còn đòi item/bionic đang active và
   nhiên liệu): thà cấm oan một phiên vuốt còn hơn cho vuốt lái xe RC/xe
   từ xa — lệch về phía an toàn và có ghi nhận.
4. `ReaderSnapshot.java`: thêm trường `swipeMovementAllowed`, parse bằng
   `optBoolean("swipeMovementAllowed", false)` — snapshot cũ/thiếu trường
   thì cấm, không suy diễn.
5. `GameReaderController.java`: `worldMovementEnabled()` nay yêu cầu
   thêm `snapshot.swipeMovementAllowed`, giữ nguyên preference
   `fast_movement` của người dùng, kiểm tra `world:here` (chỉ còn là kiểm
   tra tỉnh táo về loại màn), kiểm tra đủ cả 8 node lệnh hướng
   (`UP`…`LEFTUP`) trong snapshot — phần "đủ hướng" của R00.2 — và các
   kiểm tra phía Java vốn có (`controls` = lớp phủ điều khiển/modal phía
   Java, `movementPractice`, `editor`). Ngược lại,
   `movementPracticeEnabled()` cố ý **không** đòi cờ engine: bài luyện
   không gửi intent gameplay, chỉ đọc nhãn hướng và đếm hướng đã luyện.

### 3.3 Phần còn treo của R00.2

- Điều kiện "editor/modal" phía C++: các modal/engine dialog không đi qua
  `world_snapshot` thì snapshot world không được publish ở vị trí trên
  cùng của stack màn (`a11y_runtime` chỉ publish màn trên cùng), và phía
  Java đã chặn bằng `controls`/`editor`. Chưa có một cờ modal riêng phía
  C++ cho trường hợp một modal mở **trong lúc** màn world vẫn ở trên
  cùng; nếu giai đoạn 2 phát hiện trường hợp như vậy, phải bổ sung ở
  R03 (dispatcher/modal), không bịa thêm ở R00.
- Tư thế di chuyển (đi bộ/chạy/cúi) và địa hình (bơi/lặn/leo) không bị
  chặn riêng: vuốt gửi đúng hành động di chuyển theo hướng của engine,
  engine tự áp luật tư thế/địa hình như với phím. Đây là lựa chọn có ghi
  nhận, không phải thiếu sót âm thầm.
- Hành khách ngồi trong xe nhưng không điều khiển vẫn bị cấm vuốt (điều
  kiện `in_vehicle` rộng hơn `player_in_control`). Cố ý chọn phía an
  toàn: rời ghế/đổi chỗ phải đi qua menu lệnh, không qua cử chỉ đi bộ.

## 4. R00.3 — Hai cơ chế luyện cử chỉ, preference và deadline đơn điệu

### 4.1 Rà soát hai cơ chế trong ad5ecf0

- **Thử ánh xạ cử chỉ (gesture map trial):** thuộc `ReaderPreferences` +
  `ReaderSetupActivity` (bắt đầu) + `GameReaderController` (đếm giờ, xác
  nhận/hoàn). Thời lượng 45 giây (`GESTURE_MAP_TRIAL_MS`), bản đồ ứng viên
  ở khóa `trial_horizontal_swipe_action` / `trial_vertical_swipe_action`,
  bản đồ đã xác nhận ở `horizontal_swipe_action` /
  `vertical_swipe_action`.
- **Luyện di chuyển 8 hướng (movement practice):** thuộc
  `GameReaderController` (cờ phiên `movementPractice`, bitmask
  `movementPracticeDirections`), hoàn thành lưu ở khóa
  `movement_practice_complete`; chỉ đọc nhãn hướng trong snapshot, **không
  gửi hành động gameplay**.
- **Không trùng khóa preference:** hai cơ chế dùng các khóa khác nhau
  hoàn toàn trong cùng tệp `internal_reader`; khóa dùng chung duy nhất có
  liên quan là `fast_movement` (bật/tắt vuốt di chuyển nhanh) — practice
  chỉ là điều kiện mở khóa lần đầu của `fast_movement`, không phải một
  trial thứ hai của cùng dữ liệu. Bất đối xứng có sẵn và giữ nguyên:
  `restoreGestureDefaults()` không xóa `movement_practice_complete` và
  `fast_movement`.
- **Ở tầng phân phát cử chỉ, hai bài tập không tranh nhau:**
  `ReaderGestureSurface` rẽ nhánh theo thứ tự cứng — practice trước,
  rồi vuốt di chuyển thế giới, rồi vuốt theo ánh xạ. Nhưng ở tầng trạng
  thái Java vẫn còn một cạnh tranh thật đã tìm thấy khi đối chiếu: các
  nút mở practice trong màn điều khiển vẫn bấm được **giữa lúc trial
  đang chạy**, và trước R00 thì `beginMovementPractice()` không hủy
  đồng hồ trial — trial hết hạn giữa practice sẽ ép `controls=true`
  trong khi cờ `movementPractice` vẫn còn, tạo trạng thái lai chỉ thoát
  được bằng Back/kích hoạt. R00 sửa đúng điểm này trong
  `GameReaderController`: vào practice thì hoàn trial trước (quay về bản
  đồ đã xác nhận, hủy runnable đếm giờ), và nhánh hết hạn của runnable
  trial cũng xóa luôn trạng thái practice nếu nó còn sót — hai bài tập
  không bao giờ sống cùng lúc.
- Chuỗi Việt/Anh của hai cơ chế (`values/` và `values-vi/a11y_reader.xml`)
  nhất quán với ngữ nghĩa sau sửa (thử 45 giây, hết hạn thì hoàn về bản đồ
  đã xác nhận) nên R00 không sửa chuỗi.

### 4.2 Lỗi đã xác nhận và cách sửa deadline

Ở ad5ecf0, `ReaderPreferences` dùng `System.currentTimeMillis()` tại ba
điểm (dòng ~139, ~153, ~165) và **lưu deadline tường vào
SharedPreferences** ở khóa `gesture_map_trial_deadline`. Kế hoạch yêu
cầu: chuyển sang đồng hồ đơn điệu và không coi một deadline đơn điệu đã
lưu là hợp lệ qua khởi động lại — vì `elapsedRealtime()` tính lại từ lúc
mở máy, lưu nó xuống đĩa là vô nghĩa ở lần boot sau.

Cách đã viết lại (chỉ trong `ReaderPreferences.java`, không đổi chữ ký
công khai nào, nên mọi caller — `GameReaderController`,
`ReaderSetupActivity`, test — không phải sửa):

- Đồng hồ: `SystemClock.elapsedRealtime()`.
- Trạng thái pending và thời điểm bắt đầu chỉ nằm ở **biến tĩnh trong bộ
  nhớ tiến trình** (`trialPendingInMemory`, `trialStartedElapsedMs`),
  dùng chung cho mọi instance `ReaderPreferences` trong cùng tiến trình
  (Setup và Controller tạo instance riêng — đây là điều kiện để trial bắt
  đầu ở Setup còn hiệu lực khi quay lại game).
- Khóa `gesture_map_trial_pending` và `gesture_map_trial_deadline` không
  còn được ghi. Dữ liệu ứng viên (`trial_*_swipe_action`) vẫn nằm trong
  SharedPreferences nhưng là **dữ liệu trơ**: chỉ được đọc khi tiến trình
  hiện tại có trial đang pending trong bộ nhớ.
- Constructor dọn dẹp: nếu tiến trình không có trial sống mà trên đĩa còn
  khóa trial (từ tiến trình trước hoặc định dạng cũ), xóa chúng — tức
  restart/process chết/reboot/hết hạn đều trả về **bản đồ đã xác nhận**,
  đúng quy định R05.1 của kế hoạch.
- `startGestureMapTrialTimer()` vẫn bắt đầu đếm ở lần resume đầu tiên sau
  khi rời Setup; `gestureMapTrialRemainingMs()`,
  `isGestureMapTrialActive()`, `confirmGestureMapTrial()`,
  `revertGestureMapTrial()`, `restoreGestureDefaults()` giữ nguyên ngữ
  nghĩa đối với caller, chỉ đổi nguồn thời gian và nơi giữ trạng thái.

### 4.3 Phần còn treo của R00.3

- Trạng thái tĩnh trong bộ nhớ không có khóa đồng bộ riêng; mọi caller
  hiện tại đều chạy trên luồng UI/Android main. Nếu giai đoạn 2 thêm
  caller từ luồng khác, phải xét lại điểm này.
- Trường hợp Android giết tiến trình **trong lúc Setup đang mở** rồi khôi
  phục thẳng vào game: trial bị bỏ và bản đồ đã xác nhận được dùng — đây
  là hành vi an toàn đã chọn, người dùng chỉ cần bắt đầu lại trial.

## 5. Những gì chưa đối chiếu được (phía local thứ ba)

Các tệp dưới đây là nơi hai delta local đã biết chạm tới, và cũng là nơi
bản local có thể chứa khác biệt khác ngoài hai delta đó. Khi có bản sao
local, R00.1 phải mở lại **từng hunk** cho đúng các tệp này trước khi
đóng hẳn:

- `android/app/src/main/java/com/cleverraven/cataclysmdda/ReaderPreferences.java`
- `android/app/src/main/java/com/cleverraven/cataclysmdda/GameReaderController.java`
- `android/app/src/main/java/com/cleverraven/cataclysmdda/ReaderGestureSurface.java`
- `android/app/src/main/java/com/cleverraven/cataclysmdda/ReaderSetupActivity.java`
- `android/app/src/main/res/values/a11y_reader.xml`,
  `android/app/src/main/res/values-vi/a11y_reader.xml`
- `src/a11y_world.cpp`, `src/a11y_world.h` (nếu local có cờ phía C++)

Ngoài các tệp trên: không có bằng chứng nào cho thấy bản local chạm vào
71 tệp còn lại của bảng mục 2, nhưng cũng **không có bằng chứng ngược
lại** — các thay đổi local dạng xóa đường dẫn/asset mà kế hoạch nhắc tới
chưa rõ mục đích, và theo R00.4 chúng **không** được đưa vào commit trợ
năng này khi chưa rõ. Toàn bộ phần local ngoài hai delta đã biết giữ
trạng thái "chưa đối chiếu được", không ghi là đã hòa giải.

## 6. R00.4 — Commit

Toàn bộ thay đổi R00 (tài liệu này, R00.2, R00.3) được chốt trong một
commit trên nhánh `recovery/android-reader-ad5ecf0`, thông điệp commit
ghi nguồn gốc từng phần. Không push. Không có xóa asset nào trong commit.
Hash commit được ghi trong báo cáo bàn giao của lượt R00 (tài liệu này
không tự chứa hash của chính commit chứa nó).
