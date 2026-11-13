# Menu tổng hợp, nhóm 4: Di chuyển và tư thế

> **GHI CHÚ PHIÊN BẢN (13/11/2026):** Đây là hồ sơ chuyên đề theo thời điểm viết, không tự động là chỉ dẫn cho `master` sau PR #7. Khi cử chỉ, menu, quyền đọc bản đồ hoặc CI khác, ưu tiên [đặc tả hiện hành](CURRENT-REFERENCE.vi.md), [hướng dẫn](USER-GUIDE.vi.md) và [trạng thái](STATUS.md). **Không coi mã đã viết là nghiệm thu TalkBack trên máy thật.**

Nhánh: `integration/reader-vietnamese-20261107`

- **Chọn hướng di chuyển**: tám hướng. Kích hoạt một hướng gửi lệnh của game; Quay lại trở về nhóm 4.
- **Menu chọn tư thế**: dùng lại lệnh native `open_movement`; các lệnh chạy, cúi, bò, về đi bộ và xoay tư thế vẫn có.
- **Lời đọc tư thế**: phân biệt đang dùng và vừa chọn; tư thế có thể áp dụng khi thực hiện hành động kế tiếp.
- **Lên/xuống tầng**: nếu tầng không đổi, nói rõ chưa chuyển tầng; không đoán thông tin địa hình ẩn.
- **Chờ**: đọc lựa chọn thời gian bắt đầu chờ. Đứng yên một lượt có thông báo riêng.
- **Tự di chuyển**: đọc bật/tắt; khi bật, ra lệnh theo hướng có thể tự tìm đường xa hơn. Thông báo khi không có đường, không đặt điểm đến cố định.
- **Lưu ý**: nếu chế độ tự di chuyển đang bật, chọn một trong tám hướng vẫn tuân theo cơ chế đi xa hơn của game.

Kiểm thử: `python3 -m unittest discover -s tools/accessibility -p test_hub_movement.py -v`.

Cần thử thực tế trên Android với TalkBack, tư thế, đường lên xuống tầng, chờ và đường bị chặn. Không sửa `master`.
