# APK thử nghiệm có thể cập nhật đè

Nhánh build: `integration/reader-vietnamese-20261107`.
Workflow: `.github/workflows/android-accessibility.yml`.

- Gói Android phải giữ nguyên: `com.cleverraven.cataclysmdda.accessibility`.
- Bản thử nghiệm ARM64 được ký bằng **cùng** file `android/app/accessibility-test.keystore` đã có trong repo; workflow bắt buộc bật `-Paccessibility_test_signing=true`.
- SHA-256 chứng chỉ được xác nhận bằng `apksigner` và đối chiếu với `android/app/accessibility-test-cert.sha256` trước khi phát hành.
- `versionCode` được cấp bởi `100000 + github.run_number`. Mỗi lần workflow chạy mới, số này tăng; không quay về mã mặc định `1`.
- Workflow từ chối APK thiếu `lib/arm64-v8a/libmain.so` hoặc bản dịch tiếng Việt, sai tên gói, sai versionCode, sai chữ ký.
- APK và `SHA256SUMS`, `package.txt`, `signer.txt` nằm trong artifact `android-arm64-candidate` trên GitHub Actions.
- Đây là khóa **thử nghiệm được công khai trong repo**, không phải khóa phát hành chính thức hoặc chứng chỉ riêng tư. Không dùng để phân phối trên Play Store.

**Cập nhật đè:** Android chỉ cho phép cài đặt đè khi `applicationId` giống nhau, certificate ký giống nhau, và phiên bản không bị giảm. Nếu thiết bị có bản `.experimental` hoặc bản `.accessibility` ký bằng chứng chỉ khác, Android không cho cập nhật đè, bất kể tên tệp APK. Không chuyển khóa ký sang khóa của ứng dụng khác bởi điều đó sẽ làm mất khả năng ghi đè bản hiện tại.

**Thử trên máy:** mở Actions của repo, chọn run của workflow `Android accessibility baseline and candidate`, tải artifact `android-arm64-candidate`, giải nén ZIP và cài APK. Không cần gỡ bản Accessibility cũ đã ký bằng cùng chứng chỉ.
