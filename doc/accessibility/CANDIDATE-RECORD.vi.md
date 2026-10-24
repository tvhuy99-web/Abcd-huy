# Hồ sơ ứng viên cho giai đoạn 2 — quy trình lập hồ sơ (R22)

Tài liệu này là quy trình phải làm ở giai đoạn 2 và là khung hồ sơ cho từng bản ứng viên.

Trạng thái tại thời điểm viết: **chỉ có quy trình, chưa có ứng viên nào được lập hồ sơ theo quy trình này.** Chưa build, chưa tạo APK, chưa tính checksum, chưa cài, chưa chạy bài kiểm thử nào cho mục R22. Mọi ô số liệu trong khung hồ sơ ở mục 7 đều để trống có chủ đích: chỉ điền bằng giá trị đo hoặc tính được tại thời điểm thực hiện thật ở giai đoạn 2, kèm bằng chứng ở mục 5.

Nguồn phạm vi phải bám: `SCOPE-MANIFEST-2026-10-23.vi.md` (flavor và định danh ứng dụng ở mục 2 của manifest đó), `TEST_SIGNING.md` (khóa ký thử), `R20-SAVE-LIFECYCLE-ENDINGS.vi.md` (dữ liệu người chơi nằm ở đâu và vì sao nâng cấp giữ được), và `R21-SPEECH-AUDIO-VI-DIAGNOSTICS.vi.md` (hợp đồng nội dung của chẩn đoán).

Định danh phải kiểm lại từ nguồn tại commit ứng viên, không chép từ trí nhớ: flavor `accessibility`, mã ứng dụng `com.cleverraven.cataclysmdda.accessibility`, authority tài liệu `com.cleverraven.cataclysmdda.accessibility.documents`, tên launcher "Cataclysm DDA - Thử trợ năng". Nếu nguồn tại commit ứng viên khác các giá trị này, ghi giá trị thật tại commit đó và mở lại manifest phạm vi theo quy tắc ở mục 9 của manifest.

## 1. Ghim commit ứng viên và manifest nội dung

Làm đúng thứ tự sau cho mỗi ứng viên. Không bước nào được làm sau khi đã phát hành bản ứng viên cho người thử, vì hồ sơ phải mô tả đúng bytes đã đưa ra.

1. **Chốt cây nguồn.** Tại máy build, ghi lại: tên nhánh, hash commit đầy đủ của HEAD, và trạng thái cây làm việc có sạch hay không. Cây có thay đổi chưa commit thì ứng viên đó chưa được ghim: hoặc commit các thay đổi đó rồi ghim lại từ đầu, hoặc ghi rõ trong hồ sơ rằng bản dựng chứa thay đổi chưa commit kèm danh sách tệp đổi — lựa chọn thứ hai phải được người phụ trách giai đoạn 2 chấp thuận rõ trong hồ sơ, không mặc định.
2. **Ghi lệnh ghim đã dùng.** Ghi vào hồ sơ các lệnh đã chạy để lấy các giá trị trên, ví dụ `git rev-parse HEAD`, `git branch --show-current`, `git status --porcelain`, cùng ngày giờ và múi giờ lúc ghim. Giá trị ghi là đầu ra thật của lệnh, không gõ lại từ trí nhớ.
3. **Chụp manifest phạm vi.** Đối chiếu lại các mục của `SCOPE-MANIFEST-2026-10-23.vi.md` tại commit ứng viên: flavor, mã ứng dụng, SDK và ABI trong cấu hình build, ngôn ngữ, và danh sách chưa ghim. Mục nào đã đổi so với manifest thì ghi mục đó vào hồ sơ ứng viên kèm giá trị mới và nguồn trong cây tại commit ứng viên. Mục nào vẫn chưa ghim thì ghi nguyên trạng là chưa ghim, không điền giá trị đoán.
4. **Lập manifest nội dung của bản dựng.** Sau khi bản dựng hoàn tất ở giai đoạn 2, liệt kê mọi tệp cấu thành bản bàn giao của ứng viên, từng tệp một: đường dẫn trong cây build hoặc trong thư mục bàn giao, tên tệp chính xác như trên đĩa, và vai trò của tệp (APK theo từng ABI nếu có nhiều APK, tệp dữ liệu đi kèm nếu có, tệp manifest này, tệp checksum ở mục 2). Không liệt kê tệp không tồn tại. Số lượng APK và tên từng APK chỉ ghi sau khi build thật cho ra đúng các tệp đó.
5. **Phân biệt cấu hình build với dải đã chứng nhận.** Các số SDK và ABI trong cấu hình build chỉ nói bản dựng nhắm tới đâu. Chúng không phải dải máy và Android đã chứng nhận. Dải đã chứng nhận của ứng viên chỉ gồm đúng các máy đã thật sự chạy bằng chứng ở mục 5, liệt kê trong hồ sơ.

