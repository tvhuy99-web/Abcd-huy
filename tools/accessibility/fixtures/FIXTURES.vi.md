# Manifest fixture R22 — CDDA Android trợ năng

Trạng thái: **đã viết mã ở giai đoạn 1, chưa chạy.** Toàn bộ nội dung dưới đây
là bằng chứng nguồn: chưa biên dịch, chưa chạy test, chưa chạy giả lập hay
thiết bị. Mọi khẳng định về hành vi đều là "hàm dựng bảo đảm X theo mã nguồn
đã đối chiếu", không phải kết quả đo.

Mã fixture: `tests/a11y_fixtures.h` + `tests/a11y_fixtures.cpp`, namespace
`a11y_fixture`. Đăng ký build: **không cần sửa danh sách nào** —
`tests/CMakeLists.txt` nạp test bằng `file(GLOB tests/*.cpp)` và
`tests/Makefile` dùng `SOURCES = $(wildcard *.cpp)`, nên tệp mới tự vào
cùng bản build `cata_test` với `player_helpers.cpp`/`map_helpers_tests.cpp`.
(ID fixture là cố định: F-01…F-09, F-15, và F-10…F-14 cho minigame.)

Nguyên tắc chung:

- Chỉ dựng từ dữ liệu trong repo (core + mod TEST_DATA mà harness luôn nạp)
  qua helper test thật; **không dùng save cá nhân**.
- Mọi builder bắt đầu bằng `reset_world_for_fixture()`: `clear_avatar()`,
  `clear_map_without_vision()`, `clear_creatures()`, `clear_npcs()`,
  `clear_vehicles()`, `clear_zones()` (từ `src/map_helpers.h`),
  `set_time_to_day()` (từ `tests/map_helpers_tests.h`), avatar về ô cố định
  (60, 60, 0), `avatar::clear_map_memory()`.
- Builder `REQUIRE` đúng các bất biến mà oracle của nó dựa vào, theo phong
  cách `spawn_test_monster()`/`spawn_npc()`.

## F-01 — Cặp tri thức: biết giống nhau, bí mật khác nhau

- **Mục đích:** oracle ranh giới tri thức — đầu ra của bộ đọc là hàm của
  điều nhân vật biết, không phải của sự thật dưới đất.
- **Cách dựng:** `build_knowledge_pair(knowledge_variant::a|b)` trong
  `tests/a11y_fixtures.cpp`. Nền cỏ phẳng bằng `build_test_map(ter_id("t_grass"))`
  (`tests/map_helpers_tests.cpp`); cột tường `t_brick_wall` tại x = 63;
  phía biết giống hệt nhau được ghi thẳng vào bộ nhớ avatar bằng
  `avatar::memorize_terrain()`/`memorize_decoration()` (API thật, có dùng
  trong `tests/tiles_memory_sweep_test.cpp`) sau `clear_map_memory()`:
  hình chữ nhật x 55–66, y 55–65 là `t_grass`, cột tường là `t_brick_wall`,
  ô crate có decoration `f_crate_c`.
- **Điều kiện xác định:** không seed; mọi tọa độ/id là hằng số. Bí mật chỉ
  khác ở phía đông tường (ngoài tầm nhìn): quái `mon_zombie` tại (65, 58)
  ở bản A và (65, 62) ở bản B (`spawn_test_monster()`); bẫy ẩn
  `tr_beartrap_buried` tại (64, 55)/(64, 65) (`map::trap_set()`); mìn
  `tr_landmine_buried` tại (66, 65)/(66, 55); crate đóng `f_crate_c`
  (furniture có cờ CONTAINER + SEALED, `map::furn_set()`) tại (65, 60) ở cả
  hai bản nhưng đồ bên trong khác nhau: `knife_combat` (A) / `bandages` (B),
  đặt bằng `map::add_item_or_charges()` lên ô crate —
  `a11y::observe_tile()` trong `src/a11y_perception.cpp` chính là nơi chặn
  đồ trong container như vậy.
