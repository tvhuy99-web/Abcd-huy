# Cử chỉ trực tiếp và menu tổng hợp trong thế giới

Nhánh: integration/reader-vietnamese-20261107

## Cử chỉ khi nhân vật đang ở thế giới

* Vuốt một ngón lên/xuống/trái/phải: đi Bắc/Nam/Tây/Đông, mỗi cử chỉ là một lệnh di chuyển. Không yêu cầu bật chế độ di chuyển nhanh hay luyện tám hướng.
* Vuốt hai ngón theo bốn hướng: quét tuyến thẳng 16 ô ở hướng tương ứng. Đọc từ ô 1 đến ô 16, không ưu tiên nguy hiểm, không cần đã nhìn thấy ô. Đọc dữ liệu địa hình, đồ đạc, bẫy, sinh vật, xe, vật phẩm và trường hiệu ứng từ bản đồ đã nạp, kể cả ở sau tường.
* Chạm hai ngón một lần: đọc trạng thái nhân vật.
* Chạm hai ngón hai lần: đọc thông báo gần đây.
* Chạm hai ngón ba lần: mở menu tổng hợp.
* Chạm ba ngón một lần: đọc tọa độ X/Y/Z của nhân vật (tọa độ ô tuyệt đối trong game).

Quét và đọc thông tin không di chuyển nhân vật và không tiêu lượt. Khi đang lái xe, ngủ, tự đi, thực hiện hoạt động hoặc ở màn hình chọn mục tiêu, vuốt một ngón không phát lệnh đi bộ sai ngữ cảnh. Cử chỉ nhiều ngón chỉ có tác dụng trên màn hình chơi và khi TalkBack không chiếm quyền khám phá bằng chạm. Nút có nhãn **Menu tổng hợp** và nút Quay lại là lối vào dự phòng.

## Menu tổng hợp

Mở bằng ba lần chạm hai ngón, nút Menu tổng hợp hoặc nút Quay lại khi đang ở màn hình chơi. Menu gốc gồm 16 lựa chọn: Tìm lệnh, Việc có thể làm tại đây, Quan sát và bản đồ, Di chuyển và tư thế, Tương tác với môi trường, Túi đồ và vật phẩm, Ăn uống/nghỉ ngơi/y tế, Trang bị và sửa chữa, Chiến đấu/vũ khí/phép thuật, Chế tạo và xây dựng, Nhân vật và năng lực, Nhiệm vụ và quan hệ, Phương tiện và hành trình, Quản lý khu vực và tự động hóa, Thông tin/trình đọc/trợ giúp, Hệ thống/TTS/lưu game.

Vuốt một ngón để duyệt, chạm hai lần để chọn, Quay lại để lên tầng menu trước hoặc trở về thế giới. Khi menu mở, cử chỉ một ngón không điều khiển nhân vật. Các nhóm lấy những lệnh đang được game công bố trong snapshot, không tạo ra lệnh giả. Mục Tất cả lệnh cho phép truy cập mọi lệnh có mặt trong snapshot hiện tại, kể cả lệnh chưa được phân nhóm. Mục Tìm lệnh hỗ trợ nhập tiếng Việt có hoặc không dấu. Khi thực hiện lệnh, menu đóng, lệnh vẫn phải qua bộ kiểm tra native của game; các màn hình nhiều bước như chế tạo, ngắm bắn, sử dụng đồ vật tiếp tục dùng giao diện tương ứng đã có.

## Lưu ý kiểm thử

* Android Java phải được biên dịch và C++ native phải được biên dịch qua CI/build gốc; thử nghiệm thực tế trên thiết bị vẫn là bước độc lập.
* Trên hệ thống bật TalkBack và khám phá bằng chạm, Android có thể giữ quyền điều khiển cử chỉ đa ngón; người dùng vẫn có nút giao diện có nhãn và nút Quay lại.
* Quét 16 ô lấy từ bản đồ hiện đang nạp, không tự tạo dữ liệu ngoài vùng đã nạp; hiện các lượt quét 16 ô quanh nhân vật thông thường nằm trong vùng này.
* “Việc có thể làm tại đây” là tập lệnh tương tác nhanh, game sẽ quyết định lệnh có khả dụng tại ô đang nhắm hay không.

## Gán cử chỉ cho từng mục trong menu

Khi đang ở menu tổng hợp, di chuyển đến bất kỳ mục nào và **nhấn giữ** để mở
màn hình **Gán cử chỉ cho: tên mục**. Danh sách này cho phép duyệt, chọn và
lưu một trong **33 cử chỉ**: bốn vuốt đơn, 12 chuỗi vuốt hai hướng,
ba dạng chạm hai ngón, ba dạng chạm ba ngón, bốn vuốt ba ngón, bốn vuốt
bốn ngón và ba dạng chạm bốn ngón.

Nút **Khôi phục cử chỉ mặc định** loại bỏ gán tùy chỉnh của mục đang chọn.
Nút **Hủy** đóng màn hình gán mà không lưu. Các cử chỉ đã gán được lưu trong
SharedPreferences và sử dụng ở màn hình chơi thực, không áp dụng vào
cử chỉ duyệt menu. Các lệnh chỉ được thực thi thông qua snapshot native đang
hợp lệ. Lệnh không khả dụng được thông báo thay vì thực hiện nhầm.

**Khi xung đột:** lệnh được gán mới nhất chiếm quyền sử dụng cử chỉ.
Lệnh từng có gán tùy chỉnh ở cử chỉ đó tự mất gán và trở về cách hoạt động
mặc định của game; không thực hiện hai lệnh trong cùng một cử chỉ.
Khi một lệnh được chuyển sang cử chỉ mới, gán cũ của chính lệnh đó cũng
được giải phóng. Đối với cử chỉ mặc định như vuốt lên để đi Bắc, gán mới
sẽ ghi đè hành động mặc định đó khi đang trong thế giới; chọn khôi phục
mặc định để trả lại chức năng đi Bắc.

TalkBack có thể giữ quyền xử lý một số cử chỉ nhiều ngón. Trong trường hợp
đó, người dùng vẫn có thể tương tác bằng nút màn hình có nhãn, nhưng cần
kiểm thử thực tế trên từng thiết bị.
