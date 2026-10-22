# Bảng triển khai theo quy trình ba giai đoạn

Cập nhật 22/10/2026, giờ Việt Nam. Hiện ở **giai đoạn 1: viết mã**. Chưa bắt
đầu chạy kiểm thử cho các thay đổi ở nhánh `feature/android-reader-code-phase`.
Không có APK mới của nhánh này. Bản đã được người dùng xác nhận mở/đọc được
là APK thử giọng tại `cb3c082`, không phải mã mới ở đây.

## Quy trình đã chốt

1. Viết mã lần lượt cho toàn bộ phạm vi trong `PLAN.vi.md`. Duy trì bảng phần
   còn thiếu và các tình huống cần kiểm chứng; không đánh dấu hoàn tất chỉ vì
   có lớp/hàm, danh mục lệnh hoặc đường vào một menu.
2. Sau khi phần viết mã hoàn tất, biên dịch và chạy bộ kiểm thử đầy đủ: host,
   Android/API, giả lập, APK ARM64, máy thật và 64 hành trình/người khiếm thị.
   Kiểm thử người dùng không thể thay bằng test doubles hoặc build thành công.
3. Tổng hợp lỗi theo ảnh hưởng và nhóm tính năng, sửa, rồi chạy lại bài liên
   quan và hồi quy. Chỉ phát hành sau khi đủ bằng chứng theo tiêu chí kế hoạch.

Workflow trợ năng của nhánh mới chuyển sang `workflow_dispatch`; không chạy
build/test tự động trong đợt viết mã này. Không merge vào `master` và không
ghi PASS mới.

## Mã đã bổ sung trong đợt hiện tại, chưa kiểm thử

