# Bản thử APK và kết quả kiểm tra kho ngày 24/10/2026

## Nguồn ứng viên

- Kho: `tvhuy99-web/Abcd-huy`.
- Nhánh: `fix/reader-audit-apk-20261024`.
- Nền đối soát: `3c556a7f40b8f1249eff10241fd5b60102121f7b`.
- Mã ứng dụng dựng APK: `75390a0612f60e5b97737c9e7ee63a64fbdbd444`.
- Mốc kiểm thử mở rộng: `cd15067fb257dd17317209be2e9de853a811fe8c`.
  Các thay đổi từ mốc APK tới mốc này chỉ thuộc workflow, harness, bài kiểm
  thử và fixture TEST_DATA; không đổi mã Java/C++ ứng dụng hoặc overlay Việt.

## Các vấn đề đã sửa

1. Đưa đủ 6 MP3 cứu hộ và 4 WAV âm hiệu do người dùng cung cấp vào kho.
2. Nối Áp dụng/Hủy nháp vào menu điều khiển nội bộ; đọc thay đổi giá trị
   nháp, giá trị đã áp dụng và lỗi trường. Giữ cả nút native cho TalkBack.
3. Dùng giao dịch thư mục chung cho installer và restore. Nếu phục hồi
   thất bại, giữ bản gốc và journal để thử lại; không xóa bản duy nhất.
4. Khóa phục hồi với việc khởi động engine ở mức toàn tiến trình, kiểm
   cả ở UI và lớp lưu trữ. Sau khi đã chơi phải mở tiến trình mới để phục hồi.
5. Phân trang Study Zone: 8 đồng đội × 32 kỹ năng, giữ ID theo dữ liệu thật,
   tránh vượt giới hạn 16 cột/4096 node của hợp đồng.
6. CMake theo dõi riêng overlay Việt. Bật sinh BuildConfig cho flavor Android.
7. Khôi phục đúng fixture `TEST_DATA/lang/po/ru.po` từ tổ tiên sạch
   `c5972f82fecd677c95e3fe12bcfcb13a2e1f193a` và dựng mọi PO của TEST_DATA;
   không nới hoặc bỏ assertion dịch thuật của `EOC_run_eocs`.
8. Runner emulator chạy tất cả lớp instrumentation, xác nhận số bài và
   lớp thực sự đã chạy. Thêm bài đi tới Apply/Discard qua controller thật.

Chi tiết lỗi gốc và phạm vi từng sửa ở
[REVIEW-2026-10-24.vi.md](REVIEW-2026-10-24.vi.md) và
[AUDIT-FIXES-2026-10-24.vi.md](AUDIT-FIXES-2026-10-24.vi.md).

## Kết quả đã quan sát

| Nhóm | Kết quả | Phạm vi |
| --- | --- | --- |
| Python inventory | PASS, 4 bài | Công cụ kiểm kê nguồn |
| C++ context core | PASS, 7 bài / 38 assertion | Context và token, chạy hẹp |
| SpeechSession | PASS, 24 assertion | Hàng đợi và vòng đời lời đọc |
| AndroidSpeech | PASS, 31 assertion | Driver với test doubles |
| ReaderStorageTest | PASS, 68 assertion | Lớp archive/giao dịch thật, tệp tạm, lỗi quyền ghi và restart ở từng ranh giới đổi tên |
| Catalog Việt | PASS | msgfmt và kiểm tra CMake dựng lại sau khi chỉ đổi overlay; tra được câu mới, rồi khôi phục nguồn/catalog sạch |
| Android API/resource | PASS | Biên dịch Java với Android SDK thật |
| Emulator API 35, x86_64 | PASS, 9 bài / 4 lớp | Startup, setup, cử chỉ và điều hướng Apply/Discard; dùng APK UI-only |
| Build C++ toàn cây | PASS | cata_test liên kết thành công với toàn bộ nguồn và bài test |
| C++ `[a11y]` tích hợp | PASS, 21 bài / 146 assertion | Dữ liệu engine thật, seed 2401024 |
| C++ toàn bộ mặc định | ĐANG CHẠY | Hai shard chính thức, 16 + 2120 = 2136 bài, không trùng/thiếu |
| APK ARM64 đầy đủ | PASS | Native, assets, âm thanh, catalog và chữ ký; không phải APK UI-only |

