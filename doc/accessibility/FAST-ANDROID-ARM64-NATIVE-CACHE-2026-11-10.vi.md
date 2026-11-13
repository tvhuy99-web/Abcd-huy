# Đóng gói APK ARM64 nhanh bằng thư viện native đã kiểm chứng

> **GHI CHÚ PHIÊN BẢN (13/11/2026):** Đây là hồ sơ chuyên đề theo thời điểm viết, không tự động là chỉ dẫn cho `master` sau PR #7. Khi cử chỉ, menu, quyền đọc bản đồ hoặc CI khác, ưu tiên [đặc tả hiện hành](CURRENT-REFERENCE.vi.md), [hướng dẫn](USER-GUIDE.vi.md) và [trạng thái](STATUS.md). **Không coi mã đã viết là nghiệm thu TalkBack trên máy thật.**

## Mục tiêu và giới hạn

Workflow `.github/workflows/android-accessibility.yml` có hai đường:

- **Native không đổi**: nhận lại đúng `libmain.so` đã kiểm chứng trong APK
  thành công trước. Không chạy lại CMake/Ninja, chỉ chạy Gradle cho Java,
  tài nguyên, địa phương hóa, đóng gói và ký APK.
- **Native có đổi**: không bao giờ dùng bản cũ; chạy CMake/Ninja bình
  thường. Thử 3 luồng trên runner, dùng ccache 3 GiB và lưu tỷ lệ
  cache hit/miss để chẩn đoán.

Cả hai đều phải xác minh ELF64, tệp thư viện ARM64, assets tiếng Việt,
shader, package ID, versionCode tăng dần, chữ ký cố định và SHA-256 APK.
Cache không bao giờ có `restore-keys` đối với thư viện native: **chỉ
khôi phục khi mã fingerprint khớp hoàn toàn.**

## Tại sao fingerprint an toàn?

Fingerprint dùng `hashFiles` từ toàn bộ `src/**`, mã CMake dưới
`android/app/jni/**`, cấu hình Gradle Android và bản NDK 28 được khóa
trong tên cache. Vì vậy chỉ thay Java, TTS Java, bản dịch, dữ liệu và
đồ họa sẽ không đổi đầu vào native, cho phép dùng lại `libmain.so`.
Thay bất kỳ tệp C++/header hoặc cấu hình native nào, fingerprint
thay đổi và buộc biên dịch/kiểm thử libmain mới.

Khi biên dịch mới thành công, workflow trích `libmain.so` từ **chính
APK đã được xác minh** và đưa vào cache. Trong lần hit, Gradle nhận
`-PEXCLUDE_NATIVE_LIBS=true` đã có sẵn trong dự án, và dùng thư viện
ELF64 tại `android/app/src/main/jniLibs/arm64-v8a/libmain.so`.

Để native có cùng nhận diện giữa các APK Java-only, mã `VERSION`
nhúng trong thư viện được đặt theo fingerprint nguồn native, ví dụ
`native-ab12...`, **không phải SHA commit Java**. Song song, Gradle
nhận `-Paccessibility_apk_version=<git SHA rút gọn>` để `versionName`
trong APK và nhật ký vẫn nhận diện được **đúng commit Java đang chạy**.
APK mới có `versionCode` tăng đơn điệu để cập nhật bản đã cài;
`build-evidence/commit.txt` ghi SHA đầy đủ. Người kiểm thử có thể
phân biệt bản APK với fingerprint của `libmain.so` mà không mất
khả năng dùng lại thư viện cũ.

Không dùng cache native từ nhánh có nguồn khác; hash đã đảm bảo điều
kiện này. Phiên bản CMake/NDK hoặc phụ thuộc native thay đổi phải làm
mới khóa cache theo cấu hình workflow.

## Xem hiệu quả trên GitHub Actions

Tải artifact `android-arm64-candidate` và đọc:
- `build-evidence/native.txt`: fingerprint và chế độ
  `native_mode=prebuilt_verified` hoặc
  `native_mode=compile_and_seed_cache`.
- `build-evidence/ccache.txt`: mức tái sử dụng, số lượt biên dịch,
  giới hạn cache. Cache 3 GiB không có nghĩa tỷ lệ hit cao.
- `build-evidence/gradle.log`: thời gian nhiệm vụ Gradle/CMake.
- `build-evidence/package.txt`: package, versionCode và hash chứng chỉ.
- `build-evidence/SHA256SUMS`: bản chứng nhận APK sinh ra.

Sau lần biên dịch có mã native mới, lần tiếp theo chỉ thay Java hoặc
assets mới có thể chứng minh hiệu quả dùng `libmain.so` cũ. Không thể
đánh giá tốc độ của nhánh cache-hit từ một lần build full duy nhất.

## Giữ khả năng nâng cấp bản cài đặt

APK luôn dùng `com.cleverraven.cataclysmdda.accessibility` và cùng
khóa ký thử nghiệm đã ghim. `versionCode = 100000 + github.run_number`
được kiểm tra sau đóng gói. Không xuất APK nếu thiếu `libmain.so`,
thiếu tiếng Việt hoặc chữ ký không hợp lệ.

## Giới hạn tối ưu trong gameplay

Bộ nhớ đệm FOV và map shift phục vụ quái vật, vị trí và thông tin TTS,
không thể xóa chỉ để có số mili giây thấp. Bộ đo mới tách các bước
`WORLD_CACHE_PHASE`, `WORLD_SUBMAP_SHIFT_PHASE`,
`WORLD_AI_HOTSPOT` khi người dùng **bật nhật ký**. Khi tắt nhật ký,
không ghi hoặc đo những sự kiện này. Mọi thay đổi ảnh hưởng đến luật
đi theo lượt phải được đối chiếu với nhật ký Android thật.
