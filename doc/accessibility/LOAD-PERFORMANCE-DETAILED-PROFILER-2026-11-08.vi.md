# Nhật ký đo thời gian tải game chi tiết (Android)

> **GHI CHÚ PHIÊN BẢN (13/11/2026):** Đây là hồ sơ chuyên đề theo thời điểm viết, không tự động là chỉ dẫn cho `master` sau PR #7. Khi cử chỉ, menu, quyền đọc bản đồ hoặc CI khác, ưu tiên [đặc tả hiện hành](CURRENT-REFERENCE.vi.md), [hướng dẫn](USER-GUIDE.vi.md) và [trạng thái](STATUS.md). **Không coi mã đã viết là nghiệm thu TalkBack trên máy thật.**

Áp dụng cho nhánh `integration/reader-vietnamese-20261107` và bản APK build từ commit có thêm bộ đo.
Bản APK cũ không thể tự sinh các dòng `A11Y_LOAD_DETAIL`.

## Mục đích

Nhật ký đã có `A11Y_LOAD_TIMING`, `A11Y_LOAD_GROUP`, `A11Y_LOAD_PHASE` cho các
giai đoạn lớn. Bổ sung đo chi tiết hai điểm chậm ở bản ghi ngày 08-11-2026:
**Crafting recipes** (~135 giây) và **Mapgen weights** (~30 giây). Mục tiêu
hiện tại là *xác định nguyên nhân*, không thay đổi thuật toán hoặc dữ liệu lưu.

## Cách đọc (trong debug.log)

Lọc các dòng có tiền tố `A11Y_LOAD_DETAIL`.

### `phase=recipe`

- `step=deferred`: nạp dữ liệu JSON trì hoãn.
- `step=remove_abstract`: bỏ các công thức trừu tượng.
- `step=finalize_craft_and_prune` / `finalize_uncraft_and_prune`: hai lượt
  hoàn tất và lọc công thức.
- `step=book_links_and_reversible`: liên kết sách và xây dựng công thức tháo dỡ.
- `step=pseudo_uncraft_books`: tạo công thức tháo dỡ sách giả lập.
- `step=nested_category_search`: tìm quan hệ nhóm công thức lồng nhau.
  Dòng `step=nested_counts` đi kèm có `candidates` và `comparisons`.
- `step=build_lookup_indices`: tạo các chỉ mục công thức.
- `step=food_loops`: xây dựng và dò vòng lặp thành phần thực phẩm.
  Dòng riêng có `graph_ms`, `detect_ms`, `report_ms`, số nút và cạnh.
- `step=total`: tổng thời gian toàn bộ quá trình này.

### `phase=recipe_internal`

- `kind=craft` hoặc `kind=uncraft`.
- `step=iteration`: số công thức, tổng `elapsed_ms`,
  `cleanup_ms` (lọc tham chiếu), `finalize_ms` (xử lý công thức),
  `input_ms` (bơm sự kiện).
- `step=prune`: tổng thời gian lọc/kiểm tra lỗi, số công thức còn lại.
- `step=slow_recipe`: tối đa **5** mã công thức chậm nhất mỗi loại,
  thời gian `elapsed_us` (microgiây).
- `step=progress`: chỉ ghi khi đã qua một cụm 512 công thức và ít nhất
  15 giây kể từ bản tin trước, để xác định vị trí nếu game bị đứng.

### `phase=mapgen`

- `oter_setup` và `oter_parameters`: thiết lập, rồi hoàn tất tham số cho
  các bộ tạo địa hình chuẩn.
- `nested_setup`, `update_setup`: thiết lập bản đồ lồng nhau và bản đồ cập nhật.
- `nested_parameters`, `update_parameters`: hoàn tất tham số từng nhóm.
- `total`: tổng giai đoạn tính trọng số và thiết lập.
- `groups`, `functions`: số nhóm/hàm được duyệt.
- `slowest_group`, `slowest_ms`: nhóm lâu nhất trong mỗi lượt.
- `oter_setup_progress`: bản tin tiến độ sau mỗi khoảng 15 giây.
  Không log từng đối tượng để tránh tệp quá lớn.

## Cách kiểm thử

1. Cài **APK mới** build từ commit profiler. Để riêng bản sao lưu an toàn
   trước khi cập nhật APK.
2. Mở game, tải **cùng một nhân vật** ít nhất hai lần theo cùng điều kiện.
3. Sau lần tải, xuất nhật ký phiên từ mục Debug log. Bản xuất có
   `debug.log` của native game, không chỉ `cataclysm-session-debug.txt`.
4. Gửi lại bản xuất để nhóm các `A11Y_LOAD_DETAIL` theo `step`.
   Đối chiếu với `A11Y_LOAD_GROUP` nhằm kiểm tra sai lệch do bộ đo.
5. Nếu muốn đối chiếu bản tối ưu trước/sau, dùng cùng thế giới, bộ mod,
   bản APK, điều kiện nóng/lạnh của tiến trình và hai lần chạy tương đương.

Chỉ ghi ID công thức/mapgen và thời gian; không ghi lời thoại, tên nhân vật,
nội dung bản lưu hay vị trí riêng tư. Mã đo nằm trong `#if defined(__ANDROID__)`
và không thay đổi kết quả thuật toán. Việc gọi đồng hồ và ghi log vẫn tạo
một lượng nhỏ phụ phí, nên hãy so sánh với `A11Y_LOAD_GROUP` tổng.

**Chưa xác nhận APK hoặc thử trên thiết bị thật.**