Các nhóm có thể kiểm tra trùng một hợp đồng; không cộng số assertion của
chúng thành độ phủ tổng. Tệp biên dịch tạm hỏng trên host đã được dựng lại;
không thay mã sản xuất hoặc giảm cảnh báo để vượt qua bước liên kết.

Host dùng CMake, GCC 13, ncurses, `TILES=OFF`, `CURSES=ON`, `SOUND=OFF`,
`TESTS=ON`, bản Debug và liên kết gold. Nội dung nạp mặc định là `dda` +
`test_data`; không có tùy chọn override hoặc mod bổ sung.

Lệnh chạy C++ từ thư mục kho, mỗi lệnh có `--user-dir` riêng:

```sh
./cata_test '[a11y]' --order lex --rng-seed 2401024 --user-dir=../test-evidence/a11y-user
./cata_test '[slow] ~starting_items' --order lex --rng-seed 2401024 --durations yes --user-dir=../test-evidence/slow-user
./cata_test '~[slow] ~[.],starting_items' --order lex --rng-seed 2401024 --durations yes --user-dir=../test-evidence/fast-user
```

Danh sách `--list-test-names-only` xác nhận hợp hai shard đúng bằng toàn bộ
2.136 ca mặc định. Các test ẩn `[.]`, cấu hình mod khác và fuzz nhiều seed
không được gọi là đã đạt từ lượt chạy này.

## APK bàn giao

- Tệp: `CDDA-tro-nang-arm64-2026-10-24.apk`.
- Dung lượng: 197.157.740 byte.
- Android tối thiểu: 7.0 / API 24. Kiến trúc: ARM64 (`arm64-v8a`).
- Mã ứng dụng: `com.cleverraven.cataclysmdda.accessibility`.
- Khóa: khóa thử ổn định đã công khai trong kho; chữ ký APK v2.
- SHA-256 APK: `0bb1470e740d520d2fe96ce24b79135b6326ea4c71b874d144100a2e2321f83e`.
- SHA-256 chứng thư: `4b2aa6143aee39bf179fecc853c803113ed8bb8c911b6251088ef945babf701f`.

Đã kiểm tra CRC toàn ZIP, checksum sau tải và chữ ký bằng apksigner trên
hai môi trường. Đã kiểm tra đủ 10 âm, catalog Việt, dữ liệu JSON và tất cả
DT_NEEDED của 6 thư viện ARM64; không có thư viện ngoài Android bị thiếu.

Lượt dựng APK:
<https://github.com/tvhuy99-web/Abcd-huy/actions/runs/40052043372>.
Job `android` PASS; run này còn chứa kết quả lỗi của runner UI đời trước
(đòi đúng 2 bài dù 3 bài đã đạt), nên không dùng màu tổng run làm kết luận.
Runner đã sửa và toàn bộ checks/UI mới PASS tại:
<https://github.com/tvhuy99-web/Abcd-huy/actions/runs/40053484879>.
APK trong artifact `reader-audit-arm64-apk`; bản người dùng nhận được có
cùng checksum với tệp gốc `cataclysmdda-75390a0-accessibility-arm64-v8a-debug.apk`.

## Những kết luận còn cần nghiệm thu

Chưa chạy game native trên điện thoại ARM64 thật trong môi trường này.
9 bài emulator không chạy engine game và không xác nhận giọng nghe thật,
TalkBack thật, âm/rung/audio focus hoặc gameplay trọn hành trình.

115 hành trình trong JOURNEY-LEDGER không tự chuyển sang PASS từ các kết
quả tự động trên. Cần thử T01–T64/TX với fixture, ghi oracle và thiết bị,
rồi nghiệm thu bởi người khiếm thị không nhận chỉ dẫn bằng mắt. Các phạm
vi còn mở trong REVIEW mục 3 (như đường xem cargo đầy đủ, metadata lệnh và
cấu hình mod phát hành) vẫn cần kết quả riêng. Chưa có chứng nhận người
khiếm thị đã chơi độc lập toàn game.
