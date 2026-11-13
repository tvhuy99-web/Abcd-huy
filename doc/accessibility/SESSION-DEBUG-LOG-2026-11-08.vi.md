# Nhật ký gỡ lỗi từng phiên, trạng thái mã hiện tại

**Đối chiếu `master@f0be189`, 13/11/2026.** Bản mô tả trước đó nói “mặc định bật”, giữ `crash_recovery.txt`, xuất phiên bị văng trước. Các điều đó **không đúng ở mã hiện tại**; không dùng quy trình cũ.

## Giao diện

`DebugLogActivity` có **Ghi nhật ký**, **Xuất nhật ký**, **Xóa nhật ký**. Ghi nhật ký **mặc định tắt** và chỉ bật khi người dùng chọn; có thể bật/tắt nhiều lần trong một phiên. Không còn lựa chọn xuất log của phiên bị văng trước. Công tắc **chỉ đọc bằng giọng nói/đồ họa thế giới** nằm trong `AccessibilitySettingsActivity`, không nằm ở trang nhật ký.

## Vòng đời

- `SessionDebugLog` tạo một journal mới khi tiến trình mở. Cố xóa/truncate `active.txt` cũ, xóa `active.marker` và `crash_recovery.txt` cũ.
- Phiên ghi hiện tại giới hạn khoảng **5 MiB**, khi đầy giữ phần nội dung gần nhất. Khi tắt ghi, các sự kiện sau đó không được lưu; dữ liệu đã ghi trong phiên vẫn có thể được xuất.
- **Phải xuất trước khi khởi động lại game** nếu muốn giữ log. Game không bảo đảm lưu lại log để cho xuất trong lần mở kế tiếp.
- Xóa nhật ký là thao tác riêng có xác nhận. Không tự gửi nhật ký đi đâu.
- Công tắc ghi là điều khiển thủ công duy nhất. Không tự bật lại khi chậm, không tự dừng sau những bước đi nhanh.

## Cách báo lỗi

Bật Ghi nhật ký → tái hiện vấn đề → mở lại Nhật ký gỡ lỗi → Xuất nhật ký bằng bộ chọn tệp Android → tùy ý tắt Ghi nhật ký. Ghi thêm commit APK, mẫu máy, TTS engine/TalkBack và đường thao tác. Chưa có bằng chứng đã thử mọi đường lỗi trên thiết bị thực tế.

Nguồn mã: `SessionDebugLog.java`, `DebugLogActivity.java`, `GameReaderController.java`. [Đặc tả hiện hành](CURRENT-REFERENCE.vi.md).