- **Oracle phục vụ:** snapshot/quét của bộ đọc cho bản A và bản B phải bằng
  nhau từng byte, trong khi kiểm tra sự thật dưới đất (live map) phải thấy
  đủ bốn khác biệt trên.
- **Bài dùng:** T21, T22, T24, T25, T30.

## F-02 — Cây túi

- **Mục đích:** oracle cây container: độ sâu, cha–con, pocket niêm phong.
- **Cách dựng:** `build_pocket_tree()`. `debug_backpack` (mặc) chứa
  `backpack` chứa `test_can_drink` và `knife_combat` rời; can chứa `water`
  rồi niêm phong. Dùng `item::put_in(..., pocket_type::CONTAINER)`,
  `item::get_container_pockets()` và `item_pocket::seal()` — đúng cơ chế
  ca "sealed_containers" của `tests/item_pocket_test.cpp` với cùng loại can
  thử `test_can_drink` (TEST_DATA).
- **Điều kiện xác định:** không RNG; thứ tự dựng trong–ngoài cố định;
  builder `REQUIRE` seal thành công và can còn trong người.
- **Oracle phục vụ:** cây túi có độ sâu 3 dưới avatar; pocket đã seal không
  lộ nội dung như pocket mở; đồ ở các độ sâu khác nhau không bị trộn.
- **Bài dùng:** T31, T32, T34.

## F-03 — Kho hai phía (advanced inventory)

- **Mục đích:** oracle phân biệt nguồn kho: mặt đất / khoang xe / trên người.
- **Cách dựng:** `build_two_sided_inventory()`. Đồ mặt đất `knife_combat`
  tại ô của avatar (`map::add_item_or_charges()`, cách
  `tests/advanced_inventory_test.cpp` đặt đồ cho AIM_CENTER); xe
  `test_shopping_cart` tại ô kề đông (`map::add_vehicle()`), đồ cargo
  `jeans` qua `veh_at().cargo()` + `vehicle::add_item()` (cách
  `tests/clzones_test.cpp` lấy cargo của cùng prototype); trên người:
  `bandages` mang trực tiếp (`Character::i_add()`) và `test_rock` trong
  pocket của `debug_backpack` đang mặc (`item::put_in()` trước khi mặc).
- **Điều kiện xác định:** tọa độ/id cố định, không RNG.
- **Oracle phục vụ:** mỗi vật thuộc đúng một nguồn; chuyển vật giữa hai phía
  không đổi danh tính vật ở phía còn lại.
- **Bài dùng:** T31, T32, T34.

## F-04 — Vết thương băng bó được

- **Mục đích:** oracle y tế: chi bị thương xác định + vật tư có sẵn.
- **Cách dựng:** `build_wound_fixture()`. Tay phải `arm_r` được đặt HP bằng
  đúng 3/5 HP tối đa qua `Character::set_part_hp_cur()` — cơ chế của
  `tests/a11y_body_test.cpp`; **không** dùng `apply_damage()` nên không có
  tung xúc xắc vết thương. Kho có `bandages` và `disinfectant`
  (`Character::i_add()`); luồng băng bó thật nằm ở
  `tests/firstaid_test.cpp`.
- **Điều kiện xác định:** tỉ lệ 3/5 cố định theo `get_part_hp_max()` tại
  lúc dựng; builder `REQUIRE` chi chưa gãy và HP sau đặt khớp.
- **Oracle phục vụ:** mô tả y tế theo dải thô (không lộ số HP chính xác —
  bất biến của `a11y::limb_health_description()`), chọn đúng chi và đúng
  vật tư khi điều trị.
- **Bài dùng:** T36.

## F-05 — Trường chiến đấu

