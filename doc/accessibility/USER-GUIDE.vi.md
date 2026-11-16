# Hướng dẫn sử dụng CDDA Android trợ năng, phiên bản hiện hành

**Đối chiếu mã trên `master@f0be189ac5ebf000fc38e8c91fa3b42e325a7417`, ngày 13/11/2026.** Đây là hướng dẫn hành vi đã triển khai, **không phải chứng nhận toàn bộ APK đã được thử với TalkBack trên điện thoại thật**. Nếu thấy câu hướng dẫn cũ trái với trang này, dùng trang này và [đặc tả hiện hành](CURRENT-REFERENCE.vi.md).

Tài liệu được viết theo trình tự để người dùng trình đọc màn hình có thể nghe từ trên xuống. Không cần tự cài nhiều APK chỉ để đoán giao diện; nơi nào chỉ chắc ở tầng mã, hướng dẫn sẽ ghi “cần kiểm tra trên thiết bị”.

## Mục 0. Phân biệt bản trợ năng và bản thường

Biến thể thử trợ năng có mã ứng dụng `com.cleverraven.cataclysmdda.accessibility`. Bản này có vùng dữ liệu ứng dụng riêng, không tự thay thế bản CDDA thường. Bản candidate ARM64 của PR #7 đã xây dựng thành công, nhưng kết quả đó **không chứng minh** rằng mọi chức năng nghe và điều khiển trên máy bạn đều đúng.

Khi nâng cấp, nên sao lưu bản lưu quan trọng. Nếu cần báo lỗi, ghi commit APK, mẫu máy, Android, TalkBack/TTS engine, thao tác và kết quả quan sát được.

## Mục 1. Khởi động và nghe giọng

Ở biến thể trợ năng, trình đọc nội bộ được bật mặc định khi cài mới trừ khi bạn đã chủ động thay đổi cài đặt. Khi màn hình xuất hiện, có thể có thông báo tải dữ liệu trước khi vào menu chính. Tải mod và khởi tạo thế giới có thể kéo dài; trạng thái “đang tải” **không có nghĩa** game đã vào màn bản đồ.

Nếu không nghe được:
1. Kiểm tra âm lượng media Android và sự hiện diện của bộ máy TTS/ngôn ngữ Việt.
2. Dùng TalkBack để tìm các nút có nhãn khi giọng nội bộ không hoạt động.
3. Mở **Cài đặt TTS** để chọn engine và giọng có sẵn rồi dùng **Nghe thử**.
4. Nếu có lỗi, mở **Nhật ký gỡ lỗi**, bật **Ghi nhật ký** trước khi tái hiện; phải **Xuất nhật ký trước khi khởi động lại**.

Cử chỉ ứng dụng chỉ hoạt động khi Android không giữ cử chỉ đó cho TalkBack. Không yêu cầu tắt TalkBack chỉ để đọc menu Android.

## Mục 2. Màn hình Cài đặt TTS

`TtsSettingsActivity` có các điều khiển theo thứ tự:

1. **Bật trình đọc TTS trong game**.
2. **Bộ máy đọc**.
3. **Ngôn ngữ**.
4. **Giọng đọc**.
5. **Tốc độ**, từ **0,5 đến 10,0 lần**, bước 0,1.
6. **Độ cao**, từ **0,5 đến 2,0 lần**.
7. **Âm lượng**, từ 0 đến 100%.
8. **Nghe thử**.
9. **Hủy**.
10. **Lưu**.

Trong mã hiện tại, trang này **không có** công tắc giọng mạng, nút “Khôi phục giọng mặc định” hay “Mở cài đặt TTS Android”. Quyền dùng giọng qua mạng đã được lưu từ trước vẫn được tôn trọng. Hãy chủ động **Nghe thử** trước khi Lưu để đảm bảo bạn nghe rõ, nhưng mã hiện tại **không bắt buộc** thử thành công trước khi Lưu. Nếu bật TTS mà âm lượng bằng 0, mã từ chối Lưu.

Thay đổi tốc độ/giọng cần được nghe lại trên điện thoại vì bộ máy TTS thực có thể giới hạn tốc độ hoặc thiếu giọng tiếng Việt dù thanh trượt có đủ mức.

## Mục 3. Các cử chỉ cố định

**Khi nhân vật ở màn hình thế giới bình thường:**

