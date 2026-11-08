# R18 — Minigame và màn điều khiển riêng

Phạm vi: 5 minigame trên thiết bị trong mã nguồn hiện hành — robotfindskitten
(`src/iuse_software_kitten.cpp`), Lights On (`src/iuse_software_lightson.cpp`),
Minesweeper (`src/iuse_software_minesweeper.cpp`), Snake
(`src/iuse_software_snake.cpp`), Sokoban (`src/iuse_software_sokoban.cpp`).
Sổ hành trình R01 không kiểm kê minigame nào khác; nếu một mod/thiết bị khác
sinh minigame riêng, đó là bề mặt mới phải kiểm kê lại, không tự coi là đã phủ.

Mẫu chung cho cả 5 trò (theo tiền lệ R15.4, `smart_controller_ui.cpp`):

- Mỗi trò có `a11y::screen_scope` riêng gắn đúng `input_context` của trò,
  category riêng (`robot-finds-kitten`, `lightson`, `minesweeper`, `snake`,
  `sokoban`). Snapshot của minigame không bao giờ đặt
  `swipe_movement_allowed`, nên scope trò chơi không nhận cử chỉ thế giới
  (R18.1).
- Snapshot bắt đầu từ `a11y::command_snapshot` của chính context trò (lệnh
  hướng/QUIT/RESET… đã đăng ký), cộng node trạng thái của trò; luật chơi nằm
  trong node tài liệu (`a11y::append_document`) đọc theo yêu cầu.
- Focus luôn là quan sát: hoặc chỉ đổi con trỏ nội bộ của trò (Lights On,
  Minesweeper — con trỏ vốn là trạng thái nhìn thấy được), hoặc không inject
  action nào (`nullopt`) khi một action bị inject sẽ tự gây hệ quả (Snake,
  Kitten — xem dưới). Hành động thật luôn qua đúng nhánh xử lý gốc của vòng
  lặp trò (toggle/mở ô/đẩy kiện/chạm vật), không có đường tắt song song.
- Thoát giữa chừng giữ hệ quả gốc: cả 5 trò chạy trong `iuse` của thiết bị;
  điểm trả về và chi phí lượt/pin do luồng gọi gốc quyết định, adapter không
  đổi. Khởi động lại dùng đúng lệnh gốc (RESET của Lights On/Sokoban, NEW
  của Minesweeper; Kitten và Snake không có lệnh restart trong UI gốc nên
  không bịa thêm).

## R18.1 + R18.3 — robotfindskitten

Vấn đề cấu trúc: bản gốc tạo `input_context` tạm thời trong `show()` và lại
tạo mới trong mỗi lần `process_input()` — scope bộ đọc phải gắn đúng instance
gọi `handle_input()` nên không gắn được. Đã chuyển thành **thành viên
`input_context ctxt`** (khai báo trong `iuse_software_kitten.h`), đăng ký một
lần trong constructor (QUIT, HELP_KEYBINDINGS, ANY_INPUT, cardinal).
`get_desc()` là const nên `show() const` gọi được trên thành viên thường.
Hệ quả hành vi duy nhất: ở màn hướng dẫn, phím hướng nay trả action hướng
thay vì rơi vào ANY_INPUT; nhánh instructions đã sửa để **cả hai dạng đều bắt
đầu trò**, giữ nguyên ngữ nghĩa "phím nào cũng bắt đầu" của gốc.

Snapshot theo trạng thái (`ui_state`):

- instructions: tài liệu luật nguyên ý văn bản trên màn + node
  `kitten:start` (activate trả một action hướng để bắt đầu).
