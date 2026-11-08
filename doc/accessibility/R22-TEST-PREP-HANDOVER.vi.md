# R22 — Chuẩn bị fixture, bài kiểm thử và tài liệu bàn giao

Trạng thái của toàn bộ mục này: **đã viết, chưa chạy**. Không một bài nào
trong đợt này đã được biên dịch, chạy test, chạy giả lập hay tạo APK.
"Viết bài không đồng nghĩa đã chạy" — mọi trạng thái PASS thuộc về giai
đoạn 2, sau khi R23 ghim ứng viên.

## R22.1 — Fixture/scenario (F-01…F-15)

Nơi viết: `tests/a11y_fixtures.h` + `tests/a11y_fixtures.cpp` (builder C++
trong namespace `a11y_fixture`, dùng đúng helper của repo:
`clear_avatar`/`build_test_map`/`spawn_test_monster`/`spawn_npc`/
`map::add_vehicle`/`map::add_computer`, mẫu của `faction_camp_test` và
`item_pocket_test`), và manifest `tools/accessibility/fixtures/FIXTURES.vi.md`.
Không cần sửa danh sách build: `tests/CMakeLists.txt` glob và
`tests/Makefile` wildcard tự nạp tệp `.cpp` mới. Không fixture nào dùng
save cá nhân; mọi thứ dựng từ dữ liệu trong repo (core + TEST_DATA).

| Fixture | Dạng | Bảo đảm chính |
|---|---|---|
| F-01 cặp tri thức | builder 2 biến thể | phần biết giống hệt (ghi nhớ tường minh), bí mật khác nhau sau tường: vị trí quái/bẫy ẩn/mìn, đồ trong crate đóng kín |
| F-02 cây túi | builder | container lồng 3 tầng, một pocket bị niêm phong chứa nước, đồ rời ở tầng khác |
| F-03 kho hai phía | builder | đồ mặt đất + cargo xe + đồ trên người cho kho nâng cao |
| F-04 vết thương | builder | tay phải đúng 3/5 HP bằng gán trực tiếp (không RNG), có băng gạc + thuốc sát trùng |
| F-05 trường chiến đấu | builder | mục tiêu thấy được, ô trống, đồng minh FOLLOW, mục tiêu tầng +1, phép AOE `test_spell_lava` đã học |
| F-06 xe | builder | xe "car" nhiên liệu 50%, đúng một bộ phận hỏng (động cơ 0 HP), cargo có đồ |
| F-07 vùng/quy tắc | builder | 2 vùng loot + 1 quy tắc safemode + 1 quy tắc auto-pickup trong đúng kho gốc |
| F-08 trại | builder | trại bare-bones, 1 cư dân FOLLOW, việc đã giao, lương thực đúng 100 kcal |
| F-09 khả năng | builder | bionic đã lắp, CBM chưa lắp, phép đã biết, máy tính thật trên map có tùy chọn |
| F-10 kitten | đặc tả | hình học cố định 60×20, 21 vật; vị trí KHÔNG suy được từ seed (constructor xen RNG với snippet/màu và tự chạy UI) — cờ `positions_derivable = false` |
| F-11 minesweeper | đặc tả suy được | bàn Beginner 8×8/10 mìn phát lại đúng trình tự `rng()` của `new_level()` sau `rng_set_engine_seed` |
| F-12 sokoban | đặc tả | tầng 1 chép nguyên văn `data/raw/sokoban.txt` kèm tọa độ người/kiện/đích |
| F-13 lightson | đặc tả suy được | phát lại đúng thứ tự RNG của `new_level()`/`reset_level()`, cho cả ô bị tắt |
| F-14 snake | đặc tả | đầu/hướng/độ dài cố định từ nguồn; quả đầu tiên suy theo kích thước bàn do kịch bản khai báo (bàn phụ thuộc terminal) |
| F-15 vòng đời | builder một nửa + đặc tả | builder kiểm cô lập tri thức trong tiến trình (hai stage không chia sẻ phép/ô nhớ); nửa "hai world/hai save" là kịch bản thiết bị trong manifest vì một tiến trình Catch2 chỉ có một avatar/world |

Giới hạn đã ghi thẳng trong manifest: F-09 "sách phép" là vật thế thân
(sách thật mang biến `spell_id`) vì không có item core/TEST_DATA nào dạy
phép — oracle không được coi đó là dữ liệu spellbook của engine; đặc tả
F-11/F-13 phải suy lại nếu engine đổi thứ tự gọi RNG.