- Vuốt một ngón lên: đi Bắc. Vuốt xuống: đi Nam. Vuốt trái: đi Tây. Vuốt phải: đi Đông. Từng cử chỉ đề nghị một bước; nếu game đang bận có thể từ chối.
- Chạm hai lần bằng một ngón: kích hoạt/xác nhận lựa chọn, khi có hàng hay nút phù hợp.
- Chạm ba lần bằng một ngón: mở Menu tổng hợp.
- Nhấn giữ một ngón: mở hành động theo ngữ cảnh.
- Vuốt hai ngón lên/xuống/trái/phải: quét **tối đa 30 ô** theo hướng Bắc/Nam/Tây/Đông. Chỉ đọc ô có vật phẩm, sinh vật, phương tiện, bẫy, chướng ngại vật, trường nguy hiểm, đồ nội thất hoặc lối đi đặc biệt; bỏ qua sàn nhà, đường và đất trống. Đây là **quét riêng cho trợ năng, có thể đọc qua tường**.
- Chạm một lần bằng hai ngón: đọc chi tiết lựa chọn hiện tại. Nếu đang ở chế độ chiến đấu, thay vào đó đọc thông tin kẻ địch phù hợp.
- Chạm hai lần bằng hai ngón: đọc thông báo gần đây ở thế giới.
- Chạm ba lần bằng hai ngón: mở Menu tổng hợp ở thế giới.
- Chạm một lần bằng ba ngón: đọc tọa độ.
- Chạm hai lần bằng ba ngón: đọc đầy đủ thông tin nhân vật hiện tại.
- Chạm ba lần bằng ba ngón: **đổi chế độ điều khiển**. Khi đang ngồi trên xe: Bản đồ → Chiến đấu → Lái xe → Bản đồ. Ngoài xe: Bản đồ ↔ Chiến đấu.

**Khi lái xe (ngồi tại vị trí điều khiển):**

- Chuyển sang **Lái xe** tự nhận quyền lái và sử dụng cơ chế khởi động động cơ gốc nếu động cơ chưa chạy. Ghế hành khách không được tự nhận quyền lái.
- Vuốt **hai ngón lên, xuống, trái, phải**: quét **tối đa 30 ô** phía **trước, sau, trái, phải theo hướng thân xe hiện tại**, kể cả khi xe đang hướng chéo. Chỉ đọc ô đáng chú ý và khoảng cách, không đọc ô trống. Trong chế độ bản đồ, quét hai ngón vẫn giữ Bắc, Nam, Tây, Đông cố định.
- Vuốt **một ngón** lên / xuống để chọn tiến / lùi, trái / phải để đánh lái, lên-rồi-xuống / xuống-rồi-lên để tăng / giảm tốc độ mục tiêu.
- Vuốt **một ngón** trái-rồi-phải / phải-rồi-trái để chỉnh hướng lái trái / phải. Chương trình chỉ đọc hướng thực tế của đầu xe (tám hướng la bàn); nếu phát hiện lỗi lái, sẽ đọc nguyên nhân. Xe đứng yên có thể đã đổi góc đánh lái nhưng thân xe chưa xoay.
- Vuốt **một ngón** xuống-rồi-trái: đọc tốc độ. Xuống-rồi-phải: đọc nhanh tình trạng động cơ, nhiên liệu và hư hỏng đáng chú ý. Lên-rồi-trái: phanh khẩn cấp. Phải-rồi-xuống: bóp còi. Lên-rồi-phải: bật/tắt động cơ.
- Chuyển về **Bản đồ** tự phanh và chờ tốc độ bằng không trước khi buông quyền lái. Trong khi chờ, thao tác di chuyển bản đồ bị khóa để không đánh lái nhầm; động cơ không tự tắt.

**Trong menu:** vuốt dùng để chuyển mục hoặc đổi nhóm tùy màn; chạm đôi mới kích hoạt. Vuốt hai ngón có thể mang nghĩa xác nhận hoặc quay lại theo ngữ cảnh. Luôn nghe tên, giá trị và trạng thái hàng trước khi chạm đôi; không suy rằng cử chỉ thế giới có nghĩa tương tự trong cửa sổ.

Có 36 mã trong danh mục cử chỉ: 11 được giữ cố định và 25 cho phép gán tùy chỉnh. Các cử chỉ cố định không thể bị thay thế bởi cấu hình cũ. Hướng dẫn thử ánh xạ vuốt ngang/dọc trong 45 giây là **thiết kế lịch sử**, không còn là luồng cài đặt điều hướng hiện hành.