## 2. Tính và ghi checksum của APK và tệp dữ liệu

Quy trình này áp dụng cho mọi tệp trong manifest nội dung ở mục 1, bước 4 — không chỉ APK.

1. **Thuật toán.** Dùng SHA-256 cho mọi tệp. Ghi rõ thuật toán trong hồ sơ; không trộn thuật toán giữa các tệp của cùng một ứng viên.
2. **Cách tính.** Tính trên chính tệp sẽ bàn giao, sau khi mọi bước đổi tên hoặc đóng gói lại đã xong, bằng một lệnh ghi được vào hồ sơ, ví dụ `sha256sum <tệp>` trên Linux hoặc lệnh tương đương trên hệ build. Đổi tên tệp sau khi tính thì tính lại và ghi lại, dù nội dung không đổi, để tên trong hồ sơ khớp tên trên đĩa.
3. **Cách ghi.** Với mỗi tệp, hồ sơ ghi đủ bốn giá trị lấy tại lúc tính: tên tệp chính xác, kích thước theo byte, giá trị SHA-256 đầy đủ, và đường dẫn hoặc vị trí lưu tệp đó. Bốn giá trị này để trống cho tới khi tính thật; khung ở mục 7 không có sẵn bất kỳ giá trị nào.
4. **Kiểm lại độc lập.** Trước khi bàn giao, tính lại checksum của từng tệp một lần nữa từ bản sao sẽ thật sự gửi đi hoặc tải lên, và đối chiếu từng ký tự với giá trị đã ghi. Kết quả đối chiếu (khớp hoặc không khớp) và người đối chiếu được ghi vào hồ sơ. Không khớp thì dừng bàn giao ứng viên đó, tìm nguyên nhân, và lập lại từ mục 1 nếu bytes đã đổi.
5. **Tệp dữ liệu trong APK.** Nếu cần checksum của tệp dữ liệu nằm bên trong APK (ví dụ catalog tiếng Việt đã biên dịch), ghi rõ đó là checksum của tệp trích ra từ APK nào, trích bằng cách nào, và đường dẫn của tệp bên trong APK. Không ghi checksum của tệp nguồn trong repo thay cho checksum của tệp trong APK.

## 3. Khóa ký

Sự thật hiện tại, theo `SCOPE-MANIFEST-2026-10-23.vi.md` mục 2 và mục 8, và theo `TEST_SIGNING.md`:

- Flavor trợ năng hiện dùng `accessibility-test.keystore` qua cấu hình ký `accessibilityTest`. Đây là **khóa phát triển công khai** dùng cho bản thử, để các bản thử cài đè lên nhau mà không phải gỡ ứng dụng. Khóa này không phải khóa phát hành, và không được dùng cho bản phân phối chính thức đáng tin.
- Quyết định khóa phát hành **chưa được ghi trong nguồn và chưa ghim**. Quyết định đó thuộc R23 và giai đoạn phát hành, không thuộc hồ sơ ứng viên của giai đoạn 2 và không được quyết thay trong tài liệu này.

Quy trình ghi khóa ký cho mỗi ứng viên ở giai đoạn 2:

1. Ghi cấu hình ký đã thật sự dùng cho bản dựng đó (tên cấu hình ký trong build, không ghi mật khẩu và không ghi nội dung khóa vào hồ sơ).
2. Ghi vân tay chứng thư của APK đã dựng, lấy từ chính tệp APK bằng công cụ kiểm tra chữ ký tại thời điểm lập hồ sơ (ví dụ lệnh kiểm tra chứng thư của `apksigner` hoặc `keytool` — ghi lệnh đã dùng vào hồ sơ), và đối chiếu với vân tay kỳ vọng của khóa thử ghi trong `TEST_SIGNING.md`. Khớp hay không khớp đều ghi kết quả đối chiếu; không khớp thì dừng bàn giao và ghi rõ.
3. Nếu một ứng viên nào dùng khóa khác khóa thử, hồ sơ phải nêu khóa đó là khóa gì, ai quyết định, và quyết định đó nằm ở tài liệu nào. Không có tài liệu quyết định thì không đổi khóa.

