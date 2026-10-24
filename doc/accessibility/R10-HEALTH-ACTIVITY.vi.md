# R10 — Sức khỏe, sinh hoạt và hoạt động dài (giai đoạn 1, chỉ viết mã)

Trạng thái: **đã viết, chờ kiểm thử** (xem `REMAINING-CODE-BACKLOG.json`).
Chưa build, chưa chạy test, chưa tạo APK trong đợt này.
Test ID sẽ chạy ở giai đoạn 2: T16, T35–T38, T53–T54, T60.

Quy tắc bất biến của mục này, áp dụng cho mọi đầu việc: mô tả sức khỏe
theo **đúng dữ kiện UI gốc công bố** (mô tả định tính của game), không
thay bằng HP sống hay số nội bộ mà nhân vật không được biết; trạng thái
vốn chỉ thể hiện bằng màu phải có tương đương lời lấy từ cùng một dữ
kiện; **dừng tiếng đọc không bao giờ đồng nghĩa hủy hoạt động** — hủy là
một intent riêng và kết quả hủy là kết quả thật của engine; cảnh báo
trạng thái không đọc trùng nguyên văn một dòng log game.

## 1. R10.1 — Trạng thái đầy đủ

### 1.1 Màn trạng thái nhân vật (`src/player_display.cpp`, mới)

`Character::disp_info` trước đợt này có **0 điểm a11y**. Nay có
`screen_scope` "player_display" gắn vào `input_context` "PLAYER_INFO"
của chính màn, cập nhật trong vòng lặp của màn, và builder
`player_display_reader_snapshot` công bố:

- **Node tab** cho 8 tab (stats/encumbrance/speed/skills/traits/
  bionics/effects/proficiencies). `activate` một tab chỉ đổi
  `curtab`/`line`/`info_line` của chính màn — đúng việc phím LEFT/RIGHT
  làm trong `handle_player_display_action` — rồi trả "TIMEOUT" để vòng
  lặp vẽ lại. Duyệt màn này không khởi động điều trị hay hành động
  gameplay nào.
- **`pd:stats`**: bốn chỉ số hiện tại và chỉ số gốc khi khác nhau —
  đúng số màn stats in; cộng các mục chữ của cùng tab: cân nặng và lối
  sống bằng từ định tính của chính màn (`display::weight_string`,
  `display::health_string` — số BMI/lifestyle thô đứng sau các từ này
  không được màn in và không được đọc), chiều cao, tuổi, nhóm máu.
- **`pd:needs`**: đói/khát/buồn ngủ/đau bằng các hàm
  `display::*_text_color` (chữ game công bố), **nhiệt độ bằng từ định
  tính của `display::temp_text_color`** (từ "Freezing!"/"Warm"/... —
  màu đi kèm vốn chỉ lặp lại mức nghiêm trọng của chính từ đó),
  **morale bằng số tổng** — cùng số màn morale in ra khi liệt kê các
  nguồn morale (`player_morale::display`), thay cho mặt ASCII vốn chỉ
  là hình — và **weariness bằng từ của `display::weariness_text_color`**.
- **Bảng `pd:warmth`** (hợp đồng bảng R02): mỗi phần cơ thể một hàng
  với encumbrance **hai thành phần như tab in** (gốc + phạt lớp, từ
  `get_part_encumbrance`/`get_part_layer_penalty`) và nhiệt độ của
  chính phần đó. Điểm tinh tế đã xử lý: màu ấm của tab phân dải theo
  `get_part_temp_conv` (nhiệt hội tụ), trong khi
  `display::temp_text_color` phân dải theo `get_part_temp_cur` — hai
  giá trị có thể khác nhau, nên từ nhiệt trong bảng được suy từ
  `get_part_temp_conv` với **đúng các ngưỡng `BODYTEMP_*` và đúng bộ
  từ vựng** của `temp_text_color`, bảo đảm lời khớp với màu tab vẽ.
  Ô là trình bày; không có hành động theo ô.
- **`pd:effects`**: tài liệu dựng từ `effect_name_and_text` — chính
  các cặp tên/mô tả mà tab EFFECTS của màn hiển thị, gồm cả các mục
  màn tự tổng hợp: đau kèm mức phạt, suy dinh dưỡng, ánh nắng với
  Troglodyte, nghiện, và mô tả nguồn sửa đổi của từng effect. Đây là
  danh sách effect/thuốc đang tác dụng đầy đủ nhất mà game công bố,
  kèm mô tả — không suy từ dữ liệu effect thô.
- **`pd:speed`**: danh sách tốc độ của màn (`speedlist_entry`:
  mô tả + giá trị + cờ phần trăm) và move cost.

