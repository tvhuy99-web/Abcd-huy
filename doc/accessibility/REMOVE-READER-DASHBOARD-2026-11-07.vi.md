# Bỏ bảng điều khiển trình đọc Android

> **GHI CHÚ PHIÊN BẢN (13/11/2026):** Đây là hồ sơ chuyên đề theo thời điểm viết, không tự động là chỉ dẫn cho `master` sau PR #7. Khi cử chỉ, menu, quyền đọc bản đồ hoặc CI khác, ưu tiên [đặc tả hiện hành](CURRENT-REFERENCE.vi.md), [hướng dẫn](USER-GUIDE.vi.md) và [trạng thái](STATUS.md). **Không coi mã đã viết là nghiệm thu TalkBack trên máy thật.**

Nhánh: `integration/reader-vietnamese-20261107`.

**Đã bỏ khỏi ứng dụng**: màn hình có dòng “Đang chờ màn hình trợ năng”, các nút Trang trước/Trang sau, Hủy một tầng menu game, Menu tổng hợp và Điều khiển trình đọc, cùng các màn con và bộ mã điều khiển menu này. Nút Quay lại chỉ hủy thao tác/menu đang mở trong game, không mở bảng điều khiển riêng.

**Vẫn giữ**: luồng nhận snapshot trợ năng từ game, phát lời đọc TTS, cử chỉ duyệt và chọn mục trong game, các thông báo cảnh báo, và khả năng nhập văn bản cần thiết (như tên nhân vật). Phần cấu hình TTS riêng của Android vẫn giữ nguyên; không tự động bật TalkBack hệ thống.

**Lưu ý**: lớp chạm trong suốt vẫn nằm trên khung SDL để xử lý cử chỉ, nhưng không vẽ nút, bảng điều khiển hoặc lớp nền tối. Với một số màn nhập dữ liệu, trình soạn thảo cần thiết có thể mở tạm thời.

**Cần xác nhận thực tế**: đã vào menu chính chưa, có nghe tên mục khi vuốt hay không, TalkBack bật/tắt, phím Back, chuyển ngữ cảnh và nhập trường văn bản. Mã nguồn được thay đổi nhưng chưa được xác minh APK trên máy thật.

Bài test `ReaderOverlayRemovalTest` kiểm tra không có nút Android của dashboard lúc khởi động và sau khi bấm Quay lại.