## Khám phá địa điểm: lái xe tự động hoàn toàn

Trong **Menu tổng hợp → Khám phá địa điểm**, chọn một địa điểm đã biết rồi chọn **Đi xe**. Bạn **không phải ngồi sẵn trong xe**: nếu đang đứng gần một chiếc xe có ghế lái sử dụng được (tối đa khoảng 16 ô), trò chơi tự dùng cơ chế gốc để tìm đường tới ghế lái, lên xe, tiếp quản điều khiển và khởi động động cơ. Các bước đi bộ và khởi động vẫn dùng thời gian/thao tác thật của game, không dịch chuyển tức thời, không vượt qua xe bị khóa và không bỏ qua việc xin quyền sử dụng xe. Nếu một bước không thể hoàn thành, hệ thống đọc ngắn gọn nguyên nhân và không tự chạy.

Khi xe sẵn sàng, trò chơi dùng bộ tìm đường gốc `ACT_AUTODRIVE` để tự chọn tuyến, đánh lái và điều chỉnh tốc độ, đồng thời **tự mở giao diện Lái xe**. Bốn cử chỉ vuốt thẳng **một ngón lên, xuống, trái, phải** bị vô hiệu hóa trong chuyến tự lái để không làm lệch đường. Chạm đôi một ngón để **Tạm dừng / Tiếp tục**; chạm ba lần ba ngón để **chuyển về Bản đồ và hủy chuyến**. Các cử chỉ đọc thông tin, quét 30 ô bằng hai ngón, còi xe và menu trợ năng vẫn được hỗ trợ. Khi chuyến kết thúc, các vuốt điều khiển xe được trả về chế độ bình thường.

Nếu địa điểm nằm trong tòa nhà, giữa hồ hoặc nơi xe không thể vào, bộ tìm đường thử tuyến đi rộng hơn và thử khu vực **đường có thể tiếp cận** trong phạm vi hai ô bản đồ lớn quanh đích (khoảng 48 ô nhỏ), đồng thời báo rõ khi điểm dừng không phải đúng tọa độ đã chọn. Đây là mở rộng khả năng tiếp cận cho người khiếm thị, **không phải xuyên tường hoặc miễn nhiễm va chạm**. Khi không tìm được đường an toàn, trò chơi phải dừng và nói rõ lý do. Thiết lập **Lái thủ công** trước đó không được làm mất chức năng tránh vật cản của hành trình tự lái hoàn toàn.

Khi hành trình bắt đầu, trình đọc hiện bảng **Tạm dừng hành trình / Hủy chuyến đi**. Tạm dừng đặt tốc độ mục tiêu về 0, phanh xe bằng cơ chế vật lý gốc, **không xóa tuyến đường**; bấm **Tiếp tục hành trình** để chạy tiếp. Hủy sẽ phanh xe và kết thúc hoạt động điều hướng. Khi tới điểm đến, xe yêu cầu phanh và kết thúc hành trình, không giữ ga ở tốc độ cũ. Phanh vật lý có thể cần thời gian mới dừng hẳn.

Nếu đường bị chặn hoặc phía trước không còn đủ tầm quan sát, xe ngừng tăng ga và phanh, đồng thời chuyển sang trạng thái **Xe dừng vì vật cản** mà vẫn giữ điểm đến. Tại đây chỉ có **Tiếp tục** (thử lại khi đường đã thông thoáng) và **Hủy**, **không có nút bỏ qua vật cản**. Cảnh báo kẻ địch từ chế độ an toàn có bảng riêng, và chỉ cảnh báo kẻ địch mới cho phép yêu cầu bỏ qua theo quy tắc gốc.

Khi đứng yên và bộ tìm đường thực sự yêu cầu đổi hướng, xe có thể xoay thân xe tại chỗ từng bước 15–45 độ. Mỗi bước phải qua kiểm tra khoảng trống quanh thân xe; nếu không an toàn, xe phanh và chờ người chơi quyết định, không cố đâm xuyên vật cản. Hoạt động lái xe tự động hoàn toàn **im lặng khi đường đi bình thường**; chỉ đọc cảnh báo khẩn cấp và các thông tin bạn chủ động yêu cầu.

