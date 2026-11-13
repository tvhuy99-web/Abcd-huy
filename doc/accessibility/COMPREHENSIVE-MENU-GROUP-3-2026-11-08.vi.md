# Menu tổng hợp, mục hiển thị 3: Quan sát và bản đồ

**Đặc tả hiện hành đối chiếu `master@f0be189` (13/11/2026).** Tệp này thay thế mô tả tháng 11 cũ về “bán kính 60 ô, chỉ dữ liệu nhìn thấy”. Xem [đặc tả chung](CURRENT-REFERENCE.vi.md) và [hướng dẫn](USER-GUIDE.vi.md).

## Phân biệt các loại quan sát

- Quan sát bản đồ thông thường dùng `a11y::observe_tile` với phân biệt thấy/nhớ/chưa biết.
- **Vuốt hai ngón theo bốn hướng** trên thế giới gọi `announce_linear_scan` đọc từng ô từ 1 tới 16, qua `scan_unrestricted_tile`. Mã này **cố ý đọc dữ liệu bản đồ sau tường**, gồm các thông tin có thể chưa được nhân vật nhìn thấy.
- **Danh sách xung quanh** cho biến thể Android trợ năng quét tối đa 20 ô theo **mỗi trục** quanh nhân vật: 41 × 41 vị trí, trong vùng bản đồ đã nạp. Đừng nhầm với `tab_data::radius = 60` của **ba tab hình ảnh gốc khi trình đọc tắt**.

## Danh sách xung quanh

1. Mở **Menu tổng hợp → Quan sát và bản đồ → Danh sách xung quanh**.
2. Danh sách chỉ đưa ra sinh vật, vật phẩm, âm thanh/cảm nhận, nội thất, phương tiện, bẫy, trường nguy hiểm, địa hình/điểm tương tác đáng chú ý. Không liệt kê mọi ô sàn/cỏ/đường/tường thường.
3. Có bốn bộ lọc: **Tất cả**, **Sinh vật và NPC**, **Địa điểm và môi trường**, **Vật phẩm**. Vuốt một ngón dọc đổi nhóm; vuốt ngang duyệt từng đối tượng.
4. Sắp **từ gần đến xa**, cùng khoảng cách thì giữ thứ tự dò quét vòng; **không bảo đảm xếp theo tên chữ cái**. Phân trang tối đa **80 hàng**.
5. Hàng đọc tên, hướng và số ô. Chạm hai lần yêu cầu `safe_route_to` (tới ô vật phẩm đi được hoặc gần đối tượng/vật cản); không teleport, không tự nhặt, có thể không tìm thấy đường. Danh sách là dữ liệu tại thời điểm mở, không tự bám theo sinh vật di chuyển.
6. Trên hàng **vật phẩm**, chạm một lần hai ngón gọi `read_detail` để nghe bảng thông tin đầy đủ; **không** kích hoạt tìm đường. Engine tái xác minh đối tượng theo ô và nhóm tên, tránh đọc nhầm sau khi danh sách thay đổi.
7. Các hành động nhìn lén, kiểm tra ô hiện tại và bản đồ thời tiết vẫn theo lệnh gốc và giới hạn cụ thể từng màn. Đừng áp cơ chế quét không giới hạn cho mọi loại quan sát.

## Kiểm chứng còn thiếu

Thử sau tường (quái/bẫy/đồ vật), ô 20/21, nhiều món trùng tên khác tình trạng, danh sách trên 80 hàng, cử chỉ đổi lọc, không tìm được đường, chạm hai ngón không tốn lượt, và TalkBack thật. Các bài nguồn có thể tìm ở `test_nearby_20_tile_scan.py`, `test_nearby_category_gestures.py`, `test_nearby_accessible_navigation.py` và `test_full_detail_on_demand.py`. Không gọi đã nghiệm thu thiết bị chỉ vì các bài nguồn đạt.
