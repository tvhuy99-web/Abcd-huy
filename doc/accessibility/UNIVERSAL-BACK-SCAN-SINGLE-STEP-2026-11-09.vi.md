# Kiểm tra hồi quy bốn vấn đề trợ năng, 09-11-2026

Nhánh: `perf/reader-swipe-latency-20261109`.

## 1. Quay lại nhất quán

- Cùng một tác vụ `cancel` từ hai ngón kéo xuống, nút Back Android,
  nút Back trong giao diện và trình đọc phải đi vào cùng cửa xử lý.
- `a11y::poll_input` ưu tiên bộ xử lý riêng của màn hình; nếu bộ
  xử lý không xử lý, nó dùng chính các hành động hủy `QUIT`,
  `UILIST.QUIT`, `TEXT.QUIT` hoặc `CANCEL` đã được đăng ký bởi
  input_context **trên cùng**. Không tự tạo hành động QUIT trên
  màn hình không cho phép hủy.
- Menu chính giữ hai cấp. Địa điểm: chọn phương thức đi → danh sách
  địa điểm → thoát. Hộp thoại Yes/No vẫn ưu tiên No, tuyệt đối không
  chấp thuận một hành động nguy hiểm.
- Khóa nhận diện cử chỉ không phụ thuộc revision snapshot; vẫn chứa
  session/frame/generation và loại menu. Thay màn hình thật sẽ hủy
  cử chỉ chưa hoàn tất.
- Kiểm tra trực tiếp: chế tạo, trang bị, hành trang/túi chứa, nhật ký,
  bản đồ, tìm địa điểm, chọn hướng nhìn lén, giao tiếp, menu chính,
  nhập số/văn bản, bảng thông báo. Kiểm thử cả TalkBack và trình
  đọc riêng; không chỉ dựa trên việc mã có `send("cancel")`.

## 2. Hai chế độ quét cùng dữ liệu

- `scan_unrestricted_tile` là bộ đọc bản đồ chỉ đọc trạng thái, chạy
  trên luồng game, dùng chung cho quét thẳng 16 ô và quét xung quanh
  trong phạm vi 20 ô mỗi trục (41 × 41 vị trí).
- Quét thẳng phát thông tin từng ô theo đúng thứ tự.
- Quét xung quanh chỉ tạo **dòng có ý nghĩa**, gồm nội thất (kể cả
  tủ/chạn thường), vật phẩm, sinh vật, bẫy, phương tiện, dấu âm thanh,
  vùng nguy hiểm và địa hình đặc biệt. Không thêm các ô chỉ có
  sàn/cỏ/đường đi thông thường; không thêm tường thường nếu không
  có tính năng tương tác đặc biệt.
- Phân trang 80 dòng, khoảng cách từ gần đến xa. Không âm thầm bỏ
  tủ chỉ vì không có cờ `noteworthy` hoặc bị tường che.
- Mọi dữ liệu thô đều được làm sạch mã màu; các vật phẩm trên cùng
  ô được đọc với nhãn "Vật phẩm:", **không suy đoán là ở bên trong
  tủ** nếu chưa có bằng chứng từ dữ liệu về quan hệ vật chứa.
- Kiểm tra trực tiếp: đứng một chỗ, đặt tủ ở Tây 9 ô, chắn tường,
  quét Tây 16 ô và quét mọi thứ xung quanh 20 ô. Tủ và tên vật phẩm
  phải có trong cả hai. Các ô sàn trống không được tạo hàng.

## 3. Di chuyển không chạy chậm sau khi đã dừng vuốt

- Native JNI chỉ chấp nhận một bước đi bộ (8 hướng) khi game thread
  đã công bố đang **chờ đầu vào thế giới**. Ngay khi một bước được
  chấp nhận, nó khóa cổng nhận bước kế tiếp.
- Khi game đang xử lý một lượt, yêu cầu tiếp theo bị trả về ngay với
  `accepted=false`, không tích lại để thực hiện sau. Java phân biệt
  sự kiện `WORLD_STEP_BUSY` với lỗi chọn menu cũ. TTS thông báo
  trạng thái bận được hạn chế tối đa một lần trong 2,5 giây.
- Chỉ áp dụng giới hạn này cho token phiên thế giới hiện hành. Lệnh
  hướng trong các menu khác vẫn dùng đường input hiện có.
- Mọi hành động đi bộ vẫn cần C++ xác minh token, quyền đi bộ, ID,
  hoạt động cho phép trên snapshot hiện tại.
- Không tạo luồng sửa map off-thread, không phát cú vuốt giả, không
  tự đi nhiều ô khi game kết thúc xử lý.
- Kiểm tra trực tiếp: 20 cú vuốt rời rạc lúc game sẵn sàng, sau đó
  thử vuốt nhanh trong thời gian game bận. Đếm `INTENT_RESULT`,
  `WORLD_STEP_BUSY`, `WORLD_INPUT_TIMING` và `NATIVE_ACTION`.
  Những cú vuốt bị từ chối không được xuất hiện muộn dưới
  `NATIVE_ACTION`. Trường hợp tường cản vẫn là bước được game
  xử lý nhưng không đổi vị trí.

## Giới hạn chưa thể kết luận bằng kiểm thử nguồn

- Chưa thể kết luận thao tác đa ngón thực tế trên mẫu máy OnePlus
  trước khi cài APK mới và đọc nhật ký trên thiết bị.
- Việc một lượt game mất 0,8–1 giây là chi phí mô phỏng bản thân
  trò chơi, không thể biến thành bước tức thời bằng hàng đợi.
- Bộ test Python kiểm tra hợp đồng mã và biên dịch CI, chưa
  thay thế hoàn toàn kiểm thử giao diện thực tế, tương tác modal,
  tính đúng đắn nội dung dữ liệu được quét.
