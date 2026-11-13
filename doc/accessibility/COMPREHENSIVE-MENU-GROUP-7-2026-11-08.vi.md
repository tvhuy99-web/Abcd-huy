# Nhóm 7: Ăn uống, nghỉ ngơi và y tế

> **GHI CHÚ PHIÊN BẢN (13/11/2026):** Đây là hồ sơ chuyên đề theo thời điểm viết, không tự động là chỉ dẫn cho `master` sau PR #7. Khi cử chỉ, menu, quyền đọc bản đồ hoặc CI khác, ưu tiên [đặc tả hiện hành](CURRENT-REFERENCE.vi.md), [hướng dẫn](USER-GUIDE.vi.md) và [trạng thái](STATUS.md). **Không coi mã đã viết là nghiệm thu TalkBack trên máy thật.**

Nhánh: `integration/reader-vietnamese-20261107`

Nhóm có 9 lệnh: `eat`, `open_consume`, `sleep`, `wait`,
`medical`, `bodystatus`, `workout`, `apply`, `player_data`.

## Đã thay đổi

- **Ăn uống và thuốc:** menu cho biết số lựa chọn; từng món ăn đọc
  năng lượng ước tính, tình trạng hỏng/gần hỏng và cảnh báo thực tế từ
  `can_eat`. Lọc thức ăn, nước và thuốc vẫn dùng menu game gốc.
- **Ngủ:** trước khi xác nhận có lời đọc buồn ngủ, khát, đau và nơi nằm;
  sau khi thực sự tỉnh dậy đọc giờ hiện tại và nhu cầu. Không tự động
  ngủ khi chỉ duyệt danh sách.
- **Chờ:** giữ các lựa chọn chờ và báo thức gốc; khi chờ lâu, TTS định kỳ
  đọc thời gian còn lại. Khi bị hủy đọc cảnh báo, khi hoàn tất giữ thông
  báo game.
- **Y tế:** danh sách TTS xếp nhiễm trùng, chảy máu, gãy xương, bị cắn
  và các vết thương lên trước; giữ nguyên giới hạn kiến thức về máu và
  thương tích. Chỉ chọn phương pháp điều trị mới gọi hệ thống chữa
  thương gốc và kiểm tra vật tư/kỹ năng.
- **Tình trạng cơ thể:** sơ đồ hình ảnh nay có danh sách từng bộ phận,
  tình trạng và chi tiết bộ phận đang chọn. Chạm/nghe mục không dùng
  thuốc; mục có sơ đồ con vẫn mở bằng thao tác kích hoạt.
- **Tập thể dục:** giải thích bốn mức cường độ, bước nhập số phút, lời
  đọc khi khởi động; giữ các kiểm tra thiếu nước, mệt, gãy xương và
  thiếu điều kiện trước khi bắt đầu.
- **Dùng vật phẩm:** dùng lại giao diện và cảnh báo của nhóm 6.
- **Thông tin nhân vật:** thêm mục đọc `Tóm tắt sức khỏe nhanh` gồm
  đói, khát, buồn ngủ, đau, mệt và cảnh báo thương tích nghiêm trọng.

Danh sách và tài liệu TTS chỉ dùng dữ liệu nhân vật đang có, không làm
lộ chỉ số HP chính xác nếu nhân vật không biết. Thao tác chỉ đọc không
tiêu lượt và không thay đổi đồ/điều trị.

## Kiểm thử

`python3 -m unittest discover -s tools/accessibility -p test_hub_health.py -v`

Cần kiểm tra thêm trên Android ARM64 và thiết bị có TalkBack, đặc biệt
bộ phận giả, ngủ bị đánh thức, lựa chọn báo thức, chờ bị ngắt, thuốc
cần dụng cụ, tập luyện với thiết bị và món ăn gây ngộ độc.
