# Ma trận chứng cứ và nghiệm thu trợ năng Android

**Khóa tham chiếu:** `master` tại `f0be189ac5ebf000fc38e8c91fa3b42e325a7417`, kiểm tra 13/11/2026. [Trạng thái](STATUS.md) · [Đặc tả](CURRENT-REFERENCE.vi.md). Không suy từ CI thành nghiệm thu thực tế.

## 1. Kết quả đã quan sát, theo commit và môi trường

| Mã | Nguồn | Kết quả thực tế | Chứng minh / không chứng minh |
| --- | --- | --- | --- |
| CI-PY | [PR #7 workflow 43343666735](https://github.com/tvhuy99-web/Abcd-huy/actions/runs/43343666735) | 432 unit/source tests Python đạt | Kiểm tra hợp đồng và mẫu mã; không thực thi toàn bộ gameplay |
| CI-CORE | Cùng workflow, job inventory | 10 C++ core test cases, 77 assertion đạt | Kiểm tra primitive hẹp; không thay engine native hoàn chỉnh |
| CI-SPEECH | Cùng job | 24 assertion speech session, 35 assertion AndroidSpeech bằng test doubles | Không phát tiếng thực qua TTS Android |
| CI-APK | Cùng workflow, job Android ARM64 candidate | Thành công; APK kiểm tra các thư viện ELF64/asset/định danh/chữ ký | APK có thể đóng gói; không bảo đảm chạy trọn game trên máy thật |
| CI-MASTER | [matrix 43356871042](https://github.com/tvhuy99-web/Abcd-huy/actions/runs/43356871042) | Có nhiều job thất bại | Không kết luận Android APK bị lỗi từ riêng job desktop; cần phân tích từng nguyên nhân |
| CI-IWYU | [IWYU 43356871214](https://github.com/tvhuy99-web/Abcd-huy/actions/runs/43356871214) | Có lỗi gọi `diagnostic_event` ngoài phạm vi khai báo Android, cảnh báo unused | Cần sửa C++ đa nền tảng; chưa có báo cáo PASS cuối |
| CI-REL | [Experimental Release 43356871608](https://github.com/tvhuy99-web/Abcd-huy/actions/runs/43356871608) | Dịch thuật thiếu công cụ `msgcat` trên runner | Lỗi môi trường workflow phát hành, không khẳng định APK candidate PR #7 hỏng |
| CI-LINT | [Flake8 43356871046](https://github.com/tvhuy99-web/Abcd-huy/actions/runs/43356871046) | Không đạt lint | Cần xử lý phong cách/lỗi Python riêng, không chứng minh hành vi TTS |
| CI-CODEQL | [CodeQL 43356871039](https://github.com/tvhuy99-web/Abcd-huy/actions/runs/43356871039) | Action báo `This is a stub` | Không có kết quả phân tích mã hợp lệ |
| DEVICE | Không có hồ sơ chốt cho merge commit | **Chưa nghiệm thu toàn bộ** | Không gắn nhãn PASS hoặc “100%” |

Merge commit `f0be189` không có khác biệt tệp với commit cuối PR #7 `e58bda8`; tuy nhiên bản build kiểm chứng có định danh SHA của **PR head**. Khi phát hành mới từ `master`, cần tạo bằng chứng build gắn đúng commit phát hành.

## 2. Ma trận hành trình cần khóa trước khi công bố đủ dùng

| Nhóm | Bài bắt buộc | Bằng chứng còn thiếu |
| --- | --- | --- |
| Cài đặt lần đầu / mất giọng | Mở sạch, không có giọng Việt, chọn engine/voice, nghe thử, lưu, bật TalkBack | Video/log và phản hồi nghe thực tế |
| Menu chính / tạo nhân vật | Tạo thế giới có mod; đi qua các tab, nhập tên tiếng Việt; bắt đầu/tải lại | Thành công đầu-cuối trên máy |
| Menu/cử chỉ | Duyệt hàng, chạm đôi, quay lại, chạm giữ, 1–4 ngón; chuyển TalkBack giữa phiên | Không kích hoạt sai, không mất focus |
| Di chuyển | Vuốt nhanh nhiều lần, tường chặn, đổi vùng, chế độ combat/map, xe và tự đi | Mỗi cử chỉ được chấp nhận hoặc từ chối có phản hồi đúng, không bước muộn |
| Quét bản đồ | Quét 16 ô và xung quanh 20 ô/trục; đối chiếu quái/bẫy sau tường; đồ trên cùng ô | Chính sách thông tin ẩn được quyết định và kiểm tra |
| Sonar | Bật/tắt, cả quái và vật phẩm, 8 hướng, tai nghe/loa mono | Người nghe phân biệt hướng và nguồn |
| Đọc chi tiết | Chạm hai ngón đọc đồ/recipe/help/diary; văn bản trên 11 KB, UTF-8, đổi focus khi đang đọc | Không mất đoạn, không thao tác gameplay |
| Số lượng & pockets | Một món, nhiều món, nhiều ngăn, trùng tên, chọn 12 khi chỉ còn 5 | Số đúng, hủy đúng, không chuyển/nhặt trước xác nhận chung |
| Chiến đấu | Chọn mục tiêu, đồng minh/địch, súng/ném/phép, bắn hụt, safe mode | Không tấn công ngoài ý muốn, thông báo kết quả chuẩn |
| Sinh tồn | Ngủ bị ngắt, băng bó, chảy máu, ăn uống, đọc/chế tạo lâu | Cảnh báo khẩn và quay lại hoạt động |
| NPC / nhiệm vụ / xe | Giao dịch, túi NPC, ghi chú bản đồ, bắt đầu/lái/dừng xe | Cả tham số, hủy, kết quả và đường phục hồi |
| Lưu / khôi phục | Lưu, thoát, tải nhiều nhân vật; backup và restore bị gián đoạn | Không mất dữ liệu, thông báo lỗi đọc được |
| Biên lớn / mod ngoài | Hơn 128 lựa chọn, hơn 4096 node, mod có UI không chuẩn, minigame | Phân trang hoặc thông báo giới hạn rõ ràng |

Mỗi bài nghiệm thu phải ghi **APK SHA/commit, mẫu máy, Android/TalkBack/TTS engine, thao tác, kết quả engine, giọng đọc, PASS/FAIL/BLOCKED, log**. Bài test chỉ PASS ở môi trường thực sự đã chạy.

## 3. Chính sách phát hành và chống gắn PASS giả

1. Build Java và native ARM64 thành công với đúng commit định phát hành.
2. Kiểm tra đủ `libmain.so`, SDL3 dependencies, ngôn ngữ Việt, checksum, package ID và chữ ký.
3. Chạy đầy đủ bài kiểm tra nguồn; mô tả rõ bài nào kiểm tra thật Android và bài nào chỉ dùng double.
4. Chạy Android instrumentation và ít nhất các hành trình ưu tiên cao trên thiết bị; ghi bối cảnh và kết quả.
5. Không dùng tệp `TESTING.vi.md` tháng 10 như một sổ PASS hiện hành: nó là danh mục/kịch bản xây trước, có thể dùng tham chiếu T01–T64 nhưng cần sổ chạy mới.
6. Nếu thiếu bằng chứng, ghi **CHƯA XÁC NHẬN** thay vì tự ước lượng tỷ lệ hoàn thành.

## 4. Các lỗi cần sửa không bị đánh đồng

- Khai báo/điều kiện biên dịch `diagnostic_event` và `suppress_world_visuals` ở host có thể khác Android; kiểm chứng cả `__ANDROID__` và desktop.
- Workflow dịch cần cài gettext (`msgcat`) trước khi gọi `lang/compile_mo.py`.
- CodeQL trả stub là lỗi đường workflow/action, không phải một kết quả phát hiện vấn đề mã.
- Flake8 báo vi phạm Python có thể sửa riêng; lỗi đó không làm vô hiệu kết quả 432 bài kiểm tra nguồn đã chạy thành công trên commit PR #7.
- Workflow APK trợ năng không có `master` trong trigger `push`; cần cấu hình thêm hoặc chạy `workflow_dispatch` và ghim SHA trước khi gọi bản mới nhất.

Cập nhật ma trận mỗi khi mã/CI thay đổi, không biến kết quả tháng 11 thành trạng thái vĩnh viễn.