Sổ hành trình: 40/115 hành trình đã được gán trường `fixtures` trong
`JOURNEY-LEDGER.json` theo ánh xạ test → fixture của manifest (xác định,
truy được vết); các hành trình còn lại dùng bản cài sạch/mặc định theo
quy ước của TESTING.vi.md.

## R22.2 — Bài kiểm thử đã viết thành mã (host, chưa chạy)

| Tệp | Nội dung | Oracle |
|---|---|---|
| `tests/a11y_schema_test.cpp` | hợp đồng R02: snapshot hợp lệ tối thiểu; trùng ID (`duplicate_node_id`); vượt trần node (`too_many_nodes`) — bị từ chối nguyên vẹn, không cắt thầm; cây (thiếu cha, chu trình, con của nút đang gập); bảng (bảng/cột chưa khai báo); miền giá trị intent (`value_out_of_range`, `value_step_mismatch`, `not_a_number`, `not_a_bool`, `not_a_choice`, `not_a_field`; action không mang giá trị thì không kiểm miền) | phán quyết + mã lý do ổn định của `validate_snapshot`/`validate_intent_value` trong `src/a11y_semantics.cpp` |
| `tests/a11y_unicode_test.cpp` | tiếng Việt tổ hợp sẵn, tiếng Việt tách dấu (combining U+0302/U+0303), ký tự ngoài BMP qua `snapshot_json` | byte UTF-8 gốc xuất hiện nguyên vẹn trong JSON; chỉ ký tự điều khiển bị escape. Caret grapheme là phía Java (ICU) — thuộc bài thiết bị T17, không giả làm host test |
| `tests/a11y_knowledge_test.cpp` | F-01: mô tả công bố cho ô quái/bẫy/mìn/crate giống hệt giữa hai biến thể trong khi sự thật nền khác nhau; thuần khiết truy vấn: 100 lần quan sát/mô tả không đổi `calendar::turn`, vị trí avatar, và không tiêu thụ RNG (so dòng RNG từ cùng seed); F-15: đổi stage thì tri thức stage trước biến mất | trạng thái game quan sát được, không assertion chuỗi nguồn |

Hai tệp có từ trước giữ nguyên: `tests/a11y_context_test.cpp` (token/
ngữ cảnh R03) và `tests/a11y_body_test.cpp` (không lộ HP). Bài phía Java
đã có ở `tools/accessibility/java/` (AndroidSpeech/SpeechSession) và
`android/app/src/androidTest/` — đợt này không viết thêm vì phần còn
thiếu là bài thiết bị trong danh mục, không phải mã host.

## R22.3 — Danh mục đầy đủ trong TESTING.vi.md

`doc/accessibility/TESTING.vi.md` đã viết lại thành danh mục giai đoạn 2:

- T01–T64: giữ nguyên mã/tên/tóm tắt từ PLAN.vi.md mục 15.4, bổ sung cho
  từng bài tiền điều kiện, fixture (F-ID), bước, oracle (trạng thái game/
  hành vi) và loại môi trường (host-C++ / host-Java / Android-giả-lập /
  máy-thật-TalkBack / nghiệm-thu-người-dùng).
- 32 bài bổ sung TX-01…TX-32: form/bảng/cây R02 (gồm trần 4096 node và
  bảng Study Zone vượt 16 cột), engine/voice/âm cứu hộ, cue có tương
  đương lời và không rò thông tin, Smart controller, Study Zone, hai
  bảng quy tắc, máy tính trong game, **mỗi minigame một bài chơi trọn
  ván**, **TX-20 riêng cho Snake chính sách theo lượt** (bộ đọc bật =
  theo lượt, luật gốc không đổi; bộ đọc tắt = realtime như gốc; kèm
  điều khoản mở lại R18.4 nếu nghiệm thu bác bỏ), **TX-21 riêng cho
  điểm đứt `cast_spell` ở cây nền** (R17 mục 3.4: kỳ vọng FAIL/BLOCKED
  có lý do ở ứng viên chưa khôi phục glue — đây là điều kiện để T52
  chạy đầu-cuối, không phải bài trang trí), trại (giao việc + quét
  kho/biến đổi), backup/khôi phục trọn vòng, vòng đời (process death +
  nâng cấp giữ save), lịch sử thông báo không đọc kép, trung tâm lệnh
  đầy đủ không cần tài nguyên hình, lịch sử cảnh báo ngưỡng, kỹ
  năng/proficiency, công thức đã học, phẫu thuật bionics (oracle "không
  bịa % rủi ro"), và phạm vi debug/editmap/wish (nhãn giới hạn + đường
  thoát).
