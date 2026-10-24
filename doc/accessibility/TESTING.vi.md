# Danh mục kiểm thử giai đoạn 2 — CDDA Android trợ năng (R22)

> ## ⚠️ TRẠNG THÁI: DANH MỤC BÀI CHƯA CHẠY — CHƯA CÓ KẾT QUẢ PASS MỚI
>
> Tài liệu này là **danh mục kiểm thử của giai đoạn 2** cho mục R22: nó liệt
> kê các bài cần chạy, tiền điều kiện, fixture, các bước và oracle kỳ vọng.
> **Toàn bộ các bài trong tài liệu này đều ở trạng thái "chưa chạy — chưa có
> PASS mới".** Giai đoạn 1 chỉ viết mã và chuẩn bị bằng chứng nguồn; không
> bài nào trong danh mục dưới đây đã được build, chạy, hay nghiệm thu ở đợt
> viết tài liệu này. Không được trích một bài trong tài liệu này như bằng
> chứng đã đạt cho tới khi hồ sơ chạy thật của bài đó được ghi theo mẫu ở
> mục 1.6.
>
> Nội dung cũ của chính tệp này — quy trình thử giọng trên "màn thử giọng
> native / APK thử giọng" đời đầu — là **TÀI LIỆU LỊCH SỬ**: nó mô tả một
> bản thử chỉ có màn thử giọng và các thành phần nền, trong khi mã hiện tại
> đã có bộ đọc gameplay đầy đủ. Nội dung cũ **không bị xóa**: nó được giữ
> lại nguyên văn ở mục cuối của tài liệu này, dưới tiêu đề
> "PHẦN LỊCH SỬ — quy trình thử giọng trên APK thử giọng đời đầu (không áp
> dụng cho mã hiện tại)". Không áp dụng quy trình lịch sử đó cho mã hiện
> tại, và không dùng kết luận của nó để đánh giá bộ đọc gameplay.