- **Mục đích:** oracle mục tiêu/tầm nhìn/AOE trong chiến đấu.
- **Cách dựng:** `build_combat_field()`. Đồng cỏ `build_test_map(t_grass)`,
  ban ngày; nạp submap tới z = +1 bằng `clear_map(-2, 2)` như
  `tests/monster_stair_test.cpp`. Mục tiêu nhìn thấy: `mon_zombie` tại
  (63, 60, 0) bằng `spawn_test_monster()`; ô trống tại (61, 60, 0) (builder
  `REQUIRE` ô không có đồ); đồng minh đã biết: NPC lớp `test_talker` tại
  (59, 60, 0) bằng `spawn_npc()` + `npc::set_attitude(NPCATT_FOLLOW)`
  (mẫu follower của `tests/npc_test.cpp`); mục tiêu khác tầng: `mon_zombie`
  thứ hai tại (66, 60, +1) trên ô `t_floor` dựng tường minh theo tầng như
  `build_tower()` của monster_stair_test; phép AOE đã biết:
  `test_spell_lava` học bằng `known_magic::learn_spell(..., force = true)`
  như `tests/magic_spell_test.cpp` — trong `data/mods/TEST_DATA/magic.json`
  phép này có `min_aoe` 4 ở cấp 0, và builder `REQUIRE`
  `spell::aoe(avatar) > 0` qua `known_magic::get_spell()`.
- **Điều kiện xác định:** tọa độ cố định; học phép ép buộc (force) không
  tung RNG cho việc học.
- **Oracle phục vụ:** chỉ mục tiêu nhìn thấy/đã biết mới được nêu; ô trống
  phải trống; mục tiêu khác tầng không bị gộp vào tầng hiện tại; vùng AOE
  công bố khớp `aoe()` của phép đã biết.
- **Bài dùng:** T39, T40, T41, T42, T52.

## F-06 — Xe

- **Mục đích:** oracle sơ đồ xe/cargo/nhiên liệu.
- **Cách dựng:** `build_vehicle_fixture()`. Prototype `car` bằng
  `map::add_vehicle(proto, (62, 60, 0), 0_degrees, 50, UNDAMAGED, false)` —
  tham số nhiên liệu khởi tạo 50 (phần trăm, theo chữ ký `add_vehicle()`
  trong `src/map.h`). Bộ phận hỏng duy nhất: engine `vehicle::engines[0]`
  (cách `tests/vehicle_test.cpp` lấy bàn đạp xe đạp) đặt 0 HP bằng
  `vehicle::set_hp(part, 0, true)`, builder `REQUIRE` `is_broken()`. Cargo:
  quét theo thứ tự cố định tìm `veh_at().cargo()` đầu tiên (cách của
  clzones/vehicle test) rồi `vehicle::add_item()` hai món cố định `jeans`
  và `test_rock`.
- **Điều kiện xác định:** prototype/hướng/nhiên liệu/tọa độ là tham số cố
  định. **Giới hạn ghi rõ:** lượng nhiên liệu tuyệt đối sau spawn do engine
  suy từ phần trăm; oracle phải đọc qua `vehicle::fuel_left(map, gasoline)`,
  không giả định số lít — cái xác định là tham số 50 và `fuel_left > 0`.
- **Oracle phục vụ:** đúng một bộ phận hỏng và đúng tên của nó; cargo thuộc
  đúng bộ phận cargo; nhiên liệu công bố khớp `fuel_left()` thật.
- **Bài dùng:** T49, T50.

## F-07 — Vùng và quy tắc

- **Mục đích:** oracle quản lý vùng + bảng quy tắc safemode/auto-pickup.
- **Cách dựng:** `build_zones_and_rules()`. `zone_manager::clear()` rồi
  `zone_manager::add()` đúng dạng đối số của `create_tile_zone()` trong
  `tests/clzones_test.cpp` (faction `your_fac` từ `src/clzones.h`): vùng
  `LOOT_FOOD` tên "A11y Loot Food" phủ 2x2 tại góc (58, 58)–(59, 59) tuyệt
  đối-tương ứng, và vùng `LOOT_UNSORTED` tên "A11y Unsorted" một ô tại
  (62, 62). Quy tắc trong kho quy tắc gốc (character rules của đối tượng
  game sống, không ghi tệp): `safemode::clear_character_rules()` rồi
  `safemode::add_rule("zombie", HOSTILE, 5, BLACKLISTED)` — entry point
  mà `src/handle_action.cpp` dùng; `auto_pickup::player_settings::clear_character_rules()`
  rồi `add_rule(item test_rock, include = true)` — entry point của
  `tests/item_autopickup_test.cpp`. Builder `REQUIRE` truy vấn lại được cả
  hai (`zone_manager::has()`, `safemode::has_rule()`,
  `player_settings::has_rule()`).
