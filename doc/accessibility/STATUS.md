# Tiến độ trình đọc nội bộ Android

Mục tiêu: người khiếm thị tự thực hiện toàn bộ hành trình chơi trong phạm vi
phát hành đã nghiệm thu. Baseline: `8ef359c74cc25e256919558bf7b3340f9b56a80d`.
Nhánh triển khai: `feature/android-internal-reader`;
[PR dự thảo #2](https://github.com/tvhuy99-web/Abcd-huy/pull/2).

## Cập nhật quy trình và phản hồi từ người dùng

Ngày 22/10/2026 (giờ Việt Nam), người dùng xác nhận APK `cb3c082` đã mở và đọc
được. Đây là xác nhận về bản thử giọng đã bàn giao; chưa có thông tin mẫu máy,
engine, offline, cử chỉ hoặc khả năng chơi độc lập. Không tự suy ra các mục đó
đã đạt.

Theo yêu cầu mới, công việc chuyển sang ba giai đoạn: **viết toàn bộ mã → chạy
toàn bộ kiểm thử → tổng hợp lỗi, sửa và kiểm thử lại**. Nhánh viết mã:
`feature/android-reader-code-phase`. Mã mới chưa biên dịch/chạy test/tạo APK.
Các workflow trợ năng trên nhánh mới chỉ chạy thủ công. Không cập nhật nhánh
PR #2 bằng mã chưa thử trong đợt này, để tránh kích hoạt workflow PR cũ.

Đã viết thêm hợp đồng snapshot/intent, runtime và JNI mailbox, lớp trình bày
Android, chính sách lời đọc/cử chỉ, nhập Unicode, menu chính, danh sách và hộp
thoại dùng chung, tích hợp đầu cho tạo nhân vật, ô nhìn thấy/ghi nhớ/chưa biết,
trạng thái nhân vật và quét lân cận. **Đây là mã đang triển khai, chưa chứng
minh chạy được hoặc bao phủ toàn game.** Theo dõi phần chưa viết tại
[IMPLEMENTATION.vi.md](IMPLEMENTATION.vi.md).

Đợt tiếp theo đã bổ sung mã cho hành trang chọn đơn/chọn nhiều, nhặt/bỏ,
nạp/tháo đạn, so sánh và cửa sổ thông tin vật phẩm; danh sách công thức và
mẻ chế tạo, yêu cầu từng nhóm/công đoạn; xây dựng và chọn hướng/ô lân cận.
Tất cả vẫn ở giai đoạn viết mã, **chưa biên dịch/chạy kiểm thử**, chưa có APK
mới và chưa đủ điều kiện công bố người khiếm thị chơi độc lập.

Đã viết tiếp adapter y tế: duyệt bộ phận, hiệu ứng/vết thương, tổng quan,
đường chọn điều trị và điều kiện bằng chữ; dùng chung mô tả sức khỏe theo
mức thông tin mà game cho biết. Phần này cũng chưa biên dịch hoặc chạy thử.

Các kết quả PASS bên dưới chỉ áp dụng cho mã nền/APK cũ có định danh tương ứng.

## Trạng thái ngày 22/10/2026

Đã có mã nền và màn thử giọng Việt native chạy được trong kiểm thử giao diện
Android. **Chưa có bộ đọc hoặc điều khiển gameplay. Chưa nghiệm thu G0, G1,
G2 hay khả năng chơi độc lập.** Chưa có kết quả nghe âm thanh trên điện thoại
thật hoặc kiểm thử cùng người khiếm thị. Không suy ra các khả năng này từ CI.

Giữ TalkBack để dùng màn thử. APK thử có mã ứng dụng và vùng dữ liệu riêng;
không yêu cầu gỡ game đang có. Chưa merge vào `master`.

## Đã triển khai

- Công cụ kiểm kê C++/Java có kiểm thử, kèm kiểm kê thủ công ban đầu ở
  [COVERAGE.md](COVERAGE.md). Scan ban đầu: 954 tệp, 1125 điểm đăng ký action,
  87 ứng viên tạo context, 12 điểm phím thủ công, 15 đường input cũ, 299 lời gọi
  ImGui và 9 ứng viên dialog native. Đây là điểm nguồn, không phải số màn hình
  hay tỷ lệ tính năng tiếp cận. Công cụ heuristic không thay thế rà thủ công.
- `a11y_context`: sở hữu ngữ cảnh trên game thread, token phiên/frame/generation,
  scope RAII, overlay chặn tầng dưới, đóng cha hủy cây con, vô hiệu token sau
  suspend/load. **Chưa gắn vào `input_context` hoặc hành động gameplay.**
- `SpeechSession`: định danh từng utterance, lọc callback cũ/đúp sau cancel,
  thay engine hoặc dùng lại nhãn từ caller.
- `AndroidSpeech`: TTS bất đồng bộ, trạng thái/lỗi rõ ràng, timeout khởi tạo và
  bắt đầu, kiểm tra ngôn ngữ thực tế, từ chối voice cần mạng khi chưa opt-in,
  giới hạn đầu vào, dừng và shutdown. Không tích hàng lời khi chưa ready.
  Chưa có JNI, hàng ưu tiên/gộp lời hoặc bộ đọc ngữ nghĩa gameplay.
- `ReaderSetupActivity`: widget native cho phát thử tiếng Việt, dừng, opt-in
  giọng cần mạng, mở cài đặt giọng/trợ năng và quay lại. Không tự phát khi
  resume; hủy mẫu khi đi nền và giải phóng engine khi đóng. Opt-in mạng và
  trạng thái đang phát không được khôi phục sau recreate.
- Cảnh báo Splash Anh/Việt hướng tới màn thử và giữ trình đọc hiện có. Nút
  “Thiết lập trợ năng và giọng đọc” mở Activity. Nếu cảnh báo bị bỏ qua hoặc
  không xuất hiện, **chưa có lối vào thay thế từ gameplay**. Đây chưa phải
  bootstrap hoàn chỉnh dùng không cần TalkBack.
- Flavor `accessibility`, launcher “Cataclysm DDA - Thử trợ năng”, application ID
  `com.cleverraven.cataclysmdda.accessibility`, DocumentsProvider riêng.
  Giữ authority cũ của các flavor đang có.
- Chứng thư thử cố định công khai để cập nhật APK thử mà không phải gỡ app.
  Không dùng khóa phát hành; xem [TEST_SIGNING.md](TEST_SIGNING.md).
- CI build ARM64 đầy đủ, host tests, compile Android API/resources và hai
  instrumentation tests. Workflow chỉ có quyền đọc nội dung repository.
  Bản UI-only x86_64 của emulator không được dùng làm APK bàn giao gameplay.

## Bằng chứng đã kiểm tra

| Lớp kiểm tra | Kết quả | Giới hạn |
| --- | --- | --- |
| Công cụ kiểm kê | 4 unittest PASS | Không chứng minh đủ mọi màn hình |
| Context C++ | 7 test cases, 38 assertions PASS | Chưa điều khiển game thật |
| Token lời đọc Java | 24 assertions PASS | Chưa có TTS Android thật |
| Driver Java với test doubles | 31 assertions PASS | Không emulation Binder hoặc âm thanh |
| API 35 và Gradle Java/resources | [PASS ở cb3c082](https://github.com/tvhuy99-web/Abcd-huy/actions/runs/39723157190) | Compile không phải nghiệm thu thiết bị |
| UI emulator API 35 | [Đủ 2 tests PASS ở cb3c082](https://github.com/tvhuy99-web/Abcd-huy/actions/runs/39723157171) | Không âm thanh, không TalkBack, không engine game |
| Baseline ARM64 đầy đủ | [Build và kiểm tra gói PASS](https://github.com/tvhuy99-web/Abcd-huy/actions/runs/39717924057), đúng `8ef359c` | Chưa xác nhận khởi chạy trên máy thật |
| Catalog tiếng Việt | gzip, UTF-8 và `msgfmt --check --check-format` PASS; 116272 mục dịch được công cụ ghi nhận | Chưa chứng minh chất lượng dịch hoặc phát âm |

Hai bài emulator kiểm tra nhãn/nút, dừng khi khởi tạo, đi nền/trở lại, recreate,
opt-in mạng trở về false và thoát Activity. Chạy trực tiếp bằng adb và yêu cầu
JUnit báo `OK (2 tests)`; không chấp nhận 0 tests như kết quả thành công.

### APK thử mới nhất

[Build ARM64 tại cb3c082](https://github.com/tvhuy99-web/Abcd-huy/actions/runs/39723157038)
đã PASS: compile đầy đủ trong 19 phút 15 giây, thư viện game/catalog Việt và
chữ ký hợp lệ. Đã tải artifact và đối chiếu lại digest ZIP, commit, checksum
APK, fingerprint chứng thư và hai tệp bắt buộc trong APK.

- Nguồn chính xác: `cb3c082e76fa90c156b813a42eebd7f2ba230b45`.
- [Artifact `android-arm64-candidate`](https://github.com/tvhuy99-web/Abcd-huy/actions/runs/39723157038/artifacts/12402693550),
  lưu trên Actions tới 05/11/2026; chỉ một APK ARM64 đầy đủ, không phải UI-only.
- Tên APK trong CI: `cataclysmdda-cb3c082-accessibility-arm64-v8a-debug.apk`.
  Bản bàn giao đổi tên thành `CDDA-Thu-giong-Viet-cb3c082-arm64.apk`, giữ nguyên bytes.
- Kích thước APK: 193408390 bytes. Android tối thiểu API 24 (Android 7), ARM64.
- SHA-256 APK: `2a77571bbbdc4ed737d1c3558c4c6a3e1a60aa515d9554e1031e95d4854bce58`.
- SHA-256 chứng thư thử: `4b2aa6143aee39bf179fecc853c803113ed8bb8c911b6251088ef945babf701f`.
- APK có `lib/arm64-v8a/libmain.so` và catalog Việt đã biên dịch. Không gán
  kết quả này cho commit có thay đổi mã chạy sau cb3c082; cập nhật báo cáo
  bằng tài liệu không tạo ra một APK mới.

Cài/chạy game trên thiết bị, âm thanh nghe thật và TalkBack vẫn chưa được
nghiệm thu. Hướng dẫn thử nằm ở [TESTING.vi.md](TESTING.vi.md).

### Những lỗi kiểm thử đã xử lý

- Môi trường làm việc không có SDK/NDK và không tải được Gradle; chuyển build
  đầy đủ sang CI, không quy việc cấp công cụ thất bại thành lỗi game.
- Lượt CI đầu thiếu `sdkmanager` trên PATH; đã thêm setup Android pin commit.
  Sau đó cả baseline và candidate build được.
- Emulator đầu thất bại trước test do UTP/ddmlib báo `device offline`:
  [0 tests, không PASS](https://github.com/tvhuy99-web/Abcd-huy/actions/runs/39721549333).
  Đổi sang adb trực tiếp, kiểm tra package service, cài đúng hai APK. Hai lượt
  sau đã thực chạy đủ hai tests: [7dcf5f7](https://github.com/tvhuy99-web/Abcd-huy/actions/runs/39722154518)
  và cb3c082 ở bảng trên.
- Sửa đường dẫn version header khi Gradle chạy từ repository root.
- Sửa dừng TTS lúc chưa có utterance để không ngắt việc khởi tạo engine;
  bổ sung kiểm tra voice thực tế và giọng cần mạng.

## Việc tiếp theo theo cổng nghiệm thu

1. Hoàn tất G0: cài/chạy baseline và APK thử trên Android thật; tiếp tục kiểm kê
   thủ công các luồng. Build nền đã đạt, kiểm thử runtime chưa đạt.
2. Hoàn tất G1: nghe giọng thực tế, thiếu voice, dừng, cài đặt/quay lại, TalkBack,
   offline và vòng đời; sau đó mở rộng bootstrap, cấu hình, JNI và phục hồi.
   Dùng [hướng dẫn thử](TESTING.vi.md); không bắt người dùng tắt TalkBack.
3. G2: nối ngữ cảnh/focus/token với input, menu, popup, nhập liệu và đầu vào
   cảm ứng; kiểm thử đóng/mở menu lồng không gây di chuyển ngoài ý muốn.
4. G3: hoàn thành chuỗi mở game → tạo nhân vật → hành động cơ bản → lưu/tải
   mà người khiếm thị không cần chỉ dẫn vị trí nút.
5. G4–G6: không gian/sinh tồn, chiến đấu/chế tạo và hệ thống nâng cao.
6. G7–G8: vòng đời nhân vật, dữ liệu, kiểm thử người dùng và phát hành.

Đọc [kế hoạch đầy đủ](PLAN.vi.md) cho định nghĩa nghiệm thu và 64 tình huống.
Mọi hàng chưa có bằng chứng vẫn để chưa đạt; thêm TTS hoặc mở được menu không
đủ để đánh dấu một hệ thống đã tiếp cận.
