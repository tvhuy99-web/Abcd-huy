# R03 — Dispatcher, quyền đầu vào và modal (giai đoạn 1, chỉ viết mã)

Tài liệu này ghi cửa kiểm quyền duy nhất cho intent của bộ đọc, vòng đời
intent, chủ sở hữu modal, các đường đầu vào kép đã đóng, cơ chế nối adapter
khi bật bộ đọc giữa màn, và suspend/resume. Trạng thái R03.1–R03.4 là
**đã viết, chờ kiểm thử** (xem `REMAINING-CODE-BACKLOG.json`): chưa build,
chưa chạy test, chưa tạo APK trong đợt này. Test ID sẽ chạy ở giai đoạn 2:
T06, T09–T15, T28, T53–T55, T62.

Phần lớn khung dispatcher đã có từ trước (mailbox, stack token, revision,
request): R03 rà soát toàn bộ bằng đọc mã (kể cả hai lượt rà soát độc lập:
31 nơi tạo `screen_scope` và toàn bộ handler đọc `intent.value`; toàn bộ
đường gửi intent phía Java), khép các kẽ hở tìm thấy và ghi lại hợp đồng.
Không viết lại dispatcher.

## 1. Cửa kiểm quyền (R03.1)

Mọi intent đi đúng một đường: `GameReaderController.send()` (điểm gọi
`nativeReaderIntent` duy nhất trong toàn bộ cây Java) → JNI
`nativeReaderIntent` (chặn số không dương và giới hạn độ dài ở
`a11y_android.cpp`) → mailbox `a11y::android::enqueue` (tối đa 32 intent,
chỉ nhận khi reader bật và không suspended) → game thread lấy ra trong
`a11y::poll_input` — nơi duy nhất intent được phép chạm tới handler.
`poll_input` kiểm theo thứ tự, mọi từ chối đều **không thực thi, không
crash**, và (trừ focus, xem dưới) đều trả biên nhận qua `android::status`:

0. `activate_pending_scopes(owner)` chạy trước (mục 5).
1. Đồng bộ vòng đời: epoch Android đổi thì áp ranh giới thế hệ mới trước.
2. Chỉ màn trên cùng của stack được poll: `suspended()` hoặc owner không
   phải `screens.back()->input_owner` thì không lấy intent nào.
3. Token phải được `context_stack::accepts` chấp nhận: session, frame của
   màn trên cùng và generation phải khớp hiện tại.
4. `revision` phải bằng revision hiện tại của màn trên cùng.
5. `request` phải lớn hơn mốc cao nhất đã nhận của màn đó (chống phát
   lại); mốc được đặt lại khi đổi epoch vòng đời hoặc thay phiên game
   (mục 6) — an toàn vì mailbox đã bị xoá và ranh giới generation đã vô
   hiệu mọi intent của chủ sở hữu cũ.
6. **Cổng giá trị (mới ở R03):** chỉ `set_text`, `set_value`,
   `select_choice` được mang `value` khác rỗng; action khác mang value
   (callback editor quá hạn, intent chế tay) bị từ chối `invalid_value`
   trước khi handler thấy nó. Đã rà soát toàn bộ handler: 8 điểm đọc
   `intent.value` trong `src/` đều nằm sau guard của đúng 3 action này
   (crafting batch, nhật ký, lời cuối, bán kính/độ sâu quét thế giới,
   input_popup, string_input_popup, nháp Options), nên cổng không chặn
   luồng hợp lệ nào.
7. **Tra lại đối tượng tại lúc nhận:** với mọi action trừ `cancel`, node
   phải tồn tại trong snapshot đang hiệu lực của màn trên cùng và action
   phải nằm trong `node.actions` của chính node đó; node bị vô hiệu
   (`enabled == false`) bị từ chối `disabled`, trừ `focus`. Không có
   quyết định nào dựa trên snapshot cũ hay node phía Java gửi lên ngoài
   ID logic. Riêng `focus` bị loại **im lặng** khi node đã biến mất: nó
   được gửi lại mỗi lần duyệt và đua với đổi màn là chuyện thường, không
   đáng một thông báo "stale" (phía Java cũng không đọc to biên nhận từ
   chối của `focus` ở cổng JNI nữa — thay đổi Java của R03).
8. Với `set_value`/`select_choice`: `validate_intent_value` của R02.4
   kiểm miền/giá trị theo hợp đồng form đã publish, từ chối
   `invalid_value` trước handler.