## 4. Quy trình nâng cấp APK giữ bản lưu và thiết lập — kiểm tra T63

Mục đích: chứng minh cài bản ứng viên mới đè lên bản ứng viên cũ, cùng định danh và cùng khóa ký, mà không mất bản lưu và thiết lập. Đây là quy trình phải làm ở giai đoạn 2 cho bài T63; tại thời điểm viết tài liệu này, quy trình **chưa thực hiện**.

Cơ sở từ mã, theo `R20-SAVE-LIFECYCLE-ENDINGS.vi.md` mục R20.3: mọi dữ liệu người chơi nằm dưới thư mục tệp ngoài của ứng dụng (`save/`, `config/`, `memorial/`, `achievements/`, `templates/`), nằm ngoài ba cây `data`, `gfx`, `lang` mà quá trình cài dữ liệu của ứng dụng hoán đổi; nhật ký cài đặt nằm ở bộ nhớ trong và sống qua cập nhật. Rủi ro thật đã ghi trong tài liệu đó là **gỡ cài đặt** — gỡ ứng dụng xóa cả thư mục của ứng dụng — chứ không phải nâng cấp đè. Vì vậy quy trình dưới đây cấm gỡ ứng dụng và cấm xóa dữ liệu ứng dụng ở mọi bước.

Các bước phải làm, theo thứ tự:

1. **Chuẩn bị bản cũ.** Cài bản ứng viên cũ (hoặc bản thử trước đó, nếu bài kiểm yêu cầu) bằng đúng định danh ở đầu tài liệu này. Ghi commit, tên tệp APK và SHA-256 của bản cũ vào hồ sơ, theo mục 1 và mục 2.
2. **Tạo trạng thái phân biệt được.** Trong bản cũ, tạo dữ liệu kiểm tra riêng cho bài này: ít nhất một bản lưu kiểm tra đặt tên nhận diện được là của bài T63, và ít nhất một thiết lập đã đổi khỏi mặc định và ghi lại giá trị đã đổi vào hồ sơ. Không dùng bản lưu cá nhân của người chơi thật cho bài này.
3. **Chụp trạng thái trước nâng cấp.** Ghi lại: danh sách bản lưu nhìn thấy trong danh sách tải kèm thông tin đọc được của bản lưu kiểm tra (thế giới, thời lượng nếu có, lần lưu cuối), giá trị thiết lập đã đổi ở bước 2, và lựa chọn giọng đã lưu. Nếu có thể, xuất thêm một bản sao lưu theo quy trình sao lưu trong `USER-GUIDE.vi.md` mục 11 và giữ tệp đó ngoài ứng dụng, để có đường khôi phục nếu nâng cấp làm hỏng dữ liệu kiểm tra.
4. **Nâng cấp đè.** Cài bản ứng viên mới đè lên bản cũ bằng thao tác cài đặt cập nhật thông thường của Android. Không gỡ bản cũ trước. Không xóa dữ liệu hoặc bộ nhớ đệm của ứng dụng. Không đổi định danh và không đổi khóa ký giữa hai bản; bản mới phải ký bằng cùng khóa đã ghi ở mục 3, nếu không hệ điều hành sẽ từ chối cập nhật và bài kiểm ghi kết quả đó là lỗi quy trình, không phải lỗi đạt.
5. **Chụp trạng thái sau nâng cấp.** Mở bản mới và ghi lại đúng các mục ở bước 3: bản lưu kiểm tra còn trong danh sách tải không và thông tin đọc được có khớp không, bản lưu kiểm tra có tải vào chơi được không, giá trị thiết lập đã đổi còn nguyên không, lựa chọn giọng đã lưu còn nguyên không, và quá trình cài dữ liệu của bản mới có báo lỗi không. Mọi khác biệt đều ghi nguyên văn câu đã nghe hoặc thông báo đã hiện.
6. **Kết luận của bài.** Ghi kết luận đạt hoặc chưa đạt cho T63 kèm liên kết hoặc đường dẫn tới bằng chứng ở mục 5. Chưa đạt thì ghi rõ mất mát ở bước nào, dữ liệu kiểm tra có khôi phục được từ bản sao lưu ở bước 3 không, và không xóa dấu vết trước khi hồ sơ được người phụ trách xem.

