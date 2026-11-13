# Trung tâm tài liệu trợ năng Android

**Tài liệu hiện hành, đối chiếu mã nguồn `master` tại `f0be189ac5ebf000fc38e8c91fa3b42e325a7417` (13/11/2026).**

Không đọc các báo cáo tháng 10 hoặc các bản thiết kế theo nhánh cũ như mô tả APK hiện tại. Tài liệu trong thư mục này đã được chia thành **hiện hành**, **bằng chứng kiểm thử**, và **lịch sử**.

## Bắt đầu từ đây

1. [Hướng dẫn người chơi](USER-GUIDE.vi.md): cử chỉ, TTS, menu, quan sát, chọn đồ, lưu và xử lý sự cố.
2. [Đặc tả hành vi hiện hành](CURRENT-REFERENCE.vi.md): quy tắc thực sự được mã nguồn áp dụng, nhóm menu, tình huống ngoại lệ.
3. [Trạng thái dự án](STATUS.md): mốc mã, điều đã xác minh, vấn đề mở.
4. [Ma trận kiểm chứng](VALIDATION-MATRIX.vi.md): chứng cứ CI, phần chưa có chứng cứ máy thật và các ca cần nghiệm thu.
5. [Bảng đối chiếu tài liệu cũ](DOC-CONSOLIDATION-2026-11-13.vi.md): tài liệu nào bị thay thế, mâu thuẫn gì, nguồn quyết định.
6. [Biểu mẫu báo lỗi](BUG-REPORT-TEMPLATE.vi.md).

## Cách đọc kết quả

- **Có mã**: thấy đường triển khai trong Java/C++; không đồng nghĩa đã chạy đúng trên máy.
- **Kiểm thử nguồn**: unit/source/giả lập TTS; không đồng nghĩa đã nghe âm thật.
- **Đã dựng APK**: biên dịch, đóng gói, ký và kiểm tra tệp; không đồng nghĩa toàn bộ hành trình chơi đạt.
- **Nghiệm thu máy thật**: cần kết quả trên Android và phản hồi người khiếm thị cho một kịch bản cụ thể. Hiện chưa có hồ sơ đầy đủ cho mọi kịch bản.

Không dùng số lượng adapter, node hay bài test làm tỷ lệ phần trăm “game đã tiếp cận được”.

## Quy định với tài liệu lịch sử

Các hồ sơ `R02`–`R22`, `PLAN`, `IMPLEMENTATION`, `PHASE*`, `JOURNEY-LEDGER`, các bản ghi kiểm thử tháng 10 và những tài liệu thiết kế theo ngày **được giữ lại để truy vết**. Một câu như “chưa build”, “chưa có APK”, “đang ở giai đoạn 1”, “không thay đổi master” trong những hồ sơ đó chỉ đúng **tại thời điểm và commit ghi trong hồ sơ**.

Nếu hướng dẫn cũ mâu thuẫn tài liệu hiện hành, ưu tiên mã ở commit đã ghim; sau đó cập nhật đặc tả và hướng dẫn. Không âm thầm xóa dữ kiện lịch sử hoặc biến kết quả kiểm thử của commit trước thành kết quả của commit mới.

## Phạm vi

Tập trung biến thể Android trợ năng của kho `tvhuy99-web/Abcd-huy`. Đây không phải tài liệu của bản phát hành upstream CleverRaven. Mọi đường dẫn đến mã trong thư mục này tương ứng trạng thái ở mốc nêu trên; khi `master` đổi, phải đối chiếu lại.