## Cập nhật lái xe: yên lặng, quay tại chỗ và phanh trước vật cản

**Cả lái tự động và lái thủ công đều im lặng khi di chuyển bình thường.** Trình đọc không tự thông báo lại xe bạn đang ngồi, địa hình, tốc độ hoặc khu vực vừa đi qua. Các cảnh báo khẩn cấp như chướng ngại, va chạm, lỗi động cơ hay tình trạng nguy hiểm của nhân vật vẫn được đọc. Cử chỉ chủ động yêu cầu quét 30 ô, đọc tốc độ, xem tình trạng xe hoặc hỏi hướng đầu xe vẫn đọc đúng nội dung tương ứng.

**Quay thân xe khi đứng yên:** vuốt trái/phải khi xe đã dừng hẳn, kể cả lúc tạm dừng, sẽ xoay thân xe theo từng góc 45 độ ngay tại chỗ thay vì chỉ đánh lái. Vuốt uốn trái rồi phải hoặc phải rồi trái cũng có thể điều chỉnh hướng khi xe dừng. Game kiểm tra tất cả ô mà thân xe quét qua trước khi xoay, và nêu nguyên nhân nếu bị chặn bởi công trình, bụi cây, phương tiện hoặc sinh vật. Xe vẫn đang trôi, trượt bánh, ở dốc, bị kéo hoặc có lái/bánh bị hỏng không được xoay tại chỗ. Xoay không tự tăng ga hay tiếp tục chạy.

**Phanh tự động:** bộ cảm biến quét từng ô liên tiếp từ đầu xe trong phạm vi dự đoán, gồm cả bụi cây có thể va chạm và vật dụng cản trở. Chế độ tự động ngừng ga và phanh khi gặp chướng ngại trên đường dự kiến. Lái thủ công vẫn giữ quyền điều khiển của người chơi và chỉ cảnh báo, không tự phanh. Hệ thống phát hiện là trợ năng bổ sung, không thay thế vật lý va chạm gốc.

## Hướng dẫn cử chỉ theo từng màn hình

**Nhấn giữ một ngón tay** để mở danh sách thao tác của màn hình đang dùng. Mục cuối cùng là **Hướng dẫn cử chỉ**. Trong chế độ lái, nhấn giữ vẫn mở **menu điều khiển xe gốc**; mục Hướng dẫn cử chỉ được đặt ở cuối menu xe. Trong chế độ chiến đấu, nhấn giữ vẫn mở menu chiến đấu, với mục hướng dẫn ở cuối. Túi đồ có túi chứa vẫn giữ **Mở túi chứa** và **Thao tác khác** trước mục hướng dẫn.

Chạm đôi vào **Hướng dẫn cử chỉ** để xem các cử chỉ phù hợp với màn hình hoặc chế độ hiện tại. Mỗi cử chỉ là **một tiêu điểm độc lập**, được đọc ngắn gọn (ví dụ: “Vuốt hai ngón lên: Quét 30 ô phía trước xe”). Vuốt để đọc tiêu điểm tiếp theo; chạm đôi vào một dòng **chỉ đọc lại hướng dẫn, không thực hiện lệnh**. Quay lại để trở về đúng menu và vị trí đang chọn trước đó. Hướng dẫn cử chỉ bản đồ khác với lái xe, chiến đấu, túi đồ, chế tạo, ngắm bắn và các màn hình khác. Các phím tắt đã tự gán ở bản đồ được đọc từ cấu hình hiện hành khi lệnh đang khả dụng.

Giao diện ngắm bắn giữ lệnh **Đổi kiểu điều khiển** trong danh sách nhấn giữ, rồi bổ sung mục hướng dẫn ở cuối. Giao diện hành trình giữ **Đọc trạng thái hành trình** trước mục hướng dẫn. Các màn hình chưa có danh sách lệnh ngữ cảnh cũng có đường truy cập đến hướng dẫn; đối với ô nhập liệu hoặc thông báo hệ thống, hướng dẫn được mở trực tiếp để tránh thay đổi thao tác nhập/xác nhận đang diễn ra.

