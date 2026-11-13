# Đọc thông tin gốc đầy đủ bằng một lần chạm hai ngón (PR #7)

> **GHI CHÚ PHIÊN BẢN (13/11/2026):** Đây là hồ sơ chuyên đề theo thời điểm viết, không tự động là chỉ dẫn cho `master` sau PR #7. Khi cử chỉ, menu, quyền đọc bản đồ hoặc CI khác, ưu tiên [đặc tả hiện hành](CURRENT-REFERENCE.vi.md), [hướng dẫn](USER-GUIDE.vi.md) và [trạng thái](STATUS.md). **Không coi mã đã viết là nghiệm thu TalkBack trên máy thật.**

## Nguyên tắc thiết kế

- **Vuốt duyệt:** chỉ đọc tên và dữ liệu ngắn như trước, không mở menu hay tính thông tin chi tiết của cả danh sách.
- **Một lần chạm hai ngón:** yêu cầu thông tin đầy đủ của mục đang chọn. Chỉ đọc, không chọn số lượng, nhặt, dùng, ném, di chuyển hoặc tiêu hao lượt.
- **Chạm hai lần:** giữ nguyên hành động gốc của từng màn hình.
- **Dữ liệu:** lấy từ engine và phần thông tin thật của UI gốc, không tự tạo thông số hoặc tự dịch số liệu.
- **Ngữ cảnh:** cử chỉ hai ngón riêng của chế độ chiến đấu và Menu tổng hợp không bị đổi.

## Đường đọc cho vật phẩm

Các màn chọn vật phẩm dùng chung `inventory_selector::reader_session` (túi đồ, nhặt, thả, cất, lấy, chọn để mặc/cầm/dùng/sửa/tháo, trao đổi...) công bố quyền đọc `read_detail` trên từng hàng. Android chỉ gửi quyền này khi người dùng chủ động yêu cầu. Native đối chiếu `item_location` **với danh sách hiện tại** rồi mới gọi `item::info(true)`, giống nguồn dữ liệu của cửa sổ Examine.

Các ô số lượng còn giữ `set_value`, nút chọn/nhập vẫn giữ `activate`. Ngay cả vật phẩm bị khóa chọn vì điều kiện của hoạt động cũng được phép **đọc**, không được phép kích hoạt bằng đường đọc.

Ngăn chứa không niêm phong:
- Hàng vật phẩm nằm trong ngăn chứa cũng hỗ trợ đọc; không cần lấy đồ ra trước.
- Vật phẩm trong giao diện nhặt đồ được xác minh lại đúng ngăn, đúng vật chủ và vẫn tồn tại.
- Không đi vòng qua pocket đang niêm phong; không tự lộ thêm nội dung ngoài những hàng đã công bố.

Các màn hình độc lập:
- Kho nâng cao (`advanced_inv.cpp`): xác minh vị trí món đồ trong pane nguồn đang sống, rồi đọc.
- Sắp xếp lớp mặc (`armor_layers.cpp`): xác minh iterator thuộc danh sách áo giáp hiện mặc và tab đang xem, rồi đọc.
- Danh sách vật phẩm gần (`surroundings_menu.cpp`): kiểm tra lại nhãn stack của đúng ô trong kết quả quét hiện tại. Với nhiều vật phẩm trùng tên, chỉ đọc một lần cho từng bộ thông số khác nhau; không kích hoạt lệnh đi tới.

Mô tả đầy đủ không được tính khi cuộn danh sách. Lệnh native `read_detail` quay về `TIMEOUT` và phát thông tin TTS không qua logic gameplay. Thông tin dài chia thành các mảnh UTF-8 an toàn để không vượt trần kích thước native→Java. **Android ghép các mảnh thành một tài liệu trước khi phát TTS**, tránh bị mất đoạn do giới hạn số thông báo trong hàng đợi. Kết quả chỉ được phát nếu người chơi vẫn đang ở đúng mục và đúng màn hình đã yêu cầu.

## Đường đọc cho các đối tượng không phải vật phẩm

Một số UI đã công bố tài liệu đầy đủ thành node `document` riêng nhưng chưa nối trực tiếp đến **hàng** đang chọn. Android nay nối cử chỉ đọc chi tiết mà không đổi luồng thao tác:
- Công thức chế tạo → chi tiết, mô tả, nguyên liệu, điều kiện, sản phẩm phụ, công đoạn, tài nguyên **và bảng thông tin đầy đủ của vật phẩm thành phẩm** từ `recipe_result_info` trong giao diện gốc. Không bắt đầu chế tạo.
- Xây dựng → chi tiết công trình, điều kiện vị trí, biến thể và các tài liệu yêu cầu.
- Nhiệm vụ và địa điểm quan tâm → tài liệu chi tiết của lựa chọn hiện tại.
- Phe phái/trại/sinh vật, nhiệm vụ đồng đội, thành thạo → tài liệu chi tiết đã công bố cho lựa chọn hiện tại.
- Chủ đề **trợ giúp** → đọc toàn bộ các đoạn nguyên bản với phép thế biến của game; không cần mở chủ đề hay đánh dấu "đã đọc".
- **Trang nhật ký** → đọc nội dung trang hiện được chọn, không tự chuyển sang chế độ sửa.
- **Nhặt đồ ngay tại chỗ** (`pickup-here`) → đọc toàn bộ thông tin của từng món khác biệt trong một hàng gộp tên, không đánh dấu chọn hay nhặt.
- Nếu tiêu điểm đang là `document`, chạm hai ngón đọc qua cơ chế văn bản dài có thể tiếp tục/dừng, không dùng thông báo focus ngắn.
- Các màn công thức, nhiệm vụ, công trình và đối tượng có tài liệu phụ chỉ đọc sau khi xác nhận **mục được chọn ở engine trùng với tiêu điểm Android**; tránh đọc thông tin của mục trước khi vuốt quá nhanh.

