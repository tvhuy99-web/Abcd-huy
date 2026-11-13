# Chọn số lượng chỉ một lần, áp dụng chung cho game (trợ năng Android)

> **GHI CHÚ PHIÊN BẢN (13/11/2026):** Đây là hồ sơ chuyên đề theo thời điểm viết, không tự động là chỉ dẫn cho `master` sau PR #7. Khi cử chỉ, menu, quyền đọc bản đồ hoặc CI khác, ưu tiên [đặc tả hiện hành](CURRENT-REFERENCE.vi.md), [hướng dẫn](USER-GUIDE.vi.md) và [trạng thái](STATUS.md). **Không coi mã đã viết là nghiệm thu TalkBack trên máy thật.**

Ngày kiểm tra: 2026-11-10.
Nhánh tính năng: `feat/one-step-quantity-selection-20261110`.
Nhánh cơ sở: `perf/reader-swipe-latency-20261109`.

## Hợp đồng hành vi

1. **Vật phẩm có đúng một đơn vị**: chạm hai lần trên mục
   thì đánh dấu 1; chạm lại bỏ chọn. Không hiện bảng số.
2. **Vật phẩm có nhiều đơn vị**: mở bảng số hoàn chỉnh. Chọn
   `1`…`9`, `Tất cả`, hoặc `Bỏ chọn` sẽ **áp dụng vào
   danh sách ngay**, đóng bảng số, giữ tiêu điểm ở món vừa chọn.
3. **Số từ 10 trở lên không phải toàn bộ**: `Nhập số lượng khác`
   mở bàn phím nhập chữ số; người dùng nhập đủ số và xác nhận
   một lần. Nếu ngoài phạm vi, không được áp dụng sai hoặc làm
   mất lựa chọn cũ.
4. **Quay lại** từ bảng số không thay đổi số lượng đã chọn.
5. **Xác nhận chung** ở danh sách nhặt, thả, cho vào hộp,
   chuyển và giao dịch vẫn là hành động riêng. Chọn số chỉ
   thay đổi đánh dấu, không lấy, vứt hay bán đồ ngay.
6. **Tùy chọn, tạo nhân vật, nhập mã hoặc số không phải
   số lượng vật phẩm** giữ nguyên chế độ nhập và Áp dụng.
7. **Thông tin túi chứa / pocket** chỉ được đọc; không biến
   thành đối tượng chọn số lượng.
8. **Chặn trùng lệnh** khi một lựa chọn đang được gửi C++.
   C++ luôn kiểm tra lại vật phẩm, số lượng thực còn lại và
   phiên màn hình đang hoạt động trước khi chấp nhận.

## Những bộ điều khiển đã nối chung

- `inventory_selector::reader_session`:
  `inventory_multiselector` cho nhặt đồ, thả đồ, cho đồ
  vào hộp/túi, và bộ chọn món trong giao dịch. Bộ chọn để
  so sánh hai vật phẩm giữ nguyên hành vi đánh dấu 0/1;
  bộ chọn mở menu thao tác vật phẩm vẫn hoạt động bình thường.
- `pickup_here_accessible`: chức năng Nhặt đồ tại chỗ,
  gộp các món trùng tên và chọn số ngay.
- `advanced_inventory::query_charges`: chức năng chuyển
  đồ nhiều nguồn/đích. Nếu chỉ cho phép một đơn vị thì không
  mở bảng số; nếu có nhiều thì dùng bảng số chung.
- `number_input_popup` có phương thức
  `set_accessible_quantity_max` để các ngữ cảnh di chuyển
  số lượng khác nối vào; số thông thường không tự nhận
  hành vi chọn và đóng nếu chưa được khai báo là số lượng.
- `GameReaderController` và `ReaderTextEditor`: phân biệt
  bộ chọn số lượng đã khai báo với biểu mẫu số bình thường.
  Dùng chung cho trình đọc tích hợp và các nút tương thích
  TalkBack.

## Các ca kiểm tra quan trọng

| Tình huống | Mong đợi |
|---|---|
| 1 nĩa nhựa, 0 đã chọn | Chọn trực tiếp 1, không bật bảng số |
| 1 nĩa nhựa, 1 đã chọn | Chạm lại bỏ chọn, không bật bảng số |
| 7 cái đinh | Mở bảng; chọn 3 → quay về, đọc `Đã chọn 3` |
| 30 cái đinh | Chọn `Tất cả` → đánh dấu 30 |
| 30 cái đinh | Chọn `Nhập số lượng khác`, nhập 12 → đánh dấu 12 |
| 30 cái đinh | Chọn 0 → bỏ đánh dấu |
| 30 cái đinh | Thoát từ bảng số → giữ nguyên đánh dấu trước đó |
| Còn 5 cái khi danh sách cũ từng có 30 | Chọn 12 phải bị từ chối, không tự đổi thành 5 |
| Chọn nĩa và thìa | Xác nhận chung thực hiện **cả hai** theo số đã chọn |
| So sánh vật phẩm, chỉnh cài đặt | Không chuyển thành số lượng, không tự kết thúc |
| Chuyển đồ nâng cao | 1 món không hỏi số; chọn 3 thì thoát bảng số và chuyển 3; Quay lại không chuyển gì |
| Nhập số ngoài phạm vi, số âm, số dài gây tràn | Bị từ chối an toàn, không thay đổi lựa chọn |

## Giới hạn xác minh

Kiểm thử nguồn Python và kiểm thử parser C++ bảo đảm các hợp
đồng ở tầng mã. APK thực tế cần xác nhận bằng CI ARM64 và
nhật ký trên thiết bị, đặc biệt với TalkBack và các menu
lồng nhau. Chưa được tuyên bố đã thử trực tiếp trên OnePlus.
