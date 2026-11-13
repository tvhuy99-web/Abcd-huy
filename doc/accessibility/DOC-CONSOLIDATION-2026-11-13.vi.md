# Hồ sơ hợp nhất và sửa sai tài liệu trợ năng

**Thời điểm:** 13/11/2026. **Nền đối chiếu:** `master@f0be189ac5ebf000fc38e8c91fa3b42e325a7417` sau PR #7. Mục tiêu là xóa mâu thuẫn trong *chỉ dẫn hiện hành*, không xóa lịch sử kỹ thuật.

## Quy tắc ưu tiên

1. Mã thực tại commit ghim xác định **hành vi đã triển khai**.
2. CI/job thật xác định **bằng chứng của commit và môi trường riêng**.
3. `CURRENT-REFERENCE.vi.md`, `STATUS.md`, `USER-GUIDE.vi.md`, `VALIDATION-MATRIX.vi.md` là bộ tài liệu **hiện hành**.
4. Các hồ sơ `R*`, `PLAN`, `IMPLEMENTATION`, `TESTING`, `PHASE*`, `JOURNEY-LEDGER` và các ghi chép theo ngày chủ yếu là **lịch sử/thiết kế/bản ghi kiểm toán**, không phải đặc tả của build mới. Các thống kê ghi trong đó chỉ thuộc mốc cũ.
5. Nếu một hành vi không được xác nhận bằng mã hoặc thử thực tế, ghi giới hạn thay vì suy đoán.

## Mâu thuẫn đã xác định và cách hợp nhất

| Hồ sơ cũ | Khẳng định cũ hoặc xung đột | Quyết định hiện tại |
| --- | --- | --- |
| `STATUS.md`, `IMPLEMENTATION.vi.md` | Đang giai đoạn 1, chưa chạy test/chưa APK, nhánh PR #2 | Phần mô tả đó là lịch sử 22–23/10; thay trang STATUS bằng trạng thái merge PR #7, ghi CI theo SHA |
| `USER-GUIDE.vi.md` | Bảng điều khiển reader cũ, luyện tám hướng và đổi ánh xạ vuốt kiểu cũ là đường vào chính | Thay hướng dẫn bằng điều khiển game/menu và các trang TTS/trợ năng thực tế trên `master` |
| `COMPREHENSIVE-MENU-GROUP-3-2026-11-08.vi.md` | Quét gần bán kính 60 ô, sắp theo tên, không nói bộ lọc | Android quét hình vuông 41×41, gần trước xa sau, 80 hàng/trang, 4 bộ lọc; 60 là giao diện hình ảnh gốc |
| `FIXED-GESTURES-CHARACTER-NARRATION-2026-11-10.vi.md` | Chạm ba lần ba ngón chưa có chức năng; 10 mã cố định/26 gán | Mã hiện tại dùng cử chỉ ấy bật/tắt combat; 11 mã cố định/25 mã gán |
| `TTS-SETTINGS-2026-11-07.vi.md` | Công tắc giọng mạng, khôi phục giọng, nút Android Settings trong trang TTS; tốc độ tối đa 3 | Trang TTS hiện không có các nút đó; tốc độ thanh là 0,5–10,0; thử giọng có mặt nhưng `saveSettings` không kiểm tra `previewPassed` |
| `SESSION-DEBUG-LOG-2026-11-08.vi.md` | Mặc định bật, giữ `crash_recovery.txt`, chọn phiên trước để xuất | Mặc định tắt; phiên mới xóa nhật ký cũ; chỉ xuất phiên hiện tại trước khi mở app lại |
| `SPOKEN-ONLY-WORLD-ADAPTIVE-PROFILER-2026-11-10.vi.md` | Coi công tắc chỉ đọc nằm trong trang nhật ký | Công tắc đồ họa là ở `AccessibilitySettingsActivity`, riêng với trang `DebugLogActivity` |
| `UNIVERSAL-BACK-SCAN-SINGLE-STEP-2026-11-09.vi.md` | Đường quét và lệnh bận theo thiết kế từng mốc | Xem mã đang dùng: quét không giới hạn, cơ chế giữ tối đa một cử chỉ chờ thử lại có hạn; không hứa mọi vuốt có bước |
| `COVERAGE.md`, `JOURNEY-LEDGER.vi.md`, `PHASE1*`, `TESTING.vi.md` | 115 hành trình; 110 đường viết; T01–T64 chưa chạy ở mốc thiết kế | Số liệu giai đoạn 1, không được dùng làm tỷ lệ nghiệm thu hiện tại |
| `BUILD-AND-TEST-2026-10-24.vi.md` | Bằng chứng APK tháng 10 | Có giá trị ở commit cũ; PR #7 có build và source tests mới; chưa thay bằng phép thử thiết bị |
| `README.md` của upstream ở gốc repo | Liên kết upstream dùng cho CDDA nói chung | Không thay toàn bộ README upstream; thêm đường dẫn rõ ràng tới tài liệu Android fork nếu cần |

## Danh sách nguồn đối chiếu

- Java Android: `GameActionHub`, `GameReaderController`, `ReaderGestureSurface`, `ReaderGestureBindings`, `ReaderPreferences`, `TtsSettingsActivity`, `AccessibilitySettingsActivity`, `DebugLogActivity`, `SessionDebugLog`.
- C++: `a11y_semantics`, `a11y_android`, `a11y_world`, `a11y_perception`, `surroundings_menu`, `inventory_ui`.
- CI: [PR #7 Android Accessibility](https://github.com/tvhuy99-web/Abcd-huy/actions/runs/43343666735), [master General Matrix](https://github.com/tvhuy99-web/Abcd-huy/actions/runs/43356871042), [IWYU](https://github.com/tvhuy99-web/Abcd-huy/actions/runs/43356871214), [Release](https://github.com/tvhuy99-web/Abcd-huy/actions/runs/43356871608).

## Quy trình cập nhật tài liệu từ đây

- Thay đổi cử chỉ, TTS, radius/quét, sắp xếp menu, quyền đọc chi tiết, số lượng, nhật ký hoặc CI phải cập nhật bộ tài liệu hiện hành **cùng PR**.
- Hồ sơ có ghi thời điểm/nhánh cũ phải giữ nhãn **lịch sử**; không chỉnh ngược mốc đã xảy ra để trông như chưa từng sai.
- Các cột “đã triển khai” và “đã nghiệm thu” phải tách biệt. Việc CI xanh ở PR head không tự thay cho CI xanh ở mọi hệ nền hoặc người dùng nghe thật.
- Đối chiếu lại bản ghim khi `master` có commit mới.