9. Handler là lớp kiểm cuối: handler tra lại điều kiện engine hiện tại
   khi chạy (`registered_intent` chỉ trả action còn đăng ký trong
   `input_context` hiện tại; handler hành trang tra lại hàng → entry
   hiện tại; handler `set_text` số học tự parse có biên và try/catch).

`cancel` là trường hợp riêng có chủ ý: nó gửi cho **màn sở hữu**, không
gửi cho node, và chỉ poll của màn trên cùng mới tiêu thụ được nó, nên
cancel không bao giờ rơi xuống màn dưới một modal. ID node mà Java gửi
kèm (focus hiện tại của nó) không được tin: cancel nêu tên một đối tượng
không có trong snapshot hiện tại sẽ bị loại `stale`.

Biên nhận: `stale`, `disabled`, `invalid_value`, `unsupported`,
`text_applied`. Java khớp biên nhận của editor/ngữ cảnh form bằng
request + khóa ngữ cảnh (session:frame:generation); biên nhận không khớp
không gây hành động.

## 2. Vòng đời intent

Intent được đúc theo một snapshot đã publish (token + revision lấy từ
chính snapshot đó) và mang một request ID từ dãy tĩnh theo tiến trình
phía Java (đơn điệu kể cả khi Activity bị tạo lại trong cùng tiến
trình). Nó được tiêu thụ tối đa một lần: FIFO của mailbox cộng mốc
request cao nhất của màn đích. Mọi sai lệch (token, revision, request,
node, action, miền giá trị) đều biến intent thành biên nhận từ chối,
không biến thành lệnh hoãn. Intent không sống qua: đóng scope
(`android::clear()` trong `activate`/huỷ của `screen_scope`), thay
phiên game (`replace_game_session`), đổi epoch vòng đời (`configure`
xoá mailbox), Java suspended (`send()` tự chặn khi không foreground/
không focus cửa sổ/đang suspended), và đóng controller (chốt `closed`
tường minh mới bổ sung ở R03 cho `send`/`onResume`/
`onWindowFocusChanged`/`onBackPressed`, cộng `close()` cấu hình native
reader thành tắt).

## 3. Chủ sở hữu modal

- C++: `screens` là một stack; chỉ `screens.back()` được publish và
  được poll. Huỷ một scope xoá nó và mọi scope phía trên (đúng kỷ luật
  ngăn xếp lời gọi đơn luồng), xoá mailbox và publish lại màn bị lộ ra.
  **Mới ở R03:** màn bị lộ ra được tăng revision trước khi publish lại,
  nên intent đúc theo bản publish trước-modal của nó bị stale theo cả
  hai lớp (generation của token đã tăng khi `leave`, nay thêm revision),
  và Java buộc phải đọc lại màn trước khi thao tác tiếp. Đây là xử lý
  cho ghi chú còn treo từ R00 về modal C++ mở trong lúc màn world còn
  trên stack: quyền sở hữu nay là stack + publish-chỉ-màn-trên-cùng +
  revision mới khi lộ diện, không chỉ "chỉ publish màn trên cùng".
- Dialog native Android (`NativeUI`): cả 3 điểm gọi C++ (`query_yn` và
  `popup` trong `output.cpp`, `singleChoiceList` trong `uilist.cpp`)
  đều gate `!a11y::enabled()`; khi bộ đọc bật, dialog thuộc về adapter
  ngữ nghĩa (`query_popup`/`uilist` có scope riêng), không có hai chủ
  sở hữu cùng lúc. Khi bộ đọc tắt, dialog native là bề mặt TalkBack
  chuẩn và không có intent nào đi vào game qua nó.
- Back phía Java thuộc về lớp bộ đọc trên cùng trước: editor →
  bài luyện di chuyển → màn điều khiển của bộ đọc → mở màn điều khiển.
  Cancel gửi xuống engine là một lệnh điều khiển tường minh (mục 5
  trong danh sách điều khiển), không phải Back: Back không bao giờ tự
  thoát một dialog của engine.

## 4. Đường đầu vào kép đã đóng (R03.2)

