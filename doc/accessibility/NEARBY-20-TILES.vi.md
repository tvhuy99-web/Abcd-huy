# Danh sách xung quanh: quét 20 ô mỗi phía (Android)

Đây là **danh sách xung quanh** trên Android dành cho người dùng trình đọc,
không phải thao tác **quét một đường 16 ô theo hướng vuốt** và không thay đổi
danh sách đồ họa gốc.

- Duyệt các vòng hình vuông từ 0 đến 20 ô. Có đủ 41 × 41 = 1681 tọa độ,
  gồm hướng bắc 20, tây 20, nam 20, đông 20 và bốn góc chéo 20 ô.
- Dùng `a11y::observe_tile` để bảo vệ tính chính xác của khả năng nhìn,
  bẫy đã nhận biết, đồ trong vật chứa và cảm nhận đặc biệt. Không đọc
  tên địa hình thực từ những ô bị che khuất.
- Chỉ hiện sinh vật, vật phẩm, bẫy đã biết, mối nguy đang hoạt động,
  phương tiện (mỗi xe một dòng), dấu hiệu cảm nhận/âm thanh và các loại
  địa hình có công dụng cụ thể như cửa, cầu thang, thang, bảng, máy tính,
  thiết bị hoặc điểm tương tác.
- **Không** liệt kê sàn, cỏ, đất, đường, tường thông thường hay nội thất
  chỉ để trang trí. Không xếp quái vật trước đồ vật; thứ tự do khoảng cách
  quyết định, hòa thì giữ thứ tự duyệt vòng.
- Mục đọc chỉ gồm tên và hướng + số ô; không đọc câu hướng dẫn, số thứ tự
  dài hay tiêu đề "từ gần đến xa". Khi trống chỉ đọc thông báo ngắn.
- Chạm kích hoạt vẫn gọi `safe_route_to` cũ và tuân theo cơ chế tránh nguy
  hiểm; quét chỉ quan sát, không tự di chuyển.
- Các tab trực quan vẫn giữ logic 60 ô, nhưng không được khởi tạo thừa
  khi Android accessibility dùng danh sách riêng.

Cần thử trên điện thoại: đủ tám hướng tại 20/21 ô, ô khuất, bẫy giấu,
nhiều ô sàn giống nhau, xe dài, tên vật phẩm trong vật chứa, địa hình
chuyên biệt của mod, và hiệu suất mở menu. Không khẳng định đã thử máy
thật chỉ từ việc biên dịch thành công.