Chi tiết kỹ năng/bionics/traits/proficiencies chỉ có node tab trong
đợt này; nội dung chi tiết của chúng thuộc R17/R19 như kế hoạch.

### 1.1b Màn morale (`src/morale.cpp`, mới)

Màn con mở từ màn trạng thái (`player_morale::display`, context
"MORALE") trước đây cũng có 0 điểm a11y — mở nó là bộ đọc mất tiếng.
Nay có `screen_scope` "morale" công bố đúng những gì màn in:
`morale:sources` — từng nguồn morale theo hai nhóm dương/âm với
**tên + phần trăm đóng góp** (`get_name()`, `get_percent_contribution()`
— màn không in thời lượng của nguồn và adapter không suy ra), hai
tổng có dấu; `morale:summary` — tổng morale, hệ số "deadened", mức
phạt đau/buồn ngủ khi có, và xu hướng focus. Cuộn vẫn là UP/DOWN của
chính màn qua lệnh đã đăng ký. Lưu ý trung thực theo rà soát: không
có API công khai nào liệt kê nguồn morale kèm thời lượng (class
`morale_point` là private, trường duration là private) và màn gốc
không công bố dữ kiện đó, nên adapter không công bố nó.

### 1.2 Trạng thái trong snapshot thế giới (`src/a11y_world.cpp`)

`player_status()` (node `world:status`) được bổ sung, từ cùng nguồn
công bố như trên: nhiệt độ (từ định tính), morale (số tổng công bố),
weariness (từ), và **danh sách tên effect** qua
`Creature::get_effects()` + `effect::disp_name()` — đúng tên tab
EFFECTS hiển thị. Không thêm HP, cân nặng sống hay bất kỳ số nào các
màn gốc không in.

### 1.3 Y tế (`src/medical_ui.cpp`, giữ nguyên sau rà soát)

Adapter sẵn có đã đúng hợp đồng và được giữ: bảng chi R02 với điều
kiện theo chính sách tri thức y tế (`a11y::limb_health_description`:
cảm giác khi miễn dịch đau, đoạn thanh thô, **không HP chính xác,
không thời gian lành**), cờ BLEEDING/BITTEN/SEPTIC/WOUNDED bằng chữ,
chi tiết effect/vết thương/điểm chi lấy từ các hàm hiển thị của chính
màn, summary tốc độ/chỉ số, cân nặng chỉ khi đã từng cân (dữ kiện màn
công bố), và khi chọn điều trị: snapshot requirements
(`a11y::append_requirements`) kèm `a11y_value` cho biết điều trị làm
được hay không. `focus` một chi chỉ quan sát; `activate` mới vào menu
vết thương gốc với mọi kiểm tra kỹ năng/vật tư của engine.

## 2. R10.2 — Sinh hoạt và điều trị

Rà soát từng nhánh; tất cả đều chạy trên adapter dùng chung đã có,
không bộ nhập riêng:

