# Ghi chú khôi phục Menu tổng hợp: đã đối chiếu phiên bản hiện tại

**Mốc lịch sử ban đầu: 08/11/2026. Đính chính mã `master@f0be189` ngày 13/11/2026.** Các số “16 lựa chọn”, “36 cử chỉ đều có thể chọn” và “Tìm lệnh đầu tiên” trong bản cũ không còn đúng.

## Trạng thái hiện hành

- Menu tổng hợp có **17 mục cấp đầu**: Yêu thích; 15 nhóm; Tìm lệnh. Không phải bảng điều khiển luôn hiện.
- Có thể mở ở thế giới bằng **chạm ba lần một ngón** hoặc **ba lần hai ngón**. Chạm hai lần để chọn, điều hướng menu không tự di chuyển nhân vật.
- Tìm lệnh, Tất cả lệnh và Yêu thích dùng danh sách hành động đang được game công bố; mục không khả dụng ở ngữ cảnh hiện tại không được gửi như lệnh hợp lệ.
- `ReaderGestureBindings` có 36 **mã nhận diện**, nhưng **11 mã cố định không gán lại**, chỉ **25 mã tùy chỉnh**.
- Chạm ba lần ba ngón đã được dùng để bật/tắt chiến đấu. Nhấn giữ một ngón đã là hành động theo ngữ cảnh.
- Các hành động qua menu vẫn được C++ kiểm tra bằng token và điều kiện màn hình trước khi thực hiện.

## Kiểm chứng còn thiếu

Thử TalkBack/TTS, giữ focus khi đóng/mở menu, Yêu thích không khả dụng, nhiều trang danh sách, shortcut cũ, mở menu và quay lại sau chuyển trạng thái thế giới. Không coi build APK là bằng chứng nghiệm thu.

Tham khảo [đặc tả hiện hành](CURRENT-REFERENCE.vi.md) và [hướng dẫn sử dụng](USER-GUIDE.vi.md). Lịch sử khác biệt của bản tháng 11 được giữ trong Git.
