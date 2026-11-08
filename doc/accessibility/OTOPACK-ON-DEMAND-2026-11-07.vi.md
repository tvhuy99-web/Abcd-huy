# Tự cài Otopack ngoài APK, thay tiếng tạm thời

## Nguồn và giới hạn tương thích

- Dùng mã nguồn Otopack + Mods Updates của Kenan2000 tại
  https://github.com/Kenan2000/Otopack-Mods-Updates
- **Pin commit** `508da9f0e38ec996f14c930c58848024680ca2be` để nội dung
  không tự thay đổi theo nhánh GitHub. Tải qua HTTPS từ GitHub codeload.
- README của tác giả nói rõ dự án **không còn cập nhật** và **không bảo đảm
  tương thích với Cataclysm: DDA hiện tại**. Dùng như bản thử nghiệm; không
  coi âm thanh có đầy đủ khi chưa nghe thử thực tế.
- Soundpack là tài nguyên người dùng tải từ tác giả, **không được gộp vào APK**.
  Kiểm tra quyền phân phối trước nếu sau này muốn tái phân phối.

## Quy trình mở game Android

1. Khi đã cài dữ liệu game và chuẩn bị khởi chạy, APK trợ năng kiểm tra
   `<externalFilesDir>/data/sound/Otopack+ModsUpdates/soundpack.txt`,
   `effects.json` và `player.json`.
2. Nếu Otopack đã có, bỏ qua tải và mở game bình thường.
3. Nếu chưa có, dùng Wi-Fi không đo cước để tải một lần, không chép file
   nén vào APK hoặc giữ file ZIP nguyên trên thiết bị.
4. Nếu mạng đang đo cước (bao gồm dữ liệu di động), yêu cầu xác nhận bằng
   màn có nút và TTS; không tự tải hơn 500 MB làm phát sinh chi phí.
5. Tiến độ phản ánh **dữ liệu đã giải nén** (MB và số tệp), có thể nhấn
   “Chơi tạm không có soundpack” bất cứ lúc nào.
6. Mọi tệp được ghi vào `data/sound/.reader_otopack_staging/` riêng.
   Bộ giải nén chỉ cho phép nội dung `Otopack+ModsUpdates/`; kiểm tra
   đường dẫn, CRC ZIP, giới hạn số tệp/kích thước và dung lượng còn trống.
   Sau khi kiểm tra ít nhất 1000 tệp âm và metadata, đổi tên staging thành
   `data/sound/Otopack+ModsUpdates` theo một thao tác trên cùng ổ lưu trữ.
7. Mất mạng, hết chỗ trống, bấm Bỏ qua hay gói lỗi đều không chạm vào
   `save`, `config`, `mods`, `gfx` hoặc những soundpack khác.
   Màn lỗi đọc được và cho chọn Thử lại/Chơi tạm.
8. C++ đăng ký soundpack từ `data/sound` rồi chọn `otopack` khi lựa chọn
   trước đó vẫn là `basic` / `Menu_Sound_Test`. Lựa chọn soundpack
   khác do người chơi đã thiết lập không bị ghi đè.
9. Tiếng bước chân mẫu `a11y_cue_step.wav` đã bỏ, cùng đường native
   phát `footstep`. Những cue trợ năng gốc vẫn còn để tùy ý bật
   nhưng mặc định trở lại **tắt**, không lẫn với soundpack.

## Kiểm thử trước khi phát hành

- CI: kiểm tra Python nguồn, biên dịch Java/AndroidTest, build C++/APK ARM64.
- Trên thiết bị: một lần Wi-Fi tự tải, đóng app giữa chừng, hết dung lượng,
  mất mạng, từ chối mạng di động, bỏ qua để chơi offline, mở lại để kiểm
  tra không tải lần thứ hai, nâng cấp APK mà không mất Otopack và save.
- Trong game: xác nhận `SOUNDPACKS=otopack`, hiệu ứng tiếng súng,
  môi trường, tiếng sinh vật, mở cửa và độ lớn âm lượng; kiểm tra TTS
  không bị nhạc nền che mất.
