# Đặc tả thực thi hiện hành của trình đọc Android

**Nguồn:** `master`, commit `f0be189ac5ebf000fc38e8c91fa3b42e325a7417`, 13/11/2026. Đây là tài liệu mô tả **đường mã có mặt**, không xác nhận mọi thao tác trên APK/máy thật. [Trạng thái](STATUS.md) · [Hướng dẫn người dùng](USER-GUIDE.vi.md) · [Kiểm chứng](VALIDATION-MATRIX.vi.md).

## 1. Sở hữu và điều kiện an toàn

- `src/a11y_semantics.*`: các node gồm ID, role, nhãn, value, detail, enabled, selected và hành động được cho phép; snapshot mang session/frame/generation/revision. Ý định của Java phải qua kiểm tra token, quyền và ngữ cảnh C++.
- `src/a11y_runtime.*`, `src/a11y_android.*`: trạng thái màn hình, mailbox và cổng nhận intent; lệnh gameplay thực sự do game thread quyết định, không phải listener Android.
- `GameReaderController`: điều khiển focus, danh sách, menu, TTS, câu trả lời và cửa sổ nhập; các yêu cầu đọc thông tin dài bị buộc với mục/màn hiện hành, có chống phản hồi cũ.
- `a11y_perception`: quan sát thông thường chỉ lấy dữ liệu sống khi thấy rõ, dùng `map_memory` khi nhớ, báo chưa biết nếu không có dữ liệu. Dấu âm thanh là ước lượng, không phải tọa độ thực của sinh vật.
- Có trần cấu trúc `4096` node/snapshot, `16` bảng, `16` cột/bảng, `16` ô/node và `128` lựa chọn form. Bảng cực lớn phải phân trang, không được ngầm khẳng định đầy đủ.

## 2. Menu tổng hợp và nguồn lệnh

`GameActionHub` hiển thị theo thứ tự người dùng: **Yêu thích**, (1) Việc có thể làm tại đây, (2) Quan sát và bản đồ, (3) Di chuyển và tư thế, (4) Tương tác với môi trường, (5) Túi đồ và vật phẩm, (6) Ăn uống, nghỉ ngơi và y tế, (7) Trang bị và sửa chữa, (8) Chiến đấu, vũ khí và phép thuật, (9) Chế tạo và xây dựng, (10) Nhân vật và năng lực, (11) Nhiệm vụ và quan hệ, (12) Phương tiện và hành trình, (13) Quản lý khu vực và tự động hóa, (14) Thông tin, trình đọc và trợ giúp, (15) Hệ thống, TTS và lưu game, cuối cùng **Tìm lệnh**.

**Chú ý về số nhóm:** các tệp `COMPREHENSIVE-MENU-GROUP-3/4/5/6/7/15` dùng **vị trí hiển thị có tính Yêu thích là mục đầu**, do đó tên nhóm “3: Quan sát” trỏ đến **ID nhóm nội bộ 2**. Không đồng nhất tên tệp, thứ tự hiển thị và chỉ số nội bộ.

- Nhóm “Việc có thể làm tại đây” chỉ nêu thao tác được engine công bố là khả dụng trong ngữ cảnh.
- Tìm lệnh và Tất cả lệnh đọc danh sách live; yêu thích có thể hiện “Không khả dụng ở màn hình hiện tại”, không được coi là nút luôn có thể kích hoạt.
- Vuốt duyệt không tự thực hiện hành động; chạm hai lần mới gửi lệnh gốc có thể tiêu lượt. Không hứa rằng mọi màn con/mod đặc biệt đều đã có snapshot.

## 3. Cử chỉ ở chế độ trình đọc nội bộ