- **Chạm SDL + bề mặt bộ đọc:** khi bộ đọc bật, `sdltiles.cpp` nuốt
  mọi sự kiện finger/mouse/wheel trước ImGui và mọi nhánh touch legacy,
  nên cùng một chạm không thể vừa là intent bộ đọc vừa là click game.
  Phía Java, `ReaderGestureSurface` sở hữu mỗi chuỗi pointer đúng một
  lần (ở chế độ tự đọc nó trả `true` luôn, nút con không nhận chạm;
  sự kiện `ACTION_CANCEL`/đa điểm chỉ huỷ cử chỉ, không sinh intent).
  Ở chế độ TalkBack surface nhường sự kiện cho hệ thống: click do
  TalkBack tổng hợp là đường kích hoạt duy nhất, listener focus không
  kích hoạt.
- **Gửi hai lần từ editor:** chốt ở controller (`editorRequest == 0`,
  `formRequest != 0`) cộng chốt `editor != null` mới bổ sung ở R03 cho
  cả hai listener submit — callback của editor đã đóng không đúc được
  intent mới. `ReaderTextEditor` tự nó không có chốt; hợp đồng nằm ở
  controller và được ghi rõ ở đây.
- **Callback quá hạn phía Java:** `receive()` tự loại theo
  `presentationEpoch` và `closed`; timeout thử cử chỉ tự kiểm `closed`;
  `StartGameRunnable` của SplashScreen nay có chốt chạy-một-lần và
  kiểm `isFinishing()`/`isDestroyed()` (mới ở R03) nên một lần khởi
  động đã xếp hàng không bắn trễ sau khi Activity đóng và hai trigger
  không mở game hai lần (trước đây chỉ dựa vào `launchMode`).
- **Đổi scope:** mọi chuyển scope/scope đóng đều xoá mailbox của scope
  cũ (mục 2), intent cũ nếu lọt qua vẫn chết ở cổng token/revision.
- **Phím giả:** bộ đọc không bao giờ tổng hợp phím thô; kết quả của
  `poll_input` còn được `handle_input` kiểm lại trong
  `registered_actions` hiện tại của context trước khi thành hành động.

## 5. Nối adapter khi bật bộ đọc giữa màn (R03.3)

Cơ chế: `screen_scope::update()` khi scope chưa kích hoạt được (bộ đọc
tắt hoặc đang suspended) nay **lưu snapshot mới nhất vào scope** thay vì
thả rơi. `a11y::activate_pending_scopes(owner)` — được gọi ở đầu
`poll_input` và ở đầu đường cài scope của `input_context::handle_input`
(trước khi xét scope dùng chung) — kích hoạt các scope chờ của đúng
owner đó theo thứ tự khởi tạo (scope khởi tạo sau — ở lớp trong hơn —
nằm trên cùng), áp snapshot
đã lưu qua cùng đường validate/publish như `update()` thường, và màn
đang mở bắt đầu phục vụ bộ đọc tại chỗ, không cần đóng/mở lại.

Đối chiếu 31 nơi tạo scope: phần lớn gọi `update()` lặp trong vòng
vẽ/nhập (đã tự kích hoạt lại qua `update`); hai builder one-shot thật
là `popup.cpp` (update một lần trước vòng `handle_input(50)`) và
`ui_iteminfo.cpp` (update một lần, snapshot tĩnh) — cả hai đều poll qua
`handle_input` của owner nên cơ chế chờ bao phủ được. Scope world của
`handle_action` thuộc nhóm "one-shot theo epoch" (guard publish) nhưng
handler của nó tự `update` phản ứng, và epoch đổi khi cấu hình reader
đổi. Mọi vòng lặp của 31 điểm đều đi qua `handle_input`/`poll_input`
của owner — không có scope nào nằm ngoài tầm của cơ chế.

Thứ tự cài đặt quan trọng: kích hoạt scope chờ chạy **trước** khi scope
dùng chung ("Commands for the current screen") được cài cho cùng owner,
nên màn có adapter riêng không bao giờ bị scope dùng chung đè lên
trong cùng một lần `handle_input`.

## 6. Suspend/resume và backend không Android (R03.4)

