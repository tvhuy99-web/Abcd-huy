# Danh sách xung quanh: 20 ô mỗi phía trên Android

**Đã sửa 13/11/2026 theo `master@f0be189`.** Bản trước nói `observe_tile` chặn đọc địa hình thực sau tường; **điều đó không đúng với đường quét gần hiện tại**.

## Vùng quét

- Quét hình vuông từ −20 đến +20 ô trên **hai trục**, tức 41 × 41 = 1.681 vị trí lý thuyết, giới hạn bởi bản đồ hiện đang nạp.
- Không phải quét một ray 16 ô bằng hai ngón. Không phải ba tab hình ảnh gốc có bán kính `tab_data::radius = 60`.
- Thông tin vật thể được lấy bằng `scan_unrestricted_tile`, **có thể đọc trực tiếp dữ liệu bản đồ sau tường** (sinh vật, bẫy, địa hình, vật phẩm). `observe_tile` vẫn được sử dụng **bổ trợ** cho dấu âm và cảm nhận đặc biệt. Không mô tả toàn bộ luồng như chỉ công bố thông tin trong tầm nhìn.
- Giữ nội thất (kể cả tủ/chạn thường), cửa/cầu thang, phương tiện, vật phẩm, sinh vật, bẫy và trường nguy hiểm; không tạo hàng cho mọi sàn/cỏ/đường trống hoặc tường thông thường.
- Sắp gần trước xa sau; cùng khoảng cách giữ thứ tự quét vòng. Phân trang **80 hàng**, không cắt ngầm những trang sau.
- Có bốn nhóm bộ lọc: Tất cả, Sinh vật và NPC, Địa điểm và môi trường, Vật phẩm. Vuốt dọc đổi bộ lọc; vuốt ngang duyệt kết quả.
- Chạm **một lần hai ngón trên vật phẩm** đọc `item::info(true)` sau khi tái xác minh danh sách; không di chuyển. Chạm **đôi một ngón** xin đường `safe_route_to`; không tự nhặt và có thể bị từ chối vì đường nguy hiểm.

## Chưa nghiệm thu trên thiết bị

Quét đúng ô 20/21, dữ liệu sau tường, trường hợp trùng tên/nhiều món, nội thất có/không vật phẩm, 80+ mục, đường bị chặn, thay bộ lọc, TalkBack thật và hiệu năng mở menu. Tất cả chỉ là đặc tả mã cho tới khi có nhật ký thử thực tế. [Đặc tả hiện hành](CURRENT-REFERENCE.vi.md).
