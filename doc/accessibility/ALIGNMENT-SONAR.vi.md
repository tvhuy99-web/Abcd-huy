# Sonar căn hàng, bản Android trợ năng

## Trạng thái

Tính năng đã được thêm vào mã nguồn trên nhánh phát triển. Đây là bản mô tả theo mã,
không phải xác nhận rằng TalkBack và âm thanh đã được thử trên điện thoại thật.

## Phát âm thanh

- Mỗi lần sonar phát một đoạn âm thanh dài **đúng 1,0 giây**.
- Nếu còn mục tiêu phù hợp, khoảng cách giữa hai lần phát là **1.000 mili giây**.
- Android tự hẹn nhịp 1 giây. Nó không cần nhân vật di chuyển mỗi giây để tiếp tục báo.
- Độ lớn âm thanh tăng tuyến tính theo khoảng cách: 30 ô là mức khoảng 8%,
  1 ô là mức khoảng 90% của âm lượng mẫu phát qua SoundPool. Đây là hệ số
  của hiệu ứng, không phải phần trăm âm lượng tổng của điện thoại.
- Âm thanh được phát qua kênh âm hiệu ứng trợ năng, không gọi hàng đợi TTS
  và không phát một câu đọc dài mỗi giây.
- Sonar dừng khi không còn mục tiêu; khi đóng cửa sổ game, mở menu tổng hợp,
  mở màn hình khác không phải bản đồ nhân vật, gặp màn hình lỗi, hoặc tắt
  hoặc tạm dừng trình đọc. Mẫu âm thanh đang phát cũng bị dừng.

## Thế nào là cùng hàng

Vị trí của nhân vật là (0,0). Chỉ xét bốn hướng theo các trục:
Bắc (0,-d), Nam (0,+d), Tây (-d,0), Đông (+d,0),
trong đó d là số nguyên từ 1 đến 30.

- Quái vật, NPC và đồ vật trên mặt đất được xét nếu hệ thống quan sát
  trả về là **đang nhìn thấy**.
- Không căn theo đường chéo. Vật ở (2,2) hoặc (1,3) không kích hoạt.
- Không dùng tên hay vị trí của vật chưa nhìn thấy hoặc chỉ còn ghi nhớ.
  Dấu âm thanh đã nghe hoặc nhận biết đặc biệt không tự kích hoạt sonar.
- Nếu nhiều mục tiêu cùng thẳng hàng, độ lớn căn theo **mục tiêu gần nhất**.
- Sonar không cho biết mục tiêu thuộc hướng nào, không phân biệt quái vật
  với đồ vật và không tự dẫn đường.
- Khi mục tiêu đi khỏi hàng, biến mất khỏi tầm nhìn, hoặc cách xa quá 30 ô,
  game gửi -1 để dừng phát. Việc phát hiện lại diễn ra khi cảnh game được vẽ
  và lần quét cách lần trước ít nhất khoảng 300 mili giây.

## Ví dụ diễn giải bằng lời

Nhân vật đứng tại (0,0). Có một con quái vật nhìn thấy ở (0,-12).
Sonar phát mỗi giây một lần với âm lượng tương đối nhẹ.
Nhân vật tiến đến (0,-7): khoảng cách còn 5 ô, các tiếng sau mạnh hơn.
Nhân vật rẽ sang (1,-7): không còn cùng cột, sonar ngừng nếu không có
mục tiêu phù hợp nào khác nằm cùng hàng/cột.

Nếu một vật phẩm xuất hiện ở (3,0), sonar lại bật. Nếu có thêm quái vật ở
(9,0), độ lớn dựa vào vật phẩm ở 3 ô, không dựa vào quái vật ở 9 ô.

## Giới hạn cần kiểm thử trên Android thật

- Nhịp phát chịu ảnh hưởng việc thực thi hàng đợi chính của Android; đích
  là 1.000 ms chứ không phải hẹn giờ thời gian thực tuyệt đối.
- Nhịp quét C++ phụ thuộc vào lần vẽ màn hình. Nếu hình game không được
  cập nhật khi thế giới thay đổi, khoảng cách có thể tạm thời cũ.
- Tính dễ nghe của mẫu âm thanh bên cạnh TalkBack còn phụ thuộc điện thoại,
  tai nghe, âm lượng hệ thống và trộn âm thanh của Android.
- Cần xác nhận APK biên dịch thành công và thử tình huống có/không có vật,
  khuất tầm nhìn, mở menu, khóa màn hình, quay về ứng dụng, và TalkBack đọc.