- **Điều kiện xác định:** tên/loại/tọa độ/quy tắc là hằng số.
- **Oracle phục vụ:** vùng và quy tắc liệt kê lại đúng như đã dựng; thứ tự
  và trạng thái bật/tắt không tự thay đổi.
- **Bài dùng:** T51.

## F-08 — Trại phe

- **Mục đích:** oracle trại: cư dân, việc được giao, kho lương thực.
- **Cách dựng:** `build_faction_camp()`, chép mẫu của
  `tests/faction_camp_test.cpp` (`place_test_camp()` và ca
  `camp_calorie_counting`): `map::add_camp()` tại giữa bản đồ,
  `overmap_buffer.find_camp()`, `set_owner(your_fac)`,
  `define_camp(..., "faction_base_bare_bones_NPC_camp_0", false)`, vùng
  `CAMP_FOOD`/`CAMP_STORAGE` bằng `mapgen_place_zone()` (`src/clzones.h`).
  Cư dân: NPC `test_talker` tại ô kề trại, `NPCATT_FOLLOW`
  (`spawn_npc()`). Việc được giao: `TEST_MISSION_FIND_ITEM` bằng
  `mission::reserve_new()` + `mission::assign()` — mẫu
  `assign_test_find_item_mission()` của `tests/mission_test.cpp`; nhiệm vụ
  companion/upgrade riêng của trại vẫn liệt kê được qua
  `basecamp::get_available_missions_by_dir()`. Lương thực xác định: làm
  rỗng kho (`faction::empty_food_supply()`) rồi
  `add_to_food_supply()` 100000 calories tại `calendar::turn_zero` —
  faction_camp_test chứng minh bằng mã nguồn rằng mức này đọc lại đúng
  **100 kcal**; builder `REQUIRE` `food_supply().kcal() == 100`.
  Dọn dẹp bằng `clear_faction_camp()` (gọi `overmap_buffer.clear_camps()`
  + `empty_food_supply()`, như cleanup của faction_camp_test).
- **Điều kiện xác định:** loại trại, tọa độ, mức kcal là hằng số.
- **Oracle phục vụ:** danh sách cư dân/việc/lương thực công bố khớp trạng
  thái trại thật; số kcal là số nguyên chính xác, không phải ước lượng.
- **Bài dùng:** T48.

## F-09 — Khả năng: bionic, phép, máy tính

- **Mục đích:** oracle trang khả năng và tương tác máy tính.
- **Cách dựng:** `build_abilities()`. Bionic đã lắp: `bio_power_storage`
  bằng `Character::add_bionic()` (cơ chế `tests/bionics_test.cpp`). CBM
  chưa lắp: vật phẩm CBM `bio_power_storage` trong kho (`Character::i_add()`)
  — `data/json/items/bionics.json` dùng chính id của bionic cho loại vật
  phẩm CBM của nó. Phép đã biết: `test_spell_pew` bằng
  `known_magic::learn_spell(..., force = true)`. Máy tính thật trên map:
  `map::add_computer()` tại ô kề đông (61, 60, 0), security 0, thêm một
  tùy chọn bằng `computer::add_option("Fixture geiger counter", COMPACT_GEIGER, 0)`;
  builder `REQUIRE` `map::computer_at()` trả lại đúng con trỏ đó và đúng
  một tùy chọn.
