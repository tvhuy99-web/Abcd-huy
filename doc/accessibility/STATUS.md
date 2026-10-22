# Tiến độ trình đọc nội bộ Android

Mục tiêu: người khiếm thị tự thực hiện toàn bộ hành trình chơi trong phạm vi
phát hành đã nghiệm thu. Mã tham chiếu trước triển khai:
`8ef359c74cc25e256919558bf7b3340f9b56a80d`.

## Trạng thái hiện tại

G0 đang triển khai. Chưa công nhận build nền, G1, G2 hoặc bất kỳ khả năng chơi
độc lập nào đã hoàn thành. Chưa có kiểm thử máy thật/người dùng.

Ngày 22/10/2026:

- Tạo nhánh `feature/android-internal-reader` từ đúng baseline.
- Thêm workflow chỉ đọc quyền repository, không dùng khóa ký phát hành:
  build baseline và candidate ARM64 riêng, lưu log/định danh/SDK/checksum/APK.
- Thêm công cụ kiểm kê tĩnh cùng kiểm thử. Kết quả ban đầu: 954 tệp;
  1125 điểm đăng ký action, 87 điểm tạo context phát hiện được, 12 điểm đăng ký
  phím thủ công, 15 đường input cũ, 299 lời gọi ImGui và 9 ứng viên dialog native.
  Đây là các điểm nguồn cần rà, không phải số màn hình hoặc số tính năng tiếp cận.
- Bốn kiểm thử công cụ kiểm kê chạy đạt tại môi trường làm việc.
- Build Android tại môi trường làm việc bị chặn trước khi compile: không có
  SDK/NDK và Gradle wrapper không tải được Gradle 8.9 do network unreachable.
  Không quy lỗi này thành lỗi game. Kết quả CI phải được ghi sau khi thực chạy.

## Cổng còn phải qua

1. G0: build sạch baseline, cài/chạy trên Android, kiểm kê thủ công các luồng.
2. G1: bootstrap, thử giọng, TTS/JNI và đường phục hồi.
3. G2: ngữ cảnh/focus/token, input, menu, popup và nhập liệu.
4. G3: mở game → tạo nhân vật → hành động cơ bản → lưu/tải độc lập.
5. G4–G6: không gian/sinh tồn, chiến đấu/chế tạo, hệ thống nâng cao.
6. G7–G8: vòng đời/kết thúc, dữ liệu, kiểm thử người dùng và phát hành.

Đọc [kế hoạch đầy đủ](PLAN.vi.md) cho định nghĩa nghiệm thu và 64 tình huống.
Không đánh dấu một tính năng đã tiếp cận chỉ từ việc thêm TTS, compile thành
công, kiểm tra tĩnh hoặc mở được menu.
