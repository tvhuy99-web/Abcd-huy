# Tiến độ trình đọc nội bộ Android

Mục tiêu: người khiếm thị tự thực hiện toàn bộ hành trình chơi trong phạm vi
phát hành đã nghiệm thu. Mã tham chiếu trước triển khai:
`8ef359c74cc25e256919558bf7b3340f9b56a80d`.

## Trạng thái hiện tại

G0 đang triển khai; có mã nền chuẩn bị G1/G2. Chưa công nhận build nền, G1,
G2 hoặc bất kỳ khả năng chơi độc lập nào đã hoàn thành. Chưa có kiểm thử máy
thật/người dùng. Các lớp mới chưa được nối vào điều khiển hoặc lời đọc gameplay.

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

## Mã nền và kiểm thử đã thực hiện

- `a11y_context`: token phiên/ngữ cảnh/đời focus, overlay chặn tầng dưới,
  scope RAII, hủy cây overlay khi đóng cha, vô hiệu callback sau suspend/load.
  Chỉ truy cập trên game thread; chưa thay thế hay gắn hook vào `input_context`.
- `AndroidSpeech`: driver Android TTS bất đồng bộ, state/lỗi thiếu ngôn ngữ,
  timeout khởi tạo/bắt đầu, giới hạn đầu vào, hủy, shutdown và bỏ callback cũ.
  Không giữ hàng đợi lời chờ khi chưa ready. Kiểm tra ngôn ngữ thực tế của voice
  và cờ cần mạng; online cần lựa chọn tường minh. Chưa có JNI, bootstrap chọn
  giọng, kiểm thử voice offline thật, chính sách ưu tiên/gộp hoặc adapter menu gameplay.
- `SpeechSession`: định danh riêng từng utterance ngay cả khi caller dùng lại
  nhãn; chống callback cũ/đúp sau cancel hoặc thay engine.
- Kiểm tra local: 4 unittest kiểm kê; C++ 7 test cases/38 assertions đạt;
  Java token 24 assertions đạt; driver với Android test doubles 31 assertions đạt.
  Các con số Java không chứng minh TTS thật hoặc phát âm thanh trên Android.
- Đã khai báo truy vấn TTS service; không thêm quyền microphone hoặc service
  đọc màn hình toàn hệ thống. Driver chưa được tự bật khi mở game.
- Kiểm kê thủ công đầu ở [COVERAGE.md](COVERAGE.md) bổ sung minigame,
  màn y tế, điều khiển động cơ, study zones và nhật ký.

## Lượt CI đang theo dõi

- [Lượt đầu](https://github.com/tvhuy99-web/Abcd-huy/actions/runs/39717825397):
  kiểm kê PASS; build dừng ở cấp công cụ vì `sdkmanager` không nằm trên PATH.
- Đã sửa bằng bước setup Android được pin commit.
- [Lượt cấp công cụ đã sửa](https://github.com/tvhuy99-web/Abcd-huy/actions/runs/39717924057):
  kiểm kê PASS; job candidate ở commit `5b1c9c2` build ARM64 debug thành công
  trong 18 phút 54 giây; kiểm tra thư viện game/catalog Việt PASS, artifact
  `android-arm64-candidate` đã được tải lên. Commit này mới thay tài liệu và CI,
  chưa có các lớp C++/Java nền trợ năng. Không gán APK này cho mã nguồn mới hơn.
  Job baseline vẫn đang được theo dõi tại lần cập nhật này.
- [Kiểm tra lõi/API ở commit 0733bb3](https://github.com/tvhuy99-web/Abcd-huy/actions/runs/39719385009):
  PASS cả kiểm kê, C++, Java host tests và javac với Android API 35 thật.
  Thay đổi kiểm tra voice cần mạng sau commit này phải chờ lượt CI mới.

Workflow tự động về sau chỉ build candidate; có thể yêu cầu build lại baseline
bằng `workflow_dispatch` với `compare_baseline=true`. PR của chính nhánh này
không lặp lại lượt build từ push. Chưa merge hoặc công bố APK tiếp cận được.

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