- main / invalid_input: summary = vị trí robot ("column %d of 60, row %d of
  17") + tổng số vật thể (21); node `kitten:robot`; mỗi vật một node
  `kitten:object:<n>`.
- bogus_message: summary "That is not kitten." + tài liệu chứa **nguyên văn**
  `this_bogus_message` — thông điệp của vật vừa chạm, đúng như game hiển thị.
- end_animation: công bố kết quả "You found kitten! Way to go, robot!";
  cơ chế ngủ 1 giây/khung hình của hoạt cảnh giữ nguyên không đụng vào.

**Ranh giới tri thức (cứng):** danh tính kitten là bí mật của trò. Mọi vật —
kitten lẫn 20 vật giả — công bố bằng đúng một khuôn: ký tự hiển thị, tên màu
(`string_from_color`), vị trí tuyệt đối và vị trí tương đối so với robot.
Số thứ tự node tính theo thứ tự quét bàn (hàng rồi cột), không theo thứ tự
lưu trữ, nên không lộ vật nào là kitten. Thông điệp của vật chưa chạm không
xuất hiện ở đâu. Node vật chỉ có action `focus`; di chuyển chỉ bằng lệnh
hướng như người chơi thường.

Vá ở tầng tích hợp: handler cho `focus` trên node `kitten:*` trả `nullopt`
(không inject action). Nếu trả một action (kể cả "TIMEOUT"),
`process_input()` gốc sẽ tiêu thụ nó như một phím bất kỳ: ở trạng thái
bogus_message nó làm tắt thông điệp người dùng còn đang đọc, ở khung cuối
end_animation nó rời màn kết thúc. Trả `nullopt` thì `poll_input` bỏ qua
intent an toàn và vòng lặp tiếp tục chờ lệnh thật.

## R18.1 + R18.2 — Lights On

Adapter trong `start_game()`: node `lights:cell:<x>:<y>` cho từng ô (bàn tối
đa 6×5), label "Row %d, column %d", value On/Off, `selected` tại con trỏ;
summary nêu số đèn bật/tổng ô, vị trí con trỏ và trạng thái ô con trỏ; tài
liệu `lights:rules` nêu luật (đổi một ô thì đổi cả 4 ô kề; thắng khi mọi đèn
đều bật — đúng văn bản `on_redraw` gốc). `focus` ô chỉ đặt `position` rồi trả
"TIMEOUT"; `activate` ô đặt `position` rồi trả "TOGGLE_SPACE", đi qua đúng
nhánh toggle + kiểm tra thắng + popup gốc. Trò không có thông tin ẩn: toàn
bàn là tri thức công khai nên công bố hết là đúng luật.

## R18.1 + R18.2 — Minesweeper

Adapter trong `start_game()`: node `mine:cell:<x>:<y>` cho từng ô, actions
{"focus","activate","flag"}. Handler chỉ di chuyển con trỏ (biến local
`iPlayerX/iPlayerY`, tọa độ được phân tích lại và kiểm tra biên tại lúc
intent) rồi trả lần lượt "TIMEOUT"/"CONFIRM"/"FLAG" để vòng lặp gốc tự áp
dụng — adapter không tự lộ ô hay cắm cờ. Summary: kích thước bàn, tổng số mìn
(iBombs — tham số công khai của độ khó), số cờ đã cắm, số ô đã lộ, mô tả ô
con trỏ. Menu độ khó (`uilist`) và các prompt Custom
(`string_input_popup`) đã có adapter dùng chung nên không sửa `new_level()`.

**Ranh giới tri thức (cứng):** `mLevel` là dữ kiện ẩn. Hàm `cell_value_text`
là cổng duy nhất: ô `seen` mới đọc số thật (0 → "No adjacent mines"); ô
`flag` → "Flagged"; ô `unknown` → "Covered". Chỉ khi `boom == true` — lúc
thua, engine tự lộ toàn bàn (`reveal_all()` và ký tự `*` trong bản vẽ) — ô
mìn mới được công bố "Mine", khớp đúng hình gốc (kể cả cờ cắm đúng mìn).
Không ô nào được mô tả là "an toàn", không vị trí mìn ẩn nào bị đếm hay nêu.

Bàn lớn: trần snapshot là `a11y::max_snapshot_nodes` (4096). Expert 30×16 =
480 ô và Custom tối đa theo `FULL_SCREEN_WIDTH/HEIGHT` mặc định (~1596 ô)
đều dưới trần. Nếu một bàn vượt ngân sách node, builder chỉ công bố cửa sổ
quanh con trỏ (con trỏ luôn được công bố) và summary nói rõ khoảng hàng/cột
đang liệt kê — không cắt thầm, không để snapshot bị từ chối toàn bộ.

## R18.1 + R18.3 — Sokoban

Adapter trong `start_game()`: summary "Level %d of %d. Score %d. Moves %d
(total %d). Packages on goals: %d of %d. You are at row %d, column %d."
Node `soko:player` (focus của snapshot) mô tả 4 ô kề của người chơi; mỗi
kiện một node `soko:package:<n>` (thứ tự quét hàng rồi cột): vị trí, đã trên
đích hay chưa, và 4 ô kề của kiện để tự suy đường đẩy. Tài liệu `soko:board`
gồm luật ngắn + danh sách mọi ô đích kèm trạng thái đã có kiện hay chưa.
Node Sokoban chỉ có action `focus` — handler trả "TIMEOUT" (vô hại trong
vòng lặp này: không hướng, không lệnh thì không có gì xảy ra); di chuyển,
đẩy, UNDO/RESET/NEXT/PREV đi qua lệnh gốc của `command_snapshot`.

Đọc bàn an toàn: builder đọc `mLevel` chỉ bằng `find` trên map lồng nhau,
không dùng `operator[]`, nên việc dựng snapshot không tự chèn ô trống vào
bàn đang chơi; ô vắng trong map được đọc là tường, khớp cách engine vẽ.

Từ vựng ô kề: wall / floor / goal / package / package on goal — cộng thêm
"you" / "you, on a goal" khi ô kề của một kiện chứa chính người chơi (trường
hợp ngoài 5 từ gốc nhưng là thông tin quyết định đường đẩy; vị trí người
chơi vốn công khai).

## R18.4 — Snake và chính sách thời gian thực

Snake là trò realtime duy nhất: bản gốc gọi `ctxt.handle_input( iGameSpeed )`
với `iGameSpeed` = 100 ms, giảm 3 ms mỗi quả; rắn tự bước mỗi vòng lặp.
Đọc trạng thái bằng lời nói + ra lệnh hướng bằng bộ đọc không thể đáp ứng
nhịp 100 ms, và một snapshot thay đổi mỗi 100 ms sẽ làm hàng lời không theo
kịp. Chính sách đã chọn — **công khai, không lặng lẽ**:

> Khi bộ đọc bật (`a11y::enabled()`), vòng chính dùng `ctxt.handle_input()`
> **không timeout** thay cho `handle_input( iGameSpeed )`: rắn chờ lệnh
> hướng của người chơi ở mỗi bước, trò chạy theo lượt. **Luật không đổi:**
> rắn vẫn bước một ô mỗi lệnh, xuyên biên như cũ, đâm vào mình thì thua,
> +500 điểm và +10 đoạn mỗi quả, `iGameSpeed` vẫn giảm như cũ nhưng chỉ còn
> áp dụng khi bộ đọc tắt. Câu chính sách nguyên văn tiếng Anh
> ("Reader timing: with the screen reader on, the snake waits for your
> direction at each step instead of moving in real time. The snake, scoring
> and growth rules are unchanged.") xuất hiện trong summary của mọi
> snapshot đang chơi và trong tài liệu luật `snake:rules`.

Đánh đổi, nói thẳng: trải nghiệm Snake với bộ đọc **không còn là kiểm tra
phản xạ** như bản gốc; nó là cùng một bàn chơi, cùng luật va chạm/điểm/tăng
trưởng, nhưng nhịp theo lượt. Đây là thay đổi nhịp có công bố, đúng điều
khoản của R18.4 (không tự làm chậm/đổi luật mà không công bố). Phương án
giữ realtime + cue âm thanh hướng quả đã được cân nhắc và loại ở giai đoạn
1: cue không cho người chơi mù đủ thông tin vị trí thân rắn đang dài ra để
né, và hàng lời không thể đọc trạng thái bàn 80×24 ở nhịp 100 ms.

Adapter: một `screen_scope` "snake" sống xuyên vòng chính và vòng
game-over (cờ pha `game_over`). Snapshot đang chơi: summary gồm điểm, độ dài
rắn (số đoạn thực trên bàn), đầu rắn (cột/hàng từ 1), hướng đang đi, vị trí
quả tương đối so với đầu ("3 columns right, 2 rows up") kèm câu chính sách;
4 node `snake:dir:<up/down/left/right>` (activate trả đúng action hướng;
hướng hiện tại `selected` và là focus); node `snake:fruit` chỉ focus; tài
liệu luật. Snapshot game-over: summary "Game over. Total score: %d." + luật
+ lệnh QUIT gốc.

Vá ở tầng tích hợp: `focus` trên node hướng/quả trả `nullopt`. Bản đầu trả
"TIMEOUT" với ý "không đổi hướng", nhưng trong vòng lặp này **mọi** action
trả về đều làm rắn bước một ô — duyệt node để nghe sẽ tự đẩy rắn đi, vi phạm
nguyên tắc focus chỉ quan sát. Trả `nullopt` thì intent bị bỏ qua, vòng lặp
tiếp tục chờ; chỉ `activate` mới bước rắn.

Trạng thái hành trình: **đã viết, chờ kiểm thử** với chính sách trên. Bài
nghiệm thu riêng TX-minigame-snake ở giai đoạn 2/3 (người khiếm thị chơi
thật) là nơi quyết định giữ chính sách theo lượt hay mở lại mục này — nếu
nghiệm thu cho rằng bản theo lượt không còn là Snake, R18.4 phải mở lại
chứ không được giữ nhãn hoàn thành.

## Hồ sơ

- Backlog R18.1–R18.4 → `da-viet-cho-kiem-thu` (bài bổ sung
  TX-minigame-kitten/lightson/minesweeper/snake/sokoban giữ nguyên).
- Sổ hành trình: 5 hành trình minigame → đã viết; tổng sổ đếm lại từ dữ
  kiện: **99 đã viết / 16 chưa viết** trên 115. Bảng nhóm trong
  `JOURNEY-LEDGER.vi.md` đã sửa hàng Minigame (5/5/0); dòng tổng và vài
  hàng nhóm khác trong bản .vi.md vẫn cũ so với dữ kiện JSON — để đợt đối
  chiếu toàn bộ ở R23, như đã ghi ở R15/R16.

## Chỗ chưa chắc

1. Chưa biên dịch — chữ ký/API đã đối chiếu theo tiền lệ cùng cây
   (`screen_scope`/`command_snapshot`/`registered_intent`/`append_document`,
   `string_from_color` ở `color.h:540`, `get_desc` const ở `input_context.h`);
   xác nhận biên dịch thuộc giai đoạn 2. Các điểm riêng cần compiler xác
   nhận: odr-use của `rfkCOLS`/`rfkLINES`/`numbogus` (static constexpr)
   trong lambda của Kitten; ternary số ít/số nhiều trong `string_format`
   của Snake.
2. Kitten: việc gộp context thành viên làm phím hướng ở màn hướng dẫn bắt
   đầu trò qua nhánh mới — hành vi tương đương theo đọc mã, cần bài
   TX-minigame-kitten xác nhận cả bằng phím cứng lẫn intent bộ đọc.
3. Minesweeper: cửa sổ quanh con trỏ cho bàn vượt trần là đường dự phòng
   chưa có bàn thật nào chạm tới với kích thước terminal mặc định.
4. Snake: độ dài trong summary là số đoạn thực đang vẽ (`vSnakeBody.size()`),
   không phải độ dài mục tiêu `iSnakeBody` — đúng với những gì mắt thấy trên
   màn hình.
5. Cả 5 trò đều cần bài nghiệm thu chơi thật riêng từng trò ở giai đoạn 2/3;
   T01–T64 không chứng minh được minigame (đúng ghi chú đóng mục của kế hoạch).
