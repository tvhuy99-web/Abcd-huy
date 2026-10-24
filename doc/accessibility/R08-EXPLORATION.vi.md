# R08 — Thăm dò, tri thức và kết quả hành động (giai đoạn 1, chỉ viết mã)

Trạng thái: **đã viết, chờ kiểm thử** (xem `REMAINING-CODE-BACKLOG.json`).
Chưa build, chưa chạy test, chưa tạo APK trong đợt này.
Test ID sẽ chạy ở giai đoạn 2: T19–T30, T35, T44, T50.

Tham chiếu ý tưởng từ gói GnollHack tham khảo (không chép mã): bản đồ ngữ
nghĩa giữ tri thức đã biết và ranh giới thông tin của nó, bộ phân tích/đọc
không gian theo hướng/vùng, và việc tách con trỏ khám phá khỏi di chuyển
nhân vật. CDDA đạt tương đương bằng `observe_tile`/`observe_memory_tile`
+ quét trong `world_snapshot` + con trỏ explore reader-local (mục 2).

## 1. Rà rò rỉ tri thức — kết luận: không có đường đọc sống qua quyền biết

Rà từng đường đọc trong `a11y_perception.cpp` / `a11y_world.cpp`:

| Đường đọc | Cổng tri thức | Kết luận |
| --- | --- | --- |
| Mọi đọc sống trong `observe_tile` | Chỉ chạy khi visibility cache của ô == `CLEAR` | Đạt |
| Ô không thấy trong `observe_tile` | Chỉ `has_memory_at`/`get_memorized_tile` (ter/furn đã nhớ) | Đạt |
| `observe_memory_tile` | Chỉ memory; không heard/creature/live | Đạt |
| Vật phẩm | Visible + `sees_some_items` + loại container kín chưa nhìn vào được + chặn khi mù (effect/worn `BLIND`); nội dung qua `all_known_contents()`, không dùng `all_items_ptr` | Đạt |
| Bẫy | Visible + `trap::can_see(position, viewer)` | Đạt |
| Sinh vật thấy được | Chỉ công bố tên khi `viewer.sees(...)` | Đạt |
| Sinh vật không thấy / cảm nhận đặc biệt | `creature_at(pos, true)` — tham số là `allow_hallucination`, không phải bỏ qua tầm nhìn; pointer chỉ đi qua `describe_special_perception`, hàm tự chặn khi không có special vision, khi `cant_see`, hoặc khi vision bỏ qua aiming cone; văn bản chứa `<`/`>` bị thay bằng câu cảm nhận chung (không chạy `parse_tags`/RNG) | Đạt |
| Marker âm thanh | `sounds::sound_at` là marker "đã nghe" do chính game đặt, không chứa vị trí/định danh thật của nguồn; mô tả luôn ghi marker là gần đúng | Đạt |
| Trạng thái nhân vật, log gần đây | Chỉ trạng thái của chính avatar và log game đã hiển thị cho người chơi | Đạt |

## 2. R08.1 — Con trỏ thăm dò độc lập

- `world_snapshot` nhận thêm `explore_cursor` (offset tương đối so với
  avatar). Không active: hành vi cũ nguyên vẹn + node `explore:enter`.
- Active: focus là `explore:cursor`; origin quét dời theo con trỏ; con
  trỏ khác tầng → toàn bộ ô quét chỉ đọc bằng `observe_memory_tile`
  (memory), cùng tầng dùng `observe_tile`. Node con trỏ mô tả ô + vị trí
  tương đối (hướng, khoảng cách, tầng). Điều khiển: 8 hướng
  `explore:dir:<dx>:<dy>`, `explore:up/down`, `explore:recenter`,
  `explore:exit`. Nhảy: tối đa 24 node `explore:jump:<dx>:<dy>` tới ô
  noteworthy quanh avatar ở tầng avatar (ưu tiên bẫy → sinh vật → cảm
  nhận → field → vật cản → route → âm → xe → đồ → đồ đạc).
- Mọi intent `explore:*` bị chặn trong intent handler của màn thế giới
  **trước** `registered_intent`: chỉ đổi trạng thái reader-local và
  publish lại snapshot. Không sinh action engine, không tốn moves/thời
  gian/RNG. Kẹp biên: |x|,|y| ≤ 60, z ∈ [−10, 10]; id sai định dạng →
  `invalid_value`.
- Con trỏ gắn `lifecycle_epoch` lúc mở; đổi phiên/nhân vật tự xóa con
  trỏ. Khi explore active, `swipe_movement_allowed` bị ép tắt — duyệt
  bằng con trỏ không bao giờ thành lệnh đi bộ.

## 3. R08.2 — Chi tiết ô theo đúng nguồn tri thức

Giữ nguyên bộ lọc all/known/noteworthy/heard, quét memory đa tầng,
special perception và marker âm gần đúng. Bổ sung trong `tile_observation`:

- `door_state` (Locked/Closed/Open): chỉ khi ô có cờ `DOOR`. Locked theo
  tập id cửa khóa của engine (đối chiếu `avatar_action.cpp`); closed suy
  từ movecost của type ô — visible dùng `impassable()` của map, remembered
  tính từ chính type đã nhớ. Remembered chỉ dùng id trong memory.
- `vehicle_part` (chỉ visible): tên bộ phận xe tại ô, không kèm ký hiệu
  độ bền.
