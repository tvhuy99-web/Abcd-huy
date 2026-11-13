# Android: tối ưu lần hai (nhánh thử nghiệm)

Nối tiếp PR #5, dựa trên số đo máy thật tải bản lưu Matoaka/Granville Phelps.

## Tối ưu khởi tạo địa hình
Khi **tải nhân vật đã lưu**, không dựng sẵn tất cả ~6.552 nhóm
overmap mapgen. Vẫn chuẩn bị đầy đủ nested/update mapgen ở đúng thứ tự.
Nhóm overmap chỉ gọi `setup` rồi `finalize_parameters` **một lần**
khi được yêu cầu bởi tạo bản đồ hoặc tham số overmap special.
Nhóm được khởi tạo có khóa tuần tự để tránh đua dữ liệu JSON toàn cục.
Tạo nhân vật mới vẫn chuẩn bị tất cả như cũ. Nếu bật xác minh dữ liệu,
cơ chế kiểm tra cũng buộc chuẩn bị các nhóm cần kiểm tra.

Đây là hoãn phép tính, không phải lưu trạng thái bản đồ xuống tệp;
không biến các cấu trúc con trỏ thành dữ liệu tuần tự hóa.
**Rủi ro cần kiểm tra:** Lần đầu đi vào khu vực mới có thể chậm
một chút do biên dịch mapgen tại thời điểm đó.

## Tối ưu hỏng hóc, vật phẩm và phương tiện
- Android: `requirement_data::finalize()` gọi từ `fault_fix` chỉ
  thực hiện một lần trong vòng đời nạp dữ liệu, thay vì lặp theo mọi
  cách sửa hỏng hóc. Vẫn giữ pass finalize toàn cục sau nhóm vật phẩm.
- Xây bộ tra cứu `repair_tool -> tập nguyên liệu` một lần trước vòng
  `finalize_post` mọi loại vật phẩm. Giữ logic: công cụ phù hợp nếu
  *bất kỳ* action nào hỗ trợ *bất kỳ* nguyên liệu sửa chữa nào.
- Android: nhớ kết quả kiểm tra khả năng chứa nhiên liệu của từng cặp
  `(base_item, fuel_item)` trong cùng đợt tạo nguyên mẫu phương tiện.
  Không lưu/copy đối tượng phương tiện hoặc vật phẩm.

## Phân tích tốc độ nạp game/mod
Không bỏ xác minh dữ liệu! Giữ việc băm *nội dung từng JSON nguồn*
và dữ liệu tùy chọn thế giới để cache blueprint tự hết hiệu lực khi cập nhật.
Thêm nhật ký `A11Y_LOAD_DETAIL phase=json` để phân tách
`parse_ms` (đọc/chuyển dữ liệu), `load_ms` (tạo định nghĩa) và
`hash_ms` (đọc/băm nguồn). Chưa thay đổi định dạng JSON cache.

## Kiểm thử bắt buộc
1. Biên dịch Android ARM64; không gộp PR nếu thất bại.
2. Khởi động hai lần, so sánh `A11Y_LOAD_PHASE` trước/sau.
3. Đi vào địa hình mới và cũ, mở bản đồ đặc biệt, xây dựng căn cứ;
   so kết quả với bản APK trước, không chỉ thời gian tải.
4. Sau khi đổi game/mod/JSON, cache blueprint phải hết hiệu lực;
   bản lưu .sav không được ghi đè hoặc sửa bởi cache.
5. Kiểm tra tải nhân vật mới và khi bật kiểm tra định nghĩa game.
6. Đọc `phase=json`, `phase=faults`, `phase=items`,
   `phase=vehicle_prototypes` để chọn tối ưu tiếp theo.

**Chưa khẳng định cải thiện tốc độ cho đến khi có log máy thật.**
