# Tốc độ TTS và các cử chỉ chạm giữ: đính chính

**Đối chiếu `master@f0be189` ngày 13/11/2026.** Đây là trang chuyên đề đã cập nhật từ mốc tháng 11, không phải kết quả nghiệm thu thiết bị.

## Tốc độ TTS

Trang `TtsSettingsActivity` có thanh tốc độ từ **0,5 đến 10,0 lần**, bước 0,1. `ReaderPreferences` lưu tối đa 10,0. Độ cao 0,5–2,0 và âm lượng 0–100%. Mức tốc độ thực phụ thuộc engine/voice cài trên điện thoại; hãy nghe thử để biết có rõ hay không. Mã hiện tại **không yêu cầu `previewPassed` trước nút Lưu**.

## Nhấn giữ và gán cử chỉ

`ReaderGestureBindings.IDS` định nghĩa 36 mã gồm vuốt một ngón, vuốt nhiều ngón, chạm nhiều ngón và nhấn giữ. **Không được diễn giải là 36 lựa chọn có thể gán**: `isReserved` hiện giữ **11 mã** gồm vuốt một ngón bốn hướng, chạm 2 ngón 1/2/3 lần, chạm 3 ngón 1/2/3 lần và **`hold_1`**. Chỉ còn **25 mã có thể gán**.

Nhấn giữ **một ngón** đã là hành động theo ngữ cảnh, không thể dùng bộ gán để ghi đè. Nhấn giữ **hai hoặc ba ngón** có thể được dùng làm shortcut tùy chỉnh ở các ngữ cảnh được hỗ trợ, nhưng có thể bị TalkBack giữ cử chỉ.

Các thao tác điều khiển cơ bản còn do `ReaderGestureSurface` nhận trực tiếp, không phải mục nào cũng xuất hiện trong danh sách 36 mã.

## Còn cần thử

Kiểm tra nhận dạng giữ 2/3 ngón khi nhấc tay, chuyển ngữ cảnh, có TalkBack, lưu shortcut giữa các phiên, và khả năng nghe TTS 10,0 lần trên máy thật. [Đặc tả cử chỉ](FIXED-GESTURES-CHARACTER-NARRATION-2026-11-10.vi.md).
