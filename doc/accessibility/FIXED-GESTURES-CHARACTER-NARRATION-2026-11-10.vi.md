# Cử chỉ cố định và đọc thông tin nhân vật bằng ba ngón (PR #7)

## Quy tắc không cho thay thế cử chỉ có chức năng

**Đã có chức năng mặc định => không thể đổi.** Cử chỉ chưa có
chức năng mặc định => được gán tùy chỉnh tại Menu hành động tổng hợp.
Quy tắc chỉ liên quan tới các cử chỉ trong ứng dụng khi đang chơi game;
các cử chỉ TalkBack do Android giữ quyền vẫn thuộc về TalkBack.

| Cử chỉ | Hành động cố định | Cho đổi? |
| --- | --- | --- |
| Vuốt một ngón lên/xuống/trái/phải | Đi bộ ở thế giới; điều hướng trong menu | Không |
| Chạm hai lần một ngón | Kích hoạt và xác nhận | Không |
| Chạm ba lần một ngón | Mở menu hành động | Không |
| Nhấn giữ một ngón | Hành động ngữ cảnh | Không |
| Vuốt hai ngón lên/xuống/trái/phải | Quét 16 ô trên bản đồ; xác nhận/quay lại trong menu theo ngữ cảnh | Không |
| Chạm một lần hai ngón | Đọc chi tiết mục đang chọn | Không |
| Chạm hai lần hai ngón | Đọc các thông báo game ở thế giới | Không |
| Chạm ba lần hai ngón | Mở menu hành động tổng hợp ở thế giới | Không |
| Chạm một lần ba ngón | Đọc tọa độ | Không |
| **Chạm hai lần ba ngón** | **Đọc đầy đủ thông tin nhân vật hiện tại** | **Không** |
| Chạm ba lần ba ngón | Chưa có chức năng mặc định | Có |
| Vuốt ba/bốn ngón theo bốn hướng | Chưa có chức năng mặc định | Có |
| Chạm một, hai, ba lần bốn ngón | Chưa có chức năng mặc định | Có |
| Nhấn giữ hai/ba ngón | Chưa có chức năng mặc định | Có |
| Vuốt một ngón theo hai nét (ví dụ lên rồi phải) | Chưa có chức năng mặc định | Có |

**Tổng cộng:** 36 mã cử chỉ nhận diện trong danh mục phím tắt thế
giới, trong đó 10 mã đã có chức năng và được giữ cố định; 26 mã
còn lại cho phép gán. Những cử chỉ mặc định do nhận dạng trực tiếp
như chạm hai lần một ngón và vuốt hai ngón vốn không nằm trong 36
mã, cũng được giữ cố định.

## Cách dùng chức năng đọc nhân vật

Ở màn hình thế giới, chạm nhanh **hai lần bằng ba ngón tay**.
Game đọc nội dung hiện có trong snapshot hiện tại, theo thứ tự:

- Tên nhân vật.
- Sức mạnh, khéo léo, trí tuệ và nhận thức.
- Giờ, đói, khát, buồn ngủ, đau, thể lực, nhiệt độ, tinh thần,
  mệt mỏi, hiệu ứng đang có.
- Tình trạng sức khỏe các bộ phận cơ thể.
- Chế độ an toàn.
- Vật đang cầm; nếu điều khiển xe sẽ có các mục trạng thái xe
  có sẵn trong báo cáo nhân vật.
- Kết thúc thông tin nhân vật.

Tính năng **không thực hiện hành động gameplay và không làm trôi lượt**.
Dùng đúng tên và các giá trị do engine cung cấp; không suy đoán HP
chính xác khi engine chỉ công bố tình trạng chấn thương bằng lời.
Nếu không có vật đang cầm thì không tự dựng một tên vật mới.

## Bảo vệ trước cấu hình của bản cũ

- `ReaderGestureBindings.isReserved()` ngăn ghi đè 10 mã có chức năng.
- `CUSTOM_IDS` và `CUSTOM_LABELS` loại các mã này khỏi menu gán
  phím tắt, nhưng **không xóa** các lựa chọn chưa có chức năng.
- Trên mỗi lần đọc cài đặt, những gán đã lưu từ bản trước cho mã
  cố định bị loại bỏ và bản đồ gán được làm sạch.
- Cách gán lại vuốt ngang/dọc trong thiết lập giọng nói đã được ẩn;
  tùy chọn cũ dù còn trong bộ nhớ cũng không thay đổi thao tác điều
  hướng thực tế.
- Người dùng vẫn có thể điều chỉnh độ dài vuốt, thời gian vuốt,
  hướng điều hướng thuận tay trái/phải; các tùy chỉnh này không
  thay đổi nhiệm vụ của cử chỉ.
- Việc bấm **Khôi phục cử chỉ mặc định** vẫn có thể đưa các cài đặt
  nhận dạng và danh mục tùy chỉnh về trạng thái ban đầu.

## Kiểm chứng trên thiết bị

1. Vuốt một ngón, vuốt hai ngón và kiểm tra không bị thay thế bởi
   gán cũ, kể cả sau khi nâng cấp từ APK trước.
2. Chạm hai lần ba ngón; TTS phải đọc thông tin nhân vật thực,
   không mở menu hay di chuyển nhân vật.
3. Chạm ba lần ba ngón hoặc dùng vuốt 3 ngón; xác nhận
   có thể gán và hoạt động như phím tắt tùy chọn.
4. Nhấn giữ một ngón luôn mở hành động ngữ cảnh; nhấn giữ
   hai hoặc ba ngón vẫn có thể gán lệnh.
5. Kiểm tra menu thiết lập không còn nút gán lại vuốt ngang/dọc.
6. Kiểm tra TalkBack đang bật có quyền ưu tiên với cử chỉ hệ thống;
   không cướp cử chỉ của Android để ép chức năng game.
7. Nếu cần thu nhật ký, bật ghi thủ công trước phép thử,
   xuất trước khi thoát phiên. Tắt ghi sẽ ngừng đo hoàn toàn.