**Cử chỉ chung trên menu và hộp thoại:** chạm một lần một ngón để dừng lời đọc; chạm hai lần hai ngón để đọc lại thông báo gần đây; chạm một lần hai ngón để đọc chi tiết tiêu điểm (nếu có); **vuốt hai ngón lên để xác nhận** và **vuốt hai ngón xuống để quay lại**. Một lệnh chỉ có tác dụng nếu giao diện hiện tại thực sự cho phép. Trong bản đồ và lái xe, hai ngón vuốt lên/xuống vẫn dùng cho quét 30 ô thay vì xác nhận/quay lại.

**Tạo nhân vật mới:** các bước tạo nhân vật từ 1 đến 6, màn hình tổng kết và lịch chọn ngày/giờ là các ngữ cảnh độc lập. **Vuốt hai ngón lên** ở bước 1-2 xác nhận lựa chọn; ở các bước chọn nhiều thuộc tính (3-6) xác nhận rồi chuyển bước; tại phần tổng kết là hoàn tất tạo nhân vật; trong lịch là xác nhận ngày/giờ. **Vuốt hai ngón xuống** quay lại. Hướng dẫn cho các bước này nay được công bố riêng, không dùng nội dung menu mặc định.

**Túi chứa, đọc tài liệu và trường số liệu:** trình đọc tự nhận biết khi đang duyệt túi chứa, chọn vật phẩm trong túi hoặc đọc tài liệu theo đoạn. Hướng dẫn thay đổi theo thao tác thực sự trong màn hình con. Với trường số hoặc danh sách lựa chọn có khả năng chỉnh giá trị, phần hướng dẫn vuốt dọc được lấy từ khả năng chỉnh sửa của tiêu điểm hiện hành. Cử chỉ người chơi đã gán được đọc theo thiết lập đang dùng ở chế độ bản đồ hoặc khi khả dụng trong chế độ chiến đấu. Hướng dẫn không kích hoạt các thao tác này.

## Mục 4. Menu tổng hợp

Mở từ màn hình chơi bằng chạm ba lần một ngón hoặc chạm ba lần hai ngón. Đây là menu **chỉ xuất hiện khi yêu cầu**, không phải bảng điều khiển luôn phủ lên màn hình.

Thứ tự mục cấp đầu:
1. Yêu thích.
2. Việc có thể làm tại đây.
3. Quan sát và bản đồ.
4. Di chuyển và tư thế.
5. Tương tác với môi trường.
6. Túi đồ và vật phẩm.
7. Ăn uống, nghỉ ngơi và y tế.
8. Trang bị và sửa chữa.
9. Chiến đấu, vũ khí và phép thuật.
10. Chế tạo và xây dựng.
11. Nhân vật và năng lực.
12. Nhiệm vụ và quan hệ.
13. Phương tiện và hành trình.
14. Quản lý khu vực và tự động hóa.
15. Thông tin, trình đọc và trợ giúp.
16. Hệ thống, TTS và lưu game.
17. Tìm lệnh.

Các tệp kỹ thuật gọi “nhóm 3: Quan sát” theo **thứ tự nhìn thấy có tính cả Yêu thích**; mã `GameActionHub` gọi đó là nhóm nội bộ 2. Đừng dùng ID nội bộ làm số mục để vuốt.

Khi duyệt một mục, trình đọc nói tên và thông tin ngắn. Chạm đôi mới vào nhóm hoặc gửi hành động. Một số nhóm chỉ hiện lệnh thật sự có trong tình huống hiện tại. Yêu thích đã lưu vẫn có thể báo **Không khả dụng ở màn hình hiện tại**; điều đó không có nghĩa mất mục.

**Tìm lệnh** giúp tra các lệnh đang được công bố; **Tất cả lệnh** không tự tạo chức năng mà engine không cho phép. Nhấn giữ một lệnh có thể mở tùy chọn thêm vào yêu thích hoặc gán cử chỉ cho lệnh ấy.

## Mục 5. Quan sát bản đồ và Danh sách xung quanh

Có hai cơ chế khác nhau, dễ nhầm:

**Quan sát thông thường:** đọc thứ nhân vật nhìn thấy, đã ghi nhớ, nghe hoặc cảm nhận; ô chưa biết thì báo chưa biết. Việc quan sát tầng khác dùng dữ liệu trí nhớ, không tự phơi bày bản đồ sống.

