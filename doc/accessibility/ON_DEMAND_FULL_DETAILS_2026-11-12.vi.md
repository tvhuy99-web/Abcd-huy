# Đọc thông tin gốc đầy đủ bằng một lần chạm hai ngón (PR #7)

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

Mô tả đầy đủ không được tính khi cuộn danh sách. Lệnh native `read_detail` quay về `TIMEOUT` và phát thông tin TTS không qua logic gameplay. Thông tin dài chia thành đoạn giới hạn để không vượt trần kích thước native→Java; Java đưa vào hàng đợi `LONG_TEXT` với thời gian chờ đủ dài.

## Đường đọc cho các đối tượng không phải vật phẩm

Một số UI đã công bố tài liệu đầy đủ thành node `document` riêng nhưng chưa nối trực tiếp đến **hàng** đang chọn. Android nay nối cử chỉ đọc chi tiết mà không đổi luồng thao tác:
- Công thức chế tạo → chi tiết, mô tả, nguyên liệu, điều kiện, sản phẩm phụ, công đoạn và tài nguyên (những node đang có cho công thức đó).
- Xây dựng → chi tiết công trình, điều kiện vị trí, biến thể và các tài liệu yêu cầu.
- Nhiệm vụ và địa điểm quan tâm → tài liệu chi tiết của lựa chọn hiện tại.
- Phe phái/trại/sinh vật, nhiệm vụ đồng đội, thành thạo → tài liệu chi tiết đã công bố cho lựa chọn hiện tại.
- Nếu tiêu điểm đang là `document`, chạm hai ngón đọc qua cơ chế văn bản dài có thể tiếp tục/dừng, không dùng thông báo focus ngắn.

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
8. Chế tạo/công trình/nhiệm vụ: hai ngón trên hàng chọn đọc được tài liệu dài; chạm hai lần vẫn thực hiện lệnh gốc.
9. Bionics, đột biến, y tế, quy tắc và cấu hình: chức năng cử chỉ cũ không bị ảnh hưởng.
10. Chế độ chiến đấu: hai ngón vẫn đọc danh sách kẻ địch.
11. TalkBack hệ thống và TTS nội bộ: kiểm tra đọc tài liệu cực dài, thao tác dừng, đọc lại sau khi lời đọc bị ngắt, các dấu định dạng.
12. Mất tiêu điểm, thay đổi danh sách khi đang đọc, hủy/quay lại: tuyệt đối không kích hoạt đối tượng khác.

Các kiểm tra Python chỉ xác minh hợp đồng mã nguồn. Cần biên dịch C++/Java và thử APK sau khi có quy trình thử phù hợp; không tự động tạo APK trong đợt này.
