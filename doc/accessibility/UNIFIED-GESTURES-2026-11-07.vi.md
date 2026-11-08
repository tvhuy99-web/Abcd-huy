# Cử chỉ thống nhất trên các màn hình game và Android

Áp dụng trên bản `accessibility` của nhánh `integration/reader-vietnamese-20261107`.

| Khu vực | Hai ngón vuốt xuống | Một ngón vuốt lên/xuống trên thanh số/đơn lựa chọn |
|---|---|---|
| Cài đặt khởi động, trợ giúp và cứu hộ | Quay lại màn trước hoặc thoát khi thích hợp | Tăng/giảm thanh nếu có |
| Cài đặt TTS và cấu hình giọng phụ | Quay lại/hủy thay đổi chưa lưu | Tăng/giảm tốc độ, cao độ, âm lượng và các thiết lập có thanh trượt |
| Menu chính và các menu game có trợ năng | Hủy một cấp menu/điều hướng quay lại | Thay đổi trường có miền giá trị và bước hợp lệ; dùng cơ chế bản nháp của game |
| Trong quá trình chơi nhân vật trực tiếp | **Giữ quét bản đồ theo hướng**, không coi là Quay lại | Giữ cử chỉ di chuyển nhân vật; không tự điều chỉnh mục |
| TalkBack Android đang hoạt động | TalkBack có quyền xử lý cử chỉ của hệ thống | Sử dụng hành động trợ năng của TalkBack |

Cử chỉ hai ngón dùng phân biệt hai ngón cùng đi xuống, không mở bảng điều khiển đã bỏ. Cử chỉ trên màn hình game dùng lớp `ReaderGestureSurface`, màn Android riêng dùng `BootstrapSelfVoiceSurface` hoặc `SelfVoicingSetupSurface`.

Mọi thay đổi trường số/đơn lựa chọn được gửi vào engine qua `set_value`/`select_choice`, không tự động nhấn nút **Áp dụng**. Các trường không công bố miền/bước sẽ giữ thao tác mặc định để tránh gửi giá trị sai.

**Giới hạn:** Các hộp thoại do Android hệ thống tạo và phần Cài đặt giọng của hệ điều hành do TalkBack/hệ thống kiểm soát. Cần chạy các bài Android instrumentation trên giả lập và kiểm chứng cử chỉ bằng tay trên điện thoại thật trước khi bàn giao APK.