| Nhóm | Điểm triển khai | Trạng thái/giới hạn |
| --- | --- | --- |
| Ngữ nghĩa | `a11y_semantics`, `ReaderSnapshot` | Nút, vai trò, giá trị, trạng thái, vị trí, chi tiết, token và revision; chưa nghiệm thu schema/Unicode |
| Sở hữu và luồng | `a11y_runtime`, `a11y_android` | Scope, mailbox hữu hạn, lọc token/revision/request; Java không giữ con trỏ gameplay; chưa kiểm thử race/vòng đời |
| Android | `GameReaderController`, `CataclysmDDA` | Lớp trình bày native và gửi ý định; phối hợp touch exploration; chưa xác nhận focus/insets/Back trên máy |
| Lời đọc | `ReaderSpeechQueue` | Gộp focus, ưu tiên, hết hạn, đọc theo đoạn, dừng/đọc lại/watchdog; chưa kiểm thử TTS thật cho mã mới |
| Cử chỉ | `ReaderGestureSurface` | Duyệt/chọn/chi tiết/dừng, chặn hủy/nhiều ngón; chưa có cấu hình chơi nhanh tám hướng hoặc hướng dẫn luyện tập |
| Cấu hình | `ReaderPreferences`, `ReaderSetupActivity`, Splash | Bật bằng xác nhận người dùng đã nghe; tắt; đường vào màn thiết lập của flavor thử; bootstrap không TalkBack chưa đầy đủ |
| Nhập liệu | `ReaderTextEditor`, adapter popup cũ/ImGui/số | IME/TalkBack và bàn phím lựa chọn Việt, dấu thanh, biên ký tự ICU, nhận kết quả có token; chưa nghiệm thu giới hạn và callback tùy biến |
| Menu chính | `main_menu.cpp` | Tab, lựa chọn con, thế giới, tài liệu; chưa xác nhận tạo/tải save |
| Danh sách | `uilist.cpp` | Theo `fentries`, đúng `retval` qua đường xác nhận gốc, metadata đánh dấu/lý do vô hiệu; có helper cho vòng `query_once` ngoài |
| Hộp thoại | `popup.cpp` | Văn bản câu hỏi, chọn, hủy/tiếp tục qua action gốc; các dialog native khác vẫn cần tích hợp |
| Tạo nhân vật | `newcharacter.cpp` | Nối danh sách tab và lệnh; mô tả scenario/profession/trait/skill, trạng thái đánh dấu, giá trị chỉ số và một phần summary; chưa phủ mọi chi tiết hiển thị |
| Phiên chơi | `game.cpp`, `game_io.cpp` | Đổi định danh khi setup/load, bỏ snapshot/đầu vào cũ; chưa kiểm thử lỗi tải/quay về/Activity mới |
| Thông tin hợp lệ | `a11y_perception` | Nhánh CLEAR mới đọc dữ liệu sống; nhánh trí nhớ chỉ lấy terrain/furniture từ memory; bẫy/item/creature theo kiểm tra gốc; cảm nhận đặc biệt và nhiều tầng còn thiếu |
| Thế giới | `a11y_world`, `get_player_input` | Ô hiện tại, trạng thái, thông báo gần đây, bán kính quét và lệnh đã đăng ký; đọc ô không tự di chuyển; chưa có toàn bộ bộ thăm dò/bộ lọc/đường đi |
| Hành trang | `inventory_ui.cpp/.h` | Scope theo từng lần execute; hàng vật phẩm có số lượng chọn/hiện có, trạng thái, lý do từ chối và vị trí; nối chọn đơn/chọn nhiều/nhặt/bỏ/so sánh/nạp/tháo đạn/examine/giao dịch qua action gốc; kho nâng cao và kiểm chứng nested contents còn thiếu |
| Thông tin vật phẩm | `ui_iteminfo.cpp`, `a11y_document.h` | Đọc tài liệu từ `item_info_data`/`format_item_info`, gồm dữ liệu so sánh mà UI nhận; chưa nghiệm thu cấu trúc bảng và dấu/đơn vị |
| Chế tạo | `crafting_gui.cpp`, `a11y_requirements.h` | Danh sách công thức/nhóm, craftability, favorite, mẻ 1–50, kỹ năng/đặc tính, các nhóm lựa chọn dụng cụ/nguyên liệu, công đoạn và chi phí nhân vật; giữ mẫu byproduct của UI; chưa kiểm chứng tiêu hao/thay thế và tiến độ hoạt động |
| Xây dựng | `construction.cpp` | Nhóm, biến thể/công đoạn, mô tả, yêu cầu và trạng thái vị trí; CONFIRM/đặt công trình dùng logic gốc; blueprint và các điều kiện đặc biệt chưa thử |
| Chọn ô lân cận | `action.cpp/.h` | Scope cho choose_direction/choose_adjacent, gọi phép đổi hướng gốc kể cả isometric; chỉ quan sát ô ngang qua perception; trạng thái hợp lệ công trình chỉ nói ở ô nhìn thấy; chưa có quan sát khác tầng |
| Nhắm mục tiêu | `ranged.cpp`, `a11y_input.cpp`, `input_context.cpp`, `GameReaderController.java` | Danh sách sinh vật/phương tiện game cho phép ngắm dựa trên tri thức hiện tại, hướng/khoảng cách, điểm ngắm hiện tại và trạng thái; chọn hàng chỉ di chuyển điểm ngắm, lệnh FIRE/xác nhận gốc riêng; đọc lại mục tiêu vừa đổi; chưa kiểm chứng chiến đấu, phép, phương tiện hoặc mục tiêu đổi vị trí |
| Thông báo log | `messages.cpp`, `a11y_runtime.*`, `a11y_android.*`, `GameReaderController.java`, `ReaderSpeechQueue` | Thông báo mới của player log được chuyển qua JNI từ game thread lên Java; bad/warning/critical ưu tiên khẩn; thông tin thường chờ sau lời focus; bỏ thông báo trùng/coalesce hoặc bị cooldown ẩn; chưa thử tải cao, ngắt quãng hay audio thật |
| Y tế/cơ thể | `medical_ui.cpp`, `a11y_body.h`, `a11y_world.cpp` | Tab/bộ phận có định danh, sức khỏe theo vạch/cảm nhận, hiệu ứng/vết thương/chức năng; giữ đường chọn điều trị và use item gốc; thêm trạng thái điều kiện điều trị bằng chữ; chưa thử điều trị/hoạt động trên save thật |
| Bản địa hóa | `lang/po/accessibility/vi.po`, helper compile | Bổ sung câu qua gettext, không thay từ tiếng Anh tại runtime; chưa chạy gettext/build mới |