**Quét trợ năng không giới hạn tầm nhìn:** vuốt hai ngón một hướng kiểm tra lần lượt **tối đa 30 ô** trong bản đồ đang nạp nhưng **chỉ đọc những ô đáng chú ý**, kèm vị trí chính xác (ví dụ: “Ô 2, cốc nước”). Ô chỉ có sàn nhà, đường nhựa hoặc đất trống bị bỏ qua. Nếu không có thông tin đáng chú ý, hệ thống thông báo: **“Không phát hiện gì đáng chú ý trong phạm vi 30 ô.”** Danh sách xung quanh là cơ chế khác, quét khu vực 20 ô về mỗi phía trên cả trục ngang và dọc, tức tối đa **41 × 41 vị trí**. Các lệnh quét trợ năng này hiện cố ý có thể đọc vật phẩm, sinh vật và bẫy **sau tường**. Chỉ những ô thuộc bản đồ đã nạp mới được kiểm tra; đây không phải chứng nhận cân bằng gameplay.

Mở **Menu tổng hợp → Quan sát và bản đồ → Danh sách xung quanh**. Danh sách đọc tên đối tượng, hướng và khoảng cách; các hàng được sắp gần trước, xa sau. Danh sách không phát mọi ô đất/cỏ/sàn trống, và có trang tiếp theo nếu quá 80 hàng.

Bốn bộ lọc: **Tất cả**, **Sinh vật và NPC**, **Địa điểm và môi trường**, **Vật phẩm**. Trong danh sách này, vuốt ngang đổi đối tượng; vuốt dọc đổi bộ lọc.

- Chạm **một lần hai ngón** trên hàng vật phẩm: nghe thông tin đầy đủ, **không** đi tới vật phẩm.
- Chạm **đôi một ngón** trên hàng: yêu cầu tìm đường đến vị trí, theo kiểm tra an toàn của engine. Không dịch chuyển tức thời và không tự nhặt.
- Nếu không có đường hợp lệ, có thể nghe thông báo từ chối.
- Danh sách phản ánh đối tượng tại lúc mở; sinh vật có thể di chuyển sau đó.

**Sonar định hướng** là tùy chọn tách biệt. Trong Cài đặt trợ năng, bật/tắt **Âm thanh định hướng vật phẩm và sinh vật**; mặc định tắt. Sonar chỉ dùng vật phẩm/sinh vật có thể nhìn thấy theo tám hướng, không có cùng cơ chế xuyên tường với danh sách quét.

## Mục 6. Hành trang, đồ vật, ngăn chứa và số lượng

Duyệt danh sách không tự nhặt, mặc, cất, thả hoặc dùng đồ. Chạm một lần hai ngón trên hàng vật phẩm được hỗ trợ sẽ yêu cầu đọc `item::info(true)`, tức phần thông tin đầy đủ do engine cung cấp.

Nếu có **đúng một đơn vị**, chạm đôi để đánh dấu một món; chạm lại bỏ đánh dấu. Không hiện bảng số.

Nếu có **nhiều đơn vị**, chọn số lượng 1–9, Tất cả, Bỏ chọn hoặc Nhập số lượng khác. Lựa chọn số được áp dụng ngay về danh sách, nhưng **chưa thực sự nhặt/thả/chuyển** cho tới khi bạn dùng bước xác nhận chung của màn đó. Quay lại từ bảng số không tự thay giá trị.

Trong túi nhiều ngăn, chú ý tên vật chứa và vị trí vật phẩm. Mã nguồn có kiểm tra lại đối tượng/đúng ngăn khi thực hiện. Nếu vật đã đổi vị trí hoặc số lượng không đủ, thao tác phải bị từ chối, không tự áp dụng sai.

Nếu một vật phẩm bị vô hiệu hóa cho hành động hiện tại nhưng có quyền đọc chi tiết, bạn vẫn có thể yêu cầu đọc; **đọc được không đồng nghĩa được phép kích hoạt**.

## Mục 7. Di chuyển, chiến đấu và hoạt động dài

**Di chuyển bình thường:** mỗi cú vuốt được engine xem xét theo lượt. Đôi khi game xử lý tầm nhìn, quái vật và bản đồ nên có độ trễ. Đừng coi không có lời đọc tức thì là đã đi được; kiểm tra thông báo hoặc tọa độ khi cần.

