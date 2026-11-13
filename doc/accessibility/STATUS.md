# Trạng thái hiện hành: CDDA Android trợ năng

**Cập nhật:** 13/11/2026. **Kho:** `tvhuy99-web/Abcd-huy`. **Nhánh nguồn:** `master`. **Commit đã đối chiếu:** `f0be189ac5ebf000fc38e8c91fa3b42e325a7417` (merge PR #7). [Mục lục tài liệu](README.md).

> Tài liệu này thay thế vai trò “trạng thái mới nhất” của bản `STATUS.md` nhiều thời kỳ. Nhật ký giai đoạn 1, nhánh `feature/android-reader-code-phase`, PR #2, bản thử giọng `cb3c082` và các mốc tháng 10 chỉ còn là chứng cứ lịch sử trong lịch sử Git và hồ sơ đã lưu; **không phải cấu hình APK hiện tại**.

## Kết luận ngắn

- Đã triển khai bộ đọc Android dùng snapshot/ngữ nghĩa, chuyển ý định đến engine để xác thực, TTS nội bộ, cử chỉ, menu tổng hợp, các adapter cho phần lớn bề mặt game; PR #7 bổ sung đọc thông tin đầy đủ theo yêu cầu và điều khiển cử chỉ cố định.
- Chưa thể tuyên bố mọi giao diện, mod hay mọi hành trình người khiếm thị đã được nghiệm thu. Không có bằng chứng thử toàn bộ trò chơi bằng TalkBack/TTS thật trên điện thoại.
- Quét 16 ô và danh sách xung quanh 20 ô mỗi trục **cố ý đọc trực tiếp dữ liệu bản đồ kể cả sau tường**. Đây là ngoại lệ quyền biết cần quyết định sản phẩm, không được quảng bá như cách quan sát thông thường của nhân vật.
- Một số workflow CI của merge commit trên `master` thất bại; kết quả build ARM64 thành công là ở **commit cuối PR #7**, mã tệp trùng với merge commit. Workflow APK trợ năng không tự kích hoạt khi chỉ push `master`.

## Chứng cứ đã xem

| Cổng | Bằng chứng | Kết luận |
| --- | --- | --- |
| Source tests trợ năng trên PR #7 | [Run 43343666735](https://github.com/tvhuy99-web/Abcd-huy/actions/runs/43343666735), job `inventory` | 432 bài Python; 10 test case/77 assertion C++ core; 24 assertion speech session; 35 assertion AndroidSpeech test doubles; đạt |
| APK ARM64 trên PR #7 | Cùng run, job `Android ARM64 (candidate)` | Gradle build, kiểm tra thư viện ELF64, assets, chữ ký và SHA256 đạt |
| CI chung trên `master` | [General matrix](https://github.com/tvhuy99-web/Abcd-huy/actions/runs/43356871042), [IWYU](https://github.com/tvhuy99-web/Abcd-huy/actions/runs/43356871214) | Có thất bại; gồm lỗi biên dịch ngoài Android với `a11y::android::diagnostic_event` và cảnh báo C++ khác |
| Dịch thuật phát hành | [Experimental Release](https://github.com/tvhuy99-web/Abcd-huy/actions/runs/43356871608) | Bước `build-translations` lỗi thiếu `msgcat` trên runner |
| Kiểm tra Python | [Flake8](https://github.com/tvhuy99-web/Abcd-huy/actions/runs/43356871046) | Thất bại về lint; không phải phép thử TalkBack |
| CodeQL | [CodeQL](https://github.com/tvhuy99-web/Abcd-huy/actions/runs/43356871039) | Action trả lỗi stub; chưa có kết quả phân tích bảo mật hợp lệ |
| Máy Android thật | Chưa có bộ kết quả đầu-cuối gắn với commit trên | **Chưa nghiệm thu**, không ghi PASS |

Các trạng thái CI trên phản ánh lúc kiểm tra 13/11/2026, không phải theo dõi tự động.

## Những gì có mã

1. **Khởi động và menu:** giao diện đầu vào, màn tải, chọn/tạo thế giới, nhân vật, các danh sách và popup có đường đọc.
2. **Điều khiển:** cử chỉ thế giới và menu tách biệt; 36 mã cử chỉ trong bộ gán, 11 mã dành riêng, 25 mã có thể gán. Các cử chỉ ngoài danh mục gán như chạm đôi một ngón và vuốt hai ngón vẫn có hành vi cố định.
3. **Menu tổng hợp:** Yêu thích; 15 nhóm hành động; Tìm lệnh. Hành động lấy từ lệnh live của engine và kiểm tra tính khả dụng.
4. **Quan sát:** mô tả ô theo thấy/nhớ/chưa biết trong chế độ quan sát thông thường; quét không giới hạn riêng, sonar hướng, danh sách xung quanh có bộ lọc.
5. **Vật phẩm:** selector, cây túi/pocket, số lượng một bước, kho nâng cao, lớp mặc và `read_detail` chủ động để nghe thông số đầy đủ.
6. **Sinh tồn và chiến đấu:** trạng thái nhân vật, y tế, hoạt động dài, chọn mục tiêu, chế độ chiến đấu, chế tạo/xây, NPC/đổi đồ, xe và vùng.
7. **Tài liệu và vòng đời:** tin nhắn, trợ giúp, nhật ký, lưu/tải, backup/restore, thông báo lỗi; TTS và nhật ký chẩn đoán thủ công.

Sự hiện diện trong mã **không phải** xác nhận từng đường thao tác chạy trọn vẹn.

## Việc cần làm theo mức ưu tiên

- **P0, điều kiện nghiệm thu:** kiểm tra APK trên máy thật với TalkBack và giọng Việt; khóa các ca đi bộ, menu, chiến đấu, vật phẩm, nhập liệu, lưu/tải.
- **P0, chất lượng mã đa nền tảng:** xử lý lời gọi `diagnostic_event` ngoài `__ANDROID__`, cùng cảnh báo cấu hình liên quan.
- **P1, CI phát hành:** khắc phục `msgcat`/gettext, lint và cấu hình CodeQL; tạo luồng xây APK tương ứng với commit `master`.
- **P1, luật quan sát:** chốt chính sách đọc xuyên tường so với cách quan sát theo tri thức của nhân vật.
- **P1, độ tin cậy UI:** phân trang snapshot quá giới hạn, kiểm tra tiêu điểm và chọn lại vật phẩm khi danh sách biến động, văn bản rất dài và các hành trình hiếm.
- **P2, đồng bộ tài liệu:** theo quy ước [bảng hợp nhất](DOC-CONSOLIDATION-2026-11-13.vi.md), không dùng hướng dẫn cũ làm thông số hiện tại.

Xem [hướng dẫn](USER-GUIDE.vi.md), [đặc tả hiện hành](CURRENT-REFERENCE.vi.md) và [ma trận kiểm chứng](VALIDATION-MATRIX.vi.md). Không ghi tỷ lệ phần trăm “đã hoàn thành” nếu chưa có mẫu số và bằng chứng nghiệm thu.