- **Giới hạn ghi rõ (sách phép):** không có vật phẩm nào trong core data
  hay TEST_DATA dạy phép (đã lục `data/json/items/book/` và
  `data/mods/TEST_DATA/books.json` — chỉ có sách kỹ năng; `DEBUG_spellbook`
  thuộc mod Magiclysm, harness test mặc định không nạp — xem
  `tests/test_main.cpp`: mod mặc định là `dda` + TEST_DATA). Vì vậy "sách
  phép" ở fixture này là **vật thế thân có khai báo**: một vật phẩm BOOK
  thật (`test_textbook_fabrication`, TEST_DATA) mang biến vật phẩm
  `spell_id = "test_spell_lava"` (qua `item::set_var()`, kiểm lại bằng
  `character_has_item_with_var_val()` của player_helpers). Oracle không
  được coi biến này là dữ liệu spellbook của engine; nếu R17 cần học phép
  từ vật phẩm thật, đó là việc phải bổ sung dữ liệu test ở giai đoạn sau,
  không bịa API ở đây.
- **Điều kiện xác định:** id cố định, học phép ép buộc.
- **Oracle phục vụ:** phân biệt đã lắp/chưa lắp, đã biết/chưa biết; tùy
  chọn máy tính công bố đúng danh sách `computer::options` thật.
- **Bài dùng:** T52.

## F-15 — Vòng đời: hai nhân vật / hai world

- **Phần dựng được bằng C++:** `build_lifecycle_stage(1|2)` — nửa "cách ly
  tri thức". Mỗi stage tự xóa sạch trước khi dựng: `clear_avatar()` +
  `avatar::clear_map_memory()` (lời gọi `game.cpp` thực hiện khi thay
  phiên). Stage 1: ghi nhớ ô đánh dấu `t_floor` tại (62, 60, 0) + học
  `test_spell_pew`. Stage 2: ghi nhớ ô đánh dấu khác `t_brick_wall` tại
  (64, 60, 0) + học `test_spell_lava`. Builder `REQUIRE` tri thức của
  stage kia (ô/phép) không có mặt.
- **Phần KHÔNG dựng được bằng helper — đặc tả kịch bản thiết bị thay thế:**
  một tiến trình test chỉ có đúng một avatar và một world, nên "hai world /
  hai bản lưu" thật không tồn tại trong harness Catch2. Kịch bản thiết bị
  cho T28/T53 (thực hiện ở giai đoạn nghiệm thu, không phải mã C++):
  1. Tạo world A, nhân vật A; chơi tới khi A ghi nhớ ô đánh dấu của stage 1
     và học phép của stage 1; lưu và thoát ra menu.
  2. Tạo world B (hoặc nhân vật B trong world khác) với ô đánh dấu và phép
     của stage 2; lưu.
  3. Tải lại A: bộ đọc chỉ được nêu tri thức stage 1; tải lại B: chỉ nêu
     tri thức stage 2; không tên world/nhân vật, không ô, không phép nào
     được rò qua lại, kể cả sau process death/relaunch giữa hai lần tải.
- **Oracle phục vụ:** tri thức không sống qua việc thay nhân vật/world.
- **Bài dùng:** T28, T53.

## Minigame F-10…F-14 — đặc tả bàn xác định (TX-minigame-*)

Cả năm game giữ trạng thái trong biến cục bộ của `start_game()` hoặc thành
viên private sau một vòng lặp UI chặn (cần cửa sổ curses + input), nên
**không game nào khởi tạo được trong harness Catch2**. Fixture cho minigame
là hàm đặc tả trong `a11y_fixtures.h/.cpp`, suy từ mã nguồn engine; từng
game ghi rõ cái gì suy được và cái gì không.

### F-10 Kitten (robot_finds_kitten) — TX-minigame-kitten

- **Đặc tả:** `kitten_spec_for_seed(seed = 2203)`. Hằng số từ
  `src/iuse_software_kitten.h`: bàn 60 cột x 20 hàng (`rfkCOLS`/`rfkLINES`),
  20 vật giả (`numbogus`) + 1 kitten = 21 vật.