Định nghĩa suspended: phía Java, suspended = không foreground, hoặc
mất focus cửa sổ, hoặc (ở chế độ tự đọc) TTS chưa sẵn sàng/lỗi. Java
đẩy (enabled, suspended) xuống `nativeConfigureReader` mỗi khi đổi;
`configure` ở C++ tăng epoch và xoá mailbox. Game thread thấy epoch đổi
thì: đặt lại bộ theo dõi trạng thái, ép ranh giới generation (suspend
rồi resume theo trạng thái thật), xoá mailbox, đặt lại mốc request của
mọi màn (mục 1.5) và publish lại màn hiện tại. Activity tạo lại sinh
controller mới; controller cũ bị chặn bởi chốt `closed` và tham chiếu
đã bị Activity huỷ trước `close()`; snapshot được publish lại mang
token mới, intent của chủ sở hữu trước không còn đường hợp lệ.
Lỗi TTS đi qua `onSpeechHealth` → `updateNativeState` → suspended cho
đến khi giọng sẵn sàng lại; TalkBack bật/tắt đi qua listener khám phá
chạm: huỷ cử chỉ đang dở, tăng epoch trình bày, cấu hình lại native,
và hoàn lượt thử ánh xạ cử chỉ đang chờ (tương tác với R00.3).

Không có tệp nguồn mới ở R03: mọi thay đổi nằm trong
`a11y_runtime.h/.cpp`, `input_context.cpp` và hai tệp Java hiện hữu,
nên danh sách build không đổi — cả hai bản dựng CMake (chính và
Android JNI) đều gom `src/*.cpp` bằng GLOB, backend không Android vẫn
biên dịch các tệp này với `a11y::android` ở chế độ no-op như trước
(`enabled()` là false ngoài Android, `activate_pending_scopes` trả
false ngay).

## 7. Chỗ chưa chắc / giới hạn đã biết

1. **Chưa biên dịch/chưa chạy** (đúng giai đoạn 1): khai báo friend,
   registry `live_scopes` và việc tách `apply_snapshot` đã đối chiếu
   bằng đọc mã; tính biên dịch được chỉ xác nhận ở giai đoạn 2 (T06,
   T09–T15).
2. `live_scopes` giả định scope sống cùng khung ngăn xếp với
   `input_context` chủ sở hữu của nó (đúng ở cả 31 điểm đã rà soát:
   biến cục bộ cạnh context hoặc thành viên của session bao quanh
   context). Nếu một adapter tương lai huỷ context trước scope của nó,
   con trỏ owner đã tái sử dụng có thể khớp nhầm một scope cũ; adapter
   mới phải giữ kỷ luật này, và bài giai đoạn 2 nên có một hành trình
   modal lồng để khoá hành vi (T14/T15).
3. Huỷ scope vẫn xoá theo cascade cả các scope phía trên trong
   `screens`. Với thứ tự cài đặt mới, scope dùng chung không còn đè
   lên scope riêng đang chờ của cùng owner; trường hợp scope riêng
   đang hoạt động nhưng bị scope khác che trong lúc vòng lặp của nó
   vẫn chạy đã không quan sát thấy ở 31 điểm (vòng lặp của màn bị che
   không lặp trong ngăn xếp lời gọi đơn luồng) — nếu giai đoạn 2 tìm
   thấy một luồng như vậy, chỗ sửa đúng là kỷ luật huỷ của
   `screen_scope`, không phải thêm đường input khác.
4. Biên nhận `stale` ở lớp token (khác lớp node ở mục 1.7) vẫn tới
   nhánh biên nhận chung phía Java và có thể đọc to một lần đúng lúc
   đổi màn, kể cả khi intent gốc là `focus` — biên nhận không mang
   action nên Java không phân biệt được. Chấp nhận ở R03; muốn hết
   phải đổi schema biên nhận, để dành quyết định riêng.
5. `set_text` không có validate miền chung ở dispatcher (cố ý: văn
   bản tự do hợp lệ). Mọi consumer `set_text` dạng số hiện có đều tự
   parse có biên; consumer mới ở các mục R sau phải làm theo, dispatcher
   chỉ đảm bảo value tới đúng node/action đã publish.
6. Gate của `NativeUI` chỉ đọc `a11y::enabled()`, không đọc
   `suspended()`: reader bật nhưng đang suspended thì `query_yn`/
   `popup` rơi về đường trong game (cũng đang suspended) — game thread
   chờ, không dialog nào hiện sai chủ. Cần một bài giai đoạn 2 cho lỗi
   TTS giữa dialog (T55) để chốt trải nghiệm, không đổi mã ở đây.
7. Chốt mở game một lần của SplashScreen là theo instance; lớp bảo vệ
   theo tiến trình vẫn là `launchMode="singleInstance"` như trước.
   Vòng đời bootstrap đầy đủ thuộc R06.