- Kho: `~/workspace/cdda-work/repo` (github.com/tvhuy99-web/Abcd-huy).
- Nhánh đối chiếu khi viết danh mục: `recovery/android-reader-ad5ecf0`,
  HEAD `4350ce82` ("R21: precision notes from the diagnostics audit
  (MESSAGE_LIMIT steady state, DebugLog boundary)"). Ứng viên thực tế để
  chạy danh mục này phải được ghim lại ở R23 — xem mục 4.
- Nguồn danh sách T01–T64: `doc/accessibility/PLAN.vi.md` mục 15.4, đọc
  trực tiếp khi viết tài liệu này; tên và tóm tắt từng bài được giữ nguyên.
- Nguồn tên hành trình và mã TX của sổ: `doc/accessibility/JOURNEY-LEDGER.json`.
- Nguồn bài riêng về điểm đứt thi triển phép:
  `doc/accessibility/R17-BIONICS-MAGIC-COMPUTER.vi.md` mục 3.4.
- Nguồn bài riêng về chính sách Snake:
  `doc/accessibility/R18-MINIGAMES.vi.md` mục R18.4.
- Hợp đồng form/bảng/cây được kiểm thử: `doc/accessibility/R02-CONTRACTS.vi.md`.

---

## 1. Quy ước chung

### 1.1. Trạng thái và cách ghi kết quả

- Mọi bài bắt đầu ở trạng thái **chưa chạy**. Khi chạy, mỗi bài ghi một
  trạng thái trong các trạng thái sau: `PASS` (oracle đạt trọn vẹn),
  `FAIL` (ít nhất một oracle sai), `BLOCKED` (không chạy được vì điều kiện
  bắt đầu chưa thỏa — ghi rõ điều kiện thiếu), `KHÔNG ÁP DỤNG` (chỉ khi cấu
  hình phát hành đã ghim ở R23 loại trừ bề mặt đó một cách công khai).
- Không gộp nhiều bài thành một chữ PASS chung. Mỗi bài có hồ sơ riêng
  theo mẫu ở mục 1.6.
- Một bài chỉ được tính PASS ở lớp môi trường đã ghi cho bài đó. PASS ở
  host không thay cho PASS trên máy thật; PASS bằng test doubles không
  thay cho nghe thật bằng tai, TalkBack thật hay nghiệm thu người dùng —
  xem thêm mục 4.

### 1.2. Cấu trúc của mỗi bài

Mỗi bài trong cả hai danh mục (T và TX) đều ghi đủ năm phần:

1. **Tiền điều kiện** — trạng thái app/dữ liệu/cấu hình phải có trước bước
   đầu tiên; thiếu tiền điều kiện thì bài là `BLOCKED`, không phải `FAIL`
   của tính năng.
2. **Fixture** — một mã F cố định ở mục 1.3, hoặc đúng câu
   "không cần fixture — dùng bản cài sạch/mặc định".
3. **Các bước** — thao tác theo thứ tự, viết ở mức người chạy bài thực hiện
   được mà không phải đoán (đường vào, thao tác của bộ đọc, điểm dừng).
4. **Oracle kỳ vọng** — trạng thái game hoặc hành vi quan sát được phải
   đúng sau các bước: vị trí/trạng thái nhân vật, số lượng vật phẩm, giá
   trị đã áp dụng, nội dung snapshot, trạng thái hàng đợi, tệp/bản lưu
   trên đĩa… Oracle **không** được viết chỉ ở dạng "nghe thấy câu X": câu
   đọc là một kênh quan sát, còn điều phải đúng là trạng thái và hành vi
   mà câu đọc phản ánh. Khi oracle nhắc tới câu đọc, nó luôn đi kèm trạng
   thái kiểm chứng được độc lập (snapshot, log sự kiện, trạng thái engine
   hoặc quan sát trên máy).
5. **Môi trường** — một hoặc nhiều loại trong mục 1.4. Môi trường ghi đầu
   tiên là môi trường chính quyết định PASS của bài; các môi trường ghi
   sau là lớp xác nhận bổ sung.

### 1.3. Fixture cố định

Fixture là tình huống/dữ liệu chuẩn phải được dựng **trước** giai đoạn 2
theo đúng mô tả này; bài nào dùng fixture nào là cố định trong danh mục,
không tự thay fixture khác khi chạy. Việc dựng fixture (save/seed/kịch
bản) thuộc khâu chuẩn bị của giai đoạn 2, không thuộc đợt viết tài liệu
này.

| Mã | Tên fixture | Nội dung chuẩn |
| --- | --- | --- |
| F-01 | Cặp tri thức | Hai tình huống bản đồ có cùng phần nhân vật được biết nhưng dữ liệu bí mật khác nhau (quái sau tường, bẫy ẩn, cửa bị đổi trạng thái ngoài tầm nhìn); cộng một phòng chuẩn có tường, cửa đóng/mở, cửa sổ, đồ đạc, vật phẩm, cầu thang, vùng sáng/tối và một nguồn âm không xác định được bằng mắt. |
| F-02 | Cây túi | Hành trang có túi lồng nhiều cấp (pockets trong pockets), nhiều stack trùng tên, chất lỏng trong vật chứa, đồ hỏng, một vật phẩm sắp dùng hết, và tổng số mục đủ lớn để vượt phần danh sách ImGui vẽ trong một khung hình. |
| F-03 | Kho hai phía | Bố cục advanced inventory hai phía (nguồn/đích khác nhau: sàn, vật chứa, xe, NPC nếu có), có lọc, chọn nhiều, tổng khối lượng/thể tích khác nhau rõ rệt giữa hai phía. |
| F-04 | Vết thương | Nhân vật có vết thương ở nhiều bộ phận với mức khác nhau (chảy máu, bị cắn, đau, một hiệu ứng có/không theo thời gian), đủ vật tư băng bó/thuốc và một hoạt động điều trị có thể bắt đầu/hủy. |
| F-05 | Trường chiến đấu | Khu vực có mục tiêu nhìn thấy, mục tiêu sau vật cản, ô trống, đồng minh nhận biết được, mục tiêu khác tầng, vùng ảnh hưởng (ném/ nổ), súng có nhiều fire mode và ít nhất hai loại đạn (một loại sai). |
| F-06 | Xe | Một xe nhiều bộ phận (có bộ phận hỏng, có cargo, có smart engine controller), đỗ trong khu lái kín có điểm dừng và vật cản; nhân vật có đủ điều kiện vào ghế lái, xem sơ đồ và sửa/lắp một bộ phận mẫu. |
| F-07 | Vùng/quy tắc | Nhân vật có vùng đã tạo, đủ đồng đội/NPC để Study Zone có bảng kỹ năng × NPC (kể cả trường hợp số NPC làm bảng vượt 16 cột), và bộ quy tắc Safe Mode + auto-pickup có sẵn quy tắc ở cả hai tab Global/Character. |
| F-08 | Trại | Một trại phe đang hoạt động: có cư dân để giao việc/trả đồng đội, kho trại có vật phẩm để quét kho, công việc xây dựng/hậu cần đang dở, và ít nhất một lựa chọn mở rộng/biến đổi trại (expansion/transformation). |
| F-09 | Khả năng | Nhân vật có bionics (chủ động và bị động, có chi phí điện), phép đã học (có phép nhắm mục tiêu/AOE và phép thiếu điều kiện), và quyền truy cập một máy tính trong game có menu, văn bản terminal và nhánh hỏi YES/NO/QUIT. |
| F-10 | Kitten | Vật phẩm phần mềm chạy Robot Finds Kitten với bàn chơi chuẩn: robot, kitten và đủ vật thể bogus để duyệt; danh tính kitten không được công bố trước khi chạm vào. |
| F-11 | Minesweeper | Vật phẩm phần mềm chạy Minesweeper với bàn có mìn, ô đã mở, ô cắm cờ và ô chưa biết; gồm một bàn lớn (Custom) tiệm cận trần snapshot. |
| F-12 | Sokoban | Vật phẩm phần mềm chạy Sokoban với màn có kiện hàng, ô đích, kiện đã lên đích và nước đi đẩy được/không đẩy được. |
| F-13 | Lightson | Vật phẩm phần mềm chạy Lights On với bàn còn vài ô sáng theo mẫu giải được trong số bước hữu hạn đã biết trước. |
| F-14 | Snake | Vật phẩm phần mềm chạy Snake, chơi được cả khi bộ đọc bật và khi bộ đọc tắt trên cùng bản cài. |
| F-15 | Vòng đời/hai nhân vật | Hai nhân vật/thế giới có tri thức bản đồ và cấu hình khác nhau rõ rệt, một bản sao lưu xuất ra được, một bản lưu lỗi mẫu, và đường nâng cấp APK giữ nguyên save/config để kiểm chứng. |

### 1.4. Loại môi trường

- **host-C++** — kiểm thử ở tầng lõi/engine trên máy host: ngữ cảnh, token,
  focus, ranh giới thông tin, tọa độ, trạng thái gameplay trước/sau. Dùng
  backend lời đọc giả để so ý nghĩa sự kiện; không phụ thuộc TTS thật.
- **host-Java** — kiểm thử tầng Java/Android trên host bằng test doubles:
  hàng đợi lời, snapshot parse/validate, intent, vòng đời controller.
  Không emulation âm thanh thật và không chứng minh nghe được.
- **Android-giả-lập** — chạy trên emulator với APK thật của ứng viên: nhãn/
  nút, vòng đời Activity, recreate, process death mô phỏng, file picker.
  Emulator không có âm thanh nghe thật đáng tin; không dùng nó để kết luận
  chất lượng giọng/TalkBack.
- **máy-thật-TalkBack** — thiết bị Android thật, có TalkBack và engine/voice
  thật, nghe bằng tai. Đây là lớp duy nhất kết luận được về giọng Việt,
  audio focus, cue/rung và phối hợp TalkBack.
- **nghiệm-thu-người-dùng** — người khiếm thị mục tiêu dùng máy thật của
  mình, làm nhiệm vụ trọn vẹn, không nhận chỉ dẫn bằng mắt. Đây là lớp
  quyết định cuối cùng cho các hành trình chơi độc lập.

### 1.5. Ý nghĩa các lớp kiểm thử tự động (cập nhật theo phạm vi hiện tại)

Giữ nguyên ý nghĩa phân lớp đã nêu ở cuối tài liệu lịch sử, nhưng cập nhật
phạm vi: mã hiện tại không còn là "riêng Activity thử giọng" mà là **bộ đọc
gameplay đầy đủ** (snapshot ngữ nghĩa của các màn chơi, trung tâm lệnh, bản
đồ, hành trang, chiến đấu, xe, vùng, trại, khả năng, minigame và vòng đời
lưu/tải). Theo đó:

- **Host C++**: sở hữu ngữ cảnh/token, snapshot, ranh giới nhận biết, kết
  quả hành động và bất biến không tác dụng phụ của bộ đọc gameplay; chưa
  điều khiển game thật trên thiết bị.
- **Host Java**: token, hàng đợi lời, validate snapshot/intent và driver
  với test doubles cho toàn bộ bề mặt bộ đọc; không emulation Android/audio.
- **Compile API/Gradle**: kiểu dữ liệu, Java và resource thật của bộ đọc;
  chưa chứng minh UI chạy.
- **Emulator**: các bài về nhãn/nút, lỗi/khởi tạo, dừng lời, recreate,
  opt-in mạng, vòng đời và thoát — nay áp cho các màn của bộ đọc gameplay
  và các Activity nền (khởi động, thiết lập, cứu hộ), không chỉ Activity
  thử giọng. APK UI-only không có engine game; emulator chạy không âm
  thanh. Không coi bài emulator là kiểm thử giọng Việt nghe thật hoặc
  TalkBack.
- **APK ARM64 đầy đủ**: kiểm tra build/đóng gói và bản dịch; còn cần chạy
  trên máy.
- **Nghiệm thu người dùng**: bằng chứng độc lập, bắt buộc cho mục tiêu chơi
  trọn vẹn.

### 1.6. Hồ sơ tối thiểu của một lần chạy

Mỗi lần chạy một bài ghi: mã bài; build/commit của ứng viên (ghim ở R23);
cấu hình nội dung/mod; fixture và phiên bản fixture; thiết bị/Android/
engine/voice (với bài có thiết bị); trạng thái TalkBack và bộ đọc; thao
tác đã làm; oracle nào đạt/không đạt; kết quả thật quan sát được; bằng
chứng (snapshot, log, ảnh chụp hồ sơ, ghi âm câu đọc nếu có). Kết quả của
bài này không suy ra kết quả của bài khác.

---

## 2. Danh mục T01–T64 (từ PLAN.vi.md mục 15.4)

Danh sách dưới đây lấy nguyên 64 bài ở `PLAN.vi.md` mục 15.4: mã T01–T64,
tên bài và câu tóm tắt của từng bài được giữ đúng nguyên văn của kế hoạch
(câu tóm tắt in nghiêng ngay dưới tên bài). Phần bổ sung của R22 cho mỗi
bài — tiền điều kiện, fixture, môi trường, các bước và oracle ngắn — nằm
sau câu tóm tắt. Không bài nào bị đổi mã, gộp bài hay tách bài.

Trạng thái của cả 64 bài: **chưa chạy — chưa có PASS mới.**

### 2.1. Khởi động và cấu hình

**T01 — Cài sạch.**
*Với TalkBack đang bật, mở ứng dụng lần đầu, nghe lựa chọn, cài dữ liệu, thử giọng và tới menu. Không có hộp thoại bắt buộc không đọc được.*

- Tiền điều kiện: thiết bị chưa từng cài app; TalkBack đang bật; có voice
  Việt đã cài hoặc đường cài voice khả dụng.
- Fixture: không cần fixture — dùng bản cài sạch/mặc định.
- Môi trường: máy-thật-TalkBack.
- Các bước: (1) cài APK ứng viên và mở lần đầu; (2) đi hết luồng cài dữ
  liệu bằng TalkBack; (3) thử giọng và xác nhận nghe rõ; (4) vào menu chính.
- Oracle: app tới menu chính; mọi hộp thoại bắt buộc trên đường đều có
  nhãn và thao tác được bằng TalkBack; dữ liệu cài hoàn tất (không báo sẵn
  sàng sớm); trạng thái "đã nghe thử giọng" chỉ được lưu sau khi người
  chạy bài xác nhận đã nghe.

**T02 — Mở lại không TalkBack.**
*Từ cấu hình bộ đọc nội bộ đã lưu, cold start tới menu, nghe trợ giúp và thoát an toàn; không cần bật lại TalkBack để qua màn app.*

- Tiền điều kiện: đã hoàn tất thiết lập bộ đọc nội bộ và lưu cấu hình;
  TalkBack tắt trước khi mở app.
- Fixture: không cần fixture — dùng bản cài sạch/mặc định.
- Môi trường: máy-thật-TalkBack (thiết bị thật; TalkBack tắt có chủ ý).
- Các bước: (1) cold start app; (2) tới menu bằng bộ đọc nội bộ; (3) mở
  trợ giúp và nghe; (4) thoát an toàn.
- Oracle: toàn bộ đường đi không cần bật lại TalkBack; bộ đọc nội bộ phát
  lời ngay từ màn app đầu tiên; thoát không để lại tiến trình/hàng đợi lời
  treo.

**T03 — Thiếu tiếng Việt.**
*Máy chưa có voice Việt; tìm được hướng dẫn cài/chọn voice khác/quay lại. Không im lặng và không lặp vô hạn hộp thoại.*

- Tiền điều kiện: thiết bị không có voice Việt; ít nhất một engine TTS có
  mặt.
- Fixture: không cần fixture — dùng bản cài sạch/mặc định.
- Môi trường: máy-thật-TalkBack.
- Các bước: (1) mở thiết lập giọng; (2) đi theo hướng dẫn cài/chọn voice;
  (3) mở cài đặt TTS hệ thống rồi quay lại bằng đường của app; (4) thử lại
  hoặc thoát.
- Oracle: tại mọi trạng thái đều có ít nhất một đường nghe được (lời của
  engine còn dùng được hoặc clip cứu hộ — xem thêm TX-07); số lần hộp
  thoại lặp lại hữu hạn; quay lại được màn thiết lập mà không mất tiến
  trình thiết lập.

**T04 — TTS trả lỗi hoặc không bắt đầu.**
*Kích lỗi có chủ ý; phát hiện bằng trạng thái/timeout, không khóa game, có đường phục hồi nghe được. Nếu callback tới muộn thì không phá phiên mới.*

- Tiền điều kiện: có cơ chế kích lỗi TTS có chủ ý (engine lỗi/timeout) ở
  cả tầng host-Java và trên thiết bị.
- Fixture: không cần fixture — dùng bản cài sạch/mặc định.
- Môi trường: host-Java; máy-thật-TalkBack (xác nhận).
- Các bước: (1) kích lỗi khởi tạo/phát; (2) chờ quá thời hạn khởi tạo;
  (3) dùng đường phục hồi; (4) vào phiên mới rồi cho callback cũ tới muộn.
- Oracle: lỗi được phát hiện bằng trạng thái/timeout (không treo UI, vòng
  lặp game vẫn chạy); đường phục hồi nghe được xuất hiện; callback muộn
  bị loại theo mã phiên và không đổi focus/lời của phiên mới.

**T05 — Thiếu dung lượng/cài dữ liệu lỗi.**
*Nghe nguyên nhân, thử lại hoặc thoát; không báo "đã vào game" khi chưa sẵn sàng.*

- Tiền điều kiện: có thể làm đầy bộ nhớ hoặc phá dữ liệu cài giữa chừng;
  có bản cài ổn định trước đó để kiểm rollback.
- Fixture: không cần fixture — dùng bản cài sạch/mặc định.
- Môi trường: Android-giả-lập; máy-thật-TalkBack (xác nhận).
- Các bước: (1) cài trong điều kiện thiếu dung lượng; (2) nghe nguyên
  nhân; (3) thử lại và/hoặc thoát; (4) kill app giữa lúc cài rồi mở lại.
- Oracle: thông điệp nêu đúng nguyên nhân (thiếu dung lượng/lỗi ghi/giải
  nén); cờ "đã cài" không được ghi trước khi dữ liệu hoàn tất; dữ liệu ổn
  định cũ còn nguyên sau rollback/khôi phục.

**T06 — Dịch vụ trợ năng khác.**
*Bật một dịch vụ không đọc màn hình; không bắt tắt vô lý. Với TalkBack bật lại, người dùng vẫn điều khiển được đường phục hồi.*

- Tiền điều kiện: thiết bị có một dịch vụ trợ năng không đọc màn hình để
  bật thử; có thể bật/tắt TalkBack giữa phiên.
- Fixture: không cần fixture — dùng bản cài sạch/mặc định.
- Môi trường: máy-thật-TalkBack.
- Các bước: (1) bật dịch vụ kia và vào game; (2) kiểm tra app không yêu
  cầu tắt dịch vụ đó; (3) bật TalkBack giữa phiên chơi; (4) dùng đường
  phục hồi native.
- Oracle: không có yêu cầu tắt dịch vụ vô lý; khi TalkBack bật giữa
  phiên, cử chỉ dang dở bị hủy, dữ liệu game giữ nguyên, và chỉ còn một
  kênh đọc focus (không hai giọng đọc cùng một mục).

**T07 — Thử đổi cử chỉ.**
*Đặt cấu hình khó sử dụng rồi không xác nhận; trở lại cấu hình cũ, giữ bản lưu game.*

- Tiền điều kiện: bộ đọc đã bật; có cấu hình cử chỉ đã xác nhận trước đó;
  có bản lưu game đang chơi dở.
- Fixture: không cần fixture — dùng bản cài sạch/mặc định.
- Môi trường: host-Java; Android-giả-lập (xác nhận).
- Các bước: (1) bắt đầu thử ánh xạ cử chỉ mới (thời gian thử theo đồng hồ
  đơn điệu); (2) đặt cấu hình khó dùng; (3) để hết hạn thử mà không xác
  nhận; (4) kiểm tra cấu hình và bản lưu.
- Oracle: sau hết hạn, cấu hình điều khiển quay về đúng bản đã xác nhận
  trước đó; bản lưu game không bị đụng; không ý định gameplay nào phát
  sinh từ bài luyện/thử.

**T08 — Hồ sơ realtime cũ.**
*Tải cấu hình `TURN_DURATION>0`, nghe cảnh báo và chọn cách chơi trước khi vào nguy hiểm. Với cấu hình theo lượt, nghe lâu không tự tạo lượt.*

- Tiền điều kiện: có hồ sơ cấu hình cũ với `TURN_DURATION > 0` và một hồ
  sơ theo lượt (`TURN_DURATION = 0`) để đối chiếu.
- Fixture: không cần fixture — dùng bản cài sạch/mặc định.
- Môi trường: host-C++; Android-giả-lập (xác nhận).
- Các bước: (1) tải hồ sơ realtime; (2) nghe cảnh báo và chọn giữ
  realtime / chuyển về theo lượt / thoát; (3) với hồ sơ theo lượt, mở một
  nội dung đọc dài và chờ lâu hơn nhiều nhịp TTS.
- Oracle: cảnh báo xuất hiện trước khi vào thế giới ở mọi đường vào qua
  menu; lựa chọn được lưu đúng ý người dùng (không tự đổi luật chơi âm
  thầm); ở cấu hình theo lượt, thời gian/moves/RNG của game không đổi chỉ
  vì đang nghe đọc.

### 2.2. Ngữ cảnh, menu và văn bản

**T09 — Menu lồng.**
*Mở hành trang → hành động → số lượng → xác nhận rồi hủy từng tầng. Không có bước chân hoặc tấn công phát sinh.*

- Tiền điều kiện: bộ đọc bật; nhân vật đứng yên ở ô an toàn.
- Fixture: F-02 cây túi.
- Môi trường: host-C++; Android-giả-lập (xác nhận).
- Các bước: (1) mở hành trang; (2) mở hành động của một vật phẩm; (3) mở
  nhập số lượng; (4) hủy từng tầng trở ra; (5) lặp lại tới tầng xác nhận
  rồi hủy ở tầng xác nhận.
- Oracle: mỗi lần hủy chỉ đóng đúng một tầng và focus trở về mục còn hợp
  lệ của tầng dưới; vị trí nhân vật, moves và trạng thái mục tiêu không
  đổi — không bước chân/tấn công nào phát sinh.

**T10 — Cử chỉ bị hủy.**
*Vuốt dang dở, thêm ngón, xoay máy hoặc rời app; không sinh tap/di chuyển muộn.*

- Tiền điều kiện: bộ đọc bật ở ngữ cảnh thế giới (cấu hình nhanh) và ở
  menu.
- Fixture: F-01 cặp tri thức.
- Môi trường: host-Java; Android-giả-lập (xác nhận).
- Các bước: (1) vuốt dang dở rồi nhấc tay lệch hướng; (2) thêm ngón giữa
  cử chỉ; (3) xoay máy giữa cử chỉ; (4) rời app giữa cử chỉ rồi quay lại.
- Oracle: không trường hợp nào sinh ý định di chuyển/tap muộn; nhân vật
  giữ nguyên vị trí và trạng thái; cử chỉ mới sau đó hoạt động bình thường.

**T11 — Đầu vào kép.**
*Thực hiện 100 chuỗi chạm có định danh; mỗi chuỗi sinh đúng số ý định được thiết kế, không đồng thời đi qua joystick cũ và bộ đọc.*

- Tiền điều kiện: có harness gửi 100 chuỗi chạm có định danh ở tầng
  host-Java.
- Fixture: F-01 cặp tri thức.
- Môi trường: host-Java.
- Các bước: (1) gửi 100 chuỗi chạm hợp lệ theo thiết kế bước đơn; (2)
  đếm ý định sinh ra theo từng chuỗi; (3) kiểm tra đường joystick cũ.
- Oracle: mỗi chuỗi sinh đúng số ý định được thiết kế (không thừa, không
  thiếu); không chuỗi nào đi đồng thời qua cả joystick cũ và bộ đọc.

**T12 — Focus cũ.**
*Đóng menu rồi gửi callback/chọn mục cũ; lệnh bị loại. Menu mới giữ focus hợp lệ.*

- Tiền điều kiện: có thể đóng một menu rồi phát lại callback/chọn mục của
  menu đã đóng (token/generation cũ).
- Fixture: không cần fixture — dùng bản cài sạch/mặc định.
- Môi trường: host-C++.
- Các bước: (1) mở menu A, ghi token/focus; (2) đóng A, mở menu B; (3)
  gửi callback và intent chọn mục mang token của A; (4) kiểm tra B.
- Oracle: mọi intent mang token cũ đều bị loại và không thực thi lệnh
  nào; menu B giữ focus hợp lệ và hoạt động tiếp bình thường.

**T13 — Danh sách lớn.**
*Duyệt tới mục ngoài phần ImGui đang vẽ, tìm kiếm và mở đúng mục; số thứ tự/tổng đúng theo tập lọc.*

- Tiền điều kiện: bộ đọc bật; danh sách có nhiều mục hơn phần ImGui vẽ.
- Fixture: F-02 cây túi.
- Môi trường: host-C++.
- Các bước: (1) mở danh sách; (2) duyệt tới một mục nằm ngoài phần đang
  vẽ; (3) dùng tìm kiếm/lọc để thu hẹp; (4) mở mục đã tìm.
- Oracle: cây ngữ nghĩa chứa toàn bộ mô hình dữ liệu (không chỉ phần
  được vẽ); vị trí/tổng công bố khớp tập lọc hiện hành; mục được mở là
  đúng đối tượng logic đã chọn (đúng `retval`), không phải mục ở cùng vị
  trí hiển thị.

**T14 — Danh sách rỗng/disabled.**
*Lọc không còn kết quả, nghe cách xóa lọc; mục không dùng được có lý do và không kích hoạt nhầm mục khác.*

- Tiền điều kiện: bộ đọc bật; có bộ lọc áp được tới rỗng và có mục bị vô
  hiệu kèm lý do.
- Fixture: F-02 cây túi.
- Môi trường: host-C++.
- Các bước: (1) lọc tới khi danh sách rỗng; (2) nghe hướng dẫn xóa lọc và
  xóa lọc; (3) duyệt tới mục disabled và thử kích hoạt.
- Oracle: trạng thái rỗng được công bố kèm đường xóa lọc, và xóa lọc khôi
  phục đúng danh sách gốc; mục disabled công bố lý do, kích hoạt bị engine
  từ chối và không mục nào khác bị kích hoạt thay.

**T15 — Hai nhánh UI.**
*Lặp chuỗi menu với `ANDROID_NATIVE_UI` bật/tắt theo chế độ sản phẩm hỗ trợ; không nhánh trả về sớm thiếu lời đọc hoặc đường hủy.*

- Tiền điều kiện: bản chạy được cả hai chế độ `ANDROID_NATIVE_UI` bật/tắt
  theo cấu hình sản phẩm hỗ trợ.
- Fixture: không cần fixture — dùng bản cài sạch/mặc định.
- Môi trường: host-C++; Android-giả-lập (xác nhận).
- Các bước: (1) chạy một chuỗi menu chuẩn (mở → chọn → hộp thoại → hủy)
  với nhánh native UI; (2) chạy đúng chuỗi đó với nhánh còn lại; (3) so
  kết quả hai lần chạy.
- Oracle: cả hai nhánh đều công bố đủ lời đọc và đường hủy ở mọi bước
  (không nhánh nào trả về sớm làm mất lời đọc); kết quả gameplay của hai
  lần chạy giống nhau.

**T16 — Đọc dài bị ngắt.**
*Đang nghe mô tả thì có cảnh báo; nghe cảnh báo rồi chủ động tiếp đoạn còn lại, không lặp từ đầu và không đọc tài liệu của màn đã đóng.*

- Tiền điều kiện: có nội dung dài đọc theo đoạn (tài liệu/văn bản terminal)
  và có thể kích một cảnh báo giữa chừng.
- Fixture: F-09 khả năng (văn bản terminal dài của máy tính trong game).
- Môi trường: host-C++; host-Java (xác nhận hàng đợi).
- Các bước: (1) bắt đầu đọc dài; (2) kích cảnh báo giữa đoạn; (3) nghe
  cảnh báo rồi ra lệnh đọc tiếp; (4) đóng màn rồi thử đọc tiếp lần nữa.
- Oracle: cảnh báo được phát trước nội dung nền; đọc tiếp bắt đầu từ đoạn
  còn dở (không lặp từ đầu); sau khi màn đóng, không nội dung nào của màn
  đó còn được phát hay đọc tiếp.

**T17 — Nhập tên tiếng Việt.**
*Nhập/sửa/xóa "Nguyễn Đặng", hoa/thường, dấu tổ hợp và ký tự ngoài BMP thử nghiệm. Không hỏng encoding hoặc xóa nửa ký tự.*

- Tiền điều kiện: bộ đọc bật; mở được trường nhập tên (tên nhân vật/thế
  giới hoặc ô nhập tương đương).
- Fixture: không cần fixture — dùng bản cài sạch/mặc định.
- Môi trường: host-C++ (grapheme/encoding); host-Java (trình soạn).
- Các bước: (1) nhập "Nguyễn Đặng"; (2) đổi hoa/thường; (3) nhập một dấu
  tổ hợp và một ký tự ngoài BMP thử nghiệm; (4) xóa lùi qua các ký tự đó;
  (5) áp dụng và đọc lại giá trị đã lưu.
- Oracle: chuỗi lưu trong game khớp đúng grapheme đã nhập (không hỏng
  encoding UTF-8/UTF-16, không mất dấu); mỗi lần xóa bỏ đúng một grapheme
  (không nửa ký tự, không nửa surrogate).

**T18 — Nhập số/tìm kiếm.**
*Chọn một phần stack, nhập số không hợp lệ rồi sửa; dùng cú pháp tìm kiếm của game và hủy để phục hồi nội dung gốc.*

- Tiền điều kiện: bộ đọc bật; có stack để chọn một phần.
- Fixture: F-02 cây túi.
- Môi trường: host-C++; host-Java (xác nhận editor).
- Các bước: (1) chọn một phần stack qua trường số; (2) nhập số không hợp
  lệ (ngoài miền/sai định dạng) rồi sửa lại; (3) tìm bằng cú pháp tìm
  kiếm của game; (4) hủy nhập để về nội dung gốc.
- Oracle: giá trị ngoài miền bị từ chối tại trường kèm lỗi sửa được và
  không áp dụng một phần; số hợp lệ áp dụng đúng số lượng đã chọn; hủy
  khôi phục đúng nội dung/số lượng gốc.

### 2.3. Thế giới và bản đồ

**T19 — Tám hướng/tầng.**
*Di chuyển đủ tám hướng, lên/xuống cầu thang, đổi tư thế. Hướng đọc khớp tọa độ game và không đổi theo xoay màn hình.*

- Tiền điều kiện: bộ đọc bật; nhân vật ở khu vực đi được đủ tám hướng và
  có cầu thang.
- Fixture: F-01 cặp tri thức.
- Môi trường: host-C++; Android-giả-lập (xác nhận xoay máy).
- Các bước: (1) đi lần lượt đủ tám hướng; (2) lên/xuống cầu thang; (3)
  đổi tư thế; (4) xoay máy rồi đi lại một hướng.
- Oracle: mỗi bước làm tọa độ nhân vật đổi đúng hướng bản đồ đã ra lệnh
  (không đổi theo hướng màn hình/xoay máy); đổi tầng đổi đúng z; tư thế
  sau đổi khớp trạng thái engine.

**T20 — Kết quả bước đi.**
*Thử đi thành công, mở cửa, đánh sinh vật, bị safe mode chặn, vướng xe/đồ đạc. Phản hồi mô tả đúng kết quả, không mặc định "tường".*

- Tiền điều kiện: bộ đọc bật; bố trí sẵn đủ năm tình huống kết quả.
- Fixture: F-01 cặp tri thức.
- Môi trường: host-C++.
- Các bước: (1) đi vào ô trống; (2) đi vào cửa đóng; (3) đi vào ô có sinh
  vật; (4) đi khi safe mode chặn; (5) đi vào ô vướng xe/đồ đạc.
- Oracle: kết quả công bố cho từng bước khớp kết quả thật tại điểm engine
  xử lý (đã đi / đã mở cửa / đã tấn công thay vì đi / bị safe mode chặn /
  bị vướng đối tượng cụ thể); không trường hợp nào bị gộp thành "tường"
  mặc định; vị trí sau bước khớp loại kết quả.

**T21 — Quét không tiêu lượt.**
*Chụp trạng thái thời gian/moves/RNG trước và sau 100 truy vấn/thăm dò. Không có thay đổi gameplay do truy vấn.*

- Tiền điều kiện: chụp được trạng thái thời gian/moves/RNG trước truy vấn.
- Fixture: F-01 cặp tri thức.
- Môi trường: host-C++.
- Các bước: (1) chụp trạng thái; (2) thực hiện 100 truy vấn quét/thăm dò/
  đọc ô; (3) chụp lại trạng thái và so sánh.
- Oracle: thời gian game, moves, vị trí và trạng thái RNG giống hệt trước
  và sau; mọi truy vấn đều thuần túy quan sát.

**T22 — Quái sau tường.**
*Một quái thấy được, một quái không biết sau tường; chỉ công bố thông tin được phép, không khác qua cue/rung.*

- Tiền điều kiện: bố trí một quái nhìn thấy và một quái sau tường ngoài
  quyền biết.
- Fixture: F-01 cặp tri thức.
- Môi trường: host-C++.
- Các bước: (1) đọc ô xung quanh và quét; (2) nghe cue/rung trong cùng
  tình huống; (3) đổi dữ liệu bí mật của quái sau tường (bản đối tác của
  cặp tri thức) và lặp lại.
- Oracle: snapshot, câu đọc, cue và rung chỉ chứa thông tin của quái
  được phép biết; hai bản có bí mật khác nhau cho ra công bố giống nhau
  ở các trường nhân vật không phân biệt được.

**T23 — Âm không xác định.**
*Nghe nguồn âm mà không thấy; chỉ đọc thông tin hợp lệ từ perception, không tiết lộ tên thực thể hoặc khoảng cách chính xác hơn game cho biết.*

- Tiền điều kiện: có nguồn âm mà nhân vật không nhìn thấy.
- Fixture: F-01 cặp tri thức.
- Môi trường: host-C++.
- Các bước: (1) đứng ở vị trí chỉ nghe được âm; (2) đọc thông tin âm qua
  bộ đọc; (3) so với dữ kiện thật của nguồn âm.
- Oracle: công bố chỉ gồm mô tả âm, hướng và độ chính xác mà giao diện
  hợp lệ của game cho biết; tên thực thể và tọa độ/khoảng cách chính xác
  hơn quyền nghe không xuất hiện trong snapshot, câu đọc, cue hay lịch sử.

**T24 — Bẫy/đồ chưa biết.**
*Bẫy ẩn, container đóng và vật phẩm chưa nhận dạng không bị lộ; khi phát hiện hợp lệ thì thông tin cập nhật.*

- Tiền điều kiện: bố trí bẫy ẩn, container đóng và vật phẩm chưa nhận
  dạng trong tầm quan sát.
- Fixture: F-01 cặp tri thức.
- Môi trường: host-C++.
- Các bước: (1) quét/đọc các ô liên quan trước khi phát hiện; (2) thực
  hiện hành động phát hiện hợp lệ của game; (3) đọc lại.
- Oracle: trước phát hiện, không nguồn nào (snapshot/câu đọc/cue/lịch
  sử) chứa dữ kiện ẩn; sau phát hiện hợp lệ, đúng thông tin vừa được phép
  biết xuất hiện và thông tin vẫn bị che ở các ô chưa phát hiện giữ nguyên.

**T25 — Ghi nhớ thay đổi bí mật.**
*Nhìn một cửa rồi rời tầm nhìn, thay cửa bí mật trong tình huống thử. Bộ đọc vẫn đọc dữ liệu nhớ, gắn nguồn, không trạng thái thật mới.*

- Tiền điều kiện: nhân vật đã nhìn thấy một cửa rồi rời khỏi tầm nhìn;
  harness đổi trạng thái thật của cửa ngoài tầm nhìn.
- Fixture: F-01 cặp tri thức.
- Môi trường: host-C++.
- Các bước: (1) nhìn cửa và ghi trạng thái đã nhớ; (2) rời tầm nhìn; (3)
  đổi trạng thái cửa ở phía engine; (4) hỏi lại ô đó qua bộ đọc.
- Oracle: bộ đọc vẫn công bố dữ liệu đã ghi nhớ, gắn nguồn "đã ghi nhớ",
  và không chứa trạng thái thật mới; dữ liệu nhớ chỉ đổi khi nhân vật
  quan sát lại bằng đường hợp lệ.

**T26 — Xóa màn hình.**
*Redraw, resize, đổi tiles/ASCII hoặc đóng/mở màn; quét vẫn giữ tri thức đúng. Đây là hồi quy trực tiếp từ bản tham khảo GnollHack.*

- Tiền điều kiện: nhân vật đã có tri thức bản đồ (ô đã thấy/đã nhớ).
- Fixture: F-01 cặp tri thức.
- Môi trường: host-C++.
- Các bước: (1) ghi kết quả quét hiện tại; (2) redraw, resize, đổi
  tiles/ASCII, đóng/mở màn; (3) quét lại và so.
- Oracle: kết quả quét sau các thao tác trình bày giống kết quả trước ở
  mọi trường tri thức; không ô đã nhớ nào biến mất chỉ vì làm sạch phần
  trình bày.

**T27 — Đổi reality bubble/tầng.**
*Đi qua ranh giới tải bản đồ, đổi tầng rồi quay lại; không nhầm tọa độ hoặc xóa bộ nhớ hợp lệ.*

- Tiền điều kiện: nhân vật gần ranh giới reality bubble và có tầng kề đã
  từng tới.
- Fixture: F-01 cặp tri thức.
- Môi trường: host-C++.
- Các bước: (1) đi qua ranh giới tải bản đồ; (2) đổi tầng; (3) quay lại
  tầng cũ; (4) đối chiếu tọa độ và tri thức từng ô.
- Oracle: tọa độ tuyệt đối của các ô đã biết giữ đúng sau khi bubble
  trượt và sau khi đổi tầng đi/về; bộ nhớ hợp lệ của các tầng không bị
  xóa hay ghép nhầm sang ô khác.

**T28 — Đổi nhân vật/thế giới.**
*Tải hai nhân vật có tri thức khác nhau; không còn mục tiêu/đồ/địa hình của nhân vật trước.*

- Tiền điều kiện: có hai nhân vật/thế giới với tri thức khác nhau rõ rệt.
- Fixture: F-15 vòng đời/hai nhân vật.
- Môi trường: host-C++; host-Java (xác nhận thay phiên).
- Các bước: (1) chơi ở nhân vật A và ghi các mục tiêu/đồ/địa hình đang
  công bố; (2) tải nhân vật B; (3) kiểm tra toàn bộ bề mặt công bố.
- Oracle: sau khi tải B, không mục tiêu, vật phẩm, địa hình hay lời/lệnh
  treo nào của A còn xuất hiện; mọi công bố thuộc đúng tri thức của B.

**T29 — Thăm dò và bộ lọc.**
*Di chuyển con trỏ, nhảy tới cửa/đồ đã biết, recenter và thoát; nhân vật không đi theo con trỏ.*

- Tiền điều kiện: bộ đọc bật; có cửa/vật phẩm đã biết để nhảy tới.
- Fixture: F-01 cặp tri thức.
- Môi trường: host-C++.
- Các bước: (1) vào chế độ thăm dò; (2) di chuyển con trỏ nhiều hướng/
  tầng; (3) nhảy tới cửa/đồ đã biết; (4) recenter; (5) thoát thăm dò.
- Oracle: vị trí, moves và thời gian của nhân vật không đổi trong suốt
  bài; con trỏ chỉ tới được đối tượng đã biết; thoát trả camera/focus về
  đúng nhân vật.

**T30 — Rò gián tiếp.**
*Hai bản đồ cùng tri thức nhưng bí mật khác nhau; truy vấn quét/tổng hợp/tìm đường không phân biệt bí mật bằng nội dung, số đếm hoặc dấu hiệu phụ vượt UI gốc.*

- Tiền điều kiện: hai bản đồ cùng phần được biết, khác phần bí mật.
- Fixture: F-01 cặp tri thức.
- Môi trường: host-C++.
- Các bước: (1) chạy cùng một bộ truy vấn quét/tổng hợp/tìm đường trên
  cả hai bản; (2) so toàn bộ kết quả công bố và các dấu hiệu phụ (số
  đếm, thứ tự, có/không có đường).
- Oracle: ở mọi trường UI gốc không phân biệt được, kết quả của bộ đọc
  trên hai bản giống nhau; không kênh phụ nào (số lượng mục, thời gian
  phản hồi được công bố, cue) tách được bí mật.


### 2.4. Đồ đạc và sinh tồn

**T31 — Chọn nhiều/số lượng.**
*Nhặt một phần stack, bỏ chọn, đổi số lượng, xác nhận; số lượng và tổng đúng, không chọn nhầm do filter.*

- Tiền điều kiện: bộ đọc bật; có stack đủ lớn và bộ lọc đang áp.
- Fixture: F-02 cây túi.
- Môi trường: host-C++.
- Các bước: (1) chọn một phần stack bằng số lượng; (2) bỏ chọn một mục;
  (3) đổi số lượng; (4) xác nhận; (5) đối chiếu kho sau thao tác.
- Oracle: số lượng nhặt/để lại và tổng sau xác nhận khớp đúng các lựa
  chọn đã chốt; bộ lọc không làm chọn nhầm đối tượng khác ở cùng vị trí
  hiển thị.

**T32 — Túi lồng.**
*Cất/lấy ở các pockets, xử lý sức chứa và container không hợp lệ; nghe rõ nguồn/đích và lý do từ chối.*

- Tiền điều kiện: bộ đọc bật; có pockets lồng nhau và một container không
  hợp lệ cho vật phẩm định cất.
- Fixture: F-02 cây túi.
- Môi trường: host-C++.
- Các bước: (1) cất vật phẩm vào pocket sâu; (2) lấy ra; (3) cất vượt sức
  chứa; (4) cất vào container không hợp lệ.
- Oracle: vật phẩm tới đúng pocket đã chọn và lấy ra đúng vật phẩm đó;
  hai trường hợp bị từ chối nêu đúng nguồn/đích và lý do của engine, và
  vị trí vật phẩm không đổi sau từ chối.

**T33 — Vật phẩm biến mất.**
*Dùng hết một món rồi danh sách cập nhật; focus chuyển hợp lý, không kích hoạt món khác qua callback cũ.*

- Tiền điều kiện: bộ đọc bật; có vật phẩm dùng một lần sẽ biến mất sau
  khi dùng.
- Fixture: F-02 cây túi.
- Môi trường: host-C++.
- Các bước: (1) dùng hết vật phẩm đang focus; (2) kiểm tra danh sách và
  focus mới; (3) phát lại callback/intent của vật phẩm đã biến mất.
- Oracle: vật phẩm biến mất khỏi danh sách ngay sau khi dùng hết; focus
  chuyển về mục hợp lệ gần nhất kèm lý do; intent cũ bị từ chối và không
  kích hoạt món khác.

**T34 — Advanced inventory.**
*Chuyển đồ giữa hai bên, đổi nguồn/đích, lọc, chọn nhiều và hủy; không nhầm cột hoặc tổng khối lượng/thể tích.*

- Tiền điều kiện: bộ đọc bật; bố cục hai phía khác nhau rõ rệt.
- Fixture: F-03 kho hai phía.
- Môi trường: host-C++.
- Các bước: (1) chuyển một mục từ trái sang phải; (2) đổi nguồn/đích;
  (3) lọc một phía; (4) chọn nhiều và chuyển; (5) hủy giữa chừng một lần
  chuyển.
- Oracle: mọi vật phẩm tới đúng phía đã chọn; tổng khối lượng/thể tích
  công bố của hai phía khớp kho thật sau từng bước; lần hủy không chuyển
  vật phẩm nào.

**T35 — Ăn/uống/chất lỏng.**
*Chọn vật chứa, đổ/chuyển/dùng; nghe cảnh báo mà game cho biết và xử lý đúng toàn bộ hộp thoại.*

- Tiền điều kiện: bộ đọc bật; có thức ăn/đồ uống và chất lỏng trong vật
  chứa, kèm một trường hợp có cảnh báo của game.
- Fixture: F-02 cây túi.
- Môi trường: host-C++.
- Các bước: (1) ăn/uống một món; (2) đổ/chuyển chất lỏng giữa hai vật
  chứa; (3) dùng món có cảnh báo và đi hết hộp thoại của nó.
- Oracle: lượng chất lỏng và tình trạng vật chứa sau thao tác khớp lượng
  đã chọn; cảnh báo game cho biết được công bố trước khi hành động có
  hiệu lực; mọi hộp thoại trên đường đều xử lý được tới kết quả cuối.

**T36 — Băng bó/thuốc.**
*Chọn bộ phận cơ thể và vật tư, nghe hiệu quả/điều kiện hợp lệ; không dựa vào màu thương tích.*

- Tiền điều kiện: bộ đọc bật; nhân vật có thương tích ở nhiều bộ phận.
- Fixture: F-04 vết thương.
- Môi trường: host-C++; Android-giả-lập (xác nhận bảng Medical).
- Các bước: (1) mở bảng tình trạng các bộ phận; (2) chọn bộ phận và vật
  tư; (3) nghe hiệu quả/điều kiện; (4) thực hiện điều trị.
- Oracle: tình trạng từng bộ phận công bố bằng lời khớp dữ kiện y tế
  được phép biết (không suy ra từ màu, không lộ số HP chính xác ngoài
  chính sách hiện hành); bộ phận/vật tư đã chọn là đúng cặp được điều
  trị và trạng thái sau điều trị khớp kết quả engine.

**T37 — Ngủ/đợi/hoạt động.**
*Bắt đầu, nghe tiến độ có tiết chế, hủy hoặc bị game ngắt; báo việc hủy đã thực hiện, không chỉ dừng tiếng.*

- Tiền điều kiện: bộ đọc bật; có hoạt động dài bắt đầu được (ngủ/đợi/
  điều trị/đọc).
- Fixture: F-04 vết thương.
- Môi trường: host-C++.
- Các bước: (1) bắt đầu hoạt động; (2) nghe tiến độ; (3) hủy giữa chừng;
  (4) bắt đầu lại và để game tự ngắt bằng sự kiện.
- Oracle: tiến độ công bố có tiết chế (không phát mọi vòng lặp); khi
  hủy, hoạt động thật trong engine dừng — trạng thái nhân vật/thời gian
  dừng ở điểm hủy — chứ không chỉ dừng tiếng; khi game ngắt, lý do ngắt
  của engine được công bố.

**T38 — Trạng thái và lịch sử.**
*Vượt ngưỡng đói/khát/đau/nhiệt/sức bền, kiểm tra có và hết hiệu ứng; đọc lại thông báo đúng thứ tự không đọc kép khi redraw.*

- Tiền điều kiện: bộ đọc bật; có thể đẩy trạng thái qua ngưỡng và trở lại.
- Fixture: F-04 vết thương.
- Môi trường: host-C++.
- Các bước: (1) vượt lần lượt các ngưỡng đói/khát/đau/nhiệt/sức bền; (2)
  kiểm tra cảnh báo khi có và khi hết hiệu ứng; (3) redraw màn; (4) mở
  lịch sử thông báo và đọc lại.
- Oracle: mỗi lần vượt ngưỡng có đúng một cảnh báo tương ứng và cảnh báo
  hết hiệu ứng khớp trạng thái thật; redraw không sinh thông báo mới;
  lịch sử đọc lại đúng thứ tự, không mục nào bị đọc kép (xem thêm TX-26
  và TX-28).

### 2.5. Chiến đấu, chế tạo và công trình

**T39 — Cận chiến.**
*Chọn địch/kiểu đánh, tấn công và rút lui; nghe đủ diễn biến mà nhân vật được biết, không lộ HP bí mật.*

- Tiền điều kiện: bộ đọc bật; có địch trong tầm cận chiến.
- Fixture: F-05 trường chiến đấu.
- Môi trường: host-C++.
- Các bước: (1) chọn địch và kiểu đánh; (2) tấn công; (3) rút lui.
- Oracle: đòn đánh áp lên đúng địch đã chọn với chi phí engine quyết
  định; diễn biến công bố gồm đủ phần nhân vật được biết và không chứa
  HP bí mật của địch; rút lui đổi vị trí đúng hướng đã chọn.

**T40 — Duyệt mục tiêu.**
*Đổi mục tiêu, chọn ô trống, đổi tầng, hủy; số đạn và lượt không đổi chỉ vì duyệt.*

- Tiền điều kiện: bộ đọc bật; có vũ khí đã nạp và nhiều mục tiêu/ô trống/
  tầng để duyệt.
- Fixture: F-05 trường chiến đấu.
- Môi trường: host-C++.
- Các bước: (1) vào nhắm mục tiêu; (2) đổi mục tiêu; (3) chọn ô trống;
  (4) đổi tầng; (5) hủy nhắm.
- Oracle: trong suốt quá trình duyệt, số đạn, moves và thời gian không
  đổi; hủy không bắn/ném gì; mục tiêu công bố chỉ gồm đối tượng được
  phép biết ở tầng đang xét.

**T41 — Aim/fire/reload.**
*Chọn fire mode và đạn, nhắm rồi bắn, nạp sai loại và hủy; kết quả/chi phí do engine quyết định và đọc rõ.*

- Tiền điều kiện: bộ đọc bật; súng có nhiều fire mode và hai loại đạn
  (một loại sai).
- Fixture: F-05 trường chiến đấu.
- Môi trường: host-C++.
- Các bước: (1) chọn fire mode và đạn; (2) nhắm và bắn; (3) thử nạp loại
  đạn sai; (4) bắt đầu nạp đúng rồi hủy giữa chừng nếu engine cho hủy.
- Oracle: phát bắn tiêu đúng số đạn của fire mode đã chọn và kết quả do
  engine quyết định được công bố; nạp sai loại bị từ chối với lý do của
  engine và số đạn không đổi; hủy không làm mất đạn đã nạp đúng trước đó.

**T42 — LOS/range/vùng ảnh hưởng.**
*Thử mục tiêu ngoài tầm, bị che, ném và vùng tác dụng; engine từ chối đúng, cảnh báo không tiết lộ thực thể ẩn.*

- Tiền điều kiện: bộ đọc bật; bố trí mục tiêu ngoài tầm, mục tiêu bị che
  và một vật ném có vùng tác dụng.
- Fixture: F-05 trường chiến đấu.
- Môi trường: host-C++.
- Các bước: (1) nhắm mục tiêu ngoài tầm; (2) nhắm mục tiêu bị che LOS;
  (3) ném vật có vùng ảnh hưởng và nghe mô tả vùng; (4) kiểm tra cảnh báo
  đồng minh trong vùng.
- Oracle: engine từ chối đúng cả hai trường hợp không hợp lệ và không
  phát bắn/ném nào xảy ra; mô tả vùng tác dụng và cảnh báo chỉ chứa thực
  thể nhân vật được phép biết — thực thể ẩn không bị tiết lộ qua cảnh báo.

**T43 — Công thức thiếu/đủ.**
*Nghe yêu cầu, đổi nguyên liệu thay thế và batch; chế tạo khi đủ và giải thích thiếu khi chưa đủ.*

- Tiền điều kiện: bộ đọc bật; có công thức thiếu nguyên liệu, rồi đủ sau
  khi bổ sung, có nguyên liệu thay thế và chế tạo theo batch.
- Fixture: F-03 kho hai phía (nguyên liệu đặt ở cả hai phía kho).
- Môi trường: host-C++.
- Các bước: (1) mở công thức khi thiếu và nghe yêu cầu; (2) bổ sung
  nguyên liệu; (3) đổi sang nguyên liệu thay thế; (4) chế tạo một chiếc
  và một batch.
- Oracle: khi thiếu, giải thích nêu đúng thành phần thiếu và không chế
  tạo nào bắt đầu; khi đủ, sản phẩm/batch tạo ra khớp số lượng đã chọn
  và nguyên liệu bị trừ đúng loại đã chốt (kể cả nguyên liệu thay thế).

**T44 — Sửa/xây/tháo.**
*Chọn vị trí/hướng, xử lý ô không hợp lệ, bắt đầu/hủy/hoàn thành; hình preview có mô tả nghe tương đương.*

- Tiền điều kiện: bộ đọc bật; có công trình để xây/sửa/tháo và một ô
  không hợp lệ để thử.
- Fixture: F-07 vùng/quy tắc.
- Môi trường: host-C++.
- Các bước: (1) chọn vị trí/hướng xây; (2) nghe mô tả preview; (3) thử ô
  không hợp lệ; (4) bắt đầu rồi hủy một công trình; (5) hoàn thành một
  công trình khác.
- Oracle: mô tả nghe của preview khớp vị trí/hướng/loại công trình sẽ
  dựng; ô không hợp lệ bị từ chối với lý do của engine và không vật liệu
  nào bị trừ; hủy giữ trạng thái công trình dở dang đúng thực tế; hoàn
  thành tạo đúng công trình đã chọn.

### 2.6. Hệ thống nâng cao

**T45 — Overmap và travel.**
*Chọn địa điểm đã biết, đổi tầng, nghe tuyến, xác nhận/hủy; tự đi giữ dừng nguy hiểm của engine.*

- Tiền điều kiện: bộ đọc bật; nhân vật biết ít nhất một địa điểm trên
  overmap đủ xa để có tuyến.
- Fixture: F-01 cặp tri thức.
- Môi trường: host-C++.
- Các bước: (1) mở overmap; (2) chọn địa điểm đã biết; (3) đổi tầng; (4)
  nghe tuyến; (5) hủy một lần và xác nhận tự đi một lần khác; (6) gặp
  nguy hiểm trên đường tự đi.
- Oracle: tuyến công bố chỉ dùng tri thức overmap nhân vật đã có (độ dài/
  điểm chuyển khớp tuyến engine tính); hủy không di chuyển nào xảy ra;
  tự đi dừng đúng cơ chế dừng nguy hiểm của engine.

**T46 — Ghi chú/nhiệm vụ.**
*Tạo/sửa/xóa ghi chú; nhận/theo dõi/nộp nhiệm vụ và đọc hạn/tiến độ theo thông tin được cấp.*

- Tiền điều kiện: bộ đọc bật; có nhiệm vụ nhận được và overmap để ghi chú.
- Fixture: F-08 trại.
- Môi trường: host-C++.
- Các bước: (1) tạo, sửa rồi xóa một ghi chú; (2) nhận một nhiệm vụ; (3)
  theo dõi và đọc hạn/tiến độ; (4) nộp nhiệm vụ.
- Oracle: ghi chú sau mỗi thao tác khớp nội dung đã nhập và biến mất sau
  khi xóa; trạng thái nhiệm vụ (nhận/đang theo dõi/đã nộp) và hạn/tiến độ
  công bố khớp dữ kiện game cấp, không thêm thông tin ngoài quyền biết.

**T47 — Hội thoại/thương mại.**
*Đọc người nói, lựa chọn, lịch sử, giá trị hai bên, xác nhận mua/bán và hủy; không chọn nhầm câu khi nội dung đổi.*

- Tiền điều kiện: bộ đọc bật; có NPC để hội thoại và giao dịch hai phía.
- Fixture: F-08 trại.
- Môi trường: host-C++.
- Các bước: (1) mở hội thoại, đọc người nói và các lựa chọn; (2) chọn
  một câu; (3) vào giao dịch, nghe giá trị hai bên; (4) xác nhận một
  mua/bán; (5) hủy một giao dịch khác.
- Oracle: câu được chọn là đúng câu người dùng chốt kể cả khi nội dung
  danh sách đổi giữa chừng; giá trị hai bên công bố khớp cân bằng giao
  dịch thật; xác nhận chuyển đúng vật phẩm/tiền, hủy không chuyển gì.

**T48 — Đồng đội/trại.**
*Chọn người, công việc, nguồn lực, thời gian và kết quả; xử lý từ chối hoặc thiếu điều kiện.*

- Tiền điều kiện: bộ đọc bật; có trại và đồng đội (chi tiết trọn luồng
  trại ở TX-22 và TX-23).
- Fixture: F-08 trại.
- Môi trường: host-C++; Android-giả-lập (xác nhận).
- Các bước: (1) chọn một đồng đội; (2) giao một công việc với nguồn lực
  và thời gian cụ thể; (3) nghe kết quả; (4) thử giao việc khi thiếu
  điều kiện.
- Oracle: công việc được ghi cho đúng người với đúng nguồn lực/thời
  gian đã chọn và kết quả công bố khớp trạng thái trại sau việc; trường
  hợp thiếu điều kiện bị từ chối với lý do, không việc nào được ghi.

**T49 — Sơ đồ xe.**
*Tìm bộ phận theo ô/vị trí, nghe tình trạng và phụ thuộc lắp/sửa; hoàn thành sửa/lắp mẫu mà không nhìn sơ đồ.*

- Tiền điều kiện: bộ đọc bật; xe có nhiều bộ phận, có bộ phận hỏng.
- Fixture: F-06 xe.
- Môi trường: host-C++.
- Các bước: (1) mở sơ đồ xe; (2) tìm một bộ phận theo ô/vị trí; (3) nghe
  tình trạng và phụ thuộc lắp/sửa; (4) hoàn thành một sửa/lắp mẫu.
- Oracle: bộ phận tìm được là đúng bộ phận ở ô/vị trí đã hỏi; tình trạng
  công bố khớp trạng thái thật của bộ phận; sửa/lắp hoàn thành trên đúng
  bộ phận đó và xe cập nhật đúng (bộ phận hết hỏng/đã lắp).

**T50 — Lái xe.**
*Phân biệt đi bộ/lái, hướng xe/bản đồ, tăng/giảm tốc, dừng, lên/xuống và mở cargo; đổi chế độ không gây tăng ga ngoài ý muốn.*

- Tiền điều kiện: bộ đọc bật; xe lái được trong khu kín (chi tiết smart
  controller ở TX-10).
- Fixture: F-06 xe.
- Môi trường: host-C++; Android-giả-lập (xác nhận vuốt).
- Các bước: (1) lên xe và vào ghế lái; (2) tăng/giảm tốc; (3) rẽ theo
  hướng bản đồ; (4) dừng và xuống xe; (5) mở cargo; (6) vuốt hướng khi
  đang lái.
- Oracle: trạng thái công bố phân biệt đúng đi bộ/lái và hướng xe so với
  hướng bản đồ ở mọi thời điểm; tốc độ sau mỗi lệnh khớp lệnh đã ra;
  vuốt khi đang lái không thành lệnh ga — xe không tăng tốc ngoài ý muốn.

**T51 — Vùng.**
*Chọn hai góc/tầng, nghe kích thước, đổi loại, bật/tắt và xóa; xác nhận phạm vi trước tác vụ.*

- Tiền điều kiện: bộ đọc bật (chi tiết form/bảng vùng ở TX-11 đến TX-13).
- Fixture: F-07 vùng/quy tắc.
- Môi trường: host-C++.
- Các bước: (1) tạo vùng bằng hai góc và tầng; (2) nghe kích thước trước
  khi xác nhận; (3) đổi loại vùng; (4) bật/tắt; (5) xóa vùng với xác nhận
  riêng.
- Oracle: vùng được tạo có đúng hai góc/tầng/kích thước đã công bố trước
  xác nhận; đổi loại/bật/tắt áp lên đúng vùng đã chọn; xóa chỉ xảy ra sau
  xác nhận riêng và chỉ xóa vùng đó.

**T52 — Khả năng đặc biệt.**
*Dùng mutation/bionics/phép có trong cấu hình, chọn mục tiêu và chi phí, hủy; không gộp "menu mở được" thành PASS cả hệ thống.*

- Tiền điều kiện: bộ đọc bật; nhân vật có mutation/bionics/phép (điểm
  đứt thi triển đã biết của cây nền được kiểm riêng ở TX-21 — bài này chỉ
  chạy đầu-cuối khi điểm đứt đó đã có kết luận).
- Fixture: F-09 khả năng.
- Môi trường: host-C++.
- Các bước: (1) dùng một bionic chủ động và nghe chi phí/trạng thái; (2)
  dùng một mutation; (3) chọn một phép, chọn mục tiêu và hủy; (4) thi
  triển một phép khác tới kết quả.
- Oracle: mỗi khả năng áp đúng chi phí (điện/năng lượng/thời gian) do
  engine quyết định lên đúng mục tiêu đã chọn; hủy không tốn chi phí và
  không hiệu ứng nào xảy ra; kết quả thật sau thi triển được công bố —
  chỉ "menu mở được" không tính là đạt bất kỳ phần nào của bài này.

### 2.7. Vòng đời, độ bền và phát hành

**T53 — Lưu/kill/tải.**
*Lưu, kết thúc tiến trình thật, mở lại, chọn đúng nhân vật; không phát lại lời/lệnh/mục tiêu cũ.*

- Tiền điều kiện: có nhân vật đang chơi dở với lời/lệnh/mục tiêu đang treo.
- Fixture: F-15 vòng đời/hai nhân vật.
- Môi trường: Android-giả-lập; máy-thật-TalkBack (xác nhận).
- Các bước: (1) lưu; (2) kết thúc tiến trình thật (không chỉ đóng
  Activity); (3) mở lại app; (4) tải đúng nhân vật.
- Oracle: nhân vật tải lên đúng bản lưu đã ghi (vị trí/trạng thái khớp);
  không lời đọc, lệnh hay mục tiêu nào của phiên trước được phát lại hay
  thực thi sau khi tải.

**T54 — Cuộc gọi/khóa/mất focus.**
*Ngắt khi TTS và thao tác chạm đang diễn ra; quay lại đúng ngữ cảnh, không bước hoặc bắn muộn, không tự xác nhận dialog.*

- Tiền điều kiện: bộ đọc đang phát lời và có thao tác chạm dang dở; có
  thể mô phỏng cuộc gọi/khóa máy.
- Fixture: F-15 vòng đời/hai nhân vật.
- Môi trường: Android-giả-lập; máy-thật-TalkBack (xác nhận cuộc gọi thật).
- Các bước: (1) ngắt bằng cuộc gọi/khóa máy/mất focus giữa lời đọc và
  giữa cử chỉ; (2) quay lại app; (3) kiểm tra ngữ cảnh và hộp thoại đang
  mở.
- Oracle: quay lại đúng ngữ cảnh trước ngắt; không bước đi/phát bắn muộn
  nào xảy ra; không hộp thoại nào bị tự xác nhận trong lúc mất focus.

**T55 — Tạo lại Activity/xoay máy.**
*Không giữ Activity cũ, không mất quyền hủy, không nhân đôi backend, listener hoặc hành động.*

- Tiền điều kiện: đang ở giữa một menu/hành động có thể hủy.
- Fixture: F-15 vòng đời/hai nhân vật.
- Môi trường: Android-giả-lập.
- Các bước: (1) xoay máy và ép tạo lại Activity; (2) kiểm tra số backend/
  listener đang sống; (3) hủy hành động đang dở; (4) thực hiện một hành
  động mới.
- Oracle: chỉ một backend/listener của bộ đọc còn sống sau recreate;
  quyền hủy vẫn hoạt động; hành động mới thực thi đúng một lần.

**T56 — Đầu ra âm thanh.**
*Thử loa, mono, tai nghe dây/Bluetooth, rút tai nghe và audio focus; vẫn có đường hiểu hướng và phục hồi âm lượng.*

- Tiền điều kiện: thiết bị thật có tai nghe dây và Bluetooth; có nguồn
  giành audio focus (chi tiết cue/focus ở TX-08).
- Fixture: không cần fixture — dùng bản cài sạch/mặc định.
- Môi trường: máy-thật-TalkBack.
- Các bước: (1) nghe qua loa, rồi mono; (2) cắm tai nghe dây, chuyển
  Bluetooth; (3) rút tai nghe giữa câu; (4) để ứng dụng khác giành/trả
  audio focus.
- Oracle: ở mọi đầu ra, thông tin hướng vẫn hiểu được bằng lời (không phụ
  thuộc stereo); rút tai nghe không làm mất lời đang đọc một cách im
  lặng; sau khi focus trở lại, âm lượng/lời đọc phục hồi mà không phát
  lại nội dung đã hết ngữ cảnh.

**T57 — Bản lưu lỗi/backup.**
*Mở lỗi mẫu, nghe thông báo và xuất/nhập bản sao qua file picker bằng đường trợ năng đã hỗ trợ; không ghi đè bản gốc vô ý.*

- Tiền điều kiện: có bản lưu lỗi mẫu và đường backup qua file picker
  (trọn vòng backup/khôi phục ở TX-24).
- Fixture: F-15 vòng đời/hai nhân vật.
- Môi trường: Android-giả-lập; máy-thật-TalkBack (xác nhận picker).
- Các bước: (1) mở bản lưu lỗi; (2) nghe thông báo lỗi; (3) xuất một bản
  sao qua file picker; (4) nhập lại bản sao đó.
- Oracle: lỗi được thông báo bằng lời và bản lưu lỗi không bị ghi đè;
  bản sao xuất ra khớp bản gốc (số tệp/tổng byte); nhập lại khôi phục
  đúng dữ liệu và bản gốc hiện tại không mất khi nhập lỗi.

**T58 — Tử vong/nghỉ hưu.**
*Qua deathcam/tổng kết/nhật ký/xử lý thế giới và về menu; không kẹt trong màn chỉ có hình.*

- Tiền điều kiện: có nhân vật có thể đưa tới tử vong và một nhân vật có
  thể nghỉ hưu.
- Fixture: F-15 vòng đời/hai nhân vật.
- Môi trường: host-C++; Android-giả-lập (xác nhận).
- Các bước: (1) đưa nhân vật tới tử vong; (2) đi qua deathcam, tổng kết
  và nhật ký; (3) xử lý thế giới sau tử vong và về menu; (4) lặp lại
  đường nghỉ hưu.
- Oracle: mọi màn trên cả hai đường đều có bộ đọc và đường tiếp tục; kết
  thúc ở menu chính trong trạng thái điều khiển được; không màn nào chỉ
  có hình mà không có lối nghe/thoát.

**T59 — Tất cả lệnh.**
*Mở trung tâm lệnh ngay từ khởi động/lần đầu vào game, khi không nạp tài sản đồ họa tùy chọn; danh mục đầy đủ theo sổ đối chiếu, không chỉ còn nút Quay lại.*

- Tiền điều kiện: bản cài mà tài sản đồ họa tùy chọn chưa/không được nạp
  (mở rộng kiểm chứng ở TX-27).
- Fixture: không cần fixture — dùng bản cài sạch/mặc định.
- Môi trường: host-C++; Android-giả-lập (xác nhận).
- Các bước: (1) mở trung tâm lệnh ngay từ khởi động; (2) mở lại ở lần
  đầu vào game; (3) đối chiếu danh mục với sổ đối chiếu lệnh.
- Oracle: danh mục lệnh sinh từ dữ liệu lệnh và adapter (đầy đủ theo sổ
  đối chiếu) ở cả hai thời điểm; không phụ thuộc nút/bitmap/shortcut bar
  hay màn hình đã từng dựng; không trường hợp nào danh mục chỉ còn nút
  Quay lại.

**T60 — TTS dồn hàng đợi.**
*Vuốt nhanh qua 20 mục, thay ngữ cảnh, phát cảnh báo rồi đọc lại; chỉ focus hiện hành được phát tự động, lịch sử vẫn có nội dung cần thiết.*

- Tiền điều kiện: bộ đọc bật; có danh sách ít nhất 20 mục.
- Fixture: F-02 cây túi.
- Môi trường: host-Java.
- Các bước: (1) vuốt nhanh qua 20 mục; (2) đổi ngữ cảnh giữa chừng; (3)
  phát một cảnh báo; (4) ra lệnh đọc lại.
- Oracle: chỉ focus hiện hành được phát tự động (không tồn tại hàng chờ
  20 câu focus cũ); lời của ngữ cảnh cũ bị xóa khi đổi ngữ cảnh; cảnh
  báo được phát đúng ưu tiên; lịch sử vẫn chứa nội dung cần thiết để đọc
  lại theo yêu cầu.

**T61 — Phiên dài.**
*Chạy ít nhất hai giờ với nhiều chuyển màn và TTS; đo bộ nhớ/listener/queue/độ trễ, không chỉ kiểm tra "không crash". Tăng thời lượng khi người dùng thực tế cần phiên dài hơn.*

- Tiền điều kiện: kịch bản phiên dài có nhiều chuyển màn và TTS liên tục;
  có công cụ đo bộ nhớ/listener/queue/độ trễ.
- Fixture: F-15 vòng đời/hai nhân vật.
- Môi trường: máy-thật-TalkBack; host-Java (đo hàng đợi bổ sung).
- Các bước: (1) chạy phiên ít nhất hai giờ theo kịch bản; (2) đo các chỉ
  số ở đầu/giữa/cuối phiên; (3) kiểm tra độ trễ focus cuối phiên.
- Oracle: bộ nhớ, số listener và độ dài queue không tăng không giới hạn
  theo thời gian; độ trễ focus cuối phiên không suy giảm so với đầu
  phiên theo ngưỡng chốt ở hồ sơ chạy; "không crash" một mình không tính
  là đạt.

**T62 — Tắt bộ đọc.**
*Chạy các hành trình baseline với bộ đọc tắt; joystick, bàn phím/IME, native UI, resize và lưu vẫn theo hành vi đã chọn để bảo toàn.*

- Tiền điều kiện: bộ hành trình baseline đã chọn với bộ đọc bật để đối
  chiếu.
- Fixture: không cần fixture — dùng bản cài sạch/mặc định.
- Môi trường: Android-giả-lập; host-C++ (xác nhận).
- Các bước: (1) tắt bộ đọc; (2) chạy các hành trình baseline: joystick,
  bàn phím/IME, native UI, resize, lưu/tải; (3) bật lại bộ đọc.
- Oracle: với bộ đọc tắt, mọi hành trình baseline giữ đúng hành vi đã
  chọn để bảo toàn (không khác đường gốc ngoài phần bộ đọc chủ động thêm
  vào); bật lại khôi phục đầy đủ chức năng bộ đọc.

**T63 — Gói phát hành.**
*Cài APK có chữ ký phát hành, kiểm tra nguồn/commit, ABI, thư viện, dữ liệu, bản dịch, voice và nâng cấp giữ save/config. AAB sinh ra thành công chưa thay thế kiểm tra cài APK thực tế.*

- Tiền điều kiện: có APK ứng viên ký bằng khóa phát hành và một thiết bị
  đang có bản cũ với save/config thật.
- Fixture: không cần fixture — dùng bản cài sạch/mặc định.
- Môi trường: máy-thật-TalkBack.
- Các bước: (1) kiểm tra nguồn/commit/ABI/thư viện/dữ liệu/bản dịch của
  gói; (2) cài mới trên máy sạch; (3) nâng cấp đè lên bản cũ; (4) kiểm
  tra save/config và voice sau nâng cấp.
- Oracle: APK cài được thật trên thiết bị (không thay bằng AAB build
  thành công); sau nâng cấp, save và config còn nguyên và dùng được;
  bản dịch và voice hoạt động trong gói đã cài.

**T64 — Người dùng mới.**
*Người khiếm thị chưa biết bộ cử chỉ hoàn thành hướng dẫn, tạo nhân vật, sinh tồn, tìm một chức năng nâng cao, lưu/tải và kết thúc mà không nhận chỉ dẫn bằng mắt; ghi rõ trợ giúp đã sử dụng và chỗ mắc kẹt.*

- Tiền điều kiện: người khiếm thị chưa từng học bộ cử chỉ của bộ đọc;
  máy thật của người dùng; người quan sát chỉ ghi nhận, không chỉ dẫn
  bằng mắt.
- Fixture: không cần fixture — dùng bản cài sạch/mặc định.
- Môi trường: nghiệm-thu-người-dùng.
- Các bước: (1) hoàn thành hướng dẫn cử chỉ; (2) tạo nhân vật; (3) sinh
  tồn một đoạn chơi thật; (4) tự tìm một chức năng nâng cao; (5) lưu,
  tải lại và kết thúc phiên.
- Oracle: người dùng hoàn thành cả năm chặng bằng đường của bộ đọc; hồ
  sơ ghi rõ trợ giúp đã sử dụng (nếu có) và mọi chỗ mắc kẹt — có chỗ mắc
  kẹt phải nhờ người sáng mắt thì bài không đạt, kể cả khi cuối cùng vẫn
  hoàn thành nhiệm vụ.

---

## 3. Danh mục bổ sung TX

Các bài TX là bài ngoài T01–T64, đánh số TX-01 trở đi. Mỗi bài cũng ghi
đủ tiền điều kiện / fixture / các bước / oracle / môi trường như quy ước
ở mục 1.2. Tên TX trong sổ hành trình (nếu có) được ghi ngay ở tiêu đề
bài và đối chiếu đầy đủ ở bảng ánh xạ cuối mục này (mục 3.3).

Trạng thái của mọi bài TX: **chưa chạy — chưa có PASS mới.**

### 3.1. Form, bảng, cây và hợp đồng R02

**TX-01 — Validate schema snapshot theo hợp đồng R02.**

- Tiền điều kiện: có bộ trường hợp snapshot hợp lệ và từng loại vi phạm
  schema (ID rỗng/trùng, cha không tồn tại, con của nút đã thu gọn, độ
  sâu không khớp hoặc quá 16, chu trình cha, bảng/cột chưa khai báo, hàng
  trùng `(tableId,rowIndex)`, loại trường lạ, miền min>max/step<0, lựa
  chọn trùng, nháp sai miền/bước/lựa chọn, `dependsOn` tự trỏ hoặc trỏ
  node không tồn tại).
- Fixture: không cần fixture — dùng bản cài sạch/mặc định.
- Môi trường: host-C++; host-Java (validate gương).
- Các bước: (1) đưa từng snapshot vi phạm qua validate ở C++ và qua
  validate gương ở Java; (2) đưa intent mang giá trị vi phạm qua
  `validate_intent_value`; (3) đưa snapshot cũ không có trường mới
  (không `tables`, `depth=-1`, không `draftValue`) qua cả hai phía.
- Oracle: mọi snapshot vi phạm bị từ chối trọn vẹn kèm mã lý do ổn định
  nội bộ, phía Java nhận biên nhận `unsupported` và phía intent nhận
  `invalid_value` khớp đúng request; không snapshot vi phạm nào bị cắt
  bớt rồi publish một phần; snapshot đời cũ vẫn hợp lệ ở cả hai phía.

**TX-02 — Form: nháp (draft) phân biệt với giá trị đã áp dụng (applied).**

- Tiền điều kiện: bộ đọc bật; mở được màn Options (mẫu form + cây của
  R02) với đủ loại trường bool/int/float/choice/text.
- Fixture: không cần fixture — dùng bản cài sạch/mặc định.
- Môi trường: host-C++; host-Java (xác nhận nút áp dụng/bỏ nháp).
- Các bước: (1) ghi giá trị đã áp dụng của một trường; (2) ghi nháp mới
  bằng `set_value`/`select_choice` và kích hoạt trường bool/choice; (3)
  kiểm tra `appliedValue` và trạng thái game; (4) `apply` và kiểm tra
  lại; (5) ghi một nháp khác rồi `cancel_draft`; (6) ghi nháp rồi rời
  màn không apply.
- Oracle: khi chỉ có nháp, `appliedValue` và hành vi game không đổi và
  bộ đọc luôn công bố rõ trường đang có nháp chưa áp dụng; sau `apply`,
  giá trị áp dụng bằng đúng nháp và `cOpt::setValue` nhận đúng giá trị
  đó; sau `cancel_draft`, giá trị áp dụng giữ nguyên và nháp biến mất;
  rời màn làm mất nháp như hợp đồng đã công bố (không áp dụng ngầm).

**TX-03 — Form: `dependsOn` và trường bị khóa theo điều kiện.**

- Tiền điều kiện: bộ đọc bật; có trường phụ thuộc cùng trang trong
  Options và cặp trường attitude/proximity của Safe Mode (phụ thuộc
  category/list — xem thêm TX-12).
- Fixture: F-07 vùng/quy tắc.
- Môi trường: host-C++.
- Các bước: (1) đọc một trường và nghe trường nó phụ thuộc; (2) đổi giá
  trị áp dụng của trường tiên quyết sang giá trị làm trường phụ thuộc bị
  khóa; (3) thử apply một nháp đã ghi trước đó cho trường đang bị khóa;
  (4) đổi tiên quyết trở lại và apply lại.
- Oracle: `dependsOn` công bố trỏ đúng các node tiên quyết đang publish;
  trường bị khóa công bố `disabledReason` đúng điều kiện gốc và apply
  trong lúc điều kiện không áp dụng bị từ chối (nháp được giữ, không ghi
  nhầm); khi điều kiện áp dụng trở lại, apply ghi đúng giá trị nháp.

**TX-04 — Trần 4096 node: snapshot lớn bị từ chối nguyên vẹn, không cắt thầm.**

- Tiền điều kiện: dựng được một màn có snapshot vượt 4096 node (hành
  trang/bản đồ quét lớn) và một màn tiệm cận nhưng chưa vượt trần.
- Fixture: F-02 cây túi.
- Môi trường: host-C++; host-Java (xác nhận biên nhận).
- Các bước: (1) mở màn vượt trần; (2) ghi biên nhận phía Java; (3) mở
  màn tiệm cận trần; (4) kiểm tra phân trang render phía Java trên màn
  lớn hợp lệ.
- Oracle: snapshot vượt trần bị từ chối trọn vẹn thành `unsupported`
  (không publish một phần bị cắt, không focus vào dữ liệu thiếu); màn
  dưới trần publish đầy đủ; hồ sơ chạy ghi rõ số node thật của các màn
  lớn đã đo để chốt ngưỡng/phương án phân trang ở giai đoạn 2.

**TX-05 — Bảng Study Zone vượt giới hạn 16 cột.**

- Tiền điều kiện: bộ đọc bật; Study Zone có hơn 15 đồng đội, tức bảng
  `study:skills` cần hơn 16 cột (1 cột kỹ năng + 1 cột mỗi NPC).
- Fixture: F-07 vùng/quy tắc.
- Môi trường: host-C++.
- Các bước: (1) mở Study Zone ở cấu hình vượt 16 cột; (2) ghi hành vi
  thật của snapshot (từ chối theo R02 hay phương án phân trang đã chốt ở
  giai đoạn 2); (3) giảm số NPC xuống dưới ngưỡng và mở lại.
- Oracle: hành vi ở cấu hình vượt giới hạn là hành vi đã công bố của hợp
  đồng (từ chối trọn vẹn kèm biên nhận, hoặc phân trang đúng dữ kiện) —
  không cột NPC nào bị cắt thầm mà người dùng không biết; ở cấu hình
  dưới ngưỡng, mọi ô Allowed/Not allowed khớp `npc_skill_preferences`
  thật. Bài này là nơi chốt ngưỡng/phương án phân trang mà R16 để mở.

### 3.2. Giọng, âm cứu hộ, cue và các hệ thống chuyên biệt

**TX-06 — Engine/voice: TTS lỗi, timeout và hồi phục giữa phiên (R06/R21).**

- Tiền điều kiện: có thể đổi engine/voice, làm engine lỗi giữa phiên và
  làm khởi tạo vượt thời hạn (mở rộng của T04 cho đường engine/voice).
- Fixture: không cần fixture — dùng bản cài sạch/mặc định.
- Môi trường: host-Java; máy-thật-TalkBack (xác nhận).
- Các bước: (1) chọn engine/voice theo nháp, nghe mẫu thử, xác nhận lưu
  và hủy một lần khác để về giá trị đã lưu; (2) làm TTS lỗi giữa phiên
  chơi; (3) chờ hồi phục có giới hạn; (4) đổi voice khi voice đã lưu bị
  mất khỏi thiết bị.
- Oracle: lựa chọn engine/voice chỉ ghi sau xác nhận (nháp không làm đổi
  giọng đang dùng, giọng cần mạng không tự được chấp nhận); lỗi giữa
  phiên không khóa game và không nhận chuỗi di chuyển tự lặp; hồi phục
  thành công nối lại đúng engine/voice đã lưu, hồi phục thất bại mở
  đường cứu hộ nghe được; voice bị mất có đường phục hồi rõ ràng thay vì
  im lặng.

**TX-07 — Âm cứu hộ: clip mp3 đóng gói khi TTS lỗi/thiếu giọng (R06).**

- Tiền điều kiện: thiết bị ở trạng thái TTS lỗi hoặc thiếu giọng Việt
  (như T03/T04) tại màn khởi động/thiết lập.
- Fixture: không cần fixture — dùng bản cài sạch/mặc định.
- Môi trường: máy-thật-TalkBack.
- Các bước: (1) đưa app vào trạng thái TTS lỗi ở SplashScreen/thiết lập;
  (2) nghe các clip cứu hộ được phát (thử lại, chọn nguồn đọc, mở cài
  đặt, quay lại, thiếu dung lượng); (3) chọn một lệnh cứu hộ bằng thao
  tác một ngón; (4) xử lý xong trạng thái kẹt và quay lại luồng chính.
- Oracle: người dùng thoát khỏi trạng thái kẹt mà không cần TTS đang lỗi
  (lệnh chọn bằng một ngón thực thi đúng lệnh đã nghe); nội dung clip
  khớp lệnh thật của nút đi kèm; độ rõ của giọng clip được ghi riêng
  trong hồ sơ (asset eSpeak NG phải nghiệm thu bằng tai ở bài này —
  phát được file không tính là nghe rõ).

**TX-08 — Audio focus, đổi đầu ra và cue khi focus bị từ chối (R21).**

- Tiền điều kiện: thiết bị thật có loa/tai nghe/Bluetooth; có ứng dụng
  khác giành audio focus được (mở rộng của T56).
- Fixture: không cần fixture — dùng bản cài sạch/mặc định.
- Môi trường: máy-thật-TalkBack.
- Các bước: (1) đang nghe TTS thì để cuộc gọi/ứng dụng khác giành focus;
  (2) rút tai nghe giữa câu; (3) kích một sự kiện có cue trong lúc cue
  không được cấp focus; (4) trả focus và tiếp tục chơi.
- Oracle: TTS xử lý mất/trả focus mà không phát chồng hai giọng và không
  lặp nội dung đã hết ngữ cảnh; khi cue bị từ chối focus, chỉ tiếng cue
  bị bỏ qua (đếm `cue_focus_denied`) còn rung và lời tương đương không
  bị ảnh hưởng; thiết bị không hỗ trợ cue thì thông báo không khả dụng
  được nói một lần và lời đọc vẫn tiếp tục bình thường.

**TX-09 — Cue âm thanh/rung luôn có tương đương bằng lời và không rò thông tin.**

- Tiền điều kiện: bộ đọc bật; bật lần lượt cue âm thanh, cue rung, cả
  hai, và tắt cả hai.
- Fixture: F-01 cặp tri thức.
- Môi trường: host-C++ (điểm kích cue); máy-thật-TalkBack (nghe/rung thật).
- Các bước: (1) kích từng sự kiện có cue — bị chặn/safe mode
  (`blocked`), thông báo khẩn (`danger`), nhặt đồ xong (`pickup`), hoạt
  động dài xong (`activity_done`), intent bị từ chối (`rejected`); (2)
  với mỗi sự kiện, ghi cue phát ra và lời tương đương đi kèm; (3) lặp
  lại với cả hai loại cue tắt; (4) đặt một bẫy/quái chưa được biết ở gần
  và di chuyển.
- Oracle: mỗi cue đều nhân đôi đúng một tín hiệu lời đã công bố (node
  kết quả hành động, thông báo khẩn, thông báo hoàn thành của engine,
  biên nhận bị từ chối) — tắt cả hai cue làm mất đúng 0 thông tin; không
  cue/rung nào phát ra cho sự kiện nhân vật chưa được biết.

**TX-10 — Smart controller: form ngưỡng pin của xe (R15).**
Tên trong sổ: không có mã TX riêng — hành trình `smart-controller`.

- Tiền điều kiện: bộ đọc bật; xe có smart engine controller; biết logic
  ngưỡng gốc: `battery_lo` miền 5–90 bước 5 (giới hạn cứng), `battery_hi`
  miền 10–95 bước 5 (giới hạn mềm), hai ngưỡng `dependsOn` lẫn nhau.
- Fixture: F-06 xe.
- Môi trường: host-C++.
- Các bước: (1) mở Smart controller settings; (2) ghi nháp cho
  `smart:battery_lo`/`smart:battery_hi` rồi hủy nháp; (3) apply một cặp
  ngưỡng hợp lệ; (4) apply trường hợp nâng `lo` vượt `hi` và hạ `hi`
  xuống dưới `lo`; (5) bật/tắt `smart:enabled` bằng activate.
- Oracle: nháp chưa apply không đổi hành vi xe; sau apply, các reference
  mà slider gốc ghi nhận đúng giá trị nháp với cùng phép coupling của
  engine (nâng `lo` đẩy `hi` lên, hạ `hi` kéo `lo` xuống, luôn cách nhau
  ≥ 5); trạng thái `enabled` được đồng bộ sang các part
  SMART_ENGINE_CONTROLLER như đường gốc; ngưỡng ngoài miền bị từ chối
  tại trường.

**TX-11 — Study Zone: cấu hình kỹ năng theo đồng đội trọn luồng (R16).**
Tên trong sổ: không có mã TX riêng — hành trình `study-zone`.

- Tiền điều kiện: bộ đọc bật; trại/vùng học có đồng đội và danh sách kỹ
  năng (trường hợp vượt cột ở TX-05).
- Fixture: F-07 vùng/quy tắc.
- Môi trường: host-C++.
- Các bước: (1) mở Study Zone và đọc bảng `study:skills`; (2) đảo một ô
  Allowed/Not allowed bằng activate (ghi ngay như click gốc) và bằng
  đường nháp `select_choice` + apply/cancel; (3) dùng check-all và
  clear-all; (4) thoát bằng Done; (5) mở lại và kiểm tra dữ liệu trước/
  sau; (6) lặp lại khi không có đồng đội nào.
- Oracle: mọi thay đổi khớp đúng `npc_skill_preferences` sau từng thao
  tác (insert/erase như logic gốc), dữ liệu trước/sau đọc lại được rõ
  ràng từng ô; check-all/clear-all áp lên toàn bộ kỹ năng cho mọi đồng
  đội và detail công bố đúng phạm vi đó; trường hợp không có đồng đội
  không khai báo bảng, chỉ có tài liệu "No companions." và nút Done.

**TX-12 — Bảng quy tắc Safe Mode (R16).**
Tên trong sổ: không có mã TX riêng — hành trình `safemode-va-auto-pickup`.

- Tiền điều kiện: bộ đọc bật; có quy tắc ở cả tab Global và Character.
- Fixture: F-07 vùng/quy tắc.
- Môi trường: host-C++.
- Các bước: (1) mở Safe Mode manager, đọc bảng `safemode:rules` đủ 6
  cột (Rules / Attitude / Dist / B/W / Category / Mode); (2) sửa một
  quy tắc qua 7 trường R02 (rule/attitude/proximity/list/category/
  movement mode/active), gồm trường hợp attitude/proximity bị khóa
  theo category/list; (3) thêm, sao chép, đổi thứ tự, bật/tắt, chuyển
  tab một quy tắc; (4) xóa một quy tắc bằng xác nhận hai bước; (5) mở
  màn kiểm tra mẫu `test_pattern`.
- Oracle: trạng thái active vốn chỉ là màu được công bố bằng lời khớp
  dữ kiện quy tắc thật; apply chỉ ghi khi binding của nháp khớp hàng
  đang chọn và khớp chuỗi rule sống (không ghi nhầm quy tắc đã trượt chỉ
  số); việc chốt vào kho thật vẫn do "Save changes?" lúc thoát quyết
  định; màn kiểm tra mẫu công bố đúng số mtype khớp và danh sách của UI
  gốc.

**TX-13 — Bảng quy tắc Auto-pickup (R16).**
Tên trong sổ: không có mã TX riêng — hành trình `safemode-va-auto-pickup`.

- Tiền điều kiện: bộ đọc bật; mở được auto-pickup ở cả đường người chơi
  (2 tab Global/Character) và đường NPC (1 tab).
- Fixture: F-07 vùng/quy tắc.
- Môi trường: host-C++.
- Các bước: (1) đọc bảng `autopickup:rules` (Rules / Inc/Exc / Active)
  và trạng thái option `AUTO_PICKUP`; (2) sửa một quy tắc qua 3 trường
  R02 (rule/exclude/active); (3) thử ghi nháp rule rỗng; (4) thêm/sao
  chép/đổi thứ tự/bật/tắt/xóa hai bước; (5) bật/tắt option qua checkbox;
  (6) mở màn kiểm tra mẫu `test_pattern`.
- Oracle: nháp rule rỗng bị từ chối bằng `field_error` đúng quy tắc gốc
  và giữ nguyên nháp; sửa hợp lệ ghi qua đúng đường `wildcard_trim_rule`
  vào `new_rules` và chỉ chốt vào kho thật khi thoát có đồng ý lưu;
  option tự đổi và lưu ngay như bản gốc khi bật/tắt qua checkbox; màn
  kiểm tra mẫu công bố đúng số itype khớp của UI gốc.

**TX-14 — Máy tính trong game trọn luồng (R17).**
Tên trong sổ: `TX-may-tinh` — hành trình `may-tinh-trong-game`.

- Tiền điều kiện: bộ đọc bật; nhân vật đứng trước một máy tính có menu,
  có option cần mật khẩu và có nhánh hỏi YES/NO/QUIT.
- Fixture: F-09 khả năng.
- Môi trường: host-C++.
- Các bước: (1) mở máy tính và đọc menu chính; (2) chọn option cần mật
  khẩu và nghe cảnh báo "Password required." trước khi kích hoạt; (3)
  đọc văn bản terminal qua tài liệu `computer:text`; (4) trả lời một
  câu hỏi YES/NO/QUIT bằng intent của bộ đọc (gồm nhánh chữ hoa bắt
  buộc của engine); (5) qua một điểm chờ phím bất kỳ bằng node tiếp tục;
  (6) kiểm tra hai khoảng trống đã ghi trong R17: menu con mutations
  (đọc mô tả từng mutation) và dãy ba dòng của `action_srcf_seal`.
- Oracle: văn bản tài liệu khớp đúng member `lines` của terminal (thứ
  tự, indent, đã lột thẻ màu) tại thời điểm query; YES/NO/QUIT của bộ
  đọc cho cùng kết quả như phím gốc kể cả nhánh FORCE_CAPITAL_YN; việc
  chỉ đọc/focus không tự chạy lệnh máy tính nào; hai khoảng trống đã
  biết (mô tả mutations qua desc mới; hai dòng đầu của `action_srcf_seal`
  mất với bộ đọc) được xác nhận đúng như hồ sơ R17 ghi — nếu hành vi
  thật khác hồ sơ, bài FAIL để mở lại R17 chứ không sửa oracle cho khớp.

**TX-15 — Minigame Kitten: chơi trọn một ván.**
Tên trong sổ: `TX-minigame-kitten` — hành trình `minigame-kitten`.

- Tiền điều kiện: bộ đọc bật; có vật phẩm phần mềm Kitten; đã nghe luật.
- Fixture: F-10 kitten.
- Môi trường: host-C++; nghiệm-thu-người-dùng (xác nhận chơi thật).
- Các bước: (1) mở trò từ vật phẩm; (2) duyệt các vật thể trên bàn và
  nghe thông điệp bogus; (3) di chuyển robot bằng cả intent bộ đọc và
  phím cứng; (4) chạm vào kitten để kết thúc ván.
- Oracle: duyệt/focus không làm mất thông điệp bogus và không công bố
  danh tính kitten trước khi chạm; robot di chuyển đúng hướng đã lệnh ở
  cả hai đường nhập; ván kết thúc đúng luật gốc khi chạm kitten (kết quả
  thắng công bố khớp trạng thái trò chơi).

**TX-16 — Minigame Lights On: chơi trọn một ván.**
Tên trong sổ: `TX-minigame-lightson` — hành trình `minigame-lightson`.

- Tiền điều kiện: bộ đọc bật; có vật phẩm phần mềm Lights On; biết trước
  một lời giải hữu hạn của bàn trong fixture.
- Fixture: F-13 lightson.
- Môi trường: host-C++; nghiệm-thu-người-dùng (xác nhận chơi thật).
- Các bước: (1) mở trò; (2) duyệt bàn theo từng ô `lights:cell:<x>:<y>`;
  (3) toggle các ô theo lời giải đã biết; (4) chơi tới khi bàn tắt hết.
- Oracle: focus chỉ đổi vị trí con trỏ (không toggle ô nào); activate
  phát đúng `TOGGLE_SPACE` và trạng thái ô + các ô kề đổi đúng luật gốc;
  toàn bộ bàn là tri thức công khai nên mọi ô đều đọc được; ván thắng
  được công bố khi ô cuối cùng tắt.

**TX-17 — Minigame Minesweeper: chơi trọn một ván.**
Tên trong sổ: `TX-minigame-minesweeper` — hành trình `minigame-minesweeper`.

- Tiền điều kiện: bộ đọc bật; có vật phẩm phần mềm Minesweeper.
- Fixture: F-11 minesweeper.
- Môi trường: host-C++; nghiệm-thu-người-dùng (xác nhận chơi thật).
- Các bước: (1) mở trò; (2) đọc các ô đã mở/đã cắm cờ/chưa biết; (3) mở
  thêm ô an toàn và cắm cờ một ô nghi mìn bằng action flag riêng; (4)
  chơi tới thắng, và ở một ván riêng, mở trúng mìn để thua; (5) mở bàn
  Custom lớn trong fixture.
- Oracle: cổng tri thức `cell_value_text` là đường công bố duy nhất — ô
  chưa mở không đọc ra số/mìn, mìn chỉ được công bố sau khi nổ (boom);
  mở/cắm cờ áp đúng ô đã chọn và kết quả thắng/thua khớp luật gốc; bàn
  vượt trần chỉ công bố cửa sổ quanh con trỏ và summary nói rõ điều đó.

**TX-18 — Minigame Sokoban: chơi trọn một ván.**
Tên trong sổ: `TX-minigame-sokoban` — hành trình `minigame-sokoban`.

- Tiền điều kiện: bộ đọc bật; có vật phẩm phần mềm Sokoban.
- Fixture: F-12 sokoban.
- Môi trường: host-C++; nghiệm-thu-người-dùng (xác nhận chơi thật).
- Các bước: (1) mở trò; (2) đọc tài liệu bàn `soko:board` và vị trí
  người chơi/kiện hàng; (3) nghe từ vựng ô kề (wall/floor/goal/package/
  package on goal, và "you"/"you, on a goal" khi áp dụng); (4) đẩy một
  kiện lên ô đích và thử một nước đẩy không hợp lệ; (5) hoàn thành màn.
- Oracle: nước đẩy hợp lệ di chuyển đúng kiện đã chọn lên đúng ô; nước
  đẩy không hợp lệ không đổi trạng thái bàn; trạng thái kiện/đích công
  bố khớp bàn thật sau từng nước; hoàn thành màn được công bố đúng luật
  gốc.

**TX-19 — Minigame Snake: chơi trọn một ván.**
Tên trong sổ: `TX-minigame-snake` — hành trình `minigame-snake`.

- Tiền điều kiện: bộ đọc bật (chính sách nhịp kiểm riêng ở TX-20); có
  vật phẩm phần mềm Snake.
- Fixture: F-14 snake.
- Môi trường: host-C++; nghiệm-thu-người-dùng (quyết định giữ chính sách).
- Các bước: (1) mở trò và nghe luật kèm câu chính sách nhịp; (2) duyệt
  các node hướng và node quả; (3) ra lệnh hướng để ăn ít nhất một quả;
  (4) chơi tới game over (đâm vào thân).
- Oracle: duyệt node hướng/quả (focus) không làm rắn bước — chỉ
  activate mới bước đúng một ô theo hướng đã chọn; ăn quả cho đúng +500
  điểm và +10 đoạn như luật gốc; summary công bố điểm, độ dài thật của
  thân, đầu rắn và vị trí quả tương đối khớp bàn thật; game over công bố
  tổng điểm đúng.

**TX-20 — Snake: chính sách theo lượt khi bộ đọc bật, realtime như gốc khi bộ đọc tắt (R18.4).**

- Tiền điều kiện: cùng bản cài chơi được Snake ở cả hai chế độ bộ đọc
  bật/tắt; có cách đo nhịp bước của rắn.
- Fixture: F-14 snake.
- Môi trường: host-C++; máy-thật-TalkBack (xác nhận nhịp thật).
- Các bước: (1) bộ đọc bật: mở Snake, không ra lệnh hướng và chờ lâu
  hơn nhiều chu kỳ `iGameSpeed`; (2) ra lệnh hướng từng bước và ghi số
  ô rắn đi; (3) kiểm tra câu chính sách trong summary và trong tài liệu
  luật `snake:rules`; (4) tắt bộ đọc, mở lại Snake và quan sát nhịp.
- Oracle: khi bộ đọc bật, rắn không tự bước khi chưa có lệnh (chạy theo
  lượt), mỗi lệnh hướng làm rắn bước đúng một ô, và luật/điểm/tăng
  trưởng/xuyên biên/thua khi đâm vào mình giống hệt bản gốc; câu chính
  sách nhịp xuất hiện trong mọi snapshot đang chơi và trong tài liệu
  luật; khi bộ đọc tắt, rắn tự bước theo `iGameSpeed` như bản gốc
  (realtime). Nếu nghiệm thu người dùng ở TX-19 kết luận bản theo lượt
  không còn là Snake, kết luận đó được ghi vào hồ sơ bài này và R18.4
  phải mở lại — không giữ nhãn hoàn thành.

**TX-21 — Điểm đứt thi triển phép ở cây nền (R17 mục 3.4).**

- Tiền điều kiện: bộ đọc bật; nhân vật có phép đã học và đủ điều kiện
  thi triển (để loại trừ các lý do từ chối hợp lệ của
  `Character::cast_spell`).
- Fixture: F-09 khả năng.
- Môi trường: host-C++.
- Các bước: (1) ra lệnh Cast qua đường của bộ đọc (trung tâm lệnh/menu
  phép → chọn phép → xác nhận); (2) truy vết đường gọi thật: dispatch
  `ACTION_CAST_SPELL`/`ACTION_RECAST_SPELL` trong
  `src/handle_action.cpp` có tới được `Character::cast_spell` hay không;
  (3) nếu không tới được, ghi lỗi/công bố thực tế mà người dùng nhận
  được; (4) lặp lại với Recast.
- Oracle: thi triển qua đường reader phải tới được
  `Character::cast_spell` (rồi `spellcasting_activity_actor` và selector
  nhắm mục tiêu của R11) **hoặc** lỗi bị đứt được công bố rõ cho người
  dùng — không im lặng, không "menu mở được" giả làm thi triển được.
  Theo hồ sơ R17 mục 3.4 tại HEAD đối chiếu, `game::cast_spell` không
  có khai báo/định nghĩa trong cây và `known_magic::select_spell` không
  có caller, nên kết quả kỳ vọng trung thực của bài này ở ứng viên chưa
  sửa glue là FAIL/BLOCKED có ghi rõ điểm đứt — bài này đạt PASS chỉ khi
  glue của engine đã được khôi phục và bước (1) chạy đầu-cuối. Kết quả
  bài này là điều kiện để T52 chạy đầu-cuối.

**TX-22 — Trại phe: giao việc và trả đồng đội (companion missions, R14).**
Tên trong sổ: không có mã TX riêng — hành trình `giao-viec-va-tra-dong-doi`.

- Tiền điều kiện: bộ đọc bật; có trại/điểm giao việc và đồng đội đủ điều
  kiện lẫn thiếu điều kiện (kỹ năng) cho ít nhất một việc.
- Fixture: F-08 trại.
- Môi trường: host-C++.
- Các bước: (1) mở giao diện companion missions; (2) đọc entry việc
  (tên trại/điểm giao việc, Time/Profit/Danger, ETA/trạng thái/người
  thực hiện); (3) giao một việc cho đồng đội đủ điều kiện; (4) thử giao
  việc cho người thiếu kỹ năng; (5) cho đồng đội đi rồi trả đồng đội về.
- Oracle: activate chỉ ghi việc khi entry được kích hoạt đúng là entry
  đang chọn (không chọn nhầm khi danh sách đổi); việc được ghi cho đúng
  người với ETA/trạng thái khớp roster thật; trường hợp thiếu kỹ năng bị
  khóa kèm lý do tường minh; việc đang có người làm công bố người đang
  làm thay vì biến mất; trả đồng đội cập nhật roster đúng thực tế.

**TX-23 — Trại phe: quét kho, công việc hậu cần và mở rộng/biến đổi trại (R14).**
Tên trong sổ: không có mã TX riêng — hành trình `trai-xay-dung-va-hau-can`.

- Tiền điều kiện: bộ đọc bật; trại có kho, công việc xây dựng/hậu cần
  đang dở và một lựa chọn mở rộng trại.
- Fixture: F-08 trại.
- Môi trường: host-C++.
- Các bước: (1) mở menu trại và hai màn riêng của faction_camp (phân
  công người `faction-camp-workers`, phân công việc `faction-camp-jobs`);
  (2) quét kho trại: đọc lượng vật phẩm/nguồn lực hiện có; (3) giao một
  công việc hậu cần và nghe chuỗi kết quả; (4) bắt đầu một mở rộng/biến
  đổi trại (expansion/transformation như survey/plant/crafting của
  engine) và một trường hợp bị khóa (ví dụ plant bị lạnh, crafting đã
  đủ 3 người); (5) thử tăng/giảm lương thực tại trần/sàn.
- Oracle: số liệu quét kho khớp kho thật của trại tại thời điểm đọc;
  công việc được ghi đúng người/độ ưu tiên đã chọn và kết quả chuỗi hậu
  cần công bố đầy đủ; mở rộng bắt đầu đúng lựa chọn đã xác nhận với chi
  phí/người thực hiện khớp engine; các lựa chọn bị khóa công bố lý do
  của engine (không biến mất lặng lẽ); tăng/giảm lương thực tại trần/
  sàn bị khóa kèm lý do.

**TX-24 — Backup/khôi phục trọn vòng qua file picker (R20).**
Tên trong sổ: không có mã TX riêng — hành trình `backup-export-import`.

- Tiền điều kiện: có dữ liệu chơi thật để backup; có đích SAF/file picker
  ghi được; có thể tạo import lỗi (thiếu tệp/sai manifest/trùng tên).
- Fixture: F-15 vòng đời/hai nhân vật.
- Môi trường: Android-giả-lập; máy-thật-TalkBack (xác nhận picker thật).
- Các bước: (1) nghe hướng dẫn backup trong ReaderSetupActivity rồi mở
  picker; (2) xuất bản sao; (3) kiểm chứng bản sao (manifest khớp số
  tệp/tổng byte); (4) khôi phục bản sao bằng nút xác nhận riêng; (5)
  nhập một bản lỗi và một bản chứa đường `../`/cây ngoài phạm vi backup;
  (6) kill app giữa lúc hoán đổi rồi mở lại.
- Oracle: khôi phục chỉ xảy ra sau xác nhận riêng (không mặc định ghi
  đè); bản lỗi và bản chứa đường độc bị chặn ở khâu kiểm chứng trước khi
  thay thế, dữ liệu hiện tại nguyên vẹn trong mọi trường hợp lỗi; hoán
  đổi bị kill giữa chừng được khôi phục theo journal khi mở lại, không
  mất bản lưu gốc.

**TX-25 — Vòng đời: process death và nâng cấp APK giữ save/config.**

- Tiền điều kiện: có save/config đang dùng; có APK ứng viên mới hơn để
  nâng cấp đè (bổ sung cho T53/T63 ở mức vòng đời dữ liệu).
- Fixture: F-15 vòng đời/hai nhân vật.
- Môi trường: Android-giả-lập; máy-thật-TalkBack (xác nhận nâng cấp).
- Các bước: (1) chơi tới một trạng thái ghi nhớ được rồi để hệ điều hành
  hủy tiến trình (process death thật, không chỉ đóng Activity); (2) mở
  lại và tải save; (3) nâng cấp APK đè lên bản đang dùng; (4) mở lại,
  tải save và kiểm config trợ năng.
- Oracle: sau process death, save tải lại đúng trạng thái đã lưu và
  không lời/lệnh/mục tiêu cũ nào sống lại; sau nâng cấp, save và config
  (gồm cấu hình bộ đọc, cử chỉ đã xác nhận, lựa chọn giọng) còn nguyên
  và dùng được; không bước nào buộc gỡ app hay xóa dữ liệu.

**TX-26 — Lịch sử thông báo: đọc lại không đọc kép (R21).**

- Tiền điều kiện: bộ đọc bật; phiên chơi đã sinh nhiều thông báo (vượt
  một lần redraw và gần giới hạn lịch sử).
- Fixture: F-05 trường chiến đấu (nguồn thông báo dồi dào, có thứ tự).
- Môi trường: host-C++; host-Java (xác nhận vị trí đọc).
- Các bước: (1) mở lịch sử thông báo (message log) và đọc theo thứ tự;
  (2) đọc tới giữa lịch sử rồi để một cảnh báo ngắt; (3) quay lại đọc
  tiếp; (4) redraw màn và đổi bộ lọc của dialog; (5) kiểm tra đầu lịch
  sử khi tổng số thông báo vượt giới hạn công bố.
- Oracle: vị trí đọc được giữ qua redraw và qua cảnh báo — đọc tiếp bắt
  đầu đúng chỗ đã dừng, không thông báo nào bị phát lại từ đầu hay đọc
  kép; mỗi thông báo là một đoạn riêng và thứ tự/bộ lọc là của dialog
  gốc; khi vượt giới hạn, summary công bố đúng số đã hiện/tổng
  (giới hạn theo option `MESSAGE_LIMIT`) thay vì cắt lặng lẽ.

**TX-27 — Trung tâm lệnh đầy đủ không cần tài nguyên hình (mở rộng T59).**

- Tiền điều kiện: bản cài không nạp tài sản đồ họa tùy chọn; đối chiếu
  được sổ lệnh đầy đủ (nhóm, mô tả, phím gán, điều kiện).
- Fixture: không cần fixture — dùng bản cài sạch/mặc định.
- Môi trường: host-C++; Android-giả-lập (xác nhận).
- Các bước: (1) mở "Tất cả lệnh" từ nhiều ngữ cảnh khác nhau (thế giới,
  menu con, lần đầu vào game); (2) duyệt theo nhóm và nghe mô tả/chi
  phí/điều kiện của từng lệnh; (3) tìm một lệnh bằng tìm kiếm và một
  lệnh đang không khả dụng; (4) gọi một lệnh có tham số từ trung tâm.
- Oracle: danh mục ở mọi ngữ cảnh đầy đủ theo sổ đối chiếu và sinh từ
  dữ liệu lệnh/adapter, không phụ thuộc tài nguyên hình, nút hay màn đã
  dựng; lệnh không khả dụng công bố lý do thay vì biến mất hoặc bịa chi
  phí/tham số — nơi nguồn không có chi phí/tham số, trung tâm nói rõ
  điều đó; lệnh gọi từ trung tâm chạy đúng ngữ cảnh đích với đủ tham số.

**TX-28 — Lịch sử cảnh báo ngưỡng R10 (alert history).**

- Tiền điều kiện: bộ đọc bật; có thể vượt nhiều ngưỡng sinh tồn liên
  tiếp (đói/khát/đau/nhiệt/morale/weariness/sức bền).
- Fixture: F-04 vết thương.
- Môi trường: host-C++.
- Các bước: (1) vượt lần lượt các ngưỡng để sinh cảnh báo; (2) vượt lại
  cùng một ngưỡng trong cửa sổ cooldown 30 phút giờ game ở mức không
  khẩn cấp, rồi ở mức khẩn cấp hơn; (3) mở lịch sử cảnh báo qua node
  `world:alert_history`; (4) sinh hơn 20 cảnh báo; (5) tải nhân vật khác
  và kiểm tra lịch sử.
- Oracle: lịch sử giữ đúng 20 cảnh báo gần nhất theo thứ tự, khớp các
  cảnh báo đã phát thật (không kênh nào bị ghi hai lần — cảnh báo
  ngưỡng không đọc kép với log game); mức không khẩn cấp bị cooldown
  chặn lặp đúng cửa sổ, mức tệ hơn/khẩn cấp vượt qua cooldown; baseline
  và lịch sử được reset khi đổi phiên/tải nhân vật khác.

**TX-29 — Kỹ năng và proficiency: tiến độ nghe được thay biểu đồ/màu (R19).**
Tên trong sổ: `TX-ky-nang-proficiency` — hành trình `ky-nang-va-proficiency`.

- Tiền điều kiện: bộ đọc bật; nhân vật có kỹ năng ở nhiều cấp và
  proficiency đã học/đang học/chưa đủ điều kiện.
- Fixture: F-09 khả năng.
- Môi trường: host-C++.
- Các bước: (1) mở màn trạng thái nhân vật và đọc bảng kỹ năng
  `pd:skills`; (2) mở cửa sổ proficiency và đọc bảng `proficiency:list`
  (tên/trạng thái/tiến độ %/thời gian/nhóm); (3) lọc theo tab nhóm và
  bằng bộ lọc gốc; (4) đọc chi tiết một proficiency qua tài liệu chi tiết.
- Oracle: cấp kỹ năng, kinh nghiệm hiểu biết (knowledgeExperience), tiến
  độ % và điều kiện còn thiếu công bố bằng số/lời khớp dữ kiện thật của
  nhân vật — không trường nào chỉ còn là thanh/màu; lọc trả đúng tập
  con và vị trí/tổng khớp tập lọc; không kiến thức ẩn nào bị suy ra
  ngoài dữ kiện game cấp.

**TX-30 — Công thức đã học: sách, học và danh sách công thức biết (R19/R12).**

- Tiền điều kiện: bộ đọc bật; nhân vật có sách dạy công thức (một cuốn
  đọc được, một cuốn chưa đủ điều kiện) và công thức đã học từ trước.
- Fixture: F-09 khả năng.
- Môi trường: host-C++.
- Các bước: (1) đọc sách và nghe điều kiện học; (2) học một công thức
  đủ điều kiện và thử học một công thức chưa đủ điều kiện; (3) mở danh
  sách công thức đã biết và lọc/tìm một công thức vừa học; (4) mở công
  thức đó trong màn chế tạo.
- Oracle: công thức học thành công xuất hiện trong danh sách đã biết và
  chế tạo được ngay sau khi học; trường hợp chưa đủ điều kiện bị từ
  chối với đúng điều kiện còn thiếu và không công thức nào được cộng
  ngầm; danh sách đã biết không chứa công thức nhân vật chưa học.

**TX-31 — Phẫu thuật bionics: lắp/tháo qua autodoc và tự lắp CBM (R17).**
Tên trong tài liệu R17: `TX-phau-thuat` (chưa có mã này trong
`JOURNEY-LEDGER.json` — xem bảng ánh xạ) — hành trình `bionics`.

- Tiền điều kiện: bộ đọc bật; có CBM để lắp, một autodoc/NPC phẫu thuật
  và một bionic đã lắp để tháo.
- Fixture: F-09 khả năng.
- Môi trường: host-C++.
- Các bước: (1) mở luồng lắp bionic qua autodoc và đọc cột FAILURE
  CHANCE của preset; (2) xác nhận lắp qua hộp thoại của engine; (3)
  tháo một bionic qua NPC phẫu thuật và nghe cảnh báo SEVERE damage;
  (4) tự lắp một CBM từ vật phẩm (luồng `install_bionic_actor`).
- Oracle: tỉ lệ rủi ro engine công bố (FAILURE CHANCE, % trong
  `query_yn`) tới được bộ đọc ở mọi luồng có công bố; kết quả lắp/tháo
  (thành công/thất bại/hậu quả) khớp trạng thái bionics thật sau thao
  tác; riêng luồng tự lắp CBM, hồ sơ ghi đúng sự thật đã đối chiếu ở
  R17 — UI gốc không công bố % rủi ro ở luồng này cho bất kỳ ai, nên
  oracle là "không bịa số để bù": bộ đọc không được trình bày một tỉ lệ
  không có trong engine.

**TX-32 — Phạm vi riêng: debug menu, editmap và wish menu có nhãn giới hạn và đường thoát.**
Tên trong sổ: `TX-debug-menu`, `TX-editmap`, `TX-wish` — hành trình
`debug-menu-va-console`, `editmap`, `wish-menu`.

- Tiền điều kiện: bộ đọc bật; mở được debug menu/console, editmap và
  wish menu (công cụ phát triển — phạm vi riêng theo R01.4, không phải
  tính năng chơi).
- Fixture: không cần fixture — dùng bản cài sạch/mặc định.
- Môi trường: host-C++; Android-giả-lập (xác nhận).
- Các bước: (1) mở từng công cụ trong ba công cụ; (2) nghe nhãn/trạng
  thái hỗ trợ của nó; (3) thoát khỏi từng công cụ về luồng chơi.
- Oracle: mỗi công cụ hiển thị cho người dùng đều công bố rõ đây là
  phạm vi riêng/chưa hỗ trợ đầy đủ (không giả làm tính năng chơi đã
  phủ) và có đường thoát hoạt động được bằng bộ đọc; thoát không làm
  mất phiên chơi đang dở.

### 3.3. Bảng ánh xạ TX ↔ tên TX trong sổ ↔ hành trình

| Bài | Tên TX trong sổ (`JOURNEY-LEDGER.json`, trường `test_bo_sung`) | Hành trình (id trong sổ) |
| --- | --- | --- |
| TX-01 | — (không có) | `tuy-chon-game` và các màn dùng hợp đồng R02 |
| TX-02 | — (không có) | `tuy-chon-game` (form Options) |
| TX-03 | — (không có) | `safemode-va-auto-pickup`; `tuy-chon-game` |
| TX-04 | — (không có) | `cay-tui-va-vat-chua`; `quet-ban-do-va-bo-loc` |
| TX-05 | — (không có) | `study-zone` |
| TX-06 | — (không có) | `chon-engine-va-voice`; `thu-giong-va-bat-bo-doc` |
| TX-07 | — (không có) | `am-cuu-ho-khong-phu-thuoc-tts` |
| TX-08 | — (không có) | `nghe-thong-bao-moi` (đường âm/cue của R21) |
| TX-09 | — (không có) | `doc-o-hien-tai-va-xung-quanh`; `canh-bao-nguong-sinh-ton` |
| TX-10 | — (không có) | `smart-controller` |
| TX-11 | — (không có) | `study-zone` |
| TX-12 | — (không có) | `safemode-va-auto-pickup` |
| TX-13 | — (không có) | `safemode-va-auto-pickup` |
| TX-14 | `TX-may-tinh` | `may-tinh-trong-game` |
| TX-15 | `TX-minigame-kitten` | `minigame-kitten` |
| TX-16 | `TX-minigame-lightson` | `minigame-lightson` |
| TX-17 | `TX-minigame-minesweeper` | `minigame-minesweeper` |
| TX-18 | `TX-minigame-sokoban` | `minigame-sokoban` |
| TX-19 | `TX-minigame-snake` | `minigame-snake` |
| TX-20 | `TX-minigame-snake` (cùng hành trình; khía cạnh chính sách nhịp) | `minigame-snake` |
| TX-21 | — (không có; liên quan T52) | `phep-va-kha-nang` |
| TX-22 | — (không có) | `giao-viec-va-tra-dong-doi` |
| TX-23 | — (không có) | `trai-xay-dung-va-hau-can` |
| TX-24 | — (không có) | `backup-export-import` |
| TX-25 | — (không có) | `luu-tai-xoa-ban-luu`; `loi-ban-luu-va-phuc-hoi` |
| TX-26 | — (không có) | `lich-su-thong-bao` |
| TX-27 | — (không có; mở rộng T59) | `trung-tam-lenh-day-du` |
| TX-28 | — (không có; liên quan T38) | `canh-bao-nguong-sinh-ton` |
| TX-29 | `TX-ky-nang-proficiency` | `ky-nang-va-proficiency` |
| TX-30 | — (không có) | `doc-sach-va-hoc`; `che-tao-cong-thuc` |
| TX-31 | `TX-phau-thuat` (tên nêu trong `R17-BIONICS-MAGIC-COMPUTER.vi.md` mục 5; chưa có trong trường `test_bo_sung` của sổ JSON) | `bionics` |
| TX-32 | `TX-debug-menu`; `TX-editmap`; `TX-wish` | `debug-menu-va-console`; `editmap`; `wish-menu` |

Ghi chú ánh xạ: sổ `JOURNEY-LEDGER.json` hiện gắn đúng mười mã
`test_bo_sung` — `TX-ky-nang-proficiency`, `TX-may-tinh`,
`TX-minigame-kitten`, `TX-minigame-lightson`, `TX-minigame-minesweeper`,
`TX-minigame-snake`, `TX-minigame-sokoban`, `TX-debug-menu`,
`TX-editmap`, `TX-wish` — và cả mười mã đều có bài TX tương ứng ở trên.
Các bài TX còn lại là bài mới của R22 cho các hành trình vốn chỉ có mã T
hoặc chưa có mã bổ sung; cột "Tên TX trong sổ" ghi "— (không có)" cho
các bài đó thay vì bịa một mã sổ không tồn tại.

---

## 4. Điều kiện bắt đầu giai đoạn 2

Danh mục này chỉ được chạy khi đủ các điều kiện sau:

1. **Ứng viên được ghim ở R23.** Toàn bộ các bài chạy trên một ứng viên
   duy nhất đã chốt ở mục R23 (commit, cấu hình nội dung/mod phát hành,
   ABI, dải Android, chữ ký), kèm đối chiếu sổ hành trình cuối cùng.
   Không chạy rải rác trên các commit khác nhau rồi gộp kết quả; đổi ứng
   viên giữa chừng thì các bài đã chạy trên ứng viên cũ phải chạy lại
   hoặc ghi rõ phạm vi còn hiệu lực.
2. **Fixture đã dựng xong.** Mười lăm fixture F-01 đến F-15 ở mục 1.3 đã
   được tạo và kiểm chứng mô tả của chúng khớp dữ kiện thật trong game.
   Bài thiếu fixture là `BLOCKED`, không chạy thay bằng tình huống gần
   giống rồi ghi PASS.
3. **Điểm đứt đã có kết luận phân loại.** TX-21 (điểm đứt `cast_spell`
   của cây nền, R17 mục 3.4) được chạy sớm ở lớp host để phân loại: nếu
   glue chưa khôi phục, T52 và phần thi triển của TX-29/TX-30 liên quan
   được đánh dấu `BLOCKED` có lý do, thay vì chạy rồi gán FAIL mơ hồ cho
   bộ đọc.
4. **Thứ tự chạy đề xuất — host trước, thiết bị sau, nghiệm thu người
   dùng cuối cùng:**
   - Vòng 1 (host-C++): các bài bất biến và ranh giới thông tin
     (T12–T14, T20–T30, TX-01, TX-03–TX-05) và các bài engine
     (T31–T52, TX-10–TX-14, TX-21–TX-23, TX-28–TX-31).
   - Vòng 2 (host-Java): hàng đợi, validate gương, đầu vào
     (T04, T07, T10, T11, T60, TX-01, TX-06, TX-26).
   - Vòng 3 (Android-giả-lập): vòng đời, recreate, process death, file
     picker (T05, T15, T53–T55, T57, T58, TX-24, TX-25, TX-27, TX-32).
   - Vòng 4 (máy-thật-TalkBack): giọng Việt, âm cứu hộ, audio focus,
     cue/rung, phiên dài, gói phát hành (T01–T03, T06, T56, T61, T63,
     TX-06–TX-09, TX-20).
   - Vòng 5 (nghiệm-thu-người-dùng): T64 và các bài chơi trọn ván/trọn
     luồng trên máy thật của người dùng (TX-15–TX-20 và các hành trình
     nâng cao đã qua bốn vòng trước).
   Thứ tự này để lỗi rẻ và dễ truy vết (host) được bắt trước khi tốn
   phiên thiết bị và thời gian của người nghiệm thu; một bài FAIL ở
   vòng trước không bị "chạy bù" ở vòng sau để lấy PASS.
5. **Test doubles không thay thế kiểm thử thật.** Nhắc lại, không ngoại
   lệ: kiểm thử audio bằng backend giả, kiểm thử TalkBack bằng mô phỏng
   và kiểm thử người dùng bằng người sáng mắt nhắm mắt **không** thay
   được nghe thật bằng tai trên máy thật, TalkBack thật và nghiệm thu
   của người khiếm thị. Các bài ghi môi trường máy-thật-TalkBack hoặc
   nghiệm-thu-người-dùng chỉ được đóng khi đã chạy đúng lớp đó; kết quả
   host/emulator của cùng một hành trình chỉ là bằng chứng bổ sung.

---

## 5. PHẦN LỊCH SỬ — quy trình thử giọng trên APK thử giọng đời đầu (không áp dụng cho mã hiện tại)

> Mục này giữ **nguyên văn** toàn bộ nội dung của `TESTING.vi.md` trước
> đợt viết lại R22. Nó mô tả bản thử đời đầu chỉ có màn thử giọng native
> và các thành phần nền — **không áp dụng cho mã hiện tại**, vốn đã có bộ
> đọc gameplay đầy đủ và được kiểm thử theo các mục 1–4 ở trên. Giữ lại
> để tra cứu lịch sử; không sửa, không cập nhật nội dung trong mục này.

# Kiểm thử bản thử trợ năng Android đầu tiên

Tài liệu này dành cho flavor `accessibility`. Chỉ dùng APK ARM64 đầy đủ từ
workflow build candidate, không dùng APK UI-only của bài kiểm tra giả lập.
Mã ứng dụng: `com.cleverraven.cataclysmdda.accessibility`; tên trên launcher:
“Cataclysm DDA - Thử trợ năng”. Nó có vùng dữ liệu/nhà cung cấp tài liệu riêng,
không dùng chung bản lưu với stable/experimental.

Bản CI bàn giao dùng một chứng thư thử cố định, để các lần cập nhật của app thử
không buộc gỡ app. Đây là khóa phát triển công khai, không phải khóa phát hành.
Chỉ dùng APK có nguồn/commit/checksum được ghi rõ trong báo cáo bàn giao.

## Phạm vi của bản hiện tại

Hiện chỉ có màn thử giọng native và các thành phần nền. Chưa có bộ đọc bản đồ,
menu gameplay hoặc cử chỉ chơi độc lập. Giữ TalkBack khi thử màn này. Việc TTS
báo phát xong hoặc APK build thành công chưa chứng minh người dùng nghe được.

## Hành trình thử trên điện thoại

1. Giữ bản game đang dùng và bản lưu của nó. Cài APK thử riêng có định danh ở
   trên; không gỡ game cũ hoặc xóa dữ liệu để xử lý một lỗi cài đặt.
2. Bật TalkBack, mở “Cataclysm DDA - Thử trợ năng”. Ở cảnh báo trợ năng chọn
   “Thiết lập trợ năng và giọng đọc”. Nếu không tìm được mục này, ghi lại
   màn hình/câu đang nghe; chưa đánh dấu hành trình mở màn thử là đạt.
3. Nghe phần giải thích, trạng thái và tên từng nút. Kiểm tra có thể tới nút
   Phát thử, Dừng lời, cài đặt giọng, cài đặt trợ năng và Quay lại.
4. Giữ ô cho phép giọng cần mạng ở trạng thái bỏ chọn. Chọn phát thử. Phải nghe
   câu Việt hoặc thông báo lỗi rõ ràng, và có thể thử lại sau tối đa khoảng
   thời hạn khởi tạo. Thiếu voice Việt phải có đường mở cài đặt và quay về.
5. Nếu nghe được, ghi rõ đã nghe được bằng tai, tên engine/voice và có hiểu rõ
   từng từ không. Nếu có thông báo đã kết thúc nhưng không nghe âm, vẫn ghi là
   lỗi âm thanh, không coi là đạt.
6. Chọn Dừng lúc đang chuẩn bị hoặc đang đọc. Không có thao tác gameplay nào
   được thực hiện; màn hình vẫn dùng được. Khi chuẩn bị chưa xong, dừng lời
   không bắt buộc hủy cả quá trình khởi tạo engine.
7. Mở cài đặt giọng, quay lại; mở cài đặt trợ năng, quay lại. Không yêu cầu tắt
   TalkBack để trở về màn thử. Hành động mở cài đặt có fallback cho máy không
   nhận action giọng đọc trực tiếp; kiểm tra đường này trên từng hãng máy.
8. Đang thử giọng thì đưa app ra nền, khóa/mở khóa và trở lại. Không tự phát lại
   câu cũ. Nếu Android tạo lại Activity, màn mới phải về trạng thái chưa thử và
   ô giọng cần mạng phải bỏ chọn.
9. Chọn Quay lại. Phải trở về màn khởi động/cảnh báo và tiếp tục điều khiển được
   bằng TalkBack. Đừng đánh dấu phần chơi game đã tiếp cận từ bài thử này.

Nếu cần kiểm tra ngoại tuyến, trước hết xác nhận TalkBack của thiết bị dùng
được khi không có mạng. Thử với voice đã cài dữ liệu và không yêu cầu mạng;
kiểm tra bằng âm nghe thực tế, không chỉ dựa trên cờ metadata của engine.
Giọng online chỉ được dùng khi người thử chủ động chọn ô cho phép.

## Báo cáo tối thiểu

- APK/commit/checksum và nguồn tải.
- Hãng/mẫu máy, Android, engine/voice, trạng thái TalkBack và đường âm thanh.
- Bước đã thực hiện, điều mong đợi, câu thực tế nghe được hoặc lỗi nhận được.
- Có cần người sáng mắt trợ giúp không; nếu có, tại bước nào.
- Có trở về/khôi phục được không. Không cần gửi bản lưu cá nhân hoặc log không
  liên quan để chứng minh lỗi của màn thử giọng.

## Ý nghĩa các lớp kiểm thử tự động

- Host C++: sở hữu ngữ cảnh/token; chưa điều khiển game thật.
- Host Java: token và driver với test doubles; không emulation Android/audio.
- Compile API/Gradle: kiểu dữ liệu, Java và resource thật; chưa chứng minh UI chạy.
- Emulator: hai bài về nhãn/nút, lỗi/khởi tạo, dừng, recreate, opt-in mạng và thoát
  của riêng Activity thử giọng. APK UI-only không có engine game; emulator chạy
  không âm thanh. Không coi bài này là kiểm thử giọng Việt nghe thật hoặc TalkBack.
- APK ARM64 đầy đủ: kiểm tra build/đóng gói và bản dịch; còn cần chạy trên máy.
- Nghiệm thu người dùng: bằng chứng độc lập, bắt buộc cho mục tiêu chơi trọn vẹn.