- Bảng ánh xạ TX số ↔ tên TX trong sổ ↔ hành trình: cả 10 mã
  `test_bo_sung` hiện có trong sổ đều có bài; `TX-phau-thuat` được ghi
  rõ là tên nêu trong R17, chưa có trong sổ JSON.
- Mục điều kiện bắt đầu giai đoạn 2: ứng viên R23 ghim một commit duy
  nhất, fixture thiếu = bài BLOCKED, thứ tự 5 vòng host → Java → giả
  lập → máy thật → nghiệm thu người dùng; test audio/TalkBack thật/
  người dùng không thay bằng test doubles.
- Nội dung cũ về màn thử giọng/APK thử giọng giữ **nguyên văn** ở mục
  lịch sử cuối tài liệu, có banner nói rõ không áp dụng cho mã hiện tại.

## R22.4 — Tài liệu bàn giao

- `doc/accessibility/USER-GUIDE.vi.md` — hướng dẫn nghe cho người mới
  (viết để nghe TTS: câu ngắn, không bảng): bắt đầu lần đầu, chọn giọng
  và đồng ý giọng mạng, cử chỉ + bài luyện 8 hướng, trung tâm lệnh, cứu
  hộ một ngón, chơi cơ bản, lưu/tải, sao lưu/khôi phục đúng đường R20
  (hai bước tường minh + staging + xác nhận riêng), báo lỗi kèm chẩn
  đoán đúng đường R21 (báo cáo đọc thành lời, chỉ mã và số, không save
  cá nhân), và mục giới hạn hiện tại trung thực.
- `doc/accessibility/BUG-REPORT-TEMPLATE.vi.md` — mẫu báo lỗi 12 mục,
  gồm cam kết không đính kèm save cá nhân trừ khi được yêu cầu riêng.
- `doc/accessibility/CANDIDATE-RECORD.vi.md` — hồ sơ ứng viên **chỉ gồm
  quy trình** (mọi ô số liệu để trống có chủ đích): ghim commit +
  manifest, tính/đối chiếu checksum SHA-256, khóa ký (nêu rõ khóa
  `accessibility-test` hiện tại là khóa thử công khai; quyết định khóa
  phát hành thuộc R23/giai đoạn phát hành), nâng cấp giữ save cho T63,
  bằng chứng bắt buộc cho mỗi bài T/TX, nơi lưu hồ sơ.
- Đánh dấu lịch sử: `doc/accessibility/STATUS.md` thêm 4 ghi chú "đoạn
  lịch sử — APK thử giọng đời đầu, không phải mã hiện tại" tại các đoạn
  về APK `cb3c082` (chỉ thêm, không sửa nội dung gốc).

## Chỗ chưa chắc (để giai đoạn 2 xác nhận)

1. Chưa biên dịch: chữ ký/API của fixture và 3 tệp test mới đã đối
   chiếu theo tiền lệ cùng cây (`who.magic->knows_spell` theo đúng
   fixture, mã lý do theo `src/a11y_semantics.cpp`, `pos_bub()` theo
   test hiện có); lần biên dịch đầu ở giai đoạn 2 có thể lộ lỗi cú
   pháp/kiểu — đó là việc của giai đoạn 2, không phải PASS/FAIL của
   bài nào.
2. Đặc tả minigame F-11/F-13 là suy diễn từ mã nguồn RNG của engine;
   nếu engine đổi thứ tự gọi RNG, phải suy lại trước khi dùng làm
   oracle.
3. F-10 kitten không suy được vị trí từ seed: bài kitten chỉ có thể
   kiểm hình học/quyền biết bằng kịch bản thiết bị, không bằng bàn
   cố định.
4. Bản `JOURNEY-LEDGER.vi.md` chưa đồng bộ trường `fixtures` mới (bản
   JSON là nguồn sự thật; đối chiếu toàn bộ để ở R23 như các đợt
   trước đã ghi).
