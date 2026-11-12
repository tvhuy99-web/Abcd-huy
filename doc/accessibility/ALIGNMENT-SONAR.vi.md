# Sonar định hướng vật phẩm và sinh vật trên Android

## Bật và tắt

Trong Cài đặt trợ năng có nút **Âm thanh định hướng vật phẩm và sinh vật**.
Mặc định **tắt**. Khi tắt, C++ bỏ qua phép quét sonar, Android không
phát âm và không giữ lại kết quả đã quét. TTS và gói âm thanh game
không phụ thuộc công tắc này.

## Hai âm thanh độc lập

- **Vật phẩm nhìn thấy:** a11y_item_beep.ogg, chuyển từ beep.wav của người dùng, 0,096 giây.
- **NPC và quái vật nhìn thấy:** giữ lại a11y_alignment_sonar.ogg gốc.
- Nếu có cả hai loại, cả hai phát, kể cả khi nằm trên cùng một hướng.
- Khoảng cách của mỗi loại trong từng hướng được tính độc lập. Âm lượng giảm
  từ 90% ở 1 ô xuống 10% ở 16 ô.

## Tám đường quét

Bắc, Đông Bắc, Đông, Đông Nam, Nam, Tây Nam, Tây, Tây Bắc.
Mỗi hướng là một đường thẳng gồm 16 ô, **không phải cả hình vuông**.
Không báo qua tường hoặc đối tượng chỉ tồn tại trong trí nhớ,
không báo vật phẩm nằm dưới chân nhân vật.
Chỉ phát hiện sinh vật và vật phẩm mà bộ quan sát thực sự nhìn thấy.

## Định hướng âm thanh

Android phát luân phiên tám hướng theo nhịp khoảng 160 ms,
mỗi vòng 1,28 giây nếu vẫn có mục tiêu.
Kênh trái/phải dùng cho Đông/Tây; cao độ cao hơn ở phía Bắc và
thấp hơn ở phía Nam; các hướng chéo kết hợp cả hai.
Đây không phải âm thanh vòm 3D và không thể phân biệt trái/phải
đầy đủ khi dùng loa mono.

## Trạng thái

Quét tối đa khoảng một lần mỗi 300 ms khi có màn hình thế giới,
cả khi tắt vẽ đồ họa. Dừng âm khi mở menu, chuyển ứng dụng,
mất tiêu điểm, tắt bộ đọc hoặc tắt riêng công tắc sonar.
Chưa thay thế được kiểm thử APK và TalkBack trên điện thoại thật.