- **Giới hạn:** vị trí vật **không** suy được từ seed — constructor của game
  vừa đặt vật bằng `rng()` vừa xen `all_colors.get_random()` và
  `SNIPPET.random_from_category()`, và constructor tự chạy vòng lặp UI.
  Đặc tả chỉ chốt hình học + số vật + seed mà kịch bản thiết bị phải đặt
  trước khi mở game; `positions_derivable = false` trong struct.
- **Oracle phục vụ:** bộ đọc không bao giờ được nêu vật nào là kitten
  (bất biến ranh giới tri thức của adapter trong
  `src/iuse_software_kitten.cpp`); số vật công bố là 21.

### F-11 Minesweeper — TX-minigame-minesweeper

- **Đặc tả:** `minesweeper_beginner_spec(seed = 2201)` **suy được toàn bộ
  bàn Beginner**: `minesweeper_game::new_level()` (`src/iuse_software_minesweeper.cpp`)
  cố định Beginner là 8x8, 10 mìn, và đặt mìn bằng vòng lặp thuần RNG
  `{ x = rng(0, 7); y = rng(0, 7) }` tới khi trúng ô chưa có mìn. Hàm đặc
  tả phát lại đúng trình tự đó sau `rng_set_engine_seed(seed)` (cơ chế seed
  của `tests/test_main.cpp`/`src/rng.h`, có dùng trong
  `tests/creature_test.cpp`) và trả danh sách vị trí mìn theo thứ tự đặt.
- **Giới hạn:** `new_level()` là private và tương tác (uilist + nhập số)
  nên không gọi trực tiếp được; đặc tả là bản sao suy diễn của đoạn đặt
  mìn — nếu đoạn mã đó thay đổi, đặc tả phải suy lại. Điều kiện đúng của
  bàn suy được: game mở với cùng seed và không có consumer RNG nào chen
  vào trước khi đặt mìn.
- **Oracle phục vụ:** ô chưa mở không bao giờ bị nêu là mìn/an toàn; số
  mìn tổng (10) là thông số công khai của độ khó.

### F-12 Sokoban — TX-minigame-sokoban

- **Đặc tả:** `sokoban_level_one_spec()` — bàn **cố định hoàn toàn, không
  seed**: các hàng chép nguyên văn từ tầng 1 của `data/raw/sokoban.txt`
  (tệp mà `sokoban_game::parse_level()` đọc qua `PATH_INFO::sokoban()`);
  kích thước 9x9, người chơi (1, 7), kiện (2, 5), (2, 6), (3, 6), đích
  (7, 3), (7, 4), (7, 5) — đọc theo chú giải ký tự `# $ . * @ +` trong
  `src/iuse_software_sokoban.cpp` (tọa độ x = cột, y = hàng, gốc trái-trên).
- **Giới hạn:** `parse_level()`/`mLevel` là private và `start_game()` cần
  UI, nên đặc tả là dữ kiện của tệp dữ liệu, không phải trạng thái game
  đang chạy.
- **Oracle phục vụ:** toàn bộ bàn là tri thức công khai; bộ đọc nêu đúng
  vị trí người chơi/kiện/đích của tầng 1.

### F-13 Lights On — TX-minigame-lightson

- **Đặc tả:** `lightson_spec_for_seed(seed = 2202)` **suy được toàn bộ
  bàn**: `lightson_game::new_level()` + `generate_change_coords()` +
  `reset_level()` (`src/iuse_software_lightson.cpp`) chỉ tiêu thụ
  `rng()`/`rng_float()` theo thứ tự cố định: `rng(8, 11)` (nửa chu vi),
  `rng(4, 6)` (rộng), cao = nửa chu vi − rộng, số bước =
  `int(nửa chu vi / 2.0 + rng_float(0.0, 2.0))`, rồi rút chỉ số ô phân
  biệt bằng `rng(0, rộng*cao − 1)`; bàn bắt đầu toàn sáng và mỗi tọa độ
  đổi trạng thái tự nó + 4 ô kề trực giao. Hàm đặc tả phát lại đúng thứ tự
  đó và trả kích thước, các tọa độ đổi, và danh sách ô tối cuối cùng.