| Cử chỉ | Ở thế giới bình thường | Trong menu / ngoại lệ |
| --- | --- | --- |
| Vuốt một ngón bốn hướng | Đi bộ theo hướng game chấp nhận | Duyệt danh sách/nhóm; chức năng dọc tùy màn |
| Chạm đôi một ngón | Xác nhận/kích hoạt | Kích hoạt hàng được chọn |
| Chạm ba lần một ngón | Mở menu hành động | Phụ thuộc ngữ cảnh |
| Nhấn giữ một ngón | Mở hành động ngữ cảnh | Mở tùy chọn của mục nếu có |
| Vuốt hai ngón bốn hướng | Quét 16 ô trên ray tương ứng | Có thể là xác nhận/quay lại hoặc lệnh ngữ cảnh |
| Chạm một lần hai ngón | Đọc thông tin mục đang chọn | Đọc detail/document, không kích hoạt |
| Chạm hai lần hai ngón | Đọc thông báo thế giới | Không áp vào mọi màn |
| Chạm ba lần hai ngón | Mở Menu tổng hợp | Không áp vào mọi màn |
| Chạm một lần ba ngón | Đọc tọa độ | Ở thế giới |
| Chạm hai lần ba ngón | Đọc đầy đủ thông tin nhân vật | Ở thế giới |
| Chạm ba lần ba ngón | **Bật/tắt chế độ chiến đấu** | **Cố định, không thể gán lại** |

Cử chỉ của TalkBack do Android giữ có thể không tới bộ nhận cử chỉ nội bộ; khi TalkBack bật, ưu tiên các nút có nhãn và thao tác TalkBack. `ReaderGestureBindings.IDS` gồm **36 mã**, `isReserved` giữ **11 mã**, còn **25 mã** cho phép gán. Các cử chỉ cơ bản khác do `ReaderGestureSurface` nhận trực tiếp không nằm trong danh sách gán. Những ánh xạ vuốt ngang/dọc kiểu thử 45 giây vẫn còn mã tương thích cũ trong preferences nhưng **không còn là tùy chọn điều hướng thực tế của UI hiện hành**.

**Chiến đấu:** khi bật combat mode, một ngón dùng để ra đòn hướng; hai ngón vuốt dùng bước đi; chạm một lần hai ngón đọc danh sách địch do engine công bố, **không phải** đọc thông tin vật phẩm. Chế độ không được bật trong một số trạng thái đi bộ bị cấm (xe, hoạt động, travel...).

## 4. Quan sát, quét và sonar

- `observe_tile`: tôn trọng quyền nhìn thấy/ghi nhớ/không biết; `observe_memory_tile` không tra dữ liệu sống của tầng khác.
- **Quét 16 ô:** `announce_linear_scan` chạy trên đúng một trong bốn hướng Bắc/Nam/Tây/Đông, đọc từng ô 1–16.
- **Ngoại lệ đặc biệt đã triển khai:** `scan_unrestricted_tile` cố ý dùng dữ liệu bản đồ **không qua giới hạn nhìn thấy hoặc trí nhớ**, có thể đọc địa hình, vật phẩm, quái, bẫy hoặc xe sau tường. Đây là quyết định hành vi hiện tại và cần xác nhận chính sách, không gộp nó với `observe_tile`.
- **Danh sách xung quanh:** Android trợ năng quét nửa chiều rộng **20 ô trên cả hai trục**, tối đa 41 × 41 vị trí trong bản đồ đã tải, không phải bán kính 60 của ba tab đồ họa gốc. Danh sách lọc “Tất cả / Sinh vật và NPC / Địa điểm và môi trường / Vật phẩm”, gần trước xa sau; phân trang 80 hàng; không liệt kê mọi ô sàn/cỏ/đường thông thường.
- Khi duyệt mục trong danh sách xung quanh: vuốt ngang chuyển đối tượng, vuốt dọc đổi bộ lọc. Chạm đôi đề nghị đường đi an toàn đến mục (có thể không tìm được đường); không tự nhặt hay dịch chuyển tức thời. Chạm một lần hai ngón trên vật phẩm gửi **`read_detail`**, không di chuyển.
- Sonar hướng là chức năng **tùy chọn**, mặc định tắt, dùng tám đường thẳng dài 16 ô và dữ liệu sinh vật/vật phẩm nhìn thấy. Đây không phải quét xuyên tường và không phải âm thanh vòm chuẩn.

## 5. Đọc chi tiết và chọn số lượng theo PR #7