**Chiến đấu:** chạm ba lần bằng ba ngón để chuyển chế độ và nghe thông báo trạng thái. Trong chế độ chiến đấu, một ngón dùng cho các đòn hướng; vuốt hai ngón là đi; chạm một lần hai ngón là nghe danh sách địch. Đổi lại chế độ bản đồ trước khi dùng cử chỉ quét. Không thử các lệnh chiến đấu nguy hiểm khi chưa nghe rõ chế độ hiện tại.

**Ăn/uống/chữa trị/chế tạo/chờ/ngủ:** nghe điều kiện và số lượng trước khi xác nhận. Một hoạt động có thể bị hủy, gián đoạn hoặc kéo dài nhiều lượt; việc duyệt mô tả không tiêu lượt nhưng hành động gameplay thực có thể tiêu lượt. Trạng thái thực do engine quyết định.

## Mục 8. Thông tin nhân vật và đọc tài liệu dài

Chạm **hai lần ba ngón** khi ở thế giới để nghe báo cáo nhân vật: tên, thuộc tính, tình trạng sinh tồn, hiệu ứng, các bộ phận, chế độ an toàn và trang bị đang cầm nếu game công bố.

Chạm **một lần hai ngón** trên công thức, trang trợ giúp, trang nhật ký, nhiệm vụ hoặc hàng tài liệu được hỗ trợ sẽ đọc chi tiết. Tài liệu dài được chia và ghép thành một lượt đọc. Hãy kiểm tra trên thiết bị nếu nội dung vượt 11 KB, vì kiểm thử nguồn chưa thay cho thử giọng thật.

Không có bảo đảm mọi cửa sổ của mod bên ngoài hoặc công cụ debug chuyên biệt đều có chi tiết đầy đủ.

## Mục 9. Cài đặt trợ năng khác

Trang **Cài đặt trợ năng** hiện có công tắc bật/ẩn đồ họa thế giới; chế độ chỉ đọc bằng giọng là mặc định ở biến thể APK trợ năng. Ngoài ra có sonar, ba mức khoảng cách vuốt và ba mức thời gian vuốt.

Trang **Nhật ký gỡ lỗi** khác với trang trợ năng và trang TTS. **Ghi nhật ký** mặc định tắt; trang cũng có **Xuất nhật ký** và **Xóa nhật ký**. Không có công tắc vẽ bản đồ trong trang nhật ký.

## Mục 10. Lưu, tải, sao lưu và báo lỗi

Sử dụng các lệnh lưu/tải của game; nếu có hộp xác nhận, nghe rõ câu hỏi và lựa chọn trước khi chạm đôi. Với bản lưu quan trọng, xuất bản sao lưu trước khi thực hiện các thao tác thử nghiệm hoặc khôi phục.

Để báo lỗi:
1. Vào **Nhật ký gỡ lỗi**, bật **Ghi nhật ký** trước khi thực hiện lỗi.
2. Lặp lại chính xác cử chỉ, màn hình, vật phẩm hoặc thao tác gây lỗi.
3. Trở lại Nhật ký gỡ lỗi và **Xuất nhật ký** bằng bộ chọn tệp Android. Có thể tắt ghi rồi mới xuất.
4. **Không đóng/mở lại app trước khi xuất**: lần mở tiến trình mới tạo nhật ký mới, không còn lựa chọn “phiên bị văng trước”.
5. Báo commit APK, mẫu máy, engine TTS, TalkBack bật/tắt, chuỗi thao tác, điều đã nghe, kết quả gameplay và nhật ký.

Nhật ký chỉ được ghi khi bạn bật công tắc, không tự tắt khi máy chạy nhanh. Công tắc tắt không tiếp tục thu thập dữ liệu.

## Mục 11. Giới hạn chưa thể bảo đảm

APK ARM64 đã build được ở PR #7; nhưng vẫn cần nghiệm thu TalkBack và TTS trên máy thật, các ngữ cảnh chiến đấu/xe/giao dịch, nhập liệu dài, ngăn chứa lồng nhau, danh sách rất lớn, minigame, mod ngoài và lỗi lưu/khôi phục. Trong CI `master` vẫn có các job thất bại chưa sửa hết.

Khi nghi ngờ một màn không đọc được, dùng nút Quay lại hoặc điều hướng có nhãn, không thử nhiều lần một hành động có thể tiêu lượt. Xem [trạng thái](STATUS.md) và [ma trận thử nghiệm](VALIDATION-MATRIX.vi.md) để biết đâu là điều đã xác minh và đâu là mục còn phải thử.
