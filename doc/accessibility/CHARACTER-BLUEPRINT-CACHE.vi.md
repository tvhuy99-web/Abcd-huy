# Bộ nhớ đệm bản thiết kế cùng bản lưu (Android)

> **GHI CHÚ PHIÊN BẢN (13/11/2026):** Đây là hồ sơ chuyên đề theo thời điểm viết, không tự động là chỉ dẫn cho `master` sau PR #7. Khi cử chỉ, menu, quyền đọc bản đồ hoặc CI khác, ưu tiên [đặc tả hiện hành](CURRENT-REFERENCE.vi.md), [hướng dẫn](USER-GUIDE.vi.md) và [trạng thái](STATUS.md). **Không coi mã đã viết là nghiệm thu TalkBack trên máy thật.**

## Thiết kế
Bản thử nghiệm lưu một tệp `<save_id>.loadcache` cạnh `.sav` của từng
nhân vật. Không thay đổi tệp lưu chính, công thức, mapgen hay mod.

Chỉ lưu kết quả thô của `get_changed_ids_from_update` và
`get_build_reqs_for_furn_ter_ids` cho từng tổ hợp tham số blueprint.
Các bước `incorporate_build_reqs`, tải game, tạo bản đồ và hoàn tất
công thức khác vẫn hoạt động bình thường.

## Điều kiện sử dụng lại
Fingerprint dựa trên phiên bản chương trình, thứ tự mod, nội dung thiết lập
thế giới và từng tệp JSON thực sự được nạp (đọc nội dung, không chỉ thời
gian sửa đổi). Có checksum riêng, giới hạn 64 MiB, kích thước bản ghi, số
biến thể và số nguyên. Hỏng/hết hạn/thiếu cache sẽ tính lại như cũ.

File được ghi vào tệp `.tmp`, rồi đổi tên sau khi **nhân vật tải
thành công**. Nếu ghi thất bại, không ảnh hưởng bản lưu.

Nhật ký: `A11Y_BLUEPRINT_CACHE status=miss/ready/summary/written`.

## Kiểm thử trước khi gộp
- Build ARM64 và thử trên thiết bị thật.
- Tải lần 1: miss, written. Lần 2: ready, hits.
- So sánh raw requirements/skills/time trước và sau khi dùng cache.
- Sửa JSON/mod, lỗi checksum, xóa cache, thay thế bản APK: tải lại an toàn.
- Kiểm tra trường hợp mapgen có phần tử ngẫu nhiên hoặc phụ thuộc trạng thái
  thế giới: không được tái sử dụng nếu kết quả không ổn định.
- Kiểm tra thêm dung lượng RAM và tác động thời gian đọc nguồn.

**Chưa gộp master; giữ riêng ở nhánh thử nghiệm.**