- `read_detail` chỉ gửi khi người dùng chủ động yêu cầu ở danh sách hỗ trợ. Các selector vật phẩm dùng `item::info(true)` từ item_location đã xác minh còn đúng trong danh sách; các màn khác dùng detail/document do UI gốc công bố.
- Thông tin dài được phân đoạn UTF-8 an toàn trên đường native/Java rồi ghép thành **một yêu cầu đọc**; phản hồi phải thuộc focus và context hiện tại. Không tự nạp, mặc, thả, ném, nhặt hoặc tiêu lượt.
- Vật phẩm một đơn vị: chọn trực tiếp 0/1. Nhiều đơn vị: menu chọn 1–9, Tất cả, Bỏ chọn hoặc nhập số khác; chọn xong quay về danh sách với số lượng áp dụng. **Xác nhận hành động chung** (nhặt, thả, bán...) vẫn tách riêng.
- Bảng số chỉ dùng nơi đã khai báo rõ ngữ cảnh **số lượng vật phẩm**. Không áp tự động cho trường chỉnh tùy chọn, tạo nhân vật, mã nhập hoặc số thường.
- Danh sách xung quanh có thể chứa nhiều món cùng tên; khi đọc, engine tái kiểm tra tên stack/vị trí và đọc các mô tả vật phẩm khác nhau, không ra lệnh tìm đường.

## 6. TTS, cài đặt trợ năng, nhật ký

- `TtsSettingsActivity` hiện có: công tắc TTS; ba danh sách Bộ máy, Ngôn ngữ, Giọng; thanh Tốc độ **0,5–10,0**; Độ cao **0,5–2,0**; Âm lượng 0–100%; **Nghe thử, Hủy, Lưu**. Giao diện không có công tắc cho phép giọng mạng hay nút khôi phục/mở cài đặt Android. Quyền giọng mạng đã lưu được giữ; mã lưu TTS hiện **không bắt buộc đã nghe thử thành công** trước khi nhấn Lưu (nhưng từ chối bật với âm lượng 0).
- `AccessibilitySettingsActivity` gồm công tắc đồ họa thế giới (chế độ chỉ đọc là mặc định của flavor trợ năng), sonar, độ dài/thời gian vuốt và Quay lại. Không tự ý đưa chế độ thử đổi chức năng vuốt cũ vào hướng dẫn hiện hành.
- `DebugLogActivity` gồm **Ghi nhật ký, Xuất nhật ký, Xóa nhật ký**. Mặc định ghi **tắt**; bật/tắt thủ công, không có tự tắt do đạt ngưỡng hiệu năng. Công tắc đồ họa nằm trong cài đặt trợ năng, **không nằm trong màn nhật ký**.
- `SessionDebugLog` tạo một tệp ghi mới cho mỗi lần mở tiến trình; không giữ `crash_recovery.txt` cũ để xuất ở lần sau. **Xuất nhật ký trước khi khởi động lại** nếu muốn lưu. Dừng ghi giữ dữ liệu phiên hiện tại để xuất; Xóa phải được người dùng chủ động yêu cầu.

## 7. Phạm vi và giới hạn nghiệm thu

Có adapter cho tạo thế giới/nhân vật, menu chung, nhập, y tế, chiến đấu, craft, nhiệm vụ, NPC/giao dịch, xe, bionics/phép thuật, minigame, backup và kết thúc phiên. Đây là bằng chứng có **đường mã**, không bảo đảm mọi biến thể đều chạy trọn.

Rủi ro cần khóa bằng thiết bị: TalkBack vs cử chỉ nhiều ngón; điều khiển ở trạng thái combat; tương tác/nhặt đồ lồng nhau; tiếng Việt/giọng engine; quét xuyên tường; hiệu năng di chuyển; snapshot danh sách cực lớn; lưu/tải/phục hồi dữ liệu; những màn không có scope và mod ngoài. Xem [ma trận](VALIDATION-MATRIX.vi.md).

## 8. Điểm vào mã dùng làm nguồn quyết định

`GameActionHub.java`, `GameReaderController.java`, `ReaderGestureSurface.java`, `ReaderGestureBindings.java`, `ReaderPreferences.java`, `TtsSettingsActivity.java`, `AccessibilitySettingsActivity.java`, `DebugLogActivity.java`, `SessionDebugLog.java`, `src/a11y_world.cpp`, `src/a11y_perception.cpp`, `src/surroundings_menu.cpp`, `src/a11y_semantics.h`, `src/inventory_ui.cpp`.

Đặc tả này ghim tại một commit; sau thay đổi chức năng, sửa đặc tả trong **cùng PR** với mã. Không dùng bài test nguồn như chứng nhận khả năng nghe trên điện thoại.