## Phần viết mã còn phải làm

Các hàng dưới đây vẫn là công việc bắt buộc của kế hoạch, không phải phần đã
hoàn tất nhờ menu chung hoặc danh mục action:

- G0/G1: hoàn thiện bootstrap khi TalkBack tắt, chọn engine/voice/tốc độ,
  thông báo phục hồi có âm thu sẵn, cài/nâng dữ liệu, lỗi dung lượng và phục hồi.
- G2: cử chỉ nhanh tám hướng, luyện tập, đổi ánh xạ/khôi phục, biểu mẫu/bảng/cây,
  các callback chỉ đọc phím thô, toàn bộ dialog native và adapter ngữ cảnh lồng.
- G3: hoàn thiện chi tiết/đánh giá mọi tab tạo nhân vật, chọn thế giới/mod,
  tên nhân vật, tạo mới và tải lại, cấu hình lượt/realtime có lựa chọn rõ ràng.
- G4: thăm dò nhiều tầng/bộ lọc/điểm đáng chú ý, cảm nhận đặc biệt và âm thanh
  bất định; quan sát theo hành động thực tế, nhặt/đồ chứa, cơ thể/y tế, ăn/uống,
  mặc/cởi, nghỉ/đợi/đọc, hủy hoạt động, trạng thái và cảnh báo tự động. Đã có
  đường gửi các thông báo log mới, nhưng chưa chứng minh đủ mọi cảnh báo hoặc
  khả năng nghe ổn định.
- G5: kho nâng cao/chuyển đồ hai phía, nhắm bắn và đường bắn, cận chiến,
  chế độ bắn, ném, hoạt động/tiến độ chế tạo và xây dựng. Đã có adapter ban đầu
  cho hành trang chọn nhiều, nạp/tháo đạn, công thức và chọn vị trí lân cận,
  cùng danh sách mục tiêu có thể ngắm theo tri thức hiện tại; còn phải đối chiếu mọi
  biến thể/pocket, hướng ngắm, xác nhận tấn công và hoàn thiện chi tiết thiếu.
- G6: overmap/travel/note/mission; NPC/đối thoại/giao dịch/cộng đồng; phương tiện
  và sơ đồ bộ phận/lái/remote; vùng/tự động hóa; bionics/đột biến/phép thuật;
  các minigame, tài liệu, trợ giúp và những màn riêng trong `COVERAGE.md`.
- G7: ngủ/hoạt động dài, nguy hiểm và kết quả thật, chết/death-cam/điểm số,
  lỗi TTS/vòng đời/tiến trình, lưu/tải/xóa và sao lưu/khôi phục dữ liệu.
- G8: chẩn đoán/độ trễ/âm hiệu/rung, tài liệu người chơi và bảo trì nội dung;
  bộ kiểm thử/fixture, hoàn tất ma trận 64 bài, build/ký/bằng chứng phát hành.

Giai đoạn 1 chỉ hoàn tất khi từng hàng còn thiếu được triển khai cụ thể và
đối chiếu lại với toàn bộ ma trận tính năng của kế hoạch. Hiện chưa đạt mốc đó.

## Điểm cần giữ cho đợt hành trang/chế tạo/xây dựng

Đã viết mã, chưa biên dịch hoặc chạy bất kỳ bài kiểm thử nào cho đợt này.
Không cộng các đường vào menu vào số hành trình đã đạt.

- Hàng hành trang giữ `item_location` để tra lại danh sách hiện thời; không
  truyền con trỏ entry hoặc invlet/phím giả sang Java. Scope kết thúc cùng
  `execute()`, kể cả nhánh return/hủy. Hàng chọn nhiều dùng TOGGLE_ENTRY;
  CONFIRM riêng mới chốt danh sách. Kích hoạt một hàng đặt chế độ chọn một
  vật phẩm; lệnh chọn hàng loạt của bàn phím vẫn còn riêng trong danh mục.
