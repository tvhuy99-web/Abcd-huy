# Kế hoạch triển khai CDDA Android để người khiếm thị chơi độc lập và đầy đủ

Phiên bản kế hoạch: 1.0 — ngày 22/10/2026.

Dự án đích: [tvhuy99-web/Abcd-huy](https://github.com/tvhuy99-web/Abcd-huy).

Bản nguồn được đối chiếu: nhánh `master`, commit `8ef359c74cc25e256919558bf7b3340f9b56a80d`. Mọi đường dẫn CDDA trong tài liệu được hiểu tại commit này. Những tên mô-đun trợ năng mới là đề xuất, chưa phải mã đã tồn tại.

## 1. Kết quả cuối cùng phải đạt

Người mù hoàn toàn có thể dùng điện thoại Android, tự mở ứng dụng, cấu hình giọng đọc, học điều khiển, tạo thế giới và nhân vật, chơi các hệ thống được phát hành, lưu, tải, sao lưu và đi hết quy trình tử vong hoặc nghỉ hưu mà không cần người sáng mắt thao tác hộ.

CDDA là game sinh tồn mở. “Chơi hoàn toàn” ở đây là khả năng tiếp cận các hành động, thông tin và quyết định của bản game đã chốt phạm vi; không có nghĩa tự động thắng, tránh mọi cái chết hoặc bảo đảm hiểu ngay mọi cơ chế phức tạp.

Một tính năng chỉ được tính là tiếp cận được khi người dùng thực hiện được cả năm bước:

1. Biết tính năng tồn tại và tìm được nó.
2. Hiểu thông tin, trạng thái, điều kiện và hậu quả cần thiết để quyết định.
3. Chọn đủ tham số: vật phẩm, số lượng, hướng, vị trí, mục tiêu hoặc câu trả lời.
4. Xác nhận, hủy hoặc sửa lựa chọn mà không vô tình thực hiện hành động khác.
5. Nhận được kết quả thực tế, thông báo lỗi và đường tiếp tục.

Việc nghe được tên nút hoặc phát được TTS chưa đủ để đạt yêu cầu này.

### 1.1. Phạm vi phát hành phải được đóng băng và công bố

Ở mốc đầu, ghi rõ commit game, phiên bản nội dung, danh sách mod đi kèm được bật, ngôn ngữ, ABI và dải Android kiểm thử. Không lấy “toàn bộ CDDA cùng mọi mod hiện tại và tương lai” làm một lời hứa không thể kiểm chứng.

Bản đầy đủ phải bao phủ mọi tính năng chơi dành cho người dùng trong cấu hình được công bố, gồm hệ thống nâng cao. Nếu phương tiện, NPC, vùng tự động, chế tạo hoặc một màn hình còn chưa dùng được thì chỉ phát hành dưới tên bản thử nghiệm với danh sách giới hạn. Không đổi nhãn thành đầy đủ bằng cách âm thầm bỏ các phần khó.

Trình soạn bản đồ và công cụ phát triển có thể là phạm vi riêng nếu không được quảng bá như tính năng chơi. Bất kỳ công cụ nào vẫn hiển thị cho người dùng phải có trạng thái hỗ trợ rõ ràng và đường thoát. Menu chọn mod, cấu hình thế giới, trợ giúp và quản lý bản lưu thuộc hành trình người chơi, phải hỗ trợ.

### 1.2. Điều kiện công nhận bản đầy đủ

- Không có màn hình bắt buộc nào cần nhìn, OCR, bàn phím ngoài hoặc người trợ giúp.
- Mọi lệnh thuộc phạm vi đều có đường tìm, gọi và hoàn thành; mọi màn hình đều có bộ chuyển đổi ngữ nghĩa hoặc phương án tương đương đã nghiệm thu.
- Nội dung chỉ phân biệt bằng màu, biểu tượng, vị trí hoặc đồ thị đều có cách nghe tương đương.
- Đọc, duyệt và thăm dò thuần túy không làm thế giới tiến triển trong cấu hình chơi theo lượt chuẩn.
- Không lộ quái vật, vật phẩm, bẫy, chỉ số hoặc khu vực mà nhân vật chưa biết.
- Luôn có đường hủy, nghe lại, hỏi trợ giúp và phục hồi điều khiển.
- Người khiếm thị mục tiêu hoàn thành bộ hành trình trên máy thật; kiểm tra tĩnh và người sáng mắt nhắm mắt chỉ là các lớp kiểm tra bổ sung.
- Không còn lỗi chặn sử dụng và lỗi rò thông tin đã biết trong phạm vi công bố.

## 2. Cơ sở đã đọc và giới hạn của lần khảo sát này

### 2.1. Tài liệu người dùng cung cấp

Đã đọc kế hoạch trong `Văn bản đã dán (1)(20261022-030551).txt`, giải nén gói `GnollHack-BLIND-SCAN8-ALLCOMMANDS-BOULDER-FIX-SOURCE-20260831-VI-FINAL-R5.zip`, đọc các phần cốt lõi về ngữ cảnh, cử chỉ, tiếng nói, cây giao diện, nhập văn bản, bản đồ ngữ nghĩa, ranh giới thông tin và các bản ghi sửa lỗi liên quan.

Hai dấu kiểm tra SHA-256 để xác định đúng tài liệu tham khảo:

- Kế hoạch cũ: `958e14452ec57f016f7c8c5c2ac33dd1f471f81e5cef7e77bc1027164b2b0002`.
- Gói GnollHack: `24bc81594306d4b7e634d88604aee29a0482a57eea2341563391ba841753c519`.

Các báo cáo PASS, biên dịch và đóng gói bên trong ZIP là báo cáo kèm nguồn. Lần khảo sát này không chạy lại các báo cáo đó, không build APK CDDA, không chạy CDDA trên thiết bị và không nghiệm thu với người dùng. Vì vậy, tài liệu là kế hoạch kỹ thuật dựa trên đọc nguồn, chưa phải chứng nhận khả năng chơi của APK nào.

### 2.2. Những điểm đã xác minh trong Abcd-huy

| Phát hiện trong nguồn | Hệ quả đối với kế hoạch |
| --- | --- |
| Android sử dụng SDL3; `CataclysmDDA.java` nạp `libmain.so` | Thiết kế JNI, vòng đời và sự kiện theo nhánh hiện tại; tránh chép hướng dẫn SDL2 cũ |
| `input_context` có ngăn xếp và cơ chế `scoped_activation` | Tận dụng cơ chế nhập lệnh và vòng đời sẵn có; bổ sung quyền sở hữu, focus và phiên bản ngữ nghĩa |
| `sdltiles.cpp` giữ bản sao `touch_input_context` và có joystick, chạm lặp, nhiều ngón | Không coi bản sao này là thẩm quyền ngữ cảnh duy nhất; chặn đường cũ trước khi sinh tác dụng phụ |
| `uilist.cpp` đã dùng ImGui; có nhánh `ANDROID_NATIVE_UI` gọi Java | Phải xử lý cả hai nhánh, kể cả nhánh trả về sớm trước giao diện ImGui |
| Tạo nhân vật, chế tạo, bản đồ lớn có giao diện tùy biến | Một bộ đọc `uilist` không tự phủ mọi màn hình |
| `SCREEN_READER_MODE` mô tả tối ưu trình đọc ngoài, hướng đến Orca/curses | Tạo cấu hình bộ đọc Android riêng, giữ tương thích trợ năng có sẵn |
| `SplashScreen.java` có cài dữ liệu, hộp thoại lỗi và cảnh báo dịch vụ trợ năng | Lần mở đầu phải được làm tiếp cận trước khi vào C++ |
| Android CMake, CMake trong `src`, Makefile thu thập nhiều nguồn bằng mẫu ở thư mục `src` | Nếu thêm thư mục con phải sửa danh sách build tương ứng; kiểm tra build thật |
| `TURN_DURATION` cho phép tiến triển thời gian thực và mặc định trong nguồn là 0 | Cấu hình chơi chuẩn dùng 0; kiểm tra cả bản lưu/cấu hình cũ bật thời gian thực |
| `game_ui_tile_info.cpp` có nhánh nhìn thấy, ghi nhớ, cảm nhận đặc biệt và âm thanh | Rút trích truy vấn theo quyền nhận biết của nhân vật, không gọi hàm vẽ để lấy câu đọc |
| Có `lang/po/vi.po.gz`, hướng dẫn biên dịch bản dịch và tài nguyên Java `values-vi` | Kiểm thử tiếng Việt ở cả Java và C++; sự tồn tại bản dịch chưa chứng minh chất lượng hoặc độ phủ |

Giá trị mặc định đọc từ `android/gradle.properties`: min SDK 24, compile/target SDK 35, NDK `28.1.13356709`. Đây là cấu hình của snapshot, không phải kết luận về mọi bản APK hay yêu cầu cửa hàng tại ngày phát hành.

## 3. Những điều cần sửa trong kế hoạch cũ

### 3.1. Giữ phần đúng

Giữ nguồn dữ liệu từ engine, Android TTS qua JNI, mô hình ngữ cảnh, giọng đọc có ưu tiên, menu dùng chung, đọc bản đồ có ghi nhớ, âm hiệu bổ trợ, kiểm thử bằng hành trình và trì hoãn tối ưu bỏ dựng hình.

### 3.2. Bổ sung các phần còn thiếu

1. **Khởi động trước engine:** bật bộ đọc ở lần mở đầu, đọc tiến độ cài, lỗi dữ liệu, khôi phục cấu hình và lựa chọn cách dùng TalkBack.
2. **Bao phủ lệnh:** một trung tâm lệnh chỉ tốt khi mọi lệnh và mọi bước nhập tham số đều dùng được; cần đối chiếu có hệ thống với lệnh game và hành động từng ngữ cảnh.
3. **Ngữ cảnh lồng nhau:** phân biệt menu, hộp thoại con, nhập số, chọn hướng, nhắm mục tiêu và tác vụ đang chạy; không chỉ dùng một biến “đang ở hành trang”.
4. **Thời gian game:** TTS bất đồng bộ không đồng nghĩa thế giới đứng yên. Phải xử lý tiến triển thời gian thực, hoạt động nhiều lượt và xe đang chạy.
5. **Thông tin theo từng thuộc tính:** nghe tiếng không cho biết mặc nhiên tên quái, tọa độ chính xác, HP hoặc địa hình quanh nguồn âm.
6. **Unicode:** bộ chọn ký tự của GnollHack là tham khảo; không sao chép cách di chuyển/xóa theo đơn vị `char` nếu cần hỗ trợ đầy đủ tiếng Việt, dấu tổ hợp và ký tự ngoài BMP.
7. **Tương tác nguy hiểm:** tránh “chạm hai lần = hành động mặc định” mơ hồ trên bản đồ. Người chơi phải biết sắp đi, chờ, tấn công hay tương tác.
8. **Phục hồi:** lỗi TTS không được làm người chơi mất cả giọng đọc lẫn đường bật trợ giúp hệ thống.
9. **Hệ thống nâng cao:** không đẩy phương tiện, xây dựng, vùng, NPC, bionics, đột biến, nhiệm vụ và giao diện kết thúc ra một phần “để sau” không có tiêu chí hoàn tất.
10. **Bảo trì:** nội dung hoặc màn hình mới phải cập nhật bảng hỗ trợ và kiểm thử; nếu không, độ phủ sẽ giảm sau mỗi lần nâng nguồn.

Không nên hứa thời gian “100%” trước khi kiểm kê màn hình và hoàn thành một lát cắt chơi thật. Ước lượng ở phần 17 chỉ dùng để phân bổ nguồn lực.

## 4. Bài học cụ thể từ GnollHack

Gói tham khảo dùng C#/MAUI kết nối phần native; Abcd-huy dùng C++/SDL3/Java. Chuyển nguyên tắc và hợp đồng dữ liệu, không bê nguyên lớp C# hoặc cây MAUI sang SDL.

| Thành phần tham khảo | Điều nên áp dụng vào CDDA |
| --- | --- |
| `AccessibilityContextManager` | Khung ngữ cảnh có chủ sở hữu, chặn đầu vào bên dưới, mở/đóng theo vòng đời và có phiên bản |
| `AccessibilityUiNode`, `AccessibilityUiSession` | Nút ngữ nghĩa có vai trò, tên, giá trị, trạng thái, hành động; focus ổn định khi danh sách đổi |
| `SpeechRequest`, `AccessibilitySpeechEngine` | Ưu tiên, gộp theo loại, chống lặp, hủy và phát lại có kiểm soát |
| `AndroidAccessibilitySpeechBackend` | Theo dõi sức khỏe TTS, thời hạn khởi tạo, thử khôi phục có giới hạn, chọn engine/giọng |
| `AccessibilityInformationBoundary` | Lọc dữ liệu trước khi đưa tới tầng đọc; ô chưa biết không chứa dữ liệu bí mật |
| `AccessibilityBlindTextEditor` | Có đường nhập không phụ thuộc bàn phím ngoài; cải tiến Unicode và hiệu quả nhập |
| `AccessibilityInformationLeakAuditor`, hành trình QA | Kiểm tra snapshot và thử tình huống; không chỉ đếm số lần gọi TTS |

### 4.1. Bốn lỗi tham khảo phải biến thành kiểm thử hồi quy

**Quét mất ô đã nhớ sau khi xóa màn hình.** Phân biệt làm sạch phần trình bày với thay đổi thế giới/nhân vật. CDDA dùng bộ nhớ bản đồ của chính game; không tạo kho nhớ toàn tri và không xóa nó chỉ vì resize hoặc redraw. Ngược lại, không giữ dữ liệu động cũ khi tải nhân vật khác.

**“Tất cả lệnh” chỉ còn mục quay lại.** Danh mục hành động phải sinh từ dữ liệu lệnh và adapter, không phụ thuộc nút, bitmap, shortcut bar hay màn hình đã từng được dựng.

**Đâm vào tảng đá nhưng đọc hành lang.** Tên địa hình nền không phải nguyên nhân thất bại. CDDA cần phản hồi kết quả hành động thực tế sau kiểm tra của engine. Không áp dụng máy móc quy tắc “vật phẩm nằm trên sàn đều chặn đường”, vì luật CDDA khác GnollHack.

**Thông báo còn tiếng Anh hoặc dịch sai.** Thêm câu mới vào cơ chế bản địa hóa gốc; không dùng một bộ thay thế từ tiếng Anh diện rộng làm nền móng. Thử câu có tham số, số nhiều, đơn vị và nội dung mod.

### 4.2. Điều không nên sao chép nguyên trạng

- Ngưỡng quét 8 ô là lựa chọn giao diện của tham khảo; CDDA phải cho đổi bán kính, tầng cao và bộ lọc, vẫn giới hạn theo nhận biết.
- Tên loại sinh vật hoặc HP chính xác không tự động bị cấm trong mọi trường hợp; chỉ công bố mức thông tin tương đương giao diện hợp lệ của nhân vật.
- Không suy ra một dịch vụ trợ năng đang bật là có xung đột. Cảnh báo theo tình trạng thực tế và lựa chọn người dùng; không ép tắt toàn bộ dịch vụ.
- Không tuyên bố “build thành công” hoặc “kiểm tra tĩnh PASS” đồng nghĩa người mù chơi được trên thiết bị.

## 5. Kiến trúc triển khai

### 5.1. Phân lớp

Luồng dữ liệu: trạng thái và sự kiện game → bộ trích xuất thông tin hợp lệ → mô hình ngữ nghĩa → bộ quản lý ngữ cảnh/focus → chính sách phản hồi → TTS, âm hiệu, rung hoặc cầu nối trợ năng Android.

Luồng điều khiển: cử chỉ, nút ngữ nghĩa hoặc bàn phím tùy chọn → ý định người dùng → kiểm tra phiên ngữ cảnh và tham số → bộ nhập hành động hiện có → engine → kết quả thực tế → phản hồi.

| Mô-đun đề xuất | Trách nhiệm |
| --- | --- |
| `a11y_runtime` | Cấu hình, phiên chơi, vòng đời và phối hợp các mô-đun |
| `a11y_context` | Ngăn xếp ngữ cảnh, chủ sở hữu, focus, token phiên, thao tác quay lại |
| `a11y_semantics` | Nút, danh sách, biểu mẫu, giá trị, hành động và phiên bản snapshot |
| `a11y_actions` | Trung tâm lệnh, điều phối ý định và kiểm tra quyền thực hiện |
| `a11y_perception` | Ranh giới nhận biết; nguồn sự thật dùng chung cho các bộ đọc |
| `a11y_map` | Ô hiện tại, tám hướng, thăm dò, điểm đáng chú ý và bản đồ lớn |
| `a11y_events` | Sự kiện sau thay đổi trạng thái, kết quả hành động, thông báo |
| `a11y_speech` | Chính sách hàng đợi, đọc dài, lịch sử, ưu tiên và độ dài câu |
| `a11y_android` | JNI, liên kết TTS/âm/rung/vòng đời và đầu vào Android |
| Các `a11y_adapter_*` | Chuyển từng hệ thống UI sang hợp đồng chung |
| `a11y_diagnostics` | Đo độ trễ, ghi lỗi điều khiển, độ phủ, kiểm tra bất biến |

Đặt lớp lõi không phụ thuộc Java và không nắm quyền thay đổi luật gameplay. Trên nền tảng không hỗ trợ, backend rỗng vẫn biên dịch; chỉ adapter Android chứa JNI. Nếu tách `src/accessibility/`, thêm danh sách nguồn rõ ràng vào Android CMake, CMake desktop và Makefile tương ứng. Không dùng một đợt glob đệ quy rộng vô tình biên dịch thư mục kiểm thử hoặc thư viện lồng.

### 5.2. Quy tắc luồng và vòng đời

- Luồng game sở hữu việc đọc trạng thái gameplay, cập nhật focus ngữ nghĩa và thực hiện lệnh.
- Luồng Android UI tạo/đóng native dialog và cập nhật view; không chờ TTS bằng khóa hoặc semaphore.
- Callback TTS chỉ đưa sự kiện hoàn tất/lỗi vào hàng đợi an toàn; không gọi trực tiếp hàm game từ callback.
- Snapshot gửi sang Java là dữ liệu bất biến hoặc bản sao; không truyền con trỏ `Creature`, `item` hay view C++ sống sang Java.
- Mỗi lần tải thế giới/nhân vật hoặc tạo lại Activity tăng mã phiên phù hợp, hủy callback và đầu vào cũ.
- Gọi JNI theo đúng luồng; không chia sẻ `JNIEnv*` giữa các luồng. Quản lý reference, exception và chuyển Unicode tập trung tại cầu nối.
- Không giữ khóa engine khi gọi mã Java có thể kích hoạt callback trở lại.
- Chờ người dùng chọn một menu là hành vi UI hợp lệ; chờ một giọng nói kết thúc để tiếp tục vòng lặp game là hành vi phải tránh.

### 5.3. Hợp đồng dữ liệu tối thiểu

Các trường dưới đây mô tả thiết kế, không phải đoạn mã có thể chép vào nguồn rồi biên dịch ngay.

**Phiên ngữ cảnh:** `session_id`, `context_id`, `owner_id`, `generation`, loại màn hình, tiêu đề, quyền chặn bên dưới, node đang focus và các ý định khả dụng.

**Nút giao diện:** ID ổn định theo đối tượng logic, vai trò, nhãn, giá trị, trạng thái bật/chọn/đánh dấu, lý do không khả dụng, vị trí và tổng số trong kết quả lọc, tóm tắt, chi tiết, hành động, phiên bản dữ liệu. Không dùng địa chỉ bộ nhớ làm ID lâu dài.

**Ý định:** ID ý định, token ngữ cảnh, tham số có kiểu, node/đối tượng được chọn, phiên bản dùng để chọn, mã yêu cầu chống thực hiện trùng. Trước thực thi phải xác minh đối tượng vẫn còn và ngữ cảnh vẫn đúng.

**Kết quả:** mã yêu cầu, được thực hiện hay bị từ chối, lý do hợp lệ để nhân vật biết, thay đổi vị trí hoặc trạng thái, chi phí game do engine quyết định, sự kiện có thể phát. Không đoán “không di chuyển = đâm tường”, vì có thể là tấn công tại chỗ, đổi trạng thái hoặc bị từ chối bởi safe mode.

**Lời đọc:** văn bản đã bản địa hóa, mã phiên/ngữ cảnh, loại ưu tiên, khóa gộp, thời điểm hết hạn, nguồn, ID lời đọc và điểm tiếp tục nếu là nội dung dài.

## 6. Khởi động, TalkBack và khả năng thoát khỏi sự cố

### 6.1. Quyết định sản phẩm

Bộ đọc nội bộ là đường chơi chính, hoạt động được khi TalkBack tắt. TalkBack vẫn là công cụ quan trọng để người dùng quản lý điện thoại, cài giọng và xử lý hộp thoại hệ thống. Không xây một `AccessibilityService` toàn hệ thống chỉ để đọc game và không yêu cầu quyền thu âm nếu chưa có tính năng nhập giọng nói do người dùng chủ động bật.

Không tự ý tắt TalkBack. Không yêu cầu người dùng tắt TalkBack trước khi họ có một cách khác để nghe và điều khiển.

### 6.2. Hành trình lần mở đầu

1. Màn hình Java khởi động có nhãn, thứ tự focus và nút native dùng được với TalkBack.
2. Cho chọn “Dùng bộ đọc tích hợp”, “Giữ trình đọc hệ thống” hoặc “Hướng dẫn thiết lập”. Mô tả đúng mức hỗ trợ thực tế của từng lựa chọn.
3. Khởi tạo TTS, phát thử và yêu cầu người dùng xác nhận đã nghe rõ. Không chỉ kiểm tra mã trả về thành công.
4. Thiếu giọng tiếng Việt: giải thích bằng đường còn nghe được, cho mở cài đặt TTS, thử giọng có sẵn hoặc quay lại. Nếu mọi nguồn đọc đều không hoạt động, dùng thông báo thu âm ngắn đi kèm cho hướng dẫn phục hồi, không cho bước vào gameplay im lặng.
5. Đọc tiến độ cài/nâng dữ liệu theo mốc; báo dung lượng thiếu, lỗi giải nén và cách thử lại.
6. Hướng dẫn cách quay về điện thoại và bật lại TalkBack theo cài đặt thực tế của máy. Không giả định mọi máy đều có phím tắt hai nút âm lượng hoạt động.
7. Chỉ chuyển sang cử chỉ nội bộ sau khi người dùng chọn và hoàn tất bước thử. Lưu lựa chọn an toàn trước khi vào engine.
8. Chạy hướng dẫn cử chỉ trong vùng luyện tập không tiêu hao lượt, có trợ giúp và nút bỏ qua rõ ràng.

### 6.3. Các trạng thái phối hợp

| Trạng thái | Cách xử lý |
| --- | --- |
| Bộ đọc nội bộ tắt | Đường điều khiển hiện có; giữ khả năng mở thiết lập trợ năng |
| Bộ đọc nội bộ bật, không có touch exploration tranh cử chỉ | Cử chỉ nội bộ và TTS theo lựa chọn |
| Người dùng bật TalkBack khi đang chơi | Giữ dữ liệu, hủy cử chỉ dang dở, đưa ra màn hình phục hồi native tiếp cận được; tránh hai giọng cùng đọc một focus |
| Hộp thoại hoặc cài đặt Android nằm trên game | Đầu vào gameplay bị chặn; giải thích đường chuyển quyền đọc; khi trở lại dựng lại ngữ cảnh hiện hành |
| TTS lỗi | Ngừng nhận chuỗi di chuyển tự lặp, thử phục hồi giới hạn; luôn còn đường native/TalkBack hoặc hướng dẫn thu âm |

Chế độ dùng TalkBack xuyên suốt gameplay có thể được xây bằng cùng mô hình ngữ nghĩa và cầu nối native/virtual nodes. Nếu chưa làm, phải nói rõ đây là chế độ phục hồi và thiết lập, không quảng bá là đường chơi hoàn chỉnh thứ hai. Nếu sản phẩm yêu cầu cả hai đường chơi thì ma trận nghiệm thu phải chạy đầy đủ trên cả hai.

Không suy ra hành vi từ tên gói dịch vụ hoặc từ `isEnabled()` đơn lẻ. Trạng thái touch exploration chỉ giúp chẩn đoán/phối hợp đầu vào; lựa chọn người dùng quyết định chế độ. Tài liệu Android cũng cảnh báo việc thay đổi UI theo suy đoán về công nghệ trợ năng dễ tạo đường mã khó bảo trì.

### 6.4. Đường cứu hộ luôn tồn tại

- Lệnh “Tôi đang ở đâu?” đọc màn hình, mục đang chọn, chế độ điều khiển và cách quay lại.
- “Hủy” đóng một tầng đang hoạt động; ở gốc thì không tự thoát hay chờ một lượt.
- Bảng trợ giúp có mục dừng lời, đọc lại, thiết lập giọng, khôi phục cử chỉ và mở màn hình phục hồi native.
- Phím Back phải có hành vi được đọc và thử trên cả điều hướng nút lẫn điều hướng cử chỉ Android.
- Có thao tác cứu hộ toàn cục đã kiểm thử, nhưng không đặt toàn bộ khả năng cứu hộ vào một cử chỉ bốn ngón khó làm hoặc phím hệ thống mà app không chắc nhận được.
- Đổi ánh xạ cử chỉ phải có nút trả mặc định và thời gian thử; nếu người dùng không xác nhận cấu hình mới thì tự khôi phục cấu hình điều khiển cũ, không đụng bản lưu game.

## 7. Ngữ cảnh, điều khiển và trung tâm lệnh

### 7.1. Ngữ cảnh là một ngăn xếp có quyền sở hữu

Ví dụ: thế giới → hành trang → hành động vật phẩm → nhập số lượng → xác nhận nguy hiểm. Mỗi tầng có token và chặn đầu vào tầng dưới. Khi tầng trên đóng, trả về mục còn hợp lệ trong tầng dưới; nếu vật phẩm đã biến mất, chọn mục gần nhất và đọc lý do.

Tận dụng `input_context::input_context_stack` và `scoped_activation`; thêm phần mô tả ngữ nghĩa và token phiên. Không thay toàn bộ hệ thống nhập bằng một biến toàn cục mới. Cũng không chỉ dựa vào `touch_input_context`, vì bản sao này có chủ đích giữ thông tin cũ trong một số thời điểm.

Đăng ký/hủy đăng ký theo RAII hoặc vòng đời tương đương. Một callback từ menu đã đóng không được kích hoạt lệnh ở menu vừa mở. Hủy đầu vào đang chờ khi mất focus, đổi Activity, thay thế phiên chơi hoặc đổi chế độ.

### 7.2. Hai cách điều khiển, một mô hình hành động

**Cấu hình học dễ:** một ngón duyệt lựa chọn; chọn “Di chuyển”, duyệt tám hướng và xác nhận từng bước. Chậm hơn nhưng không phụ thuộc vuốt chéo hoặc nhiều ngón.

**Cấu hình chơi nhanh:** tại ngữ cảnh thế giới, vuốt một ngón theo tám hướng thực hiện một ý định di chuyển sau khi nhả tay; trong menu, cử chỉ đó chỉ duyệt menu. Chỉ bật sau hướng dẫn và người dùng có thể chuyển lại.

Cả hai đều dùng cùng bộ hành động, kết quả và bộ đọc. Không tạo hai luật gameplay.

| Thao tác đề xuất ban đầu | Trong menu/biểu mẫu | Trong thế giới, cấu hình nhanh |
| --- | --- | --- |
| Vuốt trái/phải | Mục trước/sau | Đi tây/đông một bước |
| Vuốt lên/xuống | Nhóm trước/sau hoặc điều chỉnh theo vai trò, có thông báo | Đi bắc/nam một bước |
| Vuốt chéo | Không bắt buộc; vẫn có đường qua danh sách | Đi chéo một bước nếu nhận dạng chắc chắn |
| Chạm hai lần | Kích hoạt mục đang chọn | Mở trung tâm lệnh; không tự tấn công hay chờ |
| Nhấn giữ | Nghe chi tiết mục | Nghe ô hiện tại và tình hình ngắn |
| Chạm hai ngón | Dừng lời đọc | Dừng lời đọc; không tiêu hao lượt |
| Nút Back hoặc mục Quay lại | Hủy một tầng | Mở bảng điều khiển/thoát có lựa chọn, không thoát tức thì |

Đây là ánh xạ cần thử với người dùng, chưa phải kết quả khảo sát cử chỉ. Mọi chức năng phải có đường qua menu một ngón; đọc lại, quét và trợ giúp không bắt buộc chạm ba ngón. Cho đổi tay thuận, khoảng vuốt và thời gian nhận dạng. Dùng đơn vị theo mật độ màn hình và thời gian đơn điệu.

### 7.3. Điểm chặn sự kiện phải đúng

Tại nguồn hiện tại, sự kiện đi qua xử lý ImGui và các đường joystick/chạm lặp. Khi bộ đọc nội bộ sở hữu đầu vào, phải kiểm soát sự kiện trước mọi nhánh có thể sinh hành động, bao gồm bộ lặp nằm trước vòng `SDL_PollEvent`.

Kiểm tra các sự kiện chuẩn hóa `CATA_FINGERDOWN`, `CATA_FINGERMOTION`, `CATA_FINGERUP`, `CATA_FINGERCANCELED` qua `sdl_version_wrappers.h`; dùng đúng lớp tương thích của repo. Không vừa xử lý cùng một lần chạm ở Java vừa ở SDL.

- Một chuỗi pointer chỉ sinh tối đa một ý định đã hoàn tất trong cấu hình bước đơn.
- Thêm ngón, chạm hủy hoặc rời app phải hủy cử chỉ cũ theo quy tắc rõ ràng.
- Không làm rơi resize, vòng đời, bàn phím, IME hoặc thao tác hệ thống khi chặn sự kiện chạm gameplay.
- Không để cử chỉ vào ImGui rồi tiếp tục thành di chuyển.
- Chuỗi di chuyển khi giữ tay là tùy chọn nâng cao; dừng ngay khi mở menu, phát hiện nguy hiểm hợp lệ hoặc nhận hủy.
- Tách hướng bắc/nam bản đồ với hướng màn hình và hướng tiến/lùi xe. Đọc rõ khi đổi hệ quy chiếu.

### 7.4. Trung tâm lệnh phải tìm được mọi chức năng

Nhóm đề xuất: di chuyển và tương tác; quan sát và âm thanh; nhân vật; đồ đạc; chiến đấu; chế tạo/xây dựng; bản đồ/nhiệm vụ; NPC/cộng đồng; phương tiện; vùng/tự động hóa; lưu/cấu hình/trợ giúp; tất cả lệnh.

Mỗi lệnh có tên, giải thích ngắn, trạng thái, lý do không khả dụng, các tham số cần chọn và đường quay lại. Có tìm kiếm, nhóm yêu thích, lệnh vừa dùng và đặt phím/cử chỉ tùy chọn. Dùng ID hành động ổn định; không hardcode ký tự bàn phím hiện hành làm ý nghĩa của lệnh.

Nguồn đối chiếu gồm `action.h`/`action.cpp`, hành động đăng ký trong `input_context`, phím thủ công, dữ liệu keybinding và các thao tác UI riêng. `NUM_ACTIONS` chỉ là một điểm kiểm kê, không phải bằng chứng đã phủ mọi tương tác.

Trình điều phối xác minh lệnh vẫn đăng ký trong ngữ cảnh đích. Nếu phải hỗ trợ một màn hình dùng phím thô thì đóng gói ngoại lệ tại adapter, giải nghĩa và thử riêng. Không xây một hàng đợi phát chuỗi phím mù dùng chung cho toàn game.

### 7.5. Luật thời gian

Cấu hình tiếp cận khởi đầu đề xuất `TURN_DURATION=0`, thông báo lựa chọn này và bảo toàn cấu hình cũ để người dùng có thể đổi lại. Với cấu hình cũ đang bật realtime, phải cảnh báo trước khi vào thế giới, cho chuyển về lượt hoặc tiếp tục có chủ ý.

Đọc trạng thái, duyệt menu ngữ nghĩa, quét và di chuyển con trỏ thăm dò không tự gửi `WAIT`, không sửa moves, RNG hay thời gian game. Những hành động vốn có chi phí trong game, như mở cửa, điều khiển xe, kiểm tra/tương tác hoặc hoạt động nhiều lượt, giữ luật engine.

Ngủ, đợi, đọc sách, chế tạo, tự đi và xe đang chuyển động cần cơ chế hủy theo nhịp xử lý hoạt động; không hứa thế giới dừng chỉ vì đang đọc TTS. Nguy hiểm và sự kiện phải ngắt theo quy tắc game. Nếu bổ sung một chế độ hỗ trợ có thay đổi thời gian thì công bố như tùy chọn gameplay riêng và kiểm thử riêng.

## 8. Mô hình giao diện ngữ nghĩa và nhập văn bản

### 8.1. Bộ đọc chung

Hỗ trợ các vai trò: nút, mục danh sách, ô đánh dấu, giá trị lựa chọn, trường số, thanh giá trị, văn bản dài, bảng nhiều cột, cây nhóm, lựa chọn nhiều mục, đối tượng vị trí và mục tiêu.

Lời đọc tối thiểu khi focus: tên → giá trị/trạng thái → vị trí trong tập lọc → lý do không khả dụng nếu có. Hướng dẫn cử chỉ chỉ đọc lần đầu hoặc khi người dùng hỏi, tránh lặp sau mọi mục.

Đồng bộ từ dữ liệu logic của màn hình. Với `uilist`, xử lý đúng `entries`, `fentries`, `selected`, `fselected`, `retval`, category và `enabled`; không nhầm chỉ số danh sách lọc với giá trị trả về. Kiểm tra nhánh `ANDROID_NATIVE_UI` trước phần ImGui.

Trong chế độ nội bộ, ưu tiên một đường menu ngữ nghĩa thống nhất; nếu giữ native dialog thì dialog cũng cần điều khiển và đọc nội bộ khi TalkBack tắt. Native widget tự có nhãn không đồng nghĩa nó tự phát lời đọc.

ImGui dùng clipper chỉ vẽ một phần danh sách. Cây ngữ nghĩa phải lấy toàn bộ mô hình dữ liệu hoặc cung cấp phân trang đúng tổng số; không chỉ xuất các hàng được vẽ. Không đọc bằng cách chụp draw list hoặc ghép mọi `TextUnformatted` trong một frame.

### 8.2. Focus và thay đổi danh sách

- Duy trì focus theo ID logic nếu mục còn tồn tại; báo khi kết quả lọc thay đổi.
- Đổi nhóm đọc tên nhóm và số kết quả; danh sách rỗng có câu mô tả và đường xóa bộ lọc.
- Mục bị vô hiệu vẫn có thể được khám phá để nghe lý do khi cần; thao tác kích hoạt vẫn bị engine từ chối đúng luật.
- Lựa chọn nhiều mục phải đọc số lượng, tổng đã chọn và trạng thái chọn; không chỉ đọc tên hàng.
- Hủy lựa chọn hàng loạt phải có cách rõ ràng; không nhầm “bỏ chọn” với “thả hết vật phẩm”.
- Bảng nhiều cột có đọc từng thuộc tính, so sánh hai đối tượng và đọc tiêu đề cột theo yêu cầu.
- Không phát lại toàn bộ màn hình sau mỗi redraw hoặc thay đổi số nhỏ không liên quan focus.

### 8.3. Đọc dài

Tóm tắt, từng thuộc tính, từng đoạn và toàn văn là các chế độ truy cập nội dung. Tóm tắt phải dựa trên trường dữ liệu xác định, không dùng mô hình sinh văn bản để đoán ý nghĩa hoặc bỏ sót cảnh báo.

Cho đọc tiếp từ đoạn, quay đoạn, đọc lại câu cuối, đánh vần tên, nghe đơn vị và đọc nguyên văn khi cần. Nội dung bị nguy hiểm ngắt không tự phát lại từ đầu. Hỗ trợ lịch sử theo phiên và tìm kiếm thông báo.

### 8.4. Nhập tiếng Việt và số lượng

Hai đường nhập: bàn phím Android qua màn hình native tiếp cận được khi người dùng chọn dùng trình đọc hệ thống; và trình soạn nội bộ không cần TalkBack cho đường chơi độc lập.

Trình soạn nội bộ cần:

- Chèn, xóa trước/sau con trỏ, di chuyển theo ký tự hiển thị và từ, chọn tất cả, đọc lại, xác nhận và hủy về nội dung gốc.
- Nhóm chữ/số/ký hiệu, chữ Việt đủ hoa/thường hoặc quy tắc tạo dấu có hướng dẫn. Đừng bắt người dùng duyệt hàng trăm ký tự chỉ để nhập tên.
- Tên ngẫu nhiên có thể nghe và sửa để giảm thao tác; đây là lựa chọn, không thay thế quyền nhập tên tùy ý.
- Trường số có tăng/giảm, bước điều chỉnh và nhập trực tiếp; đọc min/max, lỗi định dạng và giá trị bị giới hạn.
- Hỗ trợ tên nhân vật/thế giới, tìm vật phẩm/công thức, ghi chú bản đồ và cấu hình shortcut, không chỉ một ô đặt tên.
- Xử lý UTF-8/UTF-16 đúng tại JNI, dấu tổ hợp, ký tự ngoài BMP, dán văn bản và IME composition. Con trỏ/xóa dựa trên grapheme phù hợp, không cắt byte UTF-8 hoặc nửa surrogate.
- Chỉ chuẩn hóa văn bản ở nơi có quyết định rõ ràng; bảo toàn cú pháp tìm kiếm đặc biệt và dữ liệu người dùng nhập.
- Nếu thêm nhập giọng nói thì coi là tùy chọn, xác nhận nội dung nhận dạng trước khi dùng; không làm điều kiện bắt buộc để chơi offline.

## 9. Nhận biết thế giới và bản đồ không rò thông tin

### 9.1. Quyền biết nằm ở từng thuộc tính

Không dùng một cờ “ô đã biết” để mở toàn bộ nội dung ô. Một ô có thể nhớ địa hình nhưng không biết vật phẩm hiện tại; có tiếng động nhưng chưa xác định sinh vật; có khả năng cảm nhận đặc biệt chỉ mô tả một phần.

Mỗi mô tả phải có nguồn: nhìn thấy hiện tại, đã ghi nhớ, nghe thấy, nhận biết đặc biệt hoặc thông tin do game cung cấp qua tương tác. Nguồn đó quyết định những trường được phép xuất.

| Nguồn nhận biết | Có thể đọc | Không được tự suy ra |
| --- | --- | --- |
| Nhìn thấy hiện tại | Các thuộc tính nhân vật thật sự thấy theo logic game | Nội dung bị che, bẫy ẩn, đồ trong vật chứa chưa biết |
| Địa hình ghi nhớ | Dữ liệu đã ghi nhớ mà game đang có, gắn nhãn “đã ghi nhớ” | Cập nhật bí mật cửa, đồ, quái hoặc nguy hiểm ngoài tầm biết |
| Âm thanh | Mô tả âm, hướng, thời điểm và độ chính xác mà giao diện hợp lệ cho biết | Tên nguồn thật, loại quái hay tọa độ chính xác hơn khả năng nghe |
| Cảm nhận đặc biệt | Các trường do cơ chế đó cho phép, có nhãn thích hợp | Mặc định cả ô và mọi chỉ số của thực thể |
| Chưa biết | “Chưa biết”, giới hạn quan sát | Địa hình, vật phẩm, bẫy hoặc nguy cơ thật bên trong |

Tên, độ bền, hư hại, độc tính, tình trạng sinh vật và nhận dạng vật phẩm cũng tuân theo hiểu biết của nhân vật. Dùng mô tả hiển thị hợp lệ, không lấy raw ID hoặc trường nội bộ làm câu đọc mặc định.

### 9.2. Nguồn truy vấn

Tách dữ liệu khỏi các hàm vẽ ở `game_ui_tile_info.cpp`; đối chiếu nhánh visibility, memory, special vision và âm thanh. Tận dụng các bộ lọc phù hợp ở `surroundings_menu.cpp`, nhưng không mặc định mọi hàm phụ tại đó là API truy vấn không tác dụng phụ.

Sử dụng tọa độ có kiểu của CDDA: phân biệt tọa độ cục bộ trong reality bubble, tọa độ tuyệt đối và overmap. Mô hình nhớ dùng khóa thế giới/nhân vật/tầng/tọa độ thích hợp, không lấy chỉ số ô trên màn hình làm định danh.

Các bộ đọc nhận dữ liệu đã lọc. Backend Java, TTS và nhật ký phản hồi thông thường không được nhận cả bản đồ thật rồi tự quyết định che bớt. Bản ghi phát triển chứa dữ liệu thô, nếu cần, phải tách khỏi bản phát hành và khỏi báo cáo người chơi chia sẻ.

### 9.3. Năm cách đọc không gian

**Ô đang đứng:** tên địa hình, vật thể tương tác, vật phẩm được biết, nguy hiểm nhận biết, tầng cao và các lối đi. Đọc ngắn mặc định; hỏi thêm để nghe chi tiết.

**Tám ô kề:** mô tả lần lượt các hướng theo thứ tự cố định; gom hướng trống tương đương; có thể hỏi riêng “phía bắc”. Phân biệt khả năng đi qua đã biết với lời khẳng định chắc chắn về ô chưa biết.

**Quét theo hướng:** bán kính có thể đổi, mặc định ngắn để dễ nghe. Chia vùng hiện thấy, ghi nhớ và chưa biết. Có thể đọc địa hình đã nhớ xa hơn một vật cản hiện tại nếu game có dữ liệu nhớ, nhưng nói rõ không phải quan sát hiện thời. Không khẳng định hành lang liên tục hoặc an toàn qua đoạn chưa biết.

**Thăm dò bằng con trỏ:** chuyển ô, nhảy đến cửa/cầu thang/vật phẩm/mục tiêu đã biết, lọc, trở về nhân vật và thoát. Không thay tọa độ nhân vật, không gọi hành động quan sát có chi phí thay cho truy vấn thuần túy. Khoảng cách và tầng được đọc theo đơn vị ô, không tự đổi sang mét nếu chưa có quy ước của game.

**Tổng hợp và danh sách điểm đáng chú ý:** nhóm lối thoát, vật phẩm quan trọng, sinh vật nhận biết và âm thanh. Sắp xếp ổn định, cho nghe theo hướng hoặc khoảng cách; không tự kết luận “an toàn” chỉ vì không thấy địch.

Ví dụ minh họa, không phải kết quả chạy máy:

> Phía đông: cửa mở cách 2 ô. Zombie nhìn thấy cách 5 ô. Sau cửa có 3 ô sàn nhìn rõ. Phần xa hơn chưa biết.

> Phía bắc: bạn nhớ có hành lang. Hiện không nhìn thấy. Có tiếng va đập từ hướng này; chưa xác định nguồn.

### 9.4. Kết quả di chuyển và vật cản

Phản hồi dựa trên engine: đã đi; mở cửa; tấn công thay vì đi; bị safe mode chặn; gặp địa hình; phương tiện; sinh vật; hoặc lý do chưa xác định mà nhân vật được biết. Có thể cần thêm sự kiện kết quả tại điểm xử lý hành động nếu API hiện tại chưa trả đủ thông tin.

Không tiết lộ tên một sinh vật tàng hình chỉ vì nó làm bước đi thất bại. Không nói “bạn đã đi về phía đông” trước khi vị trí thật đổi. Chỉ đọc chi phí hoặc cảnh báo tương ứng với dữ liệu engine thực sự cung cấp.

Khi bước vào nguy hiểm, giữ hộp thoại xác nhận gốc và làm nó đọc được. Một lớp trợ năng không tự bảo đảm tránh bẫy ẩn hoặc tự động chiến đấu cho người chơi.

### 9.5. Bản đồ lớn và dẫn đường

Overmap có chế độ riêng: ô địa điểm, tọa độ/hướng tương đối, tầng, mức đã khám phá, ghi chú, điểm nhiệm vụ, bộ lọc và tuyến đường. Chỉ dùng tri thức overmap mà nhân vật đã có. Biết một địa danh trên overmap không có nghĩa biết từng căn phòng của bản đồ cục bộ.

Cho đọc đoạn tuyến, tổng chiều dài theo đơn vị phù hợp, điểm chuyển và lý do không thể tìm đường. “Chọn điểm đến” chỉ chuẩn bị tuyến; di chuyển tự động cần xác nhận riêng và giữ cơ chế dừng của game.

Phân biệt dẫn đường cục bộ, di chuyển dài và tự lái. Không dò vùng chưa khám phá bằng cách thử pathfinding toàn tri rồi đọc “đường tốt nhất”. Kiểm tra cả rò thông tin gián tiếp qua có/không có đường, thời gian phản hồi và tổng kết vật cản.

### 9.6. Làm mới và ghi nhớ

- Redraw, xóa vùng vẽ, chuyển tiles/ASCII, xoay máy hoặc giảm rendering không xóa tri thức game.
- Thay phiên chơi, nhân vật hoặc thế giới phải tách dữ liệu; không cho bản đồ người chơi trước xuất hiện ở người chơi sau.
- Chuyển tầng không nhất thiết xóa bộ nhớ các tầng khác; dùng bộ nhớ gốc và khóa tầng đúng. Chỉ hủy snapshot động và con trỏ không còn hợp lệ.
- Khi reality bubble trượt, quy đổi lại tọa độ cục bộ từ khóa tuyệt đối; không ghép nhầm ô cũ.
- Không tự lưu snapshot sinh vật hiện tại như dữ liệu ghi nhớ lâu dài. Nếu game có dấu vết hoặc thông tin “thấy lần cuối” thì phải gắn nguồn và thời điểm đúng.
- Khi tải bản lưu, dựng lại trạng thái ngữ nghĩa từ game và cấu hình trợ năng; bỏ hàng đợi lệnh, lời đọc và mục tiêu đang treo.

## 10. Tiếng nói, âm hiệu, rung và thông báo

### 10.1. Backend TTS Android

Tạo `AndroidSpeech.java` hoặc tên tương đương trong package hiện tại. C++ gửi lời đã chuẩn bị; Java quản lý engine, voice, tốc độ, pitch, âm lượng tương đối và callback. Dùng `TextToSpeech` với ID từng utterance và `UtteranceProgressListener`; `speak()` chỉ báo việc xếp hàng, không chứng minh đã phát hoặc đã nghe.

Trạng thái tối thiểu: chưa khởi tạo, đang khởi tạo, sẵn sàng, đang khôi phục, lỗi và đã đóng. Có timeout không chặn UI, thử lại giới hạn, chọn lại engine/voice và câu thử. Callback có thể đến từ nhiều luồng, vì vậy phải chuyển qua hàng đợi an toàn và lọc theo mã phiên.

Khởi tạo từ lớp bootstrap dùng chung cho Splash và Activity game theo thiết kế vòng đời rõ ràng. Không tạo engine mới mỗi lần mở menu hoặc mỗi câu; không giữ Activity đã hủy trong callback. Dừng khi cần, giải phóng tài nguyên lúc kết thúc chủ sở hữu, tái tạo sau lỗi binder hoặc process restart.

Khai báo truy vấn dịch vụ TTS trong manifest theo Android hiện hành, xác minh giọng Việt có sẵn/offline, xử lý giọng cần mạng và thiếu dữ liệu. Không yêu cầu mạng cho gameplay đọc bình thường nếu người dùng đã cài voice offline. Bộ đọc không gọi API AI hoặc dịch máy trực tuyến làm phụ thuộc chạy game.

Chia văn bản dài theo đoạn/câu và giới hạn đầu vào TTS thực tế; không cắt giữa grapheme. Quy định phần văn bản được gửi sang engine TTS do người dùng chọn, đặc biệt nếu voice đó dùng mạng.

### 10.2. Chính sách phát

| Loại | Ví dụ | Chính sách |
| --- | --- | --- |
| Khẩn và cần phản ứng | Hoạt động bị ngắt vì nguy hiểm, trạng thái cực hạn cần biết | Ngắt nội dung dài, phát ngắn, chống cảnh báo dồn dập |
| Quan trọng | Kết quả lệnh, lựa chọn xác nhận, thay đổi ngữ cảnh | Phát trước mô tả nền; giữ thứ tự nhân quả |
| Focus | Mục đang chọn, hướng đang duyệt | Gộp theo ngữ cảnh/nhóm; mục mới thay mục cũ chưa phát |
| Chủ động hỏi | Chi tiết vật phẩm, sách, lịch sử | Có thể ngắt/đọc tiếp; không hết hạn như lời focus |
| Nền | Thông báo phụ hoặc tình trạng lặp | Gộp, hạn chế tần suất; luôn có đường xem lại |

“Theo yêu cầu” là nguồn lời đọc và chính sách phát, không nhất thiết là mức ưu tiên thấp nhất. Yêu cầu nghe câu hỏi xác nhận có thể cần ưu tiên cao hơn lời tự động thường.

Hàng đợi phải có giới hạn số lượng và tuổi; xóa lời focus đã hết ngữ cảnh; giữ cảnh báo trong lịch sử dù bỏ phát lặp. Gộp theo ID sự kiện/đối tượng, không chỉ so chuỗi vì hai sự kiện khác nhau có thể cùng câu.

Phân biệt:

- **Dừng lời:** chỉ tắt âm đang đọc, không xác nhận menu và không hủy hành động gameplay đã thực hiện.
- **Hủy hoạt động:** gửi ý định hủy tới engine ở điểm cho phép; báo kết quả hủy thật.
- **Đọc lại:** đọc focus/trạng thái cập nhật hoặc mục lịch sử được yêu cầu; không phát một mục tiêu cũ như còn hiện diện.

Người dùng vuốt nhanh 20 mục chỉ nên nghe focus cuối cùng sau khi dừng; không tồn tại hàng chờ 20 câu. Sau cảnh báo khẩn, chỉ tiếp tục văn bản dài nếu còn đúng ngữ cảnh và người dùng chọn tiếp.

### 10.3. Thông báo game và trạng thái

Gắn hook vào đường thông báo hợp lệ sau khi xử lý debug, cooldown, coalescing và các điều kiện nhận biết tương ứng. Trong `messages.cpp`, không chỉ hook một overload bên ngoài rồi bỏ qua các nhánh khác hoặc đọc cả thông báo vốn đã bị ẩn.

Không lấy việc redraw nhật ký làm sự kiện mới. Có ID/thứ tự bản tin, xử lý bản tin bị gộp, phân loại cảnh báo và cho đọc lịch sử. Tránh đọc kép một kết quả từ cả hook hành động lẫn hook message.

Trạng thái nhân vật cần truy vấn theo nhóm: cơ thể và thương tích; máu/đau; đói/khát/nhiệt; sức bền và vận động; giấc ngủ; encumbrance/trang bị; hiệu ứng bệnh/thuốc; kỹ năng/tiến độ; đột biến/bionics/năng lượng khi có. Dùng ngưỡng và thay đổi có ý nghĩa; không phát mọi giá trị ở mỗi vòng lặp.

### 10.4. Âm hiệu và rung

Chọn bộ tín hiệu nhỏ: đổi focus, chọn thành công, không khả dụng, bị chặn, cảnh báo và đổi chế độ. Có danh mục nghe mẫu và giải thích ý nghĩa. Âm thanh không là nguồn duy nhất của quyết định nguy hiểm.

Tùy chỉnh riêng TTS, âm game, cue và rung. Trộn âm để tiếng nói không che mất dấu hiệu game; xử lý audio focus, Bluetooth, tai nghe bị rút và cuộc gọi. Không giả định stereo luôn dùng được; mọi hướng quan trọng có thể nghe bằng lời ở mono. Mẫu rung phân biệt nhưng ngắn và có thể tắt.

Tín hiệu cũng phải qua ranh giới thông tin: không phát âm cảnh báo một bẫy chưa phát hiện, không rung theo khoảng cách đến quái vật nhân vật chưa biết.

### 10.5. Tiếng Việt

Thống nhất thuật ngữ hướng, tầng, loại hành động, đơn vị, mức hư hại và tên trạng thái. Giữ đường đọc nguyên tên gốc khi bản dịch thiếu, cung cấp đánh vần hoặc mô tả. Xử lý thẻ màu/biểu tượng bằng bảng chuyển có ngữ cảnh, không xóa luôn dấu trừ, phần trăm, phép so sánh hoặc cú pháp tìm kiếm.

Kiểm tra số âm, số thập phân, phân số, nhiệt độ, thể tích, trọng lượng, sức chứa, tốc độ xe, số đạn và thời gian. Dùng cơ chế dịch/số nhiều sẵn có; tránh nối chuỗi cứng làm sai ngữ pháp. Không lưu câu đã dịch lâu dài nếu người dùng có thể đổi ngôn ngữ giữa phiên.

Catalog tiếng Việt hiện ở `vi.po.gz`; làm theo hướng dẫn snapshot, không commit cả bản nén và bản `.po` cạnh tranh nhau. Kiểm tra bản `.mo` và tài nguyên Java thực sự được đóng gói trong APK, không chỉ có nguồn dịch trong repository.

## 11. Ma trận phủ tính năng bắt buộc

Mỗi hàng dưới đây phải được tách tiếp thành các màn hình/thao tác thực tế ở giai đoạn kiểm kê. Cột “nghiệm thu” là năng lực cần đạt, chưa phải tính năng đang hoạt động trong snapshot.

| Nhóm | Nội dung cần đọc và điều khiển | Nghiệm thu thực tế |
| --- | --- | --- |
| Khởi động/cài đặt | Cài dữ liệu, nâng cấp, thiếu dung lượng, lỗi, chọn ngôn ngữ và giọng | Từ cài sạch tới menu chính, tự xử lý lỗi mẫu |
| Thế giới/mod | Tạo thế giới, tùy chọn, danh sách mod, phụ thuộc/xung đột, xóa/đổi tên | Tự tạo và quản lý một thế giới; không xóa nhầm |
| Nhân vật | Kịch bản, nghề, nền tảng, chỉ số, kỹ năng, traits, tên, điểm còn lại và xung đột | Tạo tùy chỉnh, sửa lựa chọn, lưu mẫu và bắt đầu |
| Menu/hộp thoại | Danh sách, tab, radio, nhiều lựa chọn, Yes/No, popup, tiến độ | Không màn hình bế tắc hoặc “bấm phím bất kỳ” vô danh |
| Di chuyển | Tám hướng, tầng cao, tư thế, chạy/đi/chồm, kéo/đẩy, leo, bơi | Đi qua bản đồ mẫu và hiểu chi phí/cảnh báo từ engine |
| Quan sát | Ô, hướng, vật phẩm, sinh vật nhận biết, âm thanh, thăm dò | Khám phá không dùng mắt, không tiêu lượt truy vấn |
| Tương tác môi trường | Cửa/cửa sổ, examine, công tắc, đồ đạc, chất lỏng, container, đốt lửa | Chọn đúng hướng/đối tượng và hiểu kết quả |
| Hành trang | Nhặt/thả, cầm/mặc/cởi, sử dụng, chia stack, số lượng, túi lồng | Hoàn thành chuỗi từ nhặt tới sử dụng/cất, không mất đồ do nhầm focus |
| Quản lý kho nâng cao | So sánh, sort/filter, favorite, pockets, unload, transfer, advanced inventory | Chuyển nhiều đồ giữa kho/túi/xe bằng dữ liệu đầy đủ |
| Sức khỏe/sinh tồn | Ăn/uống, nhiễm độc biết được, thuốc, băng bó, chọn bộ phận, ngủ/đợi | Tự xử lý thương tích và sinh hoạt cơ bản |
| Chiến đấu cận chiến | Chọn địch, kiểu đánh, reach attack, võ thuật, trạng thái và safe mode | Đánh/rút lui, hiểu tấn công có trúng và đổi mục tiêu |
| Tầm xa/ném | Vũ khí, đạn, reload/unload, fire mode, aim, range/LOS, vùng ảnh hưởng | Chọn mục tiêu, nhắm/bắn/hủy và không tự bắn khi chỉ duyệt |
| Chế tạo/sửa chữa | Công thức, kỹ năng, công cụ, linh kiện thay thế, số lượng, thiếu nguyên liệu | Hoàn thành công thức và sửa vật phẩm qua mọi hộp thoại |
| Xây dựng/tháo dỡ | Công trình, vị trí, hướng, điều kiện, tiến độ và hủy | Xây một công trình mẫu và xử lý vị trí không hợp lệ |
| Kiến thức/đọc | Sách, yêu cầu đọc, học công thức/kỹ năng, nội dung dài | Đọc/học, ngắt và tiếp tục hợp lệ |
| Bản đồ lớn | Tìm địa điểm, tầng, legend, ghi chú, bộ lọc, tuyến và travel | Đến điểm đã biết, tạo/sửa ghi chú, hủy tuyến |
| Nhiệm vụ | Danh sách, mô tả, hạn, tiến độ, tọa độ đã biết, phần thưởng | Nhận, theo dõi, thực hiện và nộp nhiệm vụ mẫu |
| NPC/thương mại | Hội thoại, lựa chọn, phản ứng biết được, giá trị giao dịch, đồng đội | Mua/bán, so sánh, xác nhận và ra lệnh không cần nhìn |
| Phương tiện | Sơ đồ bộ phận, tên/vị trí, tình trạng, yêu cầu sửa/lắp, điều khiển, tốc độ và hướng | Sửa/lắp, lên/xuống, lái/dừng, thao tác cargo |
| Vùng/tự động hóa | Chọn hai góc/vùng, tầng, loại vùng, nguồn/đích, bật/tắt, thứ tự ưu tiên | Tạo, xem lại, sửa, xóa vùng và chạy một tác vụ mẫu |
| Cộng đồng/trại | Menu trại, NPC giao việc, nguyên liệu, thời gian, kết quả | Hoàn thành các chuỗi có trong nội dung phát hành |
| Đột biến/bionics | Danh sách, trạng thái, công suất, kích hoạt, chi phí và phẫu thuật | Dùng khả năng và hủy lựa chọn nguy hiểm đúng luật |
| Phép/khả năng từ nội dung | Phép, tầm, chi phí, mục tiêu, vùng và tác dụng | Nếu cấu hình phát hành có tính năng này, chạy đủ luồng |
| Thống kê/trợ giúp | Điểm số, lịch sử, bảng kỹ năng, keybinding, hướng dẫn | Tự tìm hiểu cơ chế và thay thiết lập được |
| Lưu/tải/phục hồi | Nhiều nhân vật/thế giới, xác nhận ghi/xóa, lỗi bản lưu, backup | Lưu rồi kill/relaunch/tải đúng nhân vật, tự xuất/nhập bản sao |
| Tử vong/nghỉ hưu | Thông báo, xác nhận, tổng kết, nhật ký, xử lý thế giới | Không mắc kẹt ở deathcam, hậu game hoặc menu kế tiếp |

### 11.1. Ba giao diện không thể giải quyết bằng đọc tên hàng

**Phương tiện:** chuyển sơ đồ hai chiều thành danh sách theo bộ phận, tọa độ tương đối trên xe và nhóm chức năng. Có chế độ duyệt ô/bộ phận, nghe phụ thuộc lắp/sửa, chọn vị trí và kiểm tra kết quả. Khi lái, đọc hướng xe, tốc độ, trạng thái động cơ và lệnh sắp áp dụng; không dùng cử chỉ đi bộ để vô tình tăng ga.

**Vùng:** cung cấp chọn góc bằng con trỏ, nhập tọa độ tương đối hoặc lấy vị trí hiện tại; đọc kích thước, tầng và phạm vi trước xác nhận. Đối chiếu giới hạn game và tri thức được phép; việc UI gốc cho tạo vùng ở vị trí nào phải được giữ đúng, không tự thêm quyền quan sát.

**Nhắm bắn/vùng ảnh hưởng:** đọc hướng, khoảng cách, che khuất, trạng thái aim và những thông tin cơ chế đang công khai. Có duyệt thực thể, chọn ô trống, đổi tầng và mô tả footprint vùng ảnh hưởng. Kết quả “không thể bắn” do engine quyết định; lời cảnh báo không được tiết lộ thực thể ẩn trong đường bắn.

## 12. Các điểm tích hợp trong mã nguồn

| Điểm nguồn đã kiểm tra hoặc xác định trong cây repo | Công việc dự kiến |
| --- | --- |
| `android/.../SplashScreen.java` | Bootstrap, cài dữ liệu, cảnh báo trợ năng, lỗi khởi động và đường phục hồi |
| `android/.../CataclysmDDA.java` | Chủ sở hữu backend, vòng đời Activity, kết nối JNI và lớp phủ native nếu dùng |
| `android/.../NativeUI.java` | Adapter cho dialog/menu Java; kiểm tra mọi đường trả về, hủy và Activity bị hủy |
| `android/app/src/main/AndroidManifest.xml` | Truy vấn dịch vụ TTS, Activity/phục hồi và tài nguyên cần thiết |
| `android/app/jni/CMakeLists.txt`, `src/CMakeLists.txt`, `Makefile` | Đưa nguồn mới vào build và backend rỗng cho nền tảng khác |
| `src/input_context.h/.cpp`, `src/input.cpp` | Token ngữ cảnh, ý định, đăng ký hành động và ranh giới đầu vào |
| `src/sdltiles.cpp`, `src/sdl_version_wrappers.h` | Sở hữu sự kiện chạm, hủy pointer, chống đầu vào kép, không phá vòng đời/IME |
| `src/action.h/.cpp`, `src/handle_action.cpp` | Đối chiếu danh mục lệnh, điều phối và kết quả; quan hệ với realtime |
| `src/uilist.h/.cpp`, `src/cata_imgui.h/.cpp` | Adapter menu và hợp đồng ngữ nghĩa; không bóc chữ từ draw list |
| `src/popup.cpp`, `src/input_popup.cpp`, `src/string_input_popup.cpp` | Câu hỏi, xác nhận, nhập văn bản/số, lỗi và đường hủy |
| `src/main_menu.cpp`, `src/newcharacter.cpp` | Menu đầu, thế giới và toàn bộ các tab tạo nhân vật |
| `src/game_ui_tile_info.cpp`, `src/surroundings_menu.cpp`, `src/map_memory.h/.cpp` | Trích xuất dữ liệu nhận biết/ghi nhớ và đọc không gian |
| `src/messages.cpp`, `src/panels.cpp` | Thông báo hợp lệ, lịch sử và trạng thái có ý nghĩa |
| `src/inventory_ui.h/.cpp`, `src/advanced_inv.cpp`, `src/pickup.cpp`, `src/item_action.cpp` | Đồ đạc, túi, chọn nhiều và chuỗi dùng vật phẩm |
| `src/ranged.cpp` và các adapter nhắm mục tiêu liên quan khi kiểm kê | Tầm xa, nhắm, đổi mục tiêu, chọn vị trí, vùng ảnh hưởng |
| `src/crafting_gui.cpp`, `src/construction.cpp` | Công thức, điều kiện, thay thế nguyên liệu và đặt công trình |
| `src/overmap_ui.cpp`, `src/mission_ui.cpp` | Bản đồ lớn, ghi chú, tuyến và nhiệm vụ |
| `src/npctalk.cpp`, `src/veh_interact.cpp`, `src/vehicle_use.cpp` | Hội thoại, thương mại, cấu trúc và điều khiển xe |
| `src/bionics.cpp`, `src/mutation_ui.cpp`, `src/magic.cpp` | Các khả năng và lựa chọn liên quan tới cấu hình nội dung |
| `lang/po/vi.po.gz`, tài nguyên `values-vi`, công cụ biên dịch localization | Thuật ngữ, câu mới, kiểm tra tiếng Việt trong gói cài |

Đây là bản đồ điểm chạm, không phải danh sách đầy đủ mọi file phải sửa. Một số file ở hàng nâng cao mới được xác định qua cây nguồn; cần đọc luồng chi tiết khi kiểm kê trước khi chốt vị trí hook. Không sửa hàng loạt gameplay để phát lời; ưu tiên adapter và sự kiện ổn định, chỉ bổ sung hook hẹp nơi thật sự thiếu nguồn dữ liệu/kết quả.

## 13. Lộ trình triển khai và cổng nghiệm thu

### Quy trình thực hiện được người dùng cập nhật ngày 22/10/2026

Người dùng xác nhận bản thử `cb3c082` đã mở và đọc được, đồng thời yêu cầu:
(1) viết mã cho toàn bộ phạm vi kế hoạch; (2) sau khi phần viết mã hoàn tất,
chạy toàn bộ bộ kiểm thử; (3) tổng hợp vấn đề rồi sửa và kiểm thử lại.

Áp dụng yêu cầu này cho công việc tiếp theo. G0–G8 dưới đây vẫn là các nhóm
chức năng và tiêu chí nghiệm thu; không dùng cổng nghiệm thu sớm để ngừng viết
mã nhóm phụ thuộc trong giai đoạn 1. Viết mã, đọc nguồn và soát thiết kế được
thực hiện trong giai đoạn 1; biên dịch, chạy test, giả lập và tạo APK mới được
để lại cho giai đoạn 2. Workflow trợ năng chuyển sang chạy thủ công. Mã chưa
được kiểm thử phải được ghi đúng trạng thái, không gán kết quả PASS cũ cho nó.
Xem [bảng triển khai](IMPLEMENTATION.vi.md) và [tiến độ](STATUS.md).

Mỗi giai đoạn phải có mã chạy, tình huống tái hiện, bản build xác định được và bản ghi người dùng đã tự làm được gì. Có thể chuẩn bị nội dung/ngữ nghĩa song song trong cùng nhóm, nhưng không bỏ qua phụ thuộc giữa các cổng.

### G0. Chốt nền và kiểm kê

**Việc làm:** chốt commit; build APK gốc; chạy trên máy; ghi cấu hình SDK/NDK/JDK/ABI; kiểm tra dữ liệu và tiếng Việt; xác định các tùy chọn ảnh hưởng input, native UI và realtime. Kiểm kê màn hình, ngữ cảnh, lệnh, phím thô và luồng Java ngoài engine. Tạo save mẫu không phụ thuộc ngẫu nhiên để kiểm thử.

**Sản phẩm:** APK baseline, ghi chép build, danh mục độ phủ, bộ tình huống, mô tả hai cấu hình điều khiển và quyết định về phạm vi TalkBack.

**Qua cổng khi:** build sạch lặp lại được, baseline chạy được, biết rõ phần nào chưa khảo sát; không bắt đầu viết adapter trên một nền Android đang hỏng chưa hiểu nguyên nhân.

### G1. Bootstrap, TTS và phục hồi

**Việc làm:** bộ chọn cách đọc; TTS với health state/callback; mẫu giọng; cấu hình offline; dừng/đọc lại; hủy theo phiên; cảnh báo trợ năng đã sửa; hướng dẫn thu âm khẩn; đọc cài dữ liệu và lỗi.

**Qua cổng khi:** người thử đi từ cài sạch tới màn hình có thể nghe, đổi giọng và quay lại sau khi mở cài đặt; thiếu voice hoặc lỗi TTS không tạo bế tắc. Không cần đợi hoàn thiện gameplay để phát hiện lỗi nền này.

### G2. Hợp đồng ngữ nghĩa, ngữ cảnh và đầu vào

**Việc làm:** node/focus; ngăn xếp token; dispatcher; cử chỉ bước đơn; menu chung; popup; nhập số/văn bản tối thiểu; lệnh trợ giúp/cứu hộ. Thử cả native UI và ImGui; xóa đầu vào dư khi đổi ngữ cảnh.

**Qua cổng khi:** 100 lần mở/đóng chuỗi menu lồng không phát sinh di chuyển; chọn/xóa/lọc đúng đối tượng; input hủy không biến thành tap; TTS không chặn thread; có kiểm thử callback cũ.

### G3. Lát cắt chơi độc lập đầu tiên

**Việc làm:** menu chính, tạo thế giới, một luồng tạo nhân vật hoàn chỉnh, nhập tên, vào bản đồ mẫu, đi/mở cửa/nhặt/ăn/uống, xem trạng thái, lưu/thoát/tải. Làm mô hình nhận biết tối thiểu trước khi bật bộ đọc bản đồ.

**Qua cổng khi:** người khiếm thị hoàn thành toàn chuỗi từ lần mở tới tải lại mà không nhận chỉ dẫn vị trí nút. Nếu chỉ hỗ trợ một preset tạo nhân vật ở đây thì phải ghi là giới hạn alpha; chưa coi tạo nhân vật tùy chỉnh đã hoàn tất.

### G4. Không gian và sinh tồn đầy đủ

**Việc làm:** tám hướng, thăm dò, âm thanh, memory, z-level, kết quả di chuyển, hành trang/túi/nhiều lựa chọn, sức khỏe/băng bó, ngủ/đợi, hoạt động bị ngắt, trạng thái và lịch sử. Hoàn tất các tab tạo nhân vật còn thiếu.

**Qua cổng khi:** hoàn thành phiên sinh tồn với các nhiệm vụ được giao, không rò thông tin ở tối, tường, bẫy, ô ghi nhớ hoặc tải save khác; quét không tiêu lượt; hủy hoạt động có kết quả rõ ràng.

### G5. Chiến đấu, chế tạo và xây dựng

**Việc làm:** melee, ranged, ném, reload/fire mode, chọn ô/vùng/tầng, safe mode; công thức và nguyên liệu thay thế; sửa, xây, tháo dỡ; đọc yêu cầu thiếu và chọn vị trí.

**Qua cổng khi:** người dùng hoàn thành chiến đấu, chế tạo và đặt công trình; duyệt mục tiêu không bắn; engine vẫn từ chối đường bắn/vị trí không hợp lệ; cảnh báo không bỏ sót thông tin quan trọng.

### G6. Các hệ thống nâng cao — phần bắt buộc của bản đầy đủ

**Việc làm:** overmap/travel/notes, nhiệm vụ, hội thoại/thương mại/đồng đội, xe/sửa/lắp/lái, vùng và tự động hóa, trại, bionics/mutations/phép nếu có trong nội dung phát hành, kiến thức và mọi trang cấu hình còn lại.

**Qua cổng khi:** từng hàng tương ứng trong ma trận phần 11 có bằng chứng độc lập; không dùng kết quả “vào được menu” để đánh dấu cả hệ thống hoàn tất. Đây có thể là nhiều bản thử nghiệm liên tiếp, không phải một PR khổng lồ.

### G7. Vòng đời, kết thúc và quản lý dữ liệu

**Việc làm:** deathcam/tử vong/nghỉ hưu; nhiều save; lỗi save/dữ liệu; app bị kill; cuộc gọi/khóa/xoay máy; xuất/nhập bản sao qua DocumentsProvider/file picker và chuyển quyền đọc hệ thống. Kiểm thử nâng cấp APK giữ config/save.

**Qua cổng khi:** người dùng tự hoàn tất vòng đời nhân vật và phục hồi dữ liệu mẫu; không phát lại lệnh cũ sau tải; sao lưu thật và khôi phục thật, không chỉ đọc được tên nút xuất.

### G8. Kiểm kê đóng, kiểm thử người dùng và phát hành

**Việc làm:** đối chiếu toàn danh mục; sửa các ngõ cụt; đo hiệu năng; kiểm tra localization đóng gói; thử người dùng trên thiết bị; tài liệu học chơi; release notes với giới hạn. Chỉ lúc này mới cân nhắc chế độ giảm hình sâu hơn.

**Qua cổng khi:** mọi điều kiện phần 1.2 đạt trên phạm vi công bố, các lỗi chặn đã đóng, APK phát hành truy vết được tới commit và bộ dữ liệu đã kiểm thử.

### G9. Bảo trì sau phát hành

Mỗi lần đồng bộ upstream, thay mod mặc định hoặc cập nhật nội dung đều chạy đối chiếu lệnh/màn hình và các hành trình liên quan. Khi có phần mới chưa hỗ trợ, công bố thay đổi độ phủ; không giữ lời tuyên bố đầy đủ của bản cũ cho một cấu hình mới chưa thử.

## 14. Chia công việc thành các thay đổi có thể review

Trình tự PR đề xuất, có thể tách nhỏ hơn theo nguồn lực:

1. Hồ sơ baseline, phạm vi và danh mục trợ năng; chưa thay gameplay.
2. Bootstrap Java, cấu hình cách đọc và phục hồi trước engine.
3. Backend TTS/JNI, callback/health/voice/offline và kiểm thử vòng đời.
4. Mô hình ngữ nghĩa, ngữ cảnh/focus/token và backend giả để kiểm thử.
5. Dispatcher ý định và sở hữu sự kiện chạm, giữ đường cũ khi tắt.
6. `uilist`, native dialog, popup, Yes/No và văn bản dài.
7. Trình soạn nội bộ, nhập số và xử lý Unicode/IME.
8. Menu chính, thế giới và tạo nhân vật theo tab.
9. Truy vấn nhận biết, snapshot bản đồ, ranh giới thông tin và kiểm thử kín.
10. Di chuyển, kết quả, quét/thăm dò và trạng thái/thông báo.
11. Hành trang, pockets, pickup và dùng vật phẩm; lát cắt chơi độc lập.
12. Sức khỏe/hoạt động/ngủ/đợi và hủy.
13. Chiến đấu/tầm xa/chọn vùng.
14. Chế tạo/sửa/xây dựng.
15. Overmap/nhiệm vụ; NPC/thương mại; phương tiện; vùng/trại; khả năng đặc biệt — từng nhóm một chuỗi PR riêng.
16. Tử vong, backup, recovery, nâng cấp, tài liệu và release.
17. Tối ưu dựng hình sau khi có phép đo và bài so sánh ngữ nghĩa.

Mỗi PR mô tả năng lực người chơi trước/sau, nguồn sự thật, ngữ cảnh nhận input, tác động tới thời gian game và kiểm thử liên quan. Không gộp đổi nền SDL, dịch lại toàn game và viết bộ đọc thành một thay đổi duy nhất.

Tên nhánh có thể dùng `feature/android-internal-reader`. Tiến độ triển khai, nhánh, build và bằng chứng thực tế được cập nhật tại STATUS.md cùng thư mục; các giai đoạn dưới đây không tự được coi là hoàn tất khi kế hoạch được đưa vào repository.

## 15. Kiểm thử và bằng chứng nghiệm thu

### 15.1. Bốn lớp kiểm thử bổ sung cho nhau

**Lõi C++:** ngữ cảnh, token cũ, focus, hàng đợi, kết quả hành động, ranh giới thông tin, tọa độ, grapheme và không tác dụng phụ. Dùng backend lời đọc giả để so sánh ý nghĩa sự kiện; không phụ thuộc chất lượng âm thanh của một TTS cụ thể.

**Tích hợp engine:** save/tình huống có seed và trạng thái xác định, kiểm tra trước/sau trên cùng hành động gốc. So sánh trạng thái gameplay của đường trợ năng và đường nhập chuẩn, tách các trường trình bày không liên quan. Không cho bộ đọc gọi RNG chỉ để mô tả; việc đó sẽ làm thay đổi ván chơi.

**Android và thiết bị:** TTS, JNI, đời Activity, pointer cancel, native dialog, IME, cài dữ liệu, process death và chuyển TalkBack. Accessibility Scanner hoặc Espresso có ích cho native UI, nhưng không tự chứng minh dữ liệu game đúng hoặc màn SDL chơi được.

**Người dùng:** người mù/nhìn kém dùng máy thực của họ, có nhiệm vụ cụ thể, cho phép nghe tài liệu và trợ giúp nội bộ nhưng không chỉ vị trí nút bằng mắt. Ghi chỗ bế tắc, nhầm chế độ và tải nhận thức; không chỉ hỏi “có thích không”. Tài liệu Android khuyến khích kết hợp kiểm thử thủ công, công cụ, tự động và phản hồi người dùng.

### 15.2. Bộ tình huống chuẩn cần xây

- Một phòng có tường, cửa đóng/mở, cửa sổ, đồ đạc, vật phẩm và cầu thang.
- Khu sáng/tối, quái nhìn thấy, quái sau tường, âm thanh không xác định nguồn, bẫy ẩn và bẫy đã biết.
- Hai bản lưu/nhân vật có tri thức bản đồ khác nhau; có dịch chuyển reality bubble và đổi z-level.
- Hành trang có nhiều stack tên giống nhau, túi lồng, chất lỏng, đồ hỏng, yêu cầu số lượng và mục biến mất sau thao tác.
- Trường bắn với mục tiêu, ô trống, vật cản, đồng minh nhận biết được, khác tầng và vùng ảnh hưởng.
- Khu chế tạo/xây dựng thiếu rồi đủ nguyên liệu; công cụ có charge và nhiều lựa chọn thay thế.
- Một xe có nhiều bộ phận, cargo và phần hỏng; khu lái kín có điểm dừng và vật cản.
- NPC/nhiệm vụ/giao dịch; vùng phân loại kho; trại/khả năng đặc biệt theo nội dung phát hành.
- Thiếu giọng Việt, engine TTS lỗi, thiếu dung lượng, cài lỗi, bản lưu lỗi và tiến trình bị hệ điều hành hủy.

Các tình huống này là hạng mục phải tạo; không phải tuyên bố đã có bộ save đi kèm tài liệu.

### 15.3. Bất biến phải tự động hóa

1. Truy vấn thuần túy không đổi thời gian, moves, vị trí, RNG hoặc trạng thái thế giới.
2. Một chuỗi pointer hợp lệ không thực hiện trùng một lệnh trong chế độ bước đơn.
3. Token ngữ cảnh/phiên hết hạn không có quyền thực thi; callback TTS cũ không làm phục hồi focus cũ.
4. Tầng overlay chặn đầu vào tầng dưới; đóng overlay khôi phục mục hợp lệ.
5. Dữ liệu chưa biết không xuất hiện trong snapshot công khai, cue, lịch sử hoặc báo cáo người chơi.
6. Dữ liệu ghi nhớ không tự cập nhật bằng trạng thái thật ngoài quyền biết.
7. Bản đồ và danh mục lệnh không phụ thuộc việc khung hình đã được vẽ.
8. Danh sách lọc trả đúng đối tượng/`retval`; lệnh vào đối tượng đã biến mất bị từ chối có giải thích.
9. Hủy TTS không biến thành hủy game; hủy game không chỉ dừng TTS rồi để hoạt động tiếp tục.
10. Tắt bộ đọc giữ hành vi điều khiển cũ trong các tình huống baseline đã chọn.

Kiểm thử phản ví dụ đặc biệt quan trọng: tạo hai thế giới có cùng điều nhân vật biết nhưng dữ liệu bí mật khác nhau. Câu đọc, cue và kết quả truy vấn trợ năng phải giống nhau trong các trường hợp mà UI gốc cũng không phân biệt được. Cách này mạnh hơn việc chỉ quét tên trường nhạy cảm trong lớp dữ liệu.

### 15.4. Danh sách 64 bài nghiệm thu tối thiểu

Mỗi bài ghi: build/commit, cấu hình nội dung, thiết bị/Android/voice, trạng thái TalkBack, dữ liệu đầu vào, thao tác, kết quả mong đợi, kết quả thật và bằng chứng. Những lỗi khác nhau không được gộp thành một chữ PASS cho cả nhóm.

#### Khởi động và cấu hình

**T01 — Cài sạch.** Với TalkBack đang bật, mở ứng dụng lần đầu, nghe lựa chọn, cài dữ liệu, thử giọng và tới menu. Không có hộp thoại bắt buộc không đọc được.

**T02 — Mở lại không TalkBack.** Từ cấu hình bộ đọc nội bộ đã lưu, cold start tới menu, nghe trợ giúp và thoát an toàn; không cần bật lại TalkBack để qua màn app.

**T03 — Thiếu tiếng Việt.** Máy chưa có voice Việt; tìm được hướng dẫn cài/chọn voice khác/quay lại. Không im lặng và không lặp vô hạn hộp thoại.

**T04 — TTS trả lỗi hoặc không bắt đầu.** Kích lỗi có chủ ý; phát hiện bằng trạng thái/timeout, không khóa game, có đường phục hồi nghe được. Nếu callback tới muộn thì không phá phiên mới.

**T05 — Thiếu dung lượng/cài dữ liệu lỗi.** Nghe nguyên nhân, thử lại hoặc thoát; không báo “đã vào game” khi chưa sẵn sàng.

**T06 — Dịch vụ trợ năng khác.** Bật một dịch vụ không đọc màn hình; không bắt tắt vô lý. Với TalkBack bật lại, người dùng vẫn điều khiển được đường phục hồi.

**T07 — Thử đổi cử chỉ.** Đặt cấu hình khó sử dụng rồi không xác nhận; trở lại cấu hình cũ, giữ bản lưu game.

**T08 — Hồ sơ realtime cũ.** Tải cấu hình `TURN_DURATION>0`, nghe cảnh báo và chọn cách chơi trước khi vào nguy hiểm. Với cấu hình theo lượt, nghe lâu không tự tạo lượt.

#### Ngữ cảnh, menu và văn bản

**T09 — Menu lồng.** Mở hành trang → hành động → số lượng → xác nhận rồi hủy từng tầng. Không có bước chân hoặc tấn công phát sinh.

**T10 — Cử chỉ bị hủy.** Vuốt dang dở, thêm ngón, xoay máy hoặc rời app; không sinh tap/di chuyển muộn.

**T11 — Đầu vào kép.** Thực hiện 100 chuỗi chạm có định danh; mỗi chuỗi sinh đúng số ý định được thiết kế, không đồng thời đi qua joystick cũ và bộ đọc.

**T12 — Focus cũ.** Đóng menu rồi gửi callback/chọn mục cũ; lệnh bị loại. Menu mới giữ focus hợp lệ.

**T13 — Danh sách lớn.** Duyệt tới mục ngoài phần ImGui đang vẽ, tìm kiếm và mở đúng mục; số thứ tự/tổng đúng theo tập lọc.

**T14 — Danh sách rỗng/disabled.** Lọc không còn kết quả, nghe cách xóa lọc; mục không dùng được có lý do và không kích hoạt nhầm mục khác.

**T15 — Hai nhánh UI.** Lặp chuỗi menu với `ANDROID_NATIVE_UI` bật/tắt theo chế độ sản phẩm hỗ trợ; không nhánh trả về sớm thiếu lời đọc hoặc đường hủy.

**T16 — Đọc dài bị ngắt.** Đang nghe mô tả thì có cảnh báo; nghe cảnh báo rồi chủ động tiếp đoạn còn lại, không lặp từ đầu và không đọc tài liệu của màn đã đóng.

**T17 — Nhập tên tiếng Việt.** Nhập/sửa/xóa “Nguyễn Đặng”, hoa/thường, dấu tổ hợp và ký tự ngoài BMP thử nghiệm. Không hỏng encoding hoặc xóa nửa ký tự.

**T18 — Nhập số/tìm kiếm.** Chọn một phần stack, nhập số không hợp lệ rồi sửa; dùng cú pháp tìm kiếm của game và hủy để phục hồi nội dung gốc.

#### Thế giới và bản đồ

**T19 — Tám hướng/tầng.** Di chuyển đủ tám hướng, lên/xuống cầu thang, đổi tư thế. Hướng đọc khớp tọa độ game và không đổi theo xoay màn hình.

**T20 — Kết quả bước đi.** Thử đi thành công, mở cửa, đánh sinh vật, bị safe mode chặn, vướng xe/đồ đạc. Phản hồi mô tả đúng kết quả, không mặc định “tường”.

**T21 — Quét không tiêu lượt.** Chụp trạng thái thời gian/moves/RNG trước và sau 100 truy vấn/thăm dò. Không có thay đổi gameplay do truy vấn.

**T22 — Quái sau tường.** Một quái thấy được, một quái không biết sau tường; chỉ công bố thông tin được phép, không khác qua cue/rung.

**T23 — Âm không xác định.** Nghe nguồn âm mà không thấy; chỉ đọc thông tin hợp lệ từ perception, không tiết lộ tên thực thể hoặc khoảng cách chính xác hơn game cho biết.

**T24 — Bẫy/đồ chưa biết.** Bẫy ẩn, container đóng và vật phẩm chưa nhận dạng không bị lộ; khi phát hiện hợp lệ thì thông tin cập nhật.

**T25 — Ghi nhớ thay đổi bí mật.** Nhìn một cửa rồi rời tầm nhìn, thay cửa bí mật trong tình huống thử. Bộ đọc vẫn đọc dữ liệu nhớ, gắn nguồn, không trạng thái thật mới.

**T26 — Xóa màn hình.** Redraw, resize, đổi tiles/ASCII hoặc đóng/mở màn; quét vẫn giữ tri thức đúng. Đây là hồi quy trực tiếp từ bản tham khảo GnollHack.

**T27 — Đổi reality bubble/tầng.** Đi qua ranh giới tải bản đồ, đổi tầng rồi quay lại; không nhầm tọa độ hoặc xóa bộ nhớ hợp lệ.

**T28 — Đổi nhân vật/thế giới.** Tải hai nhân vật có tri thức khác nhau; không còn mục tiêu/đồ/địa hình của nhân vật trước.

**T29 — Thăm dò và bộ lọc.** Di chuyển con trỏ, nhảy tới cửa/đồ đã biết, recenter và thoát; nhân vật không đi theo con trỏ.

**T30 — Rò gián tiếp.** Hai bản đồ cùng tri thức nhưng bí mật khác nhau; truy vấn quét/tổng hợp/tìm đường không phân biệt bí mật bằng nội dung, số đếm hoặc dấu hiệu phụ vượt UI gốc.

#### Đồ đạc và sinh tồn

**T31 — Chọn nhiều/số lượng.** Nhặt một phần stack, bỏ chọn, đổi số lượng, xác nhận; số lượng và tổng đúng, không chọn nhầm do filter.

**T32 — Túi lồng.** Cất/lấy ở các pockets, xử lý sức chứa và container không hợp lệ; nghe rõ nguồn/đích và lý do từ chối.

**T33 — Vật phẩm biến mất.** Dùng hết một món rồi danh sách cập nhật; focus chuyển hợp lý, không kích hoạt món khác qua callback cũ.

**T34 — Advanced inventory.** Chuyển đồ giữa hai bên, đổi nguồn/đích, lọc, chọn nhiều và hủy; không nhầm cột hoặc tổng khối lượng/thể tích.

**T35 — Ăn/uống/chất lỏng.** Chọn vật chứa, đổ/chuyển/dùng; nghe cảnh báo mà game cho biết và xử lý đúng toàn bộ hộp thoại.

**T36 — Băng bó/thuốc.** Chọn bộ phận cơ thể và vật tư, nghe hiệu quả/điều kiện hợp lệ; không dựa vào màu thương tích.

**T37 — Ngủ/đợi/hoạt động.** Bắt đầu, nghe tiến độ có tiết chế, hủy hoặc bị game ngắt; báo việc hủy đã thực hiện, không chỉ dừng tiếng.

**T38 — Trạng thái và lịch sử.** Vượt ngưỡng đói/khát/đau/nhiệt/sức bền, kiểm tra có và hết hiệu ứng; đọc lại thông báo đúng thứ tự không đọc kép khi redraw.

#### Chiến đấu, chế tạo và công trình

**T39 — Cận chiến.** Chọn địch/kiểu đánh, tấn công và rút lui; nghe đủ diễn biến mà nhân vật được biết, không lộ HP bí mật.

**T40 — Duyệt mục tiêu.** Đổi mục tiêu, chọn ô trống, đổi tầng, hủy; số đạn và lượt không đổi chỉ vì duyệt.

**T41 — Aim/fire/reload.** Chọn fire mode và đạn, nhắm rồi bắn, nạp sai loại và hủy; kết quả/chi phí do engine quyết định và đọc rõ.

**T42 — LOS/range/vùng ảnh hưởng.** Thử mục tiêu ngoài tầm, bị che, ném và vùng tác dụng; engine từ chối đúng, cảnh báo không tiết lộ thực thể ẩn.

**T43 — Công thức thiếu/đủ.** Nghe yêu cầu, đổi nguyên liệu thay thế và batch; chế tạo khi đủ và giải thích thiếu khi chưa đủ.

**T44 — Sửa/xây/tháo.** Chọn vị trí/hướng, xử lý ô không hợp lệ, bắt đầu/hủy/hoàn thành; hình preview có mô tả nghe tương đương.

#### Hệ thống nâng cao

**T45 — Overmap và travel.** Chọn địa điểm đã biết, đổi tầng, nghe tuyến, xác nhận/hủy; tự đi giữ dừng nguy hiểm của engine.

**T46 — Ghi chú/nhiệm vụ.** Tạo/sửa/xóa ghi chú; nhận/theo dõi/nộp nhiệm vụ và đọc hạn/tiến độ theo thông tin được cấp.

**T47 — Hội thoại/thương mại.** Đọc người nói, lựa chọn, lịch sử, giá trị hai bên, xác nhận mua/bán và hủy; không chọn nhầm câu khi nội dung đổi.

**T48 — Đồng đội/trại.** Chọn người, công việc, nguồn lực, thời gian và kết quả; xử lý từ chối hoặc thiếu điều kiện.

**T49 — Sơ đồ xe.** Tìm bộ phận theo ô/vị trí, nghe tình trạng và phụ thuộc lắp/sửa; hoàn thành sửa/lắp mẫu mà không nhìn sơ đồ.

**T50 — Lái xe.** Phân biệt đi bộ/lái, hướng xe/bản đồ, tăng/giảm tốc, dừng, lên/xuống và mở cargo; đổi chế độ không gây tăng ga ngoài ý muốn.

**T51 — Vùng.** Chọn hai góc/tầng, nghe kích thước, đổi loại, bật/tắt và xóa; xác nhận phạm vi trước tác vụ.

**T52 — Khả năng đặc biệt.** Dùng mutation/bionics/phép có trong cấu hình, chọn mục tiêu và chi phí, hủy; không gộp “menu mở được” thành PASS cả hệ thống.

#### Vòng đời, độ bền và phát hành

**T53 — Lưu/kill/tải.** Lưu, kết thúc tiến trình thật, mở lại, chọn đúng nhân vật; không phát lại lời/lệnh/mục tiêu cũ.

**T54 — Cuộc gọi/khóa/mất focus.** Ngắt khi TTS và thao tác chạm đang diễn ra; quay lại đúng ngữ cảnh, không bước hoặc bắn muộn, không tự xác nhận dialog.

**T55 — Tạo lại Activity/xoay máy.** Không giữ Activity cũ, không mất quyền hủy, không nhân đôi backend, listener hoặc hành động.

**T56 — Đầu ra âm thanh.** Thử loa, mono, tai nghe dây/Bluetooth, rút tai nghe và audio focus; vẫn có đường hiểu hướng và phục hồi âm lượng.

**T57 — Bản lưu lỗi/backup.** Mở lỗi mẫu, nghe thông báo và xuất/nhập bản sao qua file picker bằng đường trợ năng đã hỗ trợ; không ghi đè bản gốc vô ý.

**T58 — Tử vong/nghỉ hưu.** Qua deathcam/tổng kết/nhật ký/xử lý thế giới và về menu; không kẹt trong màn chỉ có hình.

**T59 — Tất cả lệnh.** Mở trung tâm lệnh ngay từ khởi động/lần đầu vào game, khi không nạp tài sản đồ họa tùy chọn; danh mục đầy đủ theo sổ đối chiếu, không chỉ còn nút Quay lại.

**T60 — TTS dồn hàng đợi.** Vuốt nhanh qua 20 mục, thay ngữ cảnh, phát cảnh báo rồi đọc lại; chỉ focus hiện hành được phát tự động, lịch sử vẫn có nội dung cần thiết.

**T61 — Phiên dài.** Chạy ít nhất hai giờ với nhiều chuyển màn và TTS; đo bộ nhớ/listener/queue/độ trễ, không chỉ kiểm tra “không crash”. Tăng thời lượng khi người dùng thực tế cần phiên dài hơn.

**T62 — Tắt bộ đọc.** Chạy các hành trình baseline với bộ đọc tắt; joystick, bàn phím/IME, native UI, resize và lưu vẫn theo hành vi đã chọn để bảo toàn.

**T63 — Gói phát hành.** Cài APK có chữ ký phát hành, kiểm tra nguồn/commit, ABI, thư viện, dữ liệu, bản dịch, voice và nâng cấp giữ save/config. AAB sinh ra thành công chưa thay thế kiểm tra cài APK thực tế.

**T64 — Người dùng mới.** Người khiếm thị chưa biết bộ cử chỉ hoàn thành hướng dẫn, tạo nhân vật, sinh tồn, tìm một chức năng nâng cao, lưu/tải và kết thúc mà không nhận chỉ dẫn bằng mắt; ghi rõ trợ giúp đã sử dụng và chỗ mắc kẹt.

Đây là tập tối thiểu để bắt đầu; sau kiểm kê phải thêm trường hợp cho các màn hình chưa được biểu diễn, tính năng mod được phát hành và mọi lỗi đã từng gặp.

### 15.5. Mẫu hồ sơ độ phủ

Mỗi chức năng có một bản ghi gồm: ID, màn hình/ngữ cảnh, đường tìm, dữ liệu được đọc, tham số, kích hoạt, hủy, kết quả, ranh giới nhận biết, test ID, người kiểm thử, build và bằng chứng. Trạng thái dùng: chưa kiểm kê, chưa làm, đang làm, đã kiểm tra tự động, đã thử thiết bị, đã người dùng nghiệm thu, còn giới hạn.

Không tự đánh dấu “đã người dùng nghiệm thu” từ kết quả kiểm tra tĩnh. Tại thời điểm viết tài liệu, các hạng mục triển khai ở trên đều là đề xuất, chưa được đánh dấu đã làm.

Hai tỷ lệ có thể dùng theo dõi là số luồng người dùng đã nghiệm thu trên tổng luồng đã kiểm kê, và số thao tác đã phủ trên tổng thao tác thuộc phạm vi. Luôn hiển thị số màn hình/ngữ cảnh chưa kiểm kê. “100% của danh sách chưa đầy đủ” không phải bằng chứng phủ toàn game.

### 15.6. Quy trình người dùng tham gia

Đề xuất nhóm đầu 3–5 người với mức quen Android và CDDA khác nhau, gồm người mù hoàn toàn và người mới với game. Đây là nhóm thử formative để tìm vấn đề, không phải cỡ mẫu chứng minh mọi người dùng đều dùng được.

Thử sớm ở G1–G3, sau đó theo nhóm tính năng. Ghi tỷ lệ hoàn thành, số lần cần trợ giúp, nhầm chế độ, thao tác thừa và việc có tìm được trợ giúp hay không. Cho nghỉ để tránh mệt vì âm thanh; lấy đồng ý trước khi thu âm/ghi chép có thông tin cá nhân. Không cần ghi toàn bộ nội dung bản lưu của người dùng để chứng minh một lỗi điều khiển.

## 16. Hiệu năng và tối ưu chế độ ít hình

### 16.1. Mục tiêu đo ban đầu

Các con số dưới đây là ngân sách thiết kế đề xuất, chưa phải số đo hoặc bảo đảm của Android TTS. Chốt lại sau G0–G3 trên ít nhất một máy thấp trong dải hỗ trợ và một máy phổ biến của nhóm thử.

- Đầu vào tới phản hồi cue cục bộ: mục tiêu dưới 100 ms ở phân vị 95 sau khi app sẵn sàng.
- Quyết định lời focus và gửi tới backend: mục tiêu dưới 100 ms ở phân vị 95; đo riêng thời gian engine TTS thực sự bắt đầu.
- Dừng lời bằng lệnh người dùng: mục tiêu dưới 200 ms trên backend được kiểm thử; báo riêng trường hợp engine không đáp ứng.
- Bắt đầu lời focus ngắn với voice offline đã ấm: mục tiêu dưới 500 ms ở phân vị 95; cold start/voice cần mạng báo riêng.
- Sau duyệt nhanh, không còn lời focus thuộc mục hoặc ngữ cảnh cũ được phát tự động.
- Không rebuild toàn cây hoặc quét toàn bản đồ ở từng frame. Cập nhật theo thay đổi ngữ nghĩa và truy vấn người dùng.
- Không tăng bộ nhớ/listener/queue không giới hạn qua nhiều lần mở/đóng màn. So mức ổn định đầu/cuối phiên thay vì đặt một mức MB tùy tiện cho mọi máy.

Đo bằng timestamp đơn điệu: pointer kết thúc → ý định được nhận → engine trả kết quả → speech gửi → `onStart`/`onDone`/lỗi. Thời điểm callback TTS không hoàn toàn đồng nghĩa âm đến tai; khi cần xác nhận độ trễ nghe thật, đo thêm bằng phương pháp âm thanh phù hợp trên thiết bị.

### 16.2. Tối ưu theo thứ tự

1. Gộp lời dư, snapshot theo phiên, cache nhãn bất biến và invalidation đúng khi giá trị/ngôn ngữ đổi.
2. Tránh reflection hoặc duyệt cây UI trong vòng lặp chính; adapter biết cấu trúc dữ liệu.
3. Giảm dựng lại màn, tốc độ animation và hiệu ứng theo tùy chọn đã có hoặc thay đổi hẹp.
4. Đo tải CPU/GPU, nhiệt và pin trong cùng tình huống trước/sau.
5. Chỉ xem xét tắt sâu rendering sau khi dữ liệu bản đồ/menu đã tách khỏi đường vẽ.

Trước mỗi tối ưu hình, so snapshot ngữ nghĩa và trạng thái gameplay trên cùng kịch bản, kiểm tra danh mục lệnh, tiến độ tải, callback/wait và âm thanh. Không bỏ callback mà một luồng khác vẫn chờ. Không xóa bộ nhớ nhận biết khi làm sạch buffer. Giữ chế độ hình tùy chọn cho người nhìn kém và hỗ trợ kỹ thuật nếu phù hợp.

Một bản “BLIND_ONLY” riêng không phải điều kiện bắt buộc để người mù chơi được. Ưu tiên một codebase, tùy chọn runtime và các kiểm tra hồi quy; biến thể build chỉ thêm khi phép đo cho thấy cần và có khả năng bảo trì.

## 17. Nguồn lực, ước lượng và quản lý rủi ro

### 17.1. Nguồn lực cần có

- Người phụ trách C++/luồng game, hiểu nhận biết, hành động, dữ liệu và hệ thống UI CDDA.
- Người phụ trách Android/Java/JNI/TTS, vòng đời, cử chỉ và thiết bị; một người có thể kiêm nhưng chi phí chuyển ngữ cảnh cao.
- Người kiểm thử tiếp cận và nhóm người chơi khiếm thị tham gia đều đặn.
- Người rà thuật ngữ/giọng đọc tiếng Việt; có thể kiêm vai trò QA.

Đừng coi việc nhờ AI viết code là thay thế được build, đọc luồng engine và người dùng kiểm thử. Công việc tốn thời gian nhất có thể là các màn hình ngoại lệ và sửa lỗi chuyển ngữ cảnh, không phải tạo câu TTS.

### 17.2. Khoảng công sức lập kế hoạch

Ước lượng sau là phán đoán để lập ngân sách, dựa trên phạm vi trong tài liệu; chưa đo vận tốc của nhóm và chưa phải báo giá hay lịch cam kết. Một tuần-người là một người làm việc toàn thời gian trong một tuần, không phải một tuần lịch cho mọi đội.

| Giai đoạn | Công sức tham khảo |
| --- | --- |
| G0: baseline và kiểm kê | 1–3 tuần-người |
| G1: bootstrap/TTS/phục hồi | 2–4 tuần-người |
| G2: ngữ nghĩa/ngữ cảnh/input | 3–6 tuần-người |
| G3: lát cắt độc lập đầu tiên | 3–5 tuần-người |
| G4: không gian và sinh tồn | 6–10 tuần-người |
| G5: chiến đấu/chế tạo/xây | 6–12 tuần-người |
| G6: hệ thống nâng cao | 16–36 tuần-người |
| G7: vòng đời/kết thúc/dữ liệu | 3–6 tuần-người |
| G8: nghiệm thu/phát hành | 4–8 tuần-người |
| Tổng trước dự phòng | 44–90 tuần-người |

Cộng dự phòng khoảng 25–40% khi lập ngân sách ban đầu, nhưng phải thay bằng ước lượng từ backlog thật sau G3. Đây không phải giới hạn trên: kiểm kê có thể làm phạm vi tăng đáng kể. Không chia tổng công sức tuyến tính cho số người để hứa ngày phát hành, vì nhiều phần phụ thuộc tuần tự và cần thời gian thử người dùng.

Một lập trình viên duy nhất có thể đạt bản alpha trước rất nhiều so với bản đầy đủ, nhưng mục tiêu đầy đủ nên được quản lý như dự án dài hạn. Nếu chỉ có nguồn lực cho G0–G4, hãy công bố đó là bản sinh tồn cơ bản và giữ backlog nâng cao; không gắn nhãn hoàn thiện.

### 17.3. Rủi ro và cách xử lý

| Rủi ro | Biện pháp và tín hiệu cần theo dõi |
| --- | --- |
| Độ phủ giả vì chỉ đếm lệnh | Kiểm kê cả bước tham số, dialog và màn hình ngoại lệ; mỗi luồng có bằng chứng |
| Hai hệ thống nhận cùng cử chỉ | Một chủ sở hữu input, token và test pointer/ImGui/joystick; đo số lệnh theo chuỗi chạm |
| Rò tri thức qua dữ liệu hoặc cue | Lọc trước backend; test hai thế giới cùng tri thức; kiểm tra memory/per-field |
| TTS chậm/lỗi/mất voice | Health state, callback, timeout, offline, recovery native và hướng dẫn khẩn |
| Lỗi build nền hoặc phụ thuộc SDL | Baseline tái lập, tách thay đổi nền, kiểm tra cả APK cài thật |
| UI mới/phím thô không đi qua adapter | Đối chiếu context/action và cảnh báo chẩn đoán cho màn chưa có ngữ nghĩa |
| Người dùng quá tải lời đọc | Tóm tắt, mức verbosity, đọc thuộc tính, cue nhỏ và thử nhiệm vụ thực |
| Nhầm hướng khi lái xe/thăm dò | Tên chế độ và hệ quy chiếu đọc rõ; ý định theo ngữ cảnh, không tái dùng mù cử chỉ |
| Dữ liệu gameplay phụ thuộc rendering | Tách sớm semantic source, regression clear/resize, trì hoãn headless |
| Dịch sai câu nguy hiểm | Bản địa hóa gốc, glossary và review tình huống; giữ khả năng nghe tên gốc |
| Upstream thay đổi nhiều | Adapter hẹp, PR nhỏ, chốt phạm vi từng release và chạy lại các hành trình bị ảnh hưởng |
| Phụ thuộc duy nhất một người | Tài liệu hợp đồng, save thử, ghi build, review và backlog có tiêu chí rõ |

Giấy phép và quyền phân phối: khi dự định tái sử dụng mã/asset trực tiếp, rà giấy phép của từng dự án/thành phần và giữ thông báo cần thiết. Việc học kiến trúc không đồng nghĩa được chép mọi mã C#/âm thanh từ ZIP sang CDDA. Đây là hạng mục kiểm tra trước tái sử dụng, không phải kết luận pháp lý về tính tương thích giấy phép.

## 18. Tài liệu và gói bàn giao của dự án triển khai

Các đầu ra cần tạo trong quá trình làm, ngoài mã nguồn:

1. Hướng dẫn build tái lập, phiên bản công cụ, dữ liệu và checksum gói.
2. Tài liệu kiến trúc, hợp đồng node/context/action/perception/speech và quy tắc thread.
3. Danh mục màn hình/lệnh, trạng thái hỗ trợ và những ngõ cụt còn lại.
4. Bộ save/kịch bản kiểm thử và cách tái hiện lỗi.
5. Báo cáo thiết bị/TTS/người dùng; phân biệt kiểm tra tự động với kiểm tra thật.
6. Hướng dẫn bằng tiếng Việt dùng được với trình đọc: mở lần đầu, cử chỉ, nhập chữ, bản đồ, chiến đấu, cấu hình giọng và phục hồi.
7. Hướng dẫn ngắn tích hợp trong app và vùng luyện tập không tiêu lượt.
8. Release notes nêu đúng phạm vi, lỗi còn biết, định danh bản nguồn và cách sao lưu trước nâng cấp.
9. Mẫu báo lỗi: phiên bản game, thiết bị, cách vào màn hình, hành động cuối, mong đợi/thực tế và log tối thiểu không chứa bí mật không cần thiết.
10. Quy trình thêm màn hình/lệnh mới: cập nhật semantic adapter, localization, danh mục và test trước khi đánh dấu hỗ trợ.

Để bắt đầu ngay, ưu tiên G0 và một chuỗi dọc nhỏ: bootstrap → giọng đọc → menu → một nhân vật → một phòng → một hành động có kết quả → lưu/tải. Chuỗi này kiểm tra được kiến trúc thật. Sau đó mở rộng theo ma trận; không xây riêng mười mô-đun lớn rồi mới lần đầu ghép chúng.

## 19. Tham chiếu và dấu vết khảo sát

### 19.1. Nguồn Abcd-huy tại commit đã chốt

- [Snapshot đã khảo sát](https://github.com/tvhuy99-web/Abcd-huy/tree/8ef359c74cc25e256919558bf7b3340f9b56a80d).
- [Android CMake](https://github.com/tvhuy99-web/Abcd-huy/blob/8ef359c74cc25e256919558bf7b3340f9b56a80d/android/app/jni/CMakeLists.txt): SDL3, C++17, libmain và danh sách nguồn.
- [Cấu hình Android mặc định](https://github.com/tvhuy99-web/Abcd-huy/blob/8ef359c74cc25e256919558bf7b3340f9b56a80d/android/gradle.properties).
- [SplashScreen.java](https://github.com/tvhuy99-web/Abcd-huy/blob/8ef359c74cc25e256919558bf7b3340f9b56a80d/android/app/src/main/java/com/cleverraven/cataclysmdda/SplashScreen.java): bootstrap, cài đặt, lỗi và cảnh báo trợ năng.
- [CataclysmDDA.java](https://github.com/tvhuy99-web/Abcd-huy/blob/8ef359c74cc25e256919558bf7b3340f9b56a80d/android/app/src/main/java/com/cleverraven/cataclysmdda/CataclysmDDA.java).
- [NativeUI.java](https://github.com/tvhuy99-web/Abcd-huy/blob/8ef359c74cc25e256919558bf7b3340f9b56a80d/android/app/src/main/java/com/cleverraven/cataclysmdda/NativeUI.java).
- [Input context](https://github.com/tvhuy99-web/Abcd-huy/blob/8ef359c74cc25e256919558bf7b3340f9b56a80d/src/input_context.h) và [xử lý input](https://github.com/tvhuy99-web/Abcd-huy/blob/8ef359c74cc25e256919558bf7b3340f9b56a80d/src/input_context.cpp).
- [SDL tiles và cảm ứng](https://github.com/tvhuy99-web/Abcd-huy/blob/8ef359c74cc25e256919558bf7b3340f9b56a80d/src/sdltiles.cpp).
- [Menu uilist](https://github.com/tvhuy99-web/Abcd-huy/blob/8ef359c74cc25e256919558bf7b3340f9b56a80d/src/uilist.cpp): ImGui, filter và nhánh native.
- [Tạo nhân vật](https://github.com/tvhuy99-web/Abcd-huy/blob/8ef359c74cc25e256919558bf7b3340f9b56a80d/src/newcharacter.cpp) và [chế tạo](https://github.com/tvhuy99-web/Abcd-huy/blob/8ef359c74cc25e256919558bf7b3340f9b56a80d/src/crafting_gui.cpp).
- [Thông tin ô](https://github.com/tvhuy99-web/Abcd-huy/blob/8ef359c74cc25e256919558bf7b3340f9b56a80d/src/game_ui_tile_info.cpp), [xung quanh](https://github.com/tvhuy99-web/Abcd-huy/blob/8ef359c74cc25e256919558bf7b3340f9b56a80d/src/surroundings_menu.cpp), [bộ nhớ bản đồ](https://github.com/tvhuy99-web/Abcd-huy/blob/8ef359c74cc25e256919558bf7b3340f9b56a80d/src/map_memory.h).
- [Thông báo](https://github.com/tvhuy99-web/Abcd-huy/blob/8ef359c74cc25e256919558bf7b3340f9b56a80d/src/messages.cpp).
- [Tùy chọn](https://github.com/tvhuy99-web/Abcd-huy/blob/8ef359c74cc25e256919558bf7b3340f9b56a80d/src/options.cpp): `SCREEN_READER_MODE`, `TURN_DURATION`, lựa chọn Android.
- [Xử lý hành động](https://github.com/tvhuy99-web/Abcd-huy/blob/8ef359c74cc25e256919558bf7b3340f9b56a80d/src/handle_action.cpp) và [danh mục hành động](https://github.com/tvhuy99-web/Abcd-huy/blob/8ef359c74cc25e256919558bf7b3340f9b56a80d/src/action.h).
- [Ghi chú giao diện và trợ năng sẵn có](https://github.com/tvhuy99-web/Abcd-huy/blob/8ef359c74cc25e256919558bf7b3340f9b56a80d/doc/USER_INTERFACE_AND_ACCESSIBILITY.md).
- [Hướng dẫn catalog tiếng Việt](https://github.com/tvhuy99-web/Abcd-huy/blob/8ef359c74cc25e256919558bf7b3340f9b56a80d/lang/po/README.vi.txt).

### 19.2. Nguồn trong gói GnollHack đính kèm

Các đường dẫn dưới đây là đường dẫn bên trong ZIP người dùng đã cung cấp, không phải liên kết tải riêng:

- `BLIND-SCAN8-ALLCOMMANDS-BOULDER-FIX-20260831.md`.
- `BLIND-VISUAL-HEADLESS-FIX-20260830.md`.
- `doc/accessibility/BLIND_MODE_DEVICE_JOURNEY_QA.md`.
- `win/win32/xpl/GnollHackM/Accessibility/AccessibilityContextManager.cs`.
- `win/win32/xpl/GnollHackM/Accessibility/UI/AccessibilityUiNode.cs` và `AccessibilityBlindTextEditor.cs`.
- `win/win32/xpl/GnollHackM/Accessibility/Speech/SpeechRequest.cs` và `AndroidAccessibilitySpeechBackend.cs`.
- `win/win32/xpl/GnollHackM/Accessibility/Gameplay/Map/AccessibilityInformationBoundary.cs`, `AccessibilitySemanticMapModel.cs` và `AccessibilityMapModeController.cs`.
- `win/win32/xpl/GnollHackM/Accessibility/QA/AccessibilityInformationLeakAuditor.cs`.
- `win/win32/xpl/GnollHackM/Accessibility/Gestures/AccessibilityGestureRecognizer.cs`.
- `win/win32/xpl/GnollHackM/Accessibility/SystemScreenReader/AccessibilitySystemScreenReaderCoordinator.cs`.

### 19.3. Tài liệu Android chính thức đã đối chiếu

- [TextToSpeech](https://developer.android.com/reference/android/speech/tts/TextToSpeech): API bất đồng bộ, khởi tạo, engine/voice, giới hạn văn bản, truy vấn dịch vụ và shutdown.
- [UtteranceProgressListener](https://developer.android.com/reference/android/speech/tts/UtteranceProgressListener): ID lời đọc và callback có thể tới từ nhiều luồng.
- [JNI tips](https://developer.android.com/ndk/guides/jni-tips): giới hạn chia sẻ JNIEnv và quản lý luồng/reference.
- [Custom views accessibility](https://developer.android.com/guide/topics/ui/accessibility/views/custom-views): cung cấp cấu trúc ngữ nghĩa và virtual view hierarchy khi một view chứa nhiều thành phần logic; đây là lựa chọn cho cầu nối hệ thống, không phải lý do phải chuyển toàn bộ SDL sang Compose.
- [AccessibilityManager](https://developer.android.com/reference/android/view/accessibility/AccessibilityManager): trạng thái trợ năng và lưu ý khi suy đoán hành vi theo touch exploration.
- [Accessibility testing](https://developer.android.com/guide/topics/ui/accessibility/testing): kết hợp thử thủ công, công cụ, tự động và người dùng.

Các quyết định kiến trúc, ngưỡng hiệu năng, ánh xạ cử chỉ, số người thử và ước lượng công sức trong tài liệu là đề xuất thiết kế của kế hoạch; không phải kết quả kiểm thử hoặc yêu cầu bắt buộc từ các tài liệu Android.
