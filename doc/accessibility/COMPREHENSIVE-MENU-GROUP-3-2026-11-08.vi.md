# Menu tổng hợp, nhóm 3: Quan sát và bản đồ

## Phạm vi hiện có

Bộ tìm kiếm của game nằm trong `src/surroundings_menu.h`: `tab_data::radius = 60`. Đó là **bán kính 60 ô**, không phải giới hạn 60 dòng. Các mục chỉ xuất hiện khi game cho nhân vật phát hiện: vật phẩm và địa hình theo tầm nhìn, sinh vật theo `get_visible_creatures( 60 )`. Giữ nguyên ranh giới hiểu biết và phạm vi này.

## Danh sách xung quanh nâng cấp trên Android

Từ menu tổng hợp → **Quan sát và bản đồ** → **Danh sách xung quanh**:

- Khi trình đọc Android bật, hiển thị **một danh sách hợp nhất** của vật phẩm, sinh vật và địa hình/nội thất hiện có. Mỗi vị trí được liệt kê riêng (không dồn nhiều vị trí khác nhau dưới cùng một tên).
- **Sắp xếp từ gần tới xa**, cùng khoảng cách thì theo tên. Mỗi dòng đọc loại, tên, số ô cách xa và hướng tương đối. Chỉ tạo tối đa 80 nút cho một trang, có **Trang trước/Trang sau** để đi qua mọi kết quả; không cắt bán kính 60 ô.
- **Chạm hai lần** lên mục sẽ dùng `game::safe_route_to()` để tự tìm đường. Với vật phẩm hoặc ô đi qua được, game nhắm ô đích. Với sinh vật hoặc ô không thể đi vào, nhân vật dừng cách tối đa một ô. Đường đi tuân theo các quy tắc tránh nguy hiểm của game; không tìm được đường phải phát thông báo lỗi. Chọn mục **không dịch chuyển tức thời** và không tự nhặt vật phẩm.
- Khi đã ở đích, game thông báo rằng đã ở tại hoặc cạnh mục đã chọn; không tạo lệnh di chuyển vô ích.
- Khi trình đọc tắt, giữ giao diện danh sách ba tab trực quan nguyên bản.
- Danh sách là ảnh chụp tại thời điểm mở. Sinh vật có thể di chuyển sau khi chọn, nên tuyến đường không phải lệnh tự bám đuổi sinh vật.

## Ba chỉnh sửa còn lại

1. **Ô hiện tại**: kể cả khi ô chứa dữ liệu đáng chú ý và được phân loại `alert`, mục vẫn được giữ trong menu và khi kích hoạt sẽ đọc nội dung ô, không gửi lệnh sai sang game.
2. **Nhìn lén**: trên Android có trình đọc, chọn một trong 10 hướng (8 hướng mặt phẳng, lên, xuống). Việc nhìn lén vẫn tiêu hao thời gian và áp dụng giới hạn của engine, không biến thành quét miễn phí.
3. **Bầu trời / bản đồ thời tiết**: khi đang xem lớp thời tiết, đọc tên thời tiết tại con trỏ bản đồ nếu tầm nhìn thực tế cho phép, nếu ngoài tầm nhìn thì nói không biết thời tiết. Không dùng TTS để tiết lộ vùng thời tiết không quan sát được.

## Kiểm thử

- `python3 -m unittest discover -s tools/accessibility -p test_nearby_accessible_navigation.py -v`.
- Android Java CI và APK ARM64 native CI. Cần thử bằng điện thoại: danh sách nhiều trang, sinh vật/đồ vật cùng tên ở nhiều vị trí, ô không tới được, mở cửa, nhìn lén, bản đồ thời tiết, Ô hiện tại.
- Không sửa `master` hoặc dữ liệu bản lưu của người dùng.