- Phải kiểm chứng chọn số lượng qua MARK_WITH_COUNT, INCREASE/DECREASE,
  bộ lọc và chuyển nhóm; nhặt/chứa/lồng, đồ ẩn, stack trùng tên, bản ghi bị
  sắp xếp hoặc mất hiệu lực, hủy rồi mở lại và không gộp hai vật phẩm cùng tên.
- Phải kiểm chứng focus/đọc chi tiết không kích hoạt vật phẩm, không khớp
  invlet chưa gán (0), không chọn cả category ngoài ý định; giữ kiểm tra
  trộm cắp/chất lỏng và xác nhận của engine. So sánh chỉ chốt khi đủ hai món.
- Mẻ chế tạo dùng đường xử lý số lượng hiện hữu, nhận kết quả sau áp dụng;
  cần kiểm chứng 1/50/0/51, chuỗi hỏng, đổi crafter/batch/filter, công thức
  lồng, lựa chọn nguyên liệu thay thế và cùng vật phẩm cho nhiều yêu cầu.
  Lời đọc dùng mẫu byproduct/mô tả UI đang giữ; không tạo thêm preview ngẫu nhiên.
- Nhóm yêu cầu đọc bằng chữ “đã có đủ/còn thiếu” thay cho màu, liệt kê toàn
  bộ phương án của nhóm. Từng nhóm vẫn bắt buộc; một phương án trong nhóm
  là lựa chọn. Không coi đủ từng nhóm độc lập là bằng chứng có thể chế tạo:
  engine vẫn kiểm tra trùng nguyên liệu, bóng tối, dụng cụ và đồ chứa.
- Xây dựng giữ đường CONFIRM, cảnh báo phe phái, chọn vị trí, partial_con và
  tiêu hao gốc. Cần kiểm chứng vị trí hợp lệ/không hợp lệ, ô không nhìn thấy,
  filter mode, blueprint, ánh sáng, tài nguyên thay đổi và hướng isometric.
- Khi bật trình đọc giữa một màn đã mở lúc reader tắt, scope hiện tại có thể
  chưa có snapshot chuyên biệt. Cần hoàn thiện cơ chế đăng ký/refresh khi
  đổi enabled trong G2/G7; không giả sử wake event đã giải quyết việc đó.

## Đợt nhắm mục tiêu chiến đấu, đã viết mã, chưa kiểm thử

- Khi màn ngắm đang mở, scope riêng xuất bản hàng cho mục tiêu mà danh sách
  gốc của game cho phép ngắm theo tri thức hiện tại của nhân vật. Dữ liệu không
  chứa tọa độ thô, đối tượng
  game hoặc thái độ phe phái nội bộ. Có tên hiển thị, hướng, khoảng cách,
  loại mục tiêu, hàng đang chọn, điểm ngắm và trạng thái đạn/tầm.
- Kích hoạt hàng lưu vị trí đang có trong danh sách rồi đi qua input_context
  trên game thread. Nó chỉ gọi `set_cursor_pos`; không gọi FIRE, không tiêu
  lượt và không chấp nhận tấn công/phép. Bắn/xác nhận trung lập vẫn dùng nguyên
  luồng game. Lệnh nội bộ bị ẩn khỏi danh mục lệnh thông thường.
- Danh sách được dựng lại trước mỗi lần chờ input, giới hạn đọc trực tiếp ở
  256 hàng và giữ NEXT_TARGET/PREV_TARGET của engine cho cả danh sách. Khi
  lựa chọn mục tiêu đổi, Android đọc hàng vừa chọn; thay đổi này chỉ áp dụng
  cho category targeting để không giành focus của các màn khác.
- Cần kiểm tra: FIRE không kích hoạt khi chỉ chọn hàng; không tốn lượt khi đổi
  mục tiêu; xác nhận NPC trung lập/bạn bè; mục tiêu mất tầm/di chuyển/ẩn giữa
  các lượt ngắm; danh sách rỗng và hơn 256 mục tiêu; xe, phép, ném, reach,
  turret; đổi loại đạn/chế độ; hướng/độ cao/isometric; TalkBack và TTS nội bộ;
  bật/tắt trình đọc trong khi màn ngắm đang mở. Không có kết quả biên dịch,
  test hoặc chạy trên thiết bị cho đợt này.

