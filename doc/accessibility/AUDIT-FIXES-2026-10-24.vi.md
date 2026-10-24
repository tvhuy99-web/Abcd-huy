# Sửa lỗi sau kiểm tra kho, chuẩn bị kiểm thử và APK

Ngày 24/10/2026. Nền: 3c556a7f40b8f1249eff10241fd5b60102121f7b.
Nhánh: fix/reader-audit-apk-20261024.

## Các thay đổi

- Bổ sung đủ 6 MP3 cứu hộ và 4 WAV âm hiệu từ gói người dùng gửi; mã kiểm tra,
  dung lượng và thời lượng ở AUDIO-MANIFEST-2026-10-24.json.
- Menu điều khiển nội bộ có Áp dụng/Hủy nháp cho trường đang chọn; giá trị
  nháp/đã áp dụng và lỗi trường được đọc khi thay đổi.
- Dùng giao dịch thư mục chung cho installer và restore: ghi journal bền
  trước khi đổi dữ liệu, giữ nguyên rollback/journal nếu phục hồi thất bại;
  journal lạ hoặc thiếu nhưng còn rollback cũng giữ dữ liệu và chặn tiếp tục.
- Giữ toàn bộ nội dung gốc trong rollback đến khi commit, kể cả custom asset;
  copy custom sang cây mới thay vì chuyển khỏi bản gốc trước commit.
- Loại trừ lẫn nhau giữa khởi động engine và restore. Sau khi engine từng
  chạy trong tiến trình, phải mở tiến trình mới trước khi restore.
- Study Zone chia nhóm 8 NPC và 32 kỹ năng, ID vẫn theo NPC/kỹ năng thật.
- CMake theo dõi overlay tiếng Việt; khôi phục kế hoạch hiện hành từ backlog,
  ghi rõ đây là bản tái dựng 2.1, không giả là bản 2.0 khớp byte đã mất.

## Kiểm tra đã chạy trước khi đẩy

- ReaderStorageTest: 18 assertion đạt, gồm lỗi rollback/quyền ghi, journal
  lạ/thiếu, phục hồi có thể thử lại, archive round trip và cổng phiên game.
- Inventory scanner: 4 bài đạt. SpeechSession: 24 assertion đạt; AndroidSpeech
  với test doubles: 31 assertion đạt.
- Tất cả âm mở được bằng ffprobe. Catalog Việt qua msgfmt --check (cảnh báo
  header không làm hỏng catalog).
- Biên dịch cata_test đầy đủ và kiểm tra Android/APK đang được thực hiện;
  không có kết luận PASS cho việc chưa hoàn tất.

Bài tái hiện ban đầu của báo cáo audit dùng chuỗi SWAPPING (journal không
nhận diện) thay vì BACKUP_SWAPPING của lớp archive. Đã chỉnh lại và chạy
trực tiếp lớp ở commit nền với BACKUP_SWAPPING: bản gốc vẫn xóa save gốc
và journal khi rename thất bại. Kiểm thử hồi quy của bản sửa dùng đúng
trạng thái và xác nhận giữ dữ liệu, giữ journal, rồi thử lại thành công.

## Phạm vi bàn giao

Workflow reader-audit-build chạy checks, build Android Java/resource/native,
APK ARM64 ký bằng khóa thử ổn định của repo, kiểm nội dung/chữ ký/checksum,
và build/chạy host suite với seed cố định. Đây là APK thử, không phải chứng
nhận người khiếm thị đã nghiệm thu hoặc bản phát hành production.

Các hành trình T01–T64/TX, âm thực tế, TalkBack, vòng đời trên máy thật và
người dùng vẫn phải có kết quả riêng; không chuyển 115 hành trình sang PASS
chỉ từ build hoặc các bài host.
