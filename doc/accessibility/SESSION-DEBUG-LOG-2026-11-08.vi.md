# Nhật ký gỡ lỗi từng phiên, có khôi phục sự cố (phương án B)

## Giao diện

Ở bản Android, **Nhật ký gỡ lỗi** là mục thứ 10 của menu chính, sau **Thoát**. Mở ra một màn riêng chỉ có ba điều khiển từ trên xuống dưới: **Ghi nhật ký** (công tắc, mặc định bật), **Xuất nhật ký**, **Xóa nhật ký**. Không có phần mô tả dài trên màn.

**Xuất nhật ký** mở trình chọn vị trí lưu tệp TXT của Android, không dùng quyền truy cập toàn bộ kho tệp. Nếu có nhật ký của lần bị văng, trước khi xuất cho phép chọn **Phiên bị văng trước** hoặc **Phiên hiện tại**. Tệp đã xuất nằm ở vị trí người dùng chọn và không tự bị xóa khi game đóng.

**Xóa nhật ký** hỏi xác nhận, sau đó xóa dữ liệu phiên hiện tại và nhật ký sự cố chưa xuất.

## Vòng đời và giới hạn

- Tệp hoạt động: `files/session_debug/active.txt`, giới hạn cứng **5 MiB = 5 × 1.048.576 byte**. Khi vượt ngưỡng, xóa những dòng cũ nhất, giữ khoảng 4 MiB gần nhất và tiếp tục ghi.
- Đánh dấu phiên đang mở: `active.marker`. Khởi động ứng dụng lần đầu trong tiến trình sẽ kiểm tra và tạo tệp hoạt động mới. Không bao giờ nối nhật ký của hai phiên.
- Nếu tiến trình cũ chết mà chưa xóa dấu, giữ tệp cũ thành `crash_recovery.txt` **trước khi** tạo tệp mới. Tệp khôi phục này không bị xóa khi thoát bình thường. Một lần bị văng mới có thể thay thế nhật ký khôi phục chưa xuất của sự cố trước nữa.
- Thoát game bình thường từ mục Thoát của menu hoặc Activity thực sự kết thúc: xóa ngay `active.txt` và `active.marker`. Chuyển sang nền bằng Home hoặc đổi Activity không tính là thoát.
- Trong trường hợp ứng dụng bị hệ điều hành cưỡng chế kết thúc, Java không nhất thiết nhận callback; dấu còn lại cho phép nhận biết trên lần mở sau.
- Tắt công tắc chặn ghi sự kiện mới, giữ dữ liệu đã có để xuất hoặc xóa. Bật lại tiếp tục ghi trong tệp hiện tại.

## Ghi thông tin

Mỗi dòng ghi giờ UTC, `elapsed_ms` tính từ lúc thiết bị khởi động, tên sự kiện và dữ liệu.

- **LIFECYCLE, SCREEN, SNAPSHOT**: vòng đời màn hình; loại màn chơi; mục được chọn; số nút.
- **TOUCH, GESTURE, GESTURE_BINDING, GESTURE_ACTION**: số ngón, số lần chạm, hướng vuốt, thời lượng, cử chỉ đã nhận diện, lệnh được gán, chấp nhận hay từ chối.
- **INTENT, INTENT_RESULT**: mã yêu cầu, màn hình và nút, hành động gửi qua JNI, kết quả nhận và độ trễ.
- **NATIVE_ACTION**: tên hành động được game thực hiện (không chỉ bộ đọc), trạng thái hoàn thành/từ chối và số mili giây trong khối xử lý.
- **GAME_SPEECH, TTS_REQUEST, TTS_STATE, TTS_EVENT**: chính xác chuỗi yêu cầu đọc, mã đoạn, hàng đợi TTS, sự kiện STARTED/DONE/CANCELLED/ERROR, thời gian xếp hàng và thời gian phát được callback xác nhận.
- **NATIVE_ERROR, DEBUG_PROMPT, FATAL**: thông báo lỗi C++, mã lỗi giao diện và stacktrace lỗi Java không bắt được.
- Khi xuất, thêm phần cuối `config/debug.log`, `config/crash.log` và ở phiên bị văng thêm `debug.log.prev` (nếu có). Tệp xuất vẫn tối đa 5 MiB, các phần quá cũ sẽ bị cắt và có ghi dấu.

## Hạn chế quan sát và bảo mật

Các sự kiện do Android hoặc engine không gửi tới ứng dụng **không thể** được ghi trọn vẹn, nhất là khi kernel/OS trực tiếp giết tiến trình, lỗi native trước khi file được ghi, hoặc âm thanh phát ra thực tế từ TalkBack. Callback `STARTED` chứng minh công cụ TTS nhận đoạn, không chứng minh loa thực sự phát tiếng. Thời gian tác vụ bao gồm phạm vi đo được, không tự khẳng định đo toàn bộ các thao tác bất đồng bộ.

Không lưu nội dung ô nhập tự do, mật khẩu và giá trị trình chỉnh sửa trong `INTENT` (ghi `<redacted>`). Ngược lại, câu đưa tới TTS được ghi nguyên văn vì đó là thông tin cần tìm lỗi, có thể bao gồm tên nhân vật hoặc lời thoại. Nhật ký chỉ ở vùng riêng của ứng dụng và **không bao giờ tự gửi ra Internet**. Người dùng nên xem qua tệp trước khi chia sẻ.

## Kiểm thử

- `python3 -m unittest discover -s tools/accessibility -p test_session_debug_log.py -v`.
- `androidTest` `SessionDebugLogTest`: kiểm tra ghi vòng vượt 5 MiB, công tắc tắt, dọn bình thường, phục hồi khi có marker và xuất tệp của sự cố.
- CI Android Java và APK ARM64, sau đó cần xác nhận trên điện thoại: mở mục thứ 10, xuất và xóa, cử chỉ/TTS, thoát bình thường, buộc dừng bất thường rồi mở lại để khôi phục.