## Đợt đọc thông báo log mới, đã viết mã, chưa kiểm thử

- Sau khi game thêm một thông báo mới, gửi bản văn đã bỏ color tag và loại
  thông báo qua JNI. Java chuyển phát sang main looper, chỉ nhận khi reader đang
  bật, foreground, có snapshot sống và chưa đóng. Lời không được giữ để đọc lại
  sau khi resume nếu callback đến khi không có phiên trình bày.
- Dùng hàng lời nói đang có: bad/warning/critical đi mức URGENT; loại còn lại
  đi INFORMATION và không cắt ngang lời focus. Hàng đợi vẫn có giới hạn 8 mục
  và thời hạn; cảnh báo khẩn có thể ngắt lời thường theo chính sách hiện có.
  Với TalkBack/touch exploration, đăng thông báo qua status view.
- Chỉ đọc thông báo mới không rỗng mà player log thực sự nhận: debug bị bỏ,
  thông báo cooldown ẩn bị bỏ, thông báo coalesce không đọc lặp. Đây chưa phải
  bộ phát hiện mọi thay đổi trạng thái: nguy hiểm không phát `add_msg` vẫn cần
  adapter/cảnh báo riêng.
- Cần kiểm tra: thông điệp Việt có dấu; ưu tiên khẩn khi đang đọc văn bản dài;
  dồn trên 8 thông báo/hết hạn; trùng/coalesce/cooldown; log bị deactivate;
  debug; vào nền/đổi cửa sổ/tắt reader; chuyển màn, load save, death và voice
  chưa sẵn sàng; TalkBack bật; âm thanh trên thiết bị. Chưa biên dịch/chạy.

## Đợt y tế và tri thức sức khỏe, chưa chạy kiểm thử

- Adapter đọc bộ phận/hiệu ứng/vết thương và chức năng từ dữ liệu của màn y tế.
  Chỉ kích hoạt bộ phận mới mở menu điều trị; focus và đọc chi tiết không
  dùng thuốc, tiêu hao vật liệu hoặc gán hoạt động.
- Tổng quan đọc tốc độ/chỉ số và lần cân nặng đã ghi nhớ giống UI, không thay
  bằng cân nặng sống chưa đo. Menu điều trị nói rõ điều kiện đạt/chưa đạt,
  kiến thức thực hành bắt buộc/bổ trợ và các nhóm dụng cụ/nguyên liệu.
- Hàm mô tả sức khỏe dùng chung với world: không đọc số HP chính xác, giữ
  cảm nhận của nhân vật không cảm nhận đau; bó nẹp giữ mức chỉ báo liền xương.
  Phân biệt vạch đầy, nửa vạch, dưới nửa vạch và vạch rỗng; không lấy độ dài
  chuỗi “-----” làm năm vạch sức khỏe. Đây là sửa khi đọc nguồn trong giai
  đoạn viết mã, chưa phải lỗi được chứng minh bằng chạy kiểm thử.
- Sửa helper hiệu ứng sử dụng đối số bộ phận thay vì bộ phận đang được
  chọn, để mô tả các hàng khác không gọi nhầm tên chi. Có nhánh không có
  bộ phận để tránh truy cập phần tử đầu khi danh sách rỗng.
- Còn phải kiểm chứng miễn đau/gãy/bó nẹp/mất chi/NPC, chảy máu/nhiễm trùng,
  hủy menu, điều kiện thiếu, chọn thuốc trong inventory, thời gian/gián đoạn
  điều trị và cập nhật sau hoàn thành. Player info/morale và cảnh báo tự động
  vẫn còn trong G4/G7; không xem adapter y tế là đã hoàn tất toàn nhóm.

Đã viết thêm `tests/a11y_body_test.cpp` để giai đoạn 2 kiểm chứng bằng avatar
và dữ liệu vật phẩm thật: không lộ HP trong cùng mức thanh, chi bị gãy không
được đọc thành thanh đầy, và bó nẹp khi miễn đau không lộ tiến độ. Các test
này **chưa được biên dịch hoặc thực thi**, không có kết quả PASS/FAIL mới.