- Cường độ field: khi `get_field_intensity() > 1` mô tả ghi "intensity
  N of M"; mức 1 giữ nguyên tên field (tên đã biến thiên theo cường độ).

## 4. R08.3 — Kết quả hành động thật tại điểm engine

Kênh mới `src/a11y_outcome.h/.cpp`: `record_action_outcome(kind, detail)`
(chỉ ghi khi bộ đọc bật, có số thứ tự tăng dần), `last_action_outcome()`,
`reset_action_outcomes()`, `describe_action_outcome()`. World snapshot
công bố node `world:last_outcome` khi có kết quả. Đây không phải vòng
đọc log: log game vẫn đi đường `Messages` riêng; kênh outcome chỉ ghi
tại nhánh engine quyết định sự kiện:

- `avatar_action::move`: RAII guard lưu vị trí đầu + sequence. Record
  tường minh: mở cửa (3 nhánh, kèm tên cửa/bộ phận xe), safe-mode hủy
  sau peek (undetermined + lý do), invalid-move cuối hàm → `blocked`
  với phân loại vật cản đúng thứ tự — creature (chỉ khi nhân vật thấy
  được) → `map::obstacle_name` (đã gồm bộ phận xe/địa hình/đồ đạc như
  engine công bố); hai điểm `melee_attack` được bọc bool → `attacked`
  + tên mục tiêu khi đòn thật sự thực hiện (trúng/trượt do engine quyết
  và đã có trong log — kênh outcome không tự phán hit/miss). Guard chốt:
  record tường minh thắng; chưa có record thì vị trí đổi → `moved`,
  **không đổi → `undetermined`, tuyệt đối không suy "blocked" từ tọa
  độ không đổi**.
- `game::vertical_move`: guard tương tự (z đổi → `vertical_moved`);
  nhánh `CANNOT_MOVE` không tạo guard; **peek giữa các tầng là quan sát
  nên guard bị vô hiệu khi `peeking=true`** (vá ở bước tích hợp sau báo
  cáo nhánh B) — peek không ghi đè kết quả thật trước đó.
- `avatar::set_movement_mode`: đổi chế độ thành công và khác cũ →
  `stance_changed` + tên chế độ (`move_mode::name()`).
- `replace_game_session()` gọi `reset_action_outcomes()` — đổi nhân
  vật/load không mang kết quả cũ sang (cùng cơ chế dọn dữ liệu theo
  nhân vật của R08.4).

Lưu ý tương tác đã biết: nếu gãy chân giữa bước đi khiến
`set_movement_mode(prone)` ghi `stance_changed`, record tường minh đó
thắng và guard của `move()` không ghi thêm `moved` cho bước đó — kết
quả công bố vẫn là sự kiện thật, chỉ giữ một kết quả gần nhất.

## 5. R08.4 — Tương tác theo bằng chứng UI thật

| Biến thể | Đường engine | Phủ của bộ đọc |
| --- | --- | --- |
| Mở/đóng cửa, examine, smash, grab, chọn ô kề cho open-tile activity | `choose_direction` / `choose_adjacent_highlight` (`src/action.cpp`, `activity_actor.cpp`) | Đã có `screen_scope` "adjacent-direction": mỗi hướng đọc mô tả ô đích bằng `observe_tile`, ghi rõ "đọc không hành động" — giữ nguyên |
| Leo xuống ô hở | `point_selection_menu` trong `ACTION_MOVE_DOWN` | Menu dùng chung `uilist` (adapter R04) — giữ nguyên |
| Bơi | `avatar_action::swim` được gọi từ trong `move` (`avatar_action.cpp`, `game.cpp`) | Kết quả qua guard outcome của `move` |
| Lên/xuống tầng, leo | `game::vertical_move` | Guard outcome (mục 4) |
| Kéo/đẩy, haul | Lệnh registered + di chuyển qua `move`; đẩy vật cản là nhánh invalid-move | Node lệnh + outcome `blocked`/`moved` |
| Công tắc/cửa/lửa/chất lỏng qua examine | Luồng examine ở trên | Mô tả ô đích theo quyền biết trước khi xác nhận |

Tách bạch: con trỏ quan sát (mục 2), điều khiển avatar (lệnh engine) và
camera deathcam (`camera_offset` sẵn có) là ba thứ riêng; dữ liệu thăm
dò/outcome dọn theo phiên nhân vật (epoch + `replace_game_session`).

## 6. Chỗ chưa chắc

- Chưa biên dịch: đã đối chiếu chữ ký bằng đọc mã (vpart `part(int).name(false)`,
  `get_field_intensity`, `move_mode::name()`, `obstacle_name`, toán tử
  `tripoint_rel_ms`), tính biên dịch thật thuộc giai đoạn 2.
- `door_state` closed cho ô remembered là suy từ movecost của type đã
  nhớ — đúng với cửa địa hình; cửa xe không nằm trong đường remembered
  (đường đó vốn không đọc vehicle).
- Tổ hợp explore + death camera cùng lúc: origin quét theo avatar +
  cursor, không cộng camera offset; node `world:here` vẫn mô tả ô
  camera. `explore:enter` chỉ công bố ở snapshot thường nên tổ hợp này
  gần như không xảy ra.
- MSVC project file có thể có danh sách nguồn tường minh: nguồn C++ mới
  `a11y_outcome.cpp` được CMake/Makefile glob tự nhặt (đã kiểm), danh
  sách MSVC chưa đối chiếu — ghi nhận cho R23 rà build-list.