- **Ăn/uống**: chọn món qua selector hành trang R09
  (`game_menus::inv::consume` — denial của preset `can_eat` đã loại
  món không ăn được ngay trong menu kèm lý do), `avatar_action::eat`
  gán `consume_activity_actor`. Cảnh báo nằm ở **đầu activity, sau
  khi đã chọn món và trước khi tiêu thụ**: `will_eat` gom mọi hậu quả
  (thối, thịt người, ký sinh trùng, buồn nôn, dị ứng, quá no, độc tố
  theo đột biến) thành **một** `query_yn` gộp — adapter popup dùng
  chung R04 phục vụ; từ chối là hủy thật ở engine. Hai giới hạn của
  chính engine, ghi rõ không che: **không có cảnh báo nghiện và
  không có cảnh báo hiệu quả/morale trước khi dùng** (enum
  `edible_rating` không có mã nghiện; nghiện chỉ áp lặng lẽ lúc tiêu
  thụ) — bộ đọc không bịa thêm cảnh báo engine không có. Kết quả sau
  khi ăn/uống do actor/`Character::consume` ghi log ("You eat/drink
  your…", vị thối, nôn khi quá đầy...) và được đọc qua kênh messages;
  hook `canceled` của consume actor trong engine chỉ dọn uistate,
  không có thông điệp — hủy giữa chừng vì thế chỉ được biết qua việc
  activity kết thúc và trạng thái sau đó, đúng như người sáng mắt.
- **Dùng thuốc/băng bó**: hai đường. (a) `eat_or_use` rẽ nhánh "heal"
  cho vật y tế → `Character::pick_wound_fix` (menu uilist R04) với
  requirements và điều kiện ở mục 1.3. (b) `heal_actor` (iuse) chọn
  bệnh nhân theo vị trí và chọn bộ phận qua
  `Character::body_window` — menu uilist mà mỗi mục đã có mô tả chữ
  đầy đủ (chảy máu/băng bó/khử trùng/vết cắn/nhiễm trùng và kỳ vọng
  hiệu quả); đợt này bổ sung `a11y_value` cho từng mục bằng
  `a11y::limb_health_description` để giá trị đọc lên theo chính sách
  tri thức y tế thay cho thanh HP dạng ký tự đồ họa trong nhãn mục.
  Vật tư trừ ở `firstaid_activity_actor::finish` theo logic gốc.
- **Chất lỏng**: menu đích `get_liquid_target`/`select_liquid_target`
  trong `src/handle_liquid.cpp` là uilist (R04); chọn vật chứa qua
  `game_menus::inv::container_for` với `liquid_inventory_selector_preset`
  — sức chứa/điều kiện đổ là denial của preset, chảy qua selector R09;
  đổ xuống đất/keg qua menu và chọn ô kề (đường chọn hướng dùng chung).
  **Không có bước chọn lượng trong engine**: `pour_into` rót
  min(sức chứa còn lại, lượng đang có) — người sáng mắt cũng không
  chọn lượng ở đường này, nên bộ đọc có đúng cùng lựa chọn, không
  thiếu. Nhánh rót từ mặt đất/xe chạy qua activity `ACT_FILL_LIQUID`
  (tiến độ qua cơ chế activity ở mục 3; engine rót vào container với
  `silent=true` từng lượt nên không có thông điệp rót từng phần —
  kết quả cuối theo log/trạng thái container).
  **Ngoại lệ ghi rõ**: nhánh đổ vào bình chứa của xe đi qua
  `veh_interact::select_part` (overview đồ họa, chưa có adapter) —
  thuộc phạm vi adapter xe R15, hành trình chất lỏng trong sổ đã ghi
  rõ ngoại lệ này, không tính là đã phủ.

## 3. R10.3 — Hoạt động dài

- **Tiến độ tiết chế**: `activity_snapshot` (src/a11y_world.cpp) thêm
  node `activity:progress`: phần trăm từ `moves_total`/`moves_left` —
  cùng cách tính engine dùng trong `player_activity::get_progress_message`
  cho hoạt động theo lượt đi — **làm tròn xuống theo bậc 10%** và nói
  "About …% complete". Vì runtime chỉ tăng revision khi nội dung
  snapshot đổi, tiến độ thay đổi tối đa 10 lần trong một hoạt động dài
  thay vì mỗi tick; đọc lại khi nào là việc của người dùng (node tài
  liệu, không tự phát).
- **Ngắt và hủy**: scope hoạt động ở `do_turn.cpp`: intent `cancel`
  của bộ đọc ánh xạ riêng thành "pause" chỉ khi
  `is_interruptible_with_kb()` hoặc đang có đích tự động, rồi engine
  chạy `game::cancel_activity_query` — **engine hỏi và engine hủy**.
  Đợt này bổ sung phản hồi kết quả thật: khi `cancel_activity_query`
  trả về true (đã hủy thật, kể cả trường hợp activity nối tiếp trong
  backlog được khôi phục và sẽ được snapshot tiếp theo mô tả),
  `handle_key_blocking_activity` công bố "The activity was canceled."
  qua kênh cảnh báo; từ chối ở hộp hỏi thì không công bố gì thêm —
  activity chạy tiếp với snapshot không đổi. **Lý do bị ngắt**: rà
  soát xác nhận engine **không lưu lý do ở đâu cả** —
  `Character::cancel_activity()` không có tham số lý do, sự kiện
  `canceled` chỉ mang một bool; văn bản phân tâm ("Heard …!",
  "… spotted!", quái gần, đau, mệt...) chỉ tồn tại trong
  `query_popup`/`query_yn` tại thời điểm quyết định và được adapter
  popup dùng chung công bố ở chính thời điểm đó — đó là kênh lý do
  duy nhất trung thực, bộ đọc không dựng trường lý do suy đoán cho
  snapshot. Tiết chế còn đến từ tầng runtime: `screen_scope` chỉ tăng
  revision khi nội dung snapshot đổi (`a11y_runtime.cpp`), cộng với
  lượng tử 10% ở trên. Tiếp tục là việc không gửi intent hủy; dừng
  nghe là thao tác của hàng lời phía Java, không chạm input của
  scope này.
- **Cảnh báo thời gian trôi**: giữ các cảnh báo sẵn có — văn bản
  menu chờ và hộp xác nhận ngủ nêu thời gian tiếp tục trôi (từ
  ad5ecf0, ở `handle_action.cpp` wait/sleep) và `activity_snapshot`
  cảnh báo realtime khi `TURN_DURATION > 0.005` (thế giới vẫn cập
  nhật trong lúc nghe màn hoạt động). Phần rà soát ghi nhận chưa
  phủ, để mở có chủ ý: nhánh đặt báo thức của `wait()` bị engine
  loại khỏi câu cảnh báo (`!setting_alarm`), và ngủ không qua
  `sleep()` (ngủ do effect/bionic) vốn là ngủ không tự nguyện —
  không có thời điểm "trước khi bắt đầu" để cảnh báo; node ngủ
  trong snapshot thế giới vẫn công bố trạng thái đang ngủ.

## 4. R10.4 — Cảnh báo ngưỡng (`src/a11y_world.cpp`)

Bộ theo dõi `announce_status_changes` sẵn có (đói/khát/buồn ngủ/đau/
sức bền theo mức chữ của `display`, chảy máu/gãy/vết cắn/nhiễm trùng
theo chi) được mở rộng cho các kênh **không hề có log game**:

- **Nhiệt độ, hai phía**: mức từ chính từ của `temp_text_color`
  (Chilly=1, Very cold!=2, Freezing!=3; Warm=1, Very hot!=2,
  Scorching!=3). Ngưỡng công bố: xấu đi tới mức 2, khẩn cấp mức 3;
  cải thiện công bố khi rời vùng ≥2.
- **Morale**: dải nguy cơ từ số tổng công bố (<0 =1, ≤−50 =2,
  ≤−100 =3); ngưỡng công bố mức 1, khẩn cấp mức 3.
- **Weariness**: mức `weariness_level()` (0–5); ngưỡng công bố mức 2
  ("Moderate" trở lên), không khẩn cấp.

**Chống lặp hai lớp**: (1) bản chất chuyển mức — cùng một mức không
bao giờ công bố hai lần nếu không rời rồi quay lại; (2) **cooldown
30 phút giờ game cho từng khóa scalar**: dao động quanh một ngưỡng
không lặp lại công bố, trừ khi mức mới tệ hơn mức đã công bố hoặc đạt
mức khẩn cấp của khóa. Cảnh báo theo chi (chảy máu/gãy/cắn/nhiễm
trùng) là sự kiện rời rạc, không qua cooldown nhưng được ghi lịch sử.

**Lịch sử trong phiên**: mọi cảnh báo đã công bố được ghi (tối đa 20,
kèm thời điểm lượt) và công bố lại dạng **đọc theo yêu cầu** qua node
`world:alert_history` ("Recent status alerts", tối đa 10 mục gần
nhất, kèm tuổi tương đối `to_string_approx`) — node tài liệu không tự
phát tiếng, nên không đọc lại cảnh báo cũ ngoài ý muốn.

**Reset baseline**: `reset_status_monitor()` nay xóa cả baseline,
cooldown và lịch sử; nó đã được gọi từ `a11y::replace_game_session()`
(game.cpp khi tạo phiên, game_io.cpp khi tải) và từ đồng bộ vòng đời
(epoch R08) — đổi nhân vật/tải lại không mang cảnh báo hay baseline
của nhân vật cũ sang.

**Không đọc kép với log**: cảnh báo của bộ theo dõi là câu chuyển
trạng thái tổng hợp ("Hunger is worsening: Starving!") — một dạng câu
riêng, không phải dòng log; các sự kiện engine tự ghi log (ví dụ sát
thương tê cóng) là dữ kiện khác (sát thương, không phải trạng thái
nhiệt) và đi kênh messages như cũ. Các kênh mới ở mục này (nhiệt,
morale, weariness) không tồn tại trong log nên không thể trùng.

## 5. Chỗ chưa chắc

- Chữ ký C++/API đã đối chiếu theo tiền lệ cùng cây (temp_text_color
  theo phần: `character_attire.cpp`; `get_effects`/`disp_name`:
  `medical_ui.cpp`, `creature.h`/`effect.h`; `to_string_approx`:
  `calendar.h`) nhưng chưa biên dịch — xác nhận thuộc giai đoạn 2.
- `pd:effects` dùng lại `effect_name_and_text` của màn nên thừa hưởng
  cả giới hạn của nó (effect không tên bị màn loại bỏ).
- Nhánh chọn bình chứa xe của chuỗi chất lỏng chờ adapter xe R15.
- Tiến độ hoạt động dạng phần trăm chỉ có khi `moves_total > 0`
  (hoạt động theo actor không có tổng lượt đi thì node tiến độ không
  xuất hiện; mô tả tiến độ riêng của actor vẫn nằm trong
  `activity:current` qua `get_progress_message`).
