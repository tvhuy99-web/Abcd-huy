# Tăng tốc tải dữ liệu Android và đọc ngay thông báo game

> **GHI CHÚ PHIÊN BẢN (13/11/2026):** Đây là hồ sơ chuyên đề theo thời điểm viết, không tự động là chỉ dẫn cho `master` sau PR #7. Khi cử chỉ, menu, quyền đọc bản đồ hoặc CI khác, ưu tiên [đặc tả hiện hành](CURRENT-REFERENCE.vi.md), [hướng dẫn](USER-GUIDE.vi.md) và [trạng thái](STATUS.md). **Không coi mã đã viết là nghiệm thu TalkBack trên máy thật.**

## Vấn đề thực tế

Nhật ký người chơi `cataclysm-session-debug.txt`, phiên bản `9ea347b`, cho thấy lần tiếp tục bản lưu mất khoảng 6 phút 17 giây. `A11Y_LOAD_TIMING` báo `core_ms=49`, `mods_ms=361699` và `save_total=15566`. Các khoảng đợi dài thuộc về hoàn tất hoặc xác minh dữ liệu, nhưng nhật ký trước đây chưa có thời gian riêng từng nhóm nên **chưa thể kết luận nhóm nào chiếm bao nhiêu CPU**.

Lúc **00:33:52.593 UTC**, game hủy bản mô tả màn hình (`SNAPSHOT bytes=17`) để thực thi `open`. Lúc **00:33:52.596**, engine đã phát `GAME_SPEECH priority=1 text=Gần đây không có gì mở được.` nhưng **không có `TTS_REQUEST` tương ứng**. Lúc **00:33:52.728**, snapshot thế giới xuất hiện lại. Vì Java trước đó từ chối thông báo khi `snapshot == null`, câu bị bỏ mất; thao tác chạm hai ngón đọc lại lịch sử mới nghe được nó.

## Sửa đường đọc thông báo

- Thông báo game mới (không phải tiến độ tải) dùng **GAME_EVENT**, ưu tiên bằng lời đọc mục đang chọn, vượt lời đọc nền và được yêu cầu phát ngay; cảnh báo khẩn cấp vẫn cao hơn.
- Khi snapshot tạm thời không có trong lúc thực hiện lệnh, giữ tối đa 8 thông báo trong hàng đợi giới hạn, phát lại **sau** `speech.setContext(newSnapshot)`, không mất câu ở giai đoạn chuyển màn hình.
- Thông báo đợi quá 15 giây, vượt quá 8 mục, hoặc vượt ranh giới vòng đời ứng dụng sẽ bị loại theo quy tắc rõ ràng; các loại bỏ được ghi `GAME_MESSAGE_DROPPED`. Không phát lời cũ sau khi tắt/mở màn hình hay đổi ứng dụng.
- TalkBack vẫn nhận yêu cầu sự kiện trợ năng. Thông báo khi đọc lại bằng hai ngón hai lần vẫn mới nhất trước, cũ nhất sau.

## Sửa đường tải dữ liệu

- Android mặc định bật `SKIP_VERIFICATION=true` và tự ghi cài đặt này một lần nếu thiết bị cũ lưu `false`. Bước kiểm tra tính nhất quán sau finalize được bỏ qua; **không bỏ qua phân tích cú pháp JSON, kiểm tra mod đã nạp, hay ngoại lệ dữ liệu khi load**.
- Nhóm finalize và verify có `A11Y_LOAD_GROUP phase=finalize/verify name=... ms=...`. Các mod có `A11Y_LOAD_PACK phase=content/interaction id=... version=... ms=...`; dữ liệu mod riêng của thế giới và tổng thời gian finalize cũng được đo.
- Tận dụng bộ đệm **FlexBuffer JSON có sẵn**. Sửa khóa chỉ mục sai dùng đường dẫn tuyệt đối khiến tệp vừa ghi không thể được tìm lại trong cùng tiến trình. Dùng định dạng tên đệm **v2**, và đối chiếu đường dẫn nguồn, thời gian sửa, kích thước tệp JSON trước khi tải tệp đệm. Khi mod cập nhật làm thay đổi JSON hoặc `modinfo.json`, đệm bị hủy và phân tích lại; cache cũ không có metadata tương ứng sẽ được tái tạo ở lần tải đầu tiên.
- Đo `A11Y_JSON_CACHE hits/misses/writes` để kiểm tra thực tế cache có được dùng hay không.
- Trên Android, màn hình tải giới hạn redraw nặng không quá khoảng 8 lần/giây khi hàng loạt bước hoàn tất quá nhanh; vẫn bơm sự kiện, quản lý renderer và gửi thông báo trợ năng khi cần.
- **Không lưu thẳng dữ liệu công thức, vật phẩm, bản đồ đã hoàn tất**. Các đối tượng phụ thuộc thứ tự mod, phiên bản, cài đặt thế giới và được tái xây dựng khi chuyển nhân vật. Chỉ tái sử dụng **dữ liệu JSON đã phân tích cú pháp** sau khi xác minh nguồn. Không sửa bản lưu, xóa mod, hay giả định cache máy khác có thể tái sử dụng.

## Rủi ro và cách kiểm chứng

Bỏ qua xác minh có thể trì hoãn phát hiện lỗi mod đến khi chơi, nên đây là lựa chọn tăng tốc chủ đích theo yêu cầu người dùng. Với bản APK mới cần thử hai lần tải cùng bản lưu: lần đầu có thể chậm hơn do cache v2 được tạo lại; lần thứ hai phải báo thêm số hit/miss. So sánh các dòng `A11Y_LOAD_GROUP`, `A11Y_LOAD_PACK`, `A11Y_JSON_CACHE` để biết còn chậm ở bước nào. Chỉ khi có kết quả trên thiết bị mới xác nhận được mức tăng tốc, không hứa chắc một con số.

Cần kiểm thử mở cửa nơi không mở được, các tình huống bị chặn và thông báo trong lúc chuyển snapshot, cả giọng nội bộ và TalkBack.
