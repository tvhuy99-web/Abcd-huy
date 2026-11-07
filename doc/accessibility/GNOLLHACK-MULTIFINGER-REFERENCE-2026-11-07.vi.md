# Khắc phục cử chỉ ba và bốn ngón trên Android (tham khảo GnollHack)

Nguồn tham khảo do người dùng cung cấp:
`GnollHack-BLIND-VI-PLAYER-TTS-FIX-20260831-R7.zip`.

## Bài học từ GnollHack

- `Platforms/Android/MainActivity.cs` yêu cầu DecorView
  `RequestDisallowInterceptTouchEvent(true)` khi nhận ngón thứ hai,
  mở khóa ở `Up/Cancel`. Điều này ngăn **cha trong ứng dụng**
  giành sự kiện, không thể vô hiệu hóa cử chỉ dành riêng của hệ điều hành.
- `AndroidAccessibilityGestureAdapter.cs` chuyển chính xác mỗi
  `PointerDown`, `PointerUp`, `Move` theo ID ngón, không nhầm
  giữa `pointerIndex` (thay đổi sau khi nhấc ngón) và `pointerId`.
- Android đôi khi báo `ACTION_CANCEL` ngay khi đã đặt đủ ba ngón.
  Adapter chỉ phục hồi khi đã nhận đúng ba ID và ngữ cảnh vẫn hợp lệ.
  Sau đó nó gửi các sự kiện nhả ngón để kết thúc thao tác tap.
- `AccessibilityGestureRecognizer.cs` đo thời gian gia nhập, đường
  chuyển động từng ngón, phân biệt chạm và vuốt, giới hạn nhận nhầm.

## Áp dụng có chọn lọc sang Cataclysm

1. `CataclysmDDA.dispatchTouchEvent` bảo vệ chuỗi đa chạm từ tầng
   Activity, và `ReaderGestureSurface` bảo vệ ở tầng View.
2. Ghi nhận ID ngón, tọa độ ban đầu, thời gian tham gia. Không phục hồi
   các cử chỉ đến muộn, kéo lệch, quá thời gian hoặc mất ID.
3. `ACTION_CANCEL` khi đã chạm 3/4 ngón có thể được khôi phục **chỉ
   như một lần tap**, khi thuộc màn chơi, tất cả ID vẫn được báo và
   khoảng dịch chuyển nhỏ. Không chuyển `CANCEL` thành swipe.
4. Giữ hàng đợi tap đã được nhận trước đó để chạm hai/ba lần bằng 3/4 ngón
   không bị xóa vì Android gửi `CANCEL`.
5. Không thay đổi cử chỉ vuốt một ngón di chuyển, vuốt hai ngón
   quét bản đồ hoặc Back tại menu, hay chạm một ngón ba lần mở Actions.
6. Có bài Android instrumentation mô phỏng chạm ba ngón bị cancel,
   cancel sau khi di chuyển (phải bị loại), chạm và vuốt bốn ngón.

## Giới hạn

- Nếu Android hoặc TalkBack **không chuyển ACTION_DOWN/POINTER_DOWN**
  đến ứng dụng, game không thể khôi phục chuỗi mà nó chưa từng nhận.
- Tương tự, một số cử chỉ bốn ngón có thể dành riêng cho hệ thống;
  không nên thay đổi cài đặt hệ thống hoặc vô hiệu hóa TalkBack.
- Chạm một ngón ba lần vẫn là đường thay thế để mở menu Hành động.
  Các lệnh khác có thể gán cho cử chỉ vuốt một ngón.
- Kiểm thử mô phỏng và biên dịch không thay thế thử thật trên điện thoại;
  cần ghi lại `adb logcat` nếu máy vẫn làm mất sự kiện.
