# Hồi quy Quay lại, quét bản đồ và bước đi: đối chiếu hiện tại

**Đính chính 13/11/2026, `master@f0be189`.** Đây là báo cáo phát triển theo mốc 09/11; phần cơ chế bước đi đã được thay đổi thêm, không dùng câu “mọi cú vuốt đang bận đều bị bỏ ngay” làm đặc tả vĩnh viễn.

## 1. Quay lại theo ngữ cảnh

Trong menu, yêu cầu Back/Hủy đi qua đường cancel đang hợp lệ của màn hình, không tự gọi hành động YES. Các hộp xác nhận và hoạt động đang diễn ra có thể có logic riêng. Vuốt hai ngón xuống trên **màn thế giới** là lệnh quét 16 ô về phía Nam, **không phải Back**.

## 2. Hai cơ chế quét

`announce_linear_scan` quét một ray 16 ô; Danh sách xung quanh quét 20 ô mỗi phía theo hai trục (41×41). Cả hai hiện dùng `scan_unrestricted_tile` để đọc bản đồ trực tiếp cả sau tường; danh sách chỉ giữ các đối tượng có ý nghĩa và chia trang 80 hàng. Quan sát bản đồ thông thường vẫn dùng `observe_tile` và quyền biết của nhân vật.

## 3. Một bước đi không đồng nghĩa tự động hoàn thành

C++ có cổng nhận bước đi khi game đang chờ input. Java hiện còn cơ chế giữ **tối đa một cử chỉ mới nhất** để thử lại trong thời hạn ngắn khi lượt trước đang bận; không có backlog vô hạn hoặc cho đi nhiều lượt theo một cử chỉ. Mã vẫn kiểm tra lại màn hình, session, frame, generation và quyền đi bộ; không được đảm bảo mọi cú vuốt được chấp nhận hoặc luôn trả lời dưới 100 ms.

## 4. Kiểm chứng bắt buộc

Thử thao tác quay lại trong list/popup/hoạt động, quét xuyên tường và mục ở vị trí 16/20, gửi nhanh 20 cú vuốt, đổi modal sau khi vuốt, so log `WORLD_STEP_BUSY` / `NATIVE_ACTION` và tọa độ thực. Không ghi PASS máy thật chỉ từ test nguồn.

Xem [đặc tả hiện hành](CURRENT-REFERENCE.vi.md) và [ma trận](VALIDATION-MATRIX.vi.md). Bản thiết kế cũ được lưu trong Git history.
