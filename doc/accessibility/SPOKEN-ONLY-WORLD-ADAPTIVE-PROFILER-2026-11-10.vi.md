# Chế độ chỉ đọc và nhật ký chẩn đoán thủ công (PR #7)

> Lưu ý: tài liệu này thay thế hoàn toàn thiết kế cũ từng dùng cơ chế
> "đo thích ứng, tự tắt". **Không còn bất kỳ chế độ tự tắt hoặc tự bật nhật ký
> dựa trên kết quả hiệu năng nào.**

## Nguyên tắc người dùng quyết định

1. **Mỗi lần khởi động game tạo một phiên nhật ký hoàn toàn mới.**
   Tệp chẩn đoán của phiên trước bị xóa/trống khi khởi động phiên mới.
   Không ghép `crash_recovery.txt`, `debug.log` hoặc nhật ký phiên khác
   vào tệp xuất. Cần xuất nhật ký của phiên hiện tại trước khi mở game lại.
2. **Ghi nhật ký: Bật.** Ghi toàn bộ sự kiện do bộ ghi chẩn đoán gửi tới,
   đo chi phí hành động, mô phỏng thế giới, xử lý tầm nhìn, TTS, nhập liệu,
   dựng hình và chuyển vùng bản đồ. Không lấy mẫu theo chu kỳ 8/16 bước
   và không tự ngừng vì máy đã nhanh.
3. **Ghi nhật ký: Tắt.** Không ghi thêm bất kỳ sự kiện nào, kể cả lỗi,
   cảnh báo game bận, TTS hoặc crash. Các đồng hồ đo hiệu năng bổ sung
   do chúng ta thiết kế không được khởi động; native không chuyển sự kiện
   chẩn đoán qua JNI. Tình trạng tắt vẫn có hiệu lực khi mở game sau.
4. **Bật giữa phiên:** bắt đầu ghi tại đúng thời điểm người dùng bật.
   Không hồi tố các sự kiện trước khi bật. Bật/tắt nhiều lần trong một
   lần mở game chỉ có một nhật ký, với những khoảng trống là lúc người
   dùng đã tắt ghi.
5. **Tắt giữa phiên:** dữ liệu đã ghi được giữ để người dùng chọn **Xuất
   nhật ký** nếu muốn. Khi tắt, không tự xuất hoặc tự xóa dữ liệu.
   **Xóa nhật ký** là thao tác riêng do người dùng chủ động xác nhận.
6. **Mặc định:** nếu chưa từng lựa chọn, công tắc **Ghi nhật ký** tắt.
   Lựa chọn thủ công được giữ trong SharedPreferences qua các lần mở game.
   Không có công tắc profiler thứ hai.

## Điều khiển

Vào **Nhật ký gỡ lỗi** trong menu chính. Có:
- **Ghi nhật ký:** công tắc thủ công duy nhất điều khiển mọi phép đo.
- **Chỉ đọc bằng giọng nói, không dựng hình bản đồ:** độc lập với nhật ký;
  vẫn bật mặc định trong biến thể APK trợ năng.
- **Xuất nhật ký:** chỉ xuất tệp của lần mở game hiện tại.
- **Xóa nhật ký:** xóa những gì đã ghi của phiên hiện tại.

Khi thay đổi công tắc ghi, Java cập nhật cờ ghi ngay và gửi cờ
bật/tắt đo vào native qua JNI mà không đợi màn hình game mở lại.
Native cũng nhận lại giá trị khi game tiếp tục foreground.

## Những gì còn hoạt động trong chế độ chỉ đọc

- Xử lý lượt, quái vật, Safe Mode và tính toán gameplay không bị bỏ.
- Bộ nhớ đệm bản đồ và tầm nhìn phục vụ TTS vẫn cập nhật trước snapshot.
- Sonar thẳng hướng tiếp tục đọc vị trí có thể quan sát theo chu kỳ 300 ms.
- Menu, hộp thoại, điều khiển và cơ chế lỗi renderer vẫn có thể hoạt động.
- Tắt dựng hình không đồng nghĩa tắt SDL hay phá chức năng game.