## 5. Bằng chứng phải thu thập cho mỗi bài kiểm T và TX

Mỗi bài kiểm trong giai đoạn 2 (các bài T và các bài mở rộng TX) lập một mục bằng chứng riêng. Thiếu một trong các nhóm dưới đây thì bài kiểm đó ghi là chưa đủ bằng chứng, không ghi đạt.

1. **Định danh bản dựng.** Hash commit đầy đủ, nhánh, flavor, tên tệp APK chính xác và SHA-256 của APK — đúng các giá trị trong hồ sơ ứng viên ở mục 1 và mục 2, không ghi lại theo trí nhớ.
2. **Máy.** Hãng và mẫu máy, phiên bản Android, và đường âm thanh đã dùng (loa ngoài, tai nghe có dây, tai nghe Bluetooth). Bài kiểm chạy trên giả lập thì ghi rõ là giả lập, kèm API của giả lập, và không được tính là bằng chứng máy thật.
3. **Giọng.** Engine, giọng và ngôn ngữ đã chọn, giọng là ngoại tuyến hay cần mạng theo dòng trạng thái trong thiết lập, ô đồng ý giọng mạng bật hay tắt, và tốc độ đọc.
4. **TalkBack.** TalkBack bật hay tắt, khám phá bằng chạm bật hay tắt nếu bài kiểm liên quan cử chỉ, và phiên bản TalkBack nếu biết.
5. **Bằng chứng nghe được.** Đây là phần bắt buộc riêng của dự án này: câu hoặc âm thanh người kiểm thật sự nghe được ở từng bước quyết định của bài, chép càng đúng nguyên văn càng tốt; bước nào không nghe thấy gì thì ghi rõ không nghe thấy gì và đã chờ bao lâu. Kết quả trên màn hình, trạng thái trong mã, hoặc log kỹ thuật không thay thế bằng chứng nghe được. Nếu có ghi âm, ghi đường dẫn tệp ghi âm vào hồ sơ và nơi lưu tệp đó theo mục 6.
6. **Diễn biến và kết quả.** Các bước đã làm theo thứ tự, điều mong đợi, điều thực tế xảy ra, kết luận đạt hoặc chưa đạt, và — nếu chưa đạt — liên kết tới báo lỗi đã lập theo `BUG-REPORT-TEMPLATE.vi.md`, kèm chẩn đoán đã ghi lại theo hợp đồng ở `R21-SPEECH-AUDIO-VI-DIAGNOSTICS.vi.md` mục R21.4 (chỉ mã và số, không nội dung câu đọc, không tên bản lưu cá nhân).
7. **Người sáng mắt.** Có cần người sáng mắt giúp không, và nếu có thì ở bước nào và giúp việc gì. Bài kiểm cần người sáng mắt ở bước thuộc phạm vi bài đó thì không ghi là chơi độc lập ở bước đó.

Không đính kèm bản lưu cá nhân vào bằng chứng. Dữ liệu kiểm tra tạo riêng cho bài kiểm thì ghi tên nhận diện của nó; bản lưu cá nhân chỉ gửi khi có yêu cầu riêng theo cam kết trong mẫu báo lỗi.

## 6. Nơi lưu hồ sơ

