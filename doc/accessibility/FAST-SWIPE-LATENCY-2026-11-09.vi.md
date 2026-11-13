# Di chuyển bằng vuốt liên tục: tối ưu độ trễ 2026-11-09

> **GHI CHÚ PHIÊN BẢN (13/11/2026):** Đây là hồ sơ chuyên đề theo thời điểm viết, không tự động là chỉ dẫn cho `master` sau PR #7. Khi cử chỉ, menu, quyền đọc bản đồ hoặc CI khác, ưu tiên [đặc tả hiện hành](CURRENT-REFERENCE.vi.md), [hướng dẫn](USER-GUIDE.vi.md) và [trạng thái](STATUS.md). **Không coi mã đã viết là nghiệm thu TalkBack trên máy thật.**

## Mục tiêu

Trên APK `d49b60a`, nhật ký thiết bị OnePlus ghi nhiều bước `LEFT` chỉ
37–50 ms trong C++, trong khi đợt công bố snapshot sau đó có thể kéo dài
thêm hàng trăm mili giây. Cử chỉ vuốt tiếp theo đôi khi bị hiểu thành
điều hướng trình đọc và phát “Đang chờ game”.

Mục tiêu của thay đổi này là giảm **thời gian bị khóa giao diện điều khiển**
và một phần công việc tạo snapshot. Không tăng tốc thời gian thế giới,
không gửi các bước đi giả, không tự gộp cử chỉ và không tạo thêm hàng đợi
di chuyển bên Java.

## Kiến trúc được thay đổi

1. Scope `DEFAULTMODE` của trình đọc tồn tại qua các lượt di chuyển
   bình thường, thay vì bị hủy sau mỗi lần `get_player_input()` trả về.
   Callback tham chiếu `input_context` được gỡ ngay trước khi hàm kết thúc,
   rồi gắn lại khi game bắt đầu chờ đầu vào lượt kế tiếp. Frame và snapshot
   đã được xác nhận vẫn tồn tại qua lúc C++ mô phỏng lượt.
2. Android dùng một khóa nhận diện cử chỉ ổn định theo
   `session:frame:generation` khi đang thực sự ở chế độ đi bộ.
   Thay đổi revision do bước đi không còn hủy cử chỉ đang vuốt dở.
   Menu, lifecycle và các màn hình khác vẫn dùng khóa cũ.
3. Một lệnh hướng thuộc đúng bốn hướng `UP/DOWN/LEFT/RIGHT`, có
   `activate`, cùng token phiên điều khiển **vẫn đang hoạt động**,
   được phép dùng revision đã cũ nhiều phiên bản. C++ **vẫn** yêu cầu
   `session/frame/generation` đúng, revision không đến từ tương lai,
   và snapshot mới nhất cho phép `swipe_movement_allowed`; kiểm tra
   lại ID node, trạng thái cho phép và hành động ngay trên C++.
   Những lệnh menu/vật phẩm, chuyển modal, đổi phiên, ngủ, điều khiển
   xe và tự động di chuyển vẫn dùng cơ chế kiểm soát nghiêm ngặt.
4. Bản mô tả các lệnh mặc định không đổi được lưu lại trên game thread,
   tránh gọi lại liên tục các bước dịch nhãn và lấy mô tả phím tắt.
   Cache bị xóa khi vào modal, đổi vòng đời trình đọc hoặc đổi phiên
   nhân vật; danh sách hành động thay đổi cũng khiến cache được tạo lại.
5. Cứ 32 lần cập nhật snapshot, nhật ký ghi
   `WORLD_READER_TIMING build_ms=... update_publish_ms=... nodes=...`.
   Đây là độ trễ cục bộ của **trình đọc**, không phải thời gian toàn bộ
   lượt game; vẫn cần đối chiếu `NATIVE_ACTION` và `GESTURE`.

## Giới hạn an toàn và kỳ vọng

- Mỗi lần vuốt chỉ tạo đúng một yêu cầu bước đi qua engine gốc.
- Không có hàng đợi mới. Các intent vẫn sử dụng đường JNI hiện hành.
  Nếu engine thực sự tốn nhiều thời gian cho một lượt phức tạp, không có
  cơ chế hợp lệ nào cho nhân vật vượt qua lượt đó trong lúc game chưa xử
  lý xong. Tối ưu này không cam kết mọi bước <100 ms.