## Sự kiện khi người dùng bật nhật ký

| Sự kiện | Nội dung |
| --- | --- |
| `SESSION` | Mở game / bắt đầu ghi, mã phiên và bản ứng dụng |
| `GESTURE_ACTION`, `INTENT_RESULT` | Vuốt và việc native có chấp nhận lệnh không |
| `WORLD_INPUT_TIMING` | Thời gian từ JNI tới lần native poll |
| `WORLD_MOVE_RESULT` | Trước/sau tọa độ, bước thực sự thành công, điểm di chuyển, thời gian |
| `WORLD_STEP_PIPELINE` | Sau hành động đến khi bản dữ liệu đọc tiếp theo được xuất bản, kèm các công đoạn |
| `WORLD_DRAW_TIMING` | Thời gian đường giao diện; chế độ vẽ đầy đủ hoặc bỏ qua |
| `WORLD_RENDER_BREAKDOWN` | Cache, visibility, sonar, địa hình, các panel |
| `WORLD_VISION_TIMING` | Cache và tầm nhìn riêng cho chế độ chỉ đọc |
| `WORLD_SIM_TIMING` | Mô phỏng, bộ nhớ đệm, lượt quái vật/NPC |
| `WORLD_READER_TIMING`, `WORLD_JAVA_TIMING` | Tạo/đăng snapshot, phân tích và cập nhật giao diện Android |
| `WORLD_WALK_PROFILE`, `WORLD_PLACE_PROFILE`, `WORLD_SHIFT_PROFILE` | Đi bộ, đặt nhân vật và dịch chuyển submap |
| `TTS_REQUEST`, `TTS_EVENT`, `TTS_STATE` | Yêu cầu nói, độ trễ, trạng thái giọng nói |

Các phép đo bổ sung chỉ chạy khi bật, nên có thể tạo ra chút độ trễ do
việc ghi tệp và đo. Tắt để chơi bình thường mà không phải trả chi phí
chẩn đoán. Tệp được giới hạn 5 MiB; nếu ghi quá nhiều, những dòng đầu
tiên có thể bị loại để bảo toàn dung lượng. Xuất trước khi tắt/mở game
lại nếu cần nghiên cứu một phiên đầy đủ.

## Các kịch bản hồi quy cần kiểm tra trên APK ARM64

1. Mở game lúc ghi tắt: không có tệp ghi mới phát sinh; mọi hoạt động
   TTS/di chuyển vẫn bình thường.
2. Bật ghi ở màn hình Nhật ký gỡ lỗi, quay lại và đi vài ô: các mục
   `WORLD_MOVE_RESULT`, `WORLD_STEP_PIPELINE`, `WORLD_SIM_TIMING`
   phải xuất hiện ngay cho mọi bước được xử lý.
3. Tắt ghi khi đang chơi: số dòng/tệp không tăng thêm dù tiếp tục vuốt,
   TTS đọc, bị chặn bởi Safe Mode hoặc gặp lỗi. Không tự bật lại.
4. Xuất sau khi đã tắt: giữ đúng những dòng đã ghi trong phiên đó.
5. Bật lần nữa trong cùng phiên: tiếp tục ghi nối vào nhật ký hiện tại.
6. Đóng game, mở lại: tệp mới không có dữ liệu từ phiên trước; nếu
   công tắc vẫn bật, phiên mới bắt đầu ghi từ đầu; nếu tắt, hoàn toàn
   không ghi.
7. Kiểm tra chế độ chỉ đọc: không mất sonar, thông tin tầm nhìn, menu,
   hộp thoại hoặc cử chỉ.

CI tĩnh kiểm tra đường nối và giới hạn. Hiệu quả thực tế trên OnePlus
cần nhật ký phiên thử mới; không mặc định rằng đã đạt mức dưới 100 ms/bước.