- **Giới hạn:** `new_level()` là private, `start_game()` cần UI — đặc tả
  là bản sao suy diễn, phải suy lại nếu thứ tự gọi RNG trong engine đổi.
- **Oracle phục vụ:** trạng thái sáng/tối công bố khớp bàn suy được; ô con
  trỏ và hiệu ứng lân cận khi bấm khớp quy tắc đổi 5 ô.

### F-14 Snake — TX-minigame-snake

- **Đặc tả:** `snake_spec_for_board(width, height, seed = 2204)`. Từ
  `snake_game::start_game()` (`src/iuse_software_snake.cpp`): đầu rắn tại
  (rộng/2, cao/2), hướng ban đầu sang đông, độ dài đích 10, và quả đầu
  tiên đặt bằng vòng lặp `{ y = rng(1, cao − 2); x = rng(1, rộng − 2) }`
  tới khi không trùng thân (thân ban đầu chỉ có ô đầu). Hàm đặc tả phát
  lại vòng rút quả đó sau `rng_set_engine_seed(seed)`.
- **Giới hạn:** kích thước bàn lấy từ terminal (`FULL_SCREEN_WIDTH/HEIGHT`)
  nên **không có bàn theo seed thuần túy** — kịch bản thiết bị phải ghi lại
  kích thước bàn thực tế đã chạy và đối chiếu với đặc tả theo kích thước
  đó. Trạng thái rắn là biến cục bộ của `start_game()`, không truy cập
  được từ test.
- **Oracle phục vụ:** vị trí quả tương đối so với đầu và quy tắc điểm/tăng
  trưởng (+500 điểm, +10 đoạn mỗi quả, theo văn bản luật adapter trong
  chính `iuse_software_snake.cpp`) không bị bộ đọc làm sai lệch.

## Bảng tra nhanh

| Fixture | Hàm dựng / đặc tả | Xác định bằng | Bài |
|---|---|---|---|
| F-01 | `build_knowledge_pair(a\|b)` | tọa độ/id hằng số | T21, T22, T24, T25, T30 |
| F-02 | `build_pocket_tree()` | thứ tự put_in/seal cố định | T31, T32, T34 |
| F-03 | `build_two_sided_inventory()` | tọa độ/id hằng số | T31, T32, T34 |
| F-04 | `build_wound_fixture()` | HP đặt trực tiếp 3/5 | T36 |
| F-05 | `build_combat_field()` | tọa độ cố định, learn force | T39–T42, T52 |
| F-06 | `build_vehicle_fixture()` | add_vehicle fuel = 50, set_hp 0 | T49, T50 |
| F-07 | `build_zones_and_rules()` | tên/tọa độ/quy tắc hằng số | T51 |
| F-08 | `build_faction_camp()` | define_camp + 100 kcal chính xác | T48 |
| F-09 | `build_abilities()` | id cố định, add_computer thật | T52 |
| F-15 | `build_lifecycle_stage(1\|2)` + kịch bản thiết bị ở trên | stage hằng số | T28, T53 |
| F-10 | `kitten_spec_for_seed()` | hình học + seed (vị trí: không suy được) | TX-minigame-kitten |
| F-11 | `minesweeper_beginner_spec()` | seed 2201, phát lại vòng đặt mìn | TX-minigame-minesweeper |
| F-12 | `sokoban_level_one_spec()` | dữ liệu tệp cố định | TX-minigame-sokoban |
| F-13 | `lightson_spec_for_seed()` | seed 2202, phát lại thứ tự RNG | TX-minigame-lightson |
| F-14 | `snake_spec_for_board(w, h)` | kích thước khai báo + seed 2204 | TX-minigame-snake |