Những màn đã ghi `node.detail` đầy đủ tại hàng (y tế, kỹ năng, bionics, đột biến, quản lý vùng, menu chung, v.v.) giữ nguyên đường đọc. Không ép chúng dùng `item::info()` vì chúng không phải vật phẩm.

## Giới hạn và kiểm thử

**Đây là thay đổi mã nguồn, chưa phải chứng nhận trải nghiệm APK/TalkBack.** Không tuyên bố mọi giao diện/mod đã đủ 100% chỉ nhờ có adapter. Các màn hiếm và mod ngoài cần kiểm thử từng quy trình. Phần thông tin chưa tồn tại trong dữ liệu gốc không được bịa.

Kiểm thử tĩnh: `python3 -m unittest tools.accessibility.test_full_detail_on_demand`.

Danh sách tình huống cần thử với người dùng máy thật:
1. Túi đồ: súng (tầm bắn/sát thương/đạn), dao, áo giáp, thức ăn, thuốc, sách, dụng cụ, chai, vật liệu.
2. Nhặt và thả đồ: chạm hai ngón chỉ đọc, không tăng số đã chọn, không làm mất đồ.
3. Đưa đồ vào ngăn chứa: đọc vật được chọn ngay cả khi bị từ chối chứa; xác nhận vẫn tách biệt.
4. Balo có nhiều lớp ngăn, nhiều món trùng tên, ngăn niêm phong, vật phẩm đang bị đổi vị trí.
5. Kho nâng cao: pane nguồn/đích, đọc không di chuyển stack.
6. Sắp xếp lớp mặc: đọc đúng áo ở đúng tab, không thay thứ tự.
7. Danh sách xung quanh: một món và nhiều món cùng tên; đọc không tự đi đến món.
8. Chế tạo/công trình/nhiệm vụ: hai ngón trên hàng chọn đọc được tài liệu dài; chế tạo có thêm thông số thành phẩm; chạm hai lần vẫn thực hiện lệnh gốc.
9. Bionics, đột biến, y tế, quy tắc và cấu hình: chức năng cử chỉ cũ không bị ảnh hưởng.
10. Chế độ chiến đấu: hai ngón vẫn đọc danh sách kẻ địch.
11. TalkBack hệ thống và TTS nội bộ: kiểm tra đọc tài liệu cực dài, thao tác dừng, đọc lại sau khi lời đọc bị ngắt, các dấu định dạng.
12. Mất tiêu điểm, thay đổi danh sách khi đang đọc, hủy/quay lại: tuyệt đối không kích hoạt đối tượng khác.
13. Nhặt tại chỗ: các hàng số lượng một món và nhiều món, cùng tên khác tình trạng; đọc không thay đổi số đã chọn.
14. Trợ giúp: chạm hai ngón trên chủ đề chưa đọc, kiểm tra nội dung đầy đủ nhưng vẫn giữ trạng thái chưa đọc.
15. Nhật ký: chọn trang bất kỳ và đọc nội dung mà không vào ô sửa.
16. Mô tả dài hơn 11 KB và ký tự UTF-8 tiếng Việt qua ranh giới mảnh dữ liệu; kiểm tra không mất đoạn.

Các kiểm tra Python chỉ xác minh hợp đồng mã nguồn. Workflow hiện có của nhánh có thể tự động biên dịch APK để kiểm tra mã sau mỗi lần đẩy commit, nhưng không yêu cầu người chơi cài đặt APK sau mỗi lần sửa. Việc biên dịch Java/C++ và thử TalkBack thật phải được xác nhận riêng trước khi kết luận hoàn tất.

## Kết quả rà soát đợt hai

- Đã rà lại đường đọc thông tin cho túi đồ, các bộ chọn vật phẩm dùng chung, ngăn chứa, kho nâng cao, lớp mặc, danh sách xung quanh, nhặt đồ tại chỗ, chế tạo, xây dựng, nhiệm vụ, phe phái, nhiệm vụ đồng đội, thành thạo, trợ giúp, nhật ký và các văn bản dài.
- Đã sửa tình trạng tài liệu phụ vẫn là của mục cũ sau thao tác vuốt nhanh.
- Đã sửa việc đọc dài theo nhiều thông báo riêng có nguy cơ rơi mất đoạn. JNI vẫn giữ trần từng mảnh; Java ghép thành một tài liệu.
- Đã bảo vệ việc đọc ở mục bị vô hiệu hóa về khả năng **chọn**: quyền đọc tách biệt khỏi quyền kích hoạt.
- Bài kiểm tra hồi quy mở rộng nằm ở `tools/accessibility/test_full_detail_on_demand.py` và `tools/accessibility/test_one_step_quantity_selection.py`.
- Không suy diễn rằng mọi cửa sổ hoặc mod đều tương đương chỉ từ kiểm tra tĩnh. Quyền truy cập TalkBack thực tế, biên dịch C++ trên Android và khả năng phát hết những văn bản bất thường vẫn cần xác nhận qua quy trình kiểm thử.