- Mỗi ứng viên có **một tệp hồ sơ riêng**, tạo bằng cách chép khung ở mục 7 của tài liệu này. Tên tệp đặt theo ngày lập hồ sơ và đoạn ngắn đầu của hash commit ứng viên, đặt trong thư mục `doc/accessibility/candidate-records/` của repo, trên cùng nhánh với commit ứng viên. Thư mục này được tạo ở lần lập hồ sơ ứng viên đầu tiên ở giai đoạn 2; tại thời điểm viết tài liệu này thư mục chưa có, vì chưa có ứng viên nào.
- Tài liệu quy trình này (`CANDIDATE-RECORD.vi.md`) ở nguyên tại `doc/accessibility/` và không bị ghi đè bởi hồ sơ của từng ứng viên.
- Tệp APK và các tệp dữ liệu của ứng viên **không** chép vào thư mục hồ sơ trong repo. Hồ sơ chỉ ghi tên tệp, kích thước, SHA-256 và vị trí lưu thật của chúng (ví dụ liên kết artifact của lần build hoặc thư mục bàn giao), theo mục 2.
- Bằng chứng nghe được dạng ghi âm và mọi tệp bằng chứng cồng kềnh cũng lưu ngoài repo, tại vị trí ghi trong hồ sơ, và hồ sơ ghi đường dẫn hoặc liên kết tới chúng. Phần bằng chứng dạng chữ của mỗi bài kiểm ghi ngay trong tệp hồ sơ của ứng viên.
- Hồ sơ của ứng viên đã bàn giao thì không sửa lặng lẽ. Sai sót phát hiện sau bàn giao được ghi thêm thành mục đính chính có ngày giờ, giữ nguyên nội dung cũ.

## 7. Khung hồ sơ cho một ứng viên (chép nguyên, điền tại giai đoạn 2)

Các ô dưới đây để trống có chủ đích. Không điền giá trị nào trước khi đo hoặc tính thật.

### 7.1 Định danh

- Ngày giờ lập hồ sơ và múi giờ: [ ]
- Người lập hồ sơ: [ ]
- Nhánh: [ ]
- Hash commit đầy đủ: [ ]
- Cây làm việc sạch hay có thay đổi chưa commit: [ ]
- Các lệnh ghim đã chạy: [ ]
- Flavor: [ ]
- Mã ứng dụng tại commit này, kiểm lại từ nguồn: [ ]
- Mục nào của manifest phạm vi đã đổi so với `SCOPE-MANIFEST-2026-10-23.vi.md`: [không đổi / liệt kê]

### 7.2 Manifest nội dung và checksum (SHA-256)

Lặp lại khối này cho từng tệp trong bản bàn giao:

- Tên tệp chính xác: [ ]
- Vai trò: [APK / tệp dữ liệu / khác]
- Đường dẫn hoặc vị trí lưu: [ ]
- Kích thước theo byte: [ ]
- SHA-256: [ ]
- Lệnh đã dùng để tính: [ ]
- Đối chiếu độc lập trước bàn giao: [khớp / không khớp], người đối chiếu: [ ], ngày giờ: [ ]

### 7.3 Khóa ký

- Cấu hình ký đã dùng: [ ]
- Lệnh kiểm tra chứng thư đã dùng: [ ]
- Vân tay chứng thư lấy từ APK: [ ]
- Đối chiếu với vân tay khóa thử trong `TEST_SIGNING.md`: [khớp / không khớp]
- Khóa khác khóa thử, nếu có: [không / ghi rõ khóa, người quyết định, tài liệu quyết định]

### 7.4 Nâng cấp giữ dữ liệu (T63)

- Bản cũ: commit [ ], tên tệp APK [ ], SHA-256 [ ]
- Bản lưu kiểm tra đã tạo: [tên nhận diện]
- Thiết lập đã đổi và giá trị: [ ]
- Trạng thái trước nâng cấp (theo mục 4, bước 3): [ ]
- Cách cài bản mới đè lên bản cũ: [ghi thao tác thật đã dùng]
- Trạng thái sau nâng cấp (theo mục 4, bước 5): [ ]
- Kết luận T63: [đạt / chưa đạt], bằng chứng: [đường dẫn hoặc mục trong hồ sơ này]

### 7.5 Bằng chứng từng bài kiểm

Lặp lại khối này cho từng bài T hoặc TX đã chạy trên ứng viên này:

- Bài kiểm: [T.. / TX..]
- Máy hoặc giả lập: [ ]
- Giọng: [ ]
- TalkBack: [ ]
- Bằng chứng nghe được: [ ]
- Kết luận: [đạt / chưa đạt / chưa đủ bằng chứng]
- Báo lỗi liên quan, nếu có: [đường dẫn hoặc liên kết]

### 7.6 Đính chính

- [Ngày giờ] — [nội dung đính chính] — [người ghi]
