# Kiểm thử bản thử trợ năng Android đầu tiên

Tài liệu này dành cho flavor `accessibility`. Chỉ dùng APK ARM64 đầy đủ từ
workflow build candidate, không dùng APK UI-only của bài kiểm tra giả lập.
Mã ứng dụng: `com.cleverraven.cataclysmdda.accessibility`; tên trên launcher:
“Cataclysm DDA - Thử trợ năng”. Nó có vùng dữ liệu/nhà cung cấp tài liệu riêng,
không dùng chung bản lưu với stable/experimental.

## Phạm vi của bản hiện tại

Hiện chỉ có màn thử giọng native và các thành phần nền. Chưa có bộ đọc bản đồ,
menu gameplay hoặc cử chỉ chơi độc lập. Giữ TalkBack khi thử màn này. Việc TTS
báo phát xong hoặc APK build thành công chưa chứng minh người dùng nghe được.

## Hành trình thử trên điện thoại

1. Giữ bản game đang dùng và bản lưu của nó. Cài APK thử riêng có định danh ở
   trên; không gỡ game cũ hoặc xóa dữ liệu để xử lý một lỗi cài đặt.
2. Bật TalkBack, mở “Cataclysm DDA - Thử trợ năng”. Ở cảnh báo trợ năng chọn
   “Thiết lập trợ năng và giọng đọc”. Nếu không tìm được mục này, ghi lại
   màn hình/câu đang nghe; chưa đánh dấu hành trình mở màn thử là đạt.
3. Nghe phần giải thích, trạng thái và tên từng nút. Kiểm tra có thể tới nút
   Phát thử, Dừng lời, cài đặt giọng, cài đặt trợ năng và Quay lại.
4. Giữ ô cho phép giọng cần mạng ở trạng thái bỏ chọn. Chọn phát thử. Phải nghe
   câu Việt hoặc thông báo lỗi rõ ràng, và có thể thử lại sau tối đa khoảng
   thời hạn khởi tạo. Thiếu voice Việt phải có đường mở cài đặt và quay về.
5. Nếu nghe được, ghi rõ đã nghe được bằng tai, tên engine/voice và có hiểu rõ
   từng từ không. Nếu có thông báo đã kết thúc nhưng không nghe âm, vẫn ghi là
   lỗi âm thanh, không coi là đạt.
6. Chọn Dừng lúc đang chuẩn bị hoặc đang đọc. Không có thao tác gameplay nào
   được thực hiện; màn hình vẫn dùng được. Khi chuẩn bị chưa xong, dừng lời
   không bắt buộc hủy cả quá trình khởi tạo engine.
7. Mở cài đặt giọng, quay lại; mở cài đặt trợ năng, quay lại. Không yêu cầu tắt
   TalkBack để trở về màn thử. Hành động mở cài đặt có fallback cho máy không
   nhận action giọng đọc trực tiếp; kiểm tra đường này trên từng hãng máy.
8. Đang thử giọng thì đưa app ra nền, khóa/mở khóa và trở lại. Không tự phát lại
   câu cũ. Nếu Android tạo lại Activity, màn mới phải về trạng thái chưa thử và
   ô giọng cần mạng phải bỏ chọn.
9. Chọn Quay lại. Phải trở về màn khởi động/cảnh báo và tiếp tục điều khiển được
   bằng TalkBack. Đừng đánh dấu phần chơi game đã tiếp cận từ bài thử này.

Nếu cần kiểm tra ngoại tuyến, trước hết xác nhận TalkBack của thiết bị dùng
được khi không có mạng. Thử với voice đã cài dữ liệu và không yêu cầu mạng;
kiểm tra bằng âm nghe thực tế, không chỉ dựa trên cờ metadata của engine.
Giọng online chỉ được dùng khi người thử chủ động chọn ô cho phép.

## Báo cáo tối thiểu

- APK/commit/checksum và nguồn tải.
- Hãng/mẫu máy, Android, engine/voice, trạng thái TalkBack và đường âm thanh.
- Bước đã thực hiện, điều mong đợi, câu thực tế nghe được hoặc lỗi nhận được.
- Có cần người sáng mắt trợ giúp không; nếu có, tại bước nào.
- Có trở về/khôi phục được không. Không cần gửi bản lưu cá nhân hoặc log không
  liên quan để chứng minh lỗi của màn thử giọng.

## Ý nghĩa các lớp kiểm thử tự động

- Host C++: sở hữu ngữ cảnh/token; chưa điều khiển game thật.
- Host Java: token và driver với test doubles; không emulation Android/audio.
- Compile API/Gradle: kiểu dữ liệu, Java và resource thật; chưa chứng minh UI chạy.
- Emulator: hai bài về nhãn/nút, lỗi/khởi tạo, dừng, recreate, opt-in mạng và thoát
  của riêng Activity thử giọng. APK UI-only không có engine game; emulator chạy
  không âm thanh. Không coi bài này là kiểm thử giọng Việt nghe thật hoặc TalkBack.
- APK ARM64 đầy đủ: kiểm tra build/đóng gói và bản dịch; còn cần chạy trên máy.
- Nghiệm thu người dùng: bằng chứng độc lập, bắt buộc cho mục tiêu chơi trọn vẹn.