- Đường truyền cử chỉ hoạt động trong trạng thái thế giới bình thường,
  không thay đổi hoạt động lái xe, ngủ, menu, hội thoại hay chết.
- Sau khi kết thúc một bước có thể xuất hiện menu/hội thoại hoặc hoạt
  động tự động. Khi đó C++ kiểm tra trạng thái `swipe_movement_allowed`
  ở snapshot **mới** trước khi nhận một lệnh cũ.
- Mục Quan sát và bản đồ vẫn quét bình thường; không loại bỏ thông tin
  vật phẩm, quái vật hoặc bẫy khỏi snapshot.
- TTS vẫn đọc cảnh báo quan trọng, không thay đổi quy tắc ưu tiên tiếng nói.

## Kiểm chứng

Bộ kiểm thử nguồn:
`python3 -m unittest discover -s tools/accessibility -p test_fast_swipe_latency.py -v`.

Trên điện thoại, bật nhật ký phiên và thử:

- Vuốt liên tục 10, 20, 30 lần theo một hướng trên đường trống.
- Đổi hướng xen kẽ, kiểm tra số bước tương ứng với các lần vuốt.
- Vuốt trong lúc game xử lý lâu; không phát “Đang chờ game” chỉ vì
  snapshot giữa lượt.
- Gặp tường, vật phẩm, zombie và cửa: thông báo và quy tắc game không đổi.
- Vuốt trước/sau hội thoại, menu, lái xe, ngủ, tự di chuyển: tuyệt đối
  không có lệnh đi bộ bị phát nhầm.
- Thử tạm dừng/tiếp tục ứng dụng, tải lại nhân vật, đổi tùy chọn phím
  và bật/tắt trình đọc.
- So sánh độ trễ trung vị và p95 của `NATIVE_ACTION`, số lần
  `GESTURE_ACTION` bị chuyển thành `NAVIGATE`, và các dòng
  `WORLD_READER_TIMING` trước/sau trên cùng thiết bị.

Thay đổi này là một vòng tối ưu kiến trúc có cơ sở từ nhật ký, chưa phải
bằng chứng về mức tăng tốc đo được trên thiết bị.

## Bản sửa bổ sung sau khi thử `23e40c2`

Nhật ký `cataclysm-session-debug_6.txt` ghi 65 yêu cầu đi bộ, trong đó
10 yêu cầu bị `stale`; quá trình này không còn làm đổi vuốt thành điều
hướng nhưng vẫn mất bước khi engine cập nhật nhiều revision trước khi
xử lý được lệnh.

- Bỏ giới hạn revision chỉ được cũ một bản **cho lệnh đi bộ bốn hướng**.
  Đối với bước đi, snapshot là một mô tả thế giới thay đổi thường xuyên,
  không phải định danh lượt điều khiển. Token `session/frame/generation`,
  số request tăng dần và quyền đi bộ **trong snapshot native hiện tại**
  vẫn phải đúng. Mọi hành động khác không thay đổi quy tắc.
- Khi người chơi mở Menu tổng hợp chỉ có ở Android, JNI xóa **chỉ các
  lệnh đi bộ chưa xử lý trong mailbox sẵn có**. Không tạo hàng đợi mới
  và không xóa các lệnh menu. Modal native vẫn tự chặn qua generation.
- Sự kiện `WORLD_INPUT_TIMING` (1/16 bước) đo thời gian từ lúc JNI
  nhận cú vuốt đến khi game thread lấy nó ra, cùng độ trễ revision.
- Sự kiện `WORLD_JAVA_TIMING` (1/16 snapshot thế giới) tách thời gian
  chờ UI thread, phân tích JSON và cập nhật giao diện.
- Thêm kiểm thử C++ về bốn hướng, revision cũ nhiều bản, revision tương
  lai, node vô hiệu, dữ liệu bất hợp lệ, modal/lifecycle và token cũ.

**Chưa khẳng định hết trễ:** game có những lượt xử lý 0,8–1 giây,
không thể cho bước tiếp qua trước khi game thực sự xong một lượt.
Không chạy bước giả và không dùng nhiều luồng cùng sửa trạng thái game.
