# Chế độ chỉ đọc thế giới và nhật ký hiệu năng tự dừng (PR #7)

## Mục tiêu

Bản Android **accessibility** ưu tiên phản hồi vuốt, TTS và dữ liệu trợ năng.
Không được rút ngắn lượt chơi bằng cách bỏ qua AI, thời tiết, bản đồ,
Safe Mode hoặc thời gian di chuyển. Khi không cần hình ảnh bản đồ, tránh gọi
đường vẽ SDL/ImGui không cần thiết trước mỗi hành động đi bộ.

## Bật/tắt tại điện thoại

Từ menu chính vào **Nhật ký gỡ lỗi**, trong đó có:

- **Chỉ đọc bằng giọng nói, không dựng hình bản đồ (ưu tiên tốc độ)**:
  mặc định bật trên biến thể APK `accessibility`. Bật để bỏ qua đường vẽ
  thế giới thông thường; tắt để khôi phục hiển thị đồ họa.
- **Đo hiệu năng tự động, tự tắt khi di chuyển ổn định**:
  mặc định bật trong bản `accessibility` khi chưa từng chọn. Bật thủ công
  để bắt đầu một đợt đo mới. Tắt để dừng đo và các dòng log tần suất cao.
- **Ghi nhật ký**: công tắc nhật ký chung **độc lập** với profiler.
  Nhật ký thông thường giữ thông tin lỗi, vòng đời và sự kiện cần điều tra;
  profiler điều khiển phần đo chi tiết, không tự xóa nhật ký đã ghi.

Hai tùy chọn trên lưu trong `internal_reader` (SharedPreferences), có hiệu
lực khi quay về màn hình game và được giữ sau khi mở lại ứng dụng.
Chỉ bản có trình đọc đang bật và ở foreground mới bỏ dựng hình thế giới.

## Những gì được bỏ qua và những gì vẫn giữ

- Vòng lặp chơi ở màn hình thế giới không yêu cầu vẽ lại tất cả UI trước
  mỗi lần chờ thao tác; cũng không chạy vòng hoạt ảnh, mưa/bụi, minimap.
- Nếu có callback vẽ thế giới do màn hình khác yêu cầu, `game::draw()`
  vẫn cập nhật `build_map_cache`, `update_visibility_cache` và cảm biến
  định hướng rồi bỏ qua `draw_ter`, hiệu ứng và panel khi chế độ chỉ đọc bật.
- Trước khi dựng bản dữ liệu cho trình đọc (`make_reader_snapshot`),
  đường chỉ đọc bảo đảm `build_map_cache` và `update_visibility_cache`.
  Điều này **không** được bỏ: danh sách vật phẩm, quái vật và tầm nhìn của
  TTS phải tuân theo luật quan sát bình thường.
- Menu, lựa chọn vật phẩm, hộp thoại và UI Android vẫn hoạt động. SDL và
  vòng đời renderer vẫn tồn tại để xử lý input, lỗi và chuyển màn hình.
- Không sửa logic `monmove`, `do_regular_action`, di chuyển bình thường,
  trừ điểm hiển thị. Không tự tắt Safe Mode.

Đây là chế độ cắt phần hiển thị, không phải bản chuyển game sang một engine
không có SDL. Một số repaint từ menu hay cơ chế renderer vẫn có thể xảy ra.

## Đo hiệu năng thích ứng, tự dừng

Khi profiler bật, native phát các sự kiện:

| Tên sự kiện | Dữ liệu |
| --- | --- |
| `WORLD_MOVE_RESULT` | Lệnh, tọa độ trước/sau, đã đổi ô, điểm di chuyển, thời gian hành động |
| `WORLD_DRAW_TIMING` | Thời gian cập nhật UI và `mode=headless_skip` hoặc chế độ vẽ |
| `WORLD_RENDER_BREAKDOWN` | Chi phí cụ thể của cache, visibility, sonar, địa hình và panel nếu có vẽ |
| `WORLD_VISION_TIMING` | Chi phí cập nhật map cache và visibility phục vụ TTS |
| `WORLD_SIM_TIMING` | Tổng mô phỏng, cache, monster/NPC, phần cập nhật nhân vật |
| `WORLD_READER_TIMING` | Tạo snapshot và gửi snapshot |
| `WORLD_WALK_PROFILE` | Đi bộ trong bản đồ / cập nhật bản đồ / hoàn tất lượt |
| `WORLD_PLACE_PROFILE` | Đặt nhân vật vào vị trí và cập nhật map |
| `WORLD_SHIFT_PROFILE` | Dịch submap, cache, NPC, spawn, reveal |
| `WORLD_INPUT_TIMING` | Thời gian từ nhận JNI đến native poll |
| `WORLD_JAVA_TIMING` | Đưa dữ liệu qua Java, parse, cập nhật UI |
| `WORLD_PROFILE_STATE` | Profiler tự dừng và ngưỡng được dùng |

Để **tự dừng**, phải có **48 lần di chuyển liên tiếp thực sự đổi tọa độ**
trong khi: thời gian cập nhật đồ họa của lượt quan sát gần nhất không quá
40 ms; thời gian mô phỏng được đo gần nhất không quá 120 ms;
làm mới tầm nhìn không quá 60 ms và cập nhật dữ liệu trình đọc không quá
35 ms; không gặp lệnh thế giới bị từ chối vì busy; không chuyển vùng
bản đồ; không gặp bước đi không đổi vị trí. Đây là **điều kiện dừng đo mang tính kỹ thuật**,
không phải khẳng định APK đã đạt một mức hiệu năng nào đó. Khi game còn
chậm hoặc có cú vuốt thất bại thì profiler tiếp tục đo.

Native phát `WORLD_PROFILE_STATE state=auto_paused ...`, sau đó Java lưu
`adaptive_performance_profiling=false`, dừng các đồng hồ đo tần suất cao và
ngừng ghi những dòng trace thường xuyên như SNAPSHOT/GESTURE/WORLD_*.
Nhật ký sẵn có vẫn giữ nguyên, sự cố hoặc lỗi quan trọng vẫn có thể được
ghi khi chế độ nhật ký chung đang bật. Lần mở app sau không tự bật profiler
lại. Để điều tra lỗi mới, vào Nhật ký gỡ lỗi và bật lại profiler.

Khi profiler tắt, các đường hot-path không tạo chuỗi nội dung timing hoặc
gửi chúng qua JNI. Một số sự kiện trạng thái/lỗi thông thường vẫn diễn ra;
điều này được chủ ý giữ riêng để tránh mất dấu crash.

## Kịch bản kiểm tra thực tế

1. Đứng ở 2 ô trống, kiểm tra mô tả **Ô hiện tại** và hướng quét; chặn bằng
   Safe Mode phải có cùng quy tắc như bản cũ.
2. Vuốt 30 lần đi/đi lại trong vùng bản đồ không dịch submap. So sánh
   `WORLD_MOVE_RESULT`, `WORLD_DRAW_TIMING`,
   `WORLD_VISION_TIMING`, `WORLD_SIM_TIMING` và thời gian từ
   `INTENT` đến `SCREEN` trong hai cấu hình.
3. Đi qua ranh giới submap; kiểm tra `WORLD_SHIFT_PROFILE` và chắc chắn
   không coi lượt nặng là ổn định.
4. Vào menu mở vật phẩm, hộp thoại, bản đồ, danh sách xung quanh; TTS phải
   phát đầy đủ và menu phải dùng được dù không dựng hình bản đồ.
5. Nếu profiler chưa tự dừng vì vẫn có lượt > ngưỡng, **không** tắt cưỡng
   bức. Nếu đã tự dừng, mở lại Nhật ký gỡ lỗi để bật đo đợt mới.

## Giới hạn kiểm chứng

Kiểm thử Python trong nguồn xác nhận đường nối và điều kiện bảo vệ.
Hiệu năng, lỗi rendering, tầm nhìn và TalkBack trên OnePlus cần nhật ký
thiết bị sau khi build ARM64 và thử nghiệm. Không coi kiểm thử cấu trúc là
bằng chứng đã đạt mục tiêu dưới 100 ms/bước.
