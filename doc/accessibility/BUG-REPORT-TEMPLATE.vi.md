# Mẫu báo lỗi — CDDA bản trợ năng trên Android

Dùng mẫu này cho mọi báo lỗi của bản trợ năng.

Chép toàn bộ mẫu, điền vào các chỗ trong ngoặc vuông, rồi gửi kèm chẩn đoán đã nghe theo hướng dẫn trong `USER-GUIDE.vi.md`, mục 12.

Nguyên tắc khi điền:

- Ghi đúng điều bạn nghe được, không đoán điều lẽ ra phải nghe.
- Không rút gọn commit thành vài ký tự đầu nếu bạn có commit đầy đủ.
- Không đính kèm bản lưu cá nhân. Cam kết này có mục riêng ở cuối mẫu và là một phần của mẫu, không phải ghi chú tùy chọn.

---

## 1. Định danh bản dựng

- Commit đầy đủ của bản dựng đang thử: [điền hash commit đầy đủ]
- Nhánh: [ví dụ recovery/android-reader-ad5ecf0 — điền đúng nhánh của bản đã cài]
- Flavor: [accessibility]
- Mã ứng dụng: [com.cleverraven.cataclysmdda.accessibility]
- Tên tệp APK đã cài: [điền đúng tên tệp như trong hồ sơ ứng viên — không tự đặt lại]
- SHA-256 của APK: [điền đúng giá trị trong hồ sơ ứng viên của bản đó — không tự suy ra]
- Nguồn nhận APK: [điền nơi nhận, ví dụ hồ sơ ứng viên / người bàn giao]
- Ngày giờ cài hoặc nâng cấp: [điền ngày, giờ và múi giờ]
- Cài mới hay nâng cấp đè lên bản trước: [cài mới / nâng cấp đè]
- Nếu là nâng cấp đè: bản trước là commit nào, nếu biết: [điền hoặc ghi không biết]

## 2. Máy và hệ điều hành

- Hãng và mẫu máy: [điền]
- Phiên bản Android: [điền]
- Đường âm thanh đã dùng: [loa ngoài / tai nghe có dây / tai nghe Bluetooth / khác — ghi rõ]
- Dung lượng trống lúc gặp lỗi, nếu lỗi liên quan cài dữ liệu hoặc sao lưu: [điền hoặc ghi không rõ]

## 3. Giọng đọc

- Chế độ đọc đang dùng: [bộ đọc nội bộ / TalkBack / cả hai]
- Engine giọng đọc: [điền tên engine đã chọn trong thiết lập, hoặc ghi mặc định hệ thống]
- Giọng đã chọn: [điền tên giọng và ngôn ngữ, ví dụ giọng tiếng Việt vi-VN]
- Giọng đó là giọng ngoại tuyến hay giọng cần mạng, theo dòng trạng thái trong thiết lập: [ngoại tuyến / cần mạng / không rõ]
- Ô đồng ý giọng mạng đang bật hay tắt: [bật / tắt]
- Tốc độ đọc đã đặt: [điền, ví dụ 1,0 lần]
- Cue âm thanh đang bật hay tắt: [bật / tắt / không rõ]
- Cue rung đang bật hay tắt: [bật / tắt / không rõ]

## 4. TalkBack

- TalkBack đang bật hay tắt lúc gặp lỗi: [bật / tắt]
- Khám phá bằng chạm (touch exploration) đang bật hay tắt, nếu biết: [bật / tắt / không rõ]
- Phiên bản TalkBack hoặc bộ dịch vụ trợ năng, nếu biết: [điền hoặc ghi không biết]
- Lỗi còn xảy ra khi đổi trạng thái TalkBack không: [có / không / chưa thử]

## 5. Bước tái hiện

Viết từng bước, đánh số, theo đúng thứ tự đã làm. Mỗi bước một hành động.

1. [Bước 1]
2. [Bước 2]
3. [Bước 3]
4. [Thêm hoặc xóa dòng theo số bước thật]

Thêm các chi tiết sau nếu liên quan:

- Màn hình đang ở trước khi bắt đầu: [điền tên màn hình hoặc câu vừa nghe]
- Cử chỉ hoặc nút đã dùng ở bước gây lỗi: [điền đúng cử chỉ, trục vuốt, hướng, hoặc tên nút]
- Lỗi xảy ra ngay lần đầu hay phải lặp lại: [ngay lần đầu / sau số lần lặp — ghi số lần]
- Thoát ra rồi vào lại có hết lỗi không: [có / không / chưa thử]
- Đóng hẳn ứng dụng rồi mở lại có hết lỗi không: [có / không / chưa thử]

## 6. Mong đợi

[Điền điều bạn mong sẽ nghe hoặc sẽ xảy ra. Viết theo câu nghe được, ví dụ: mong nghe tên hướng và tiến độ luyện tập sau mỗi lần vuốt.]

## 7. Thực tế nghe được

[Điền đúng câu hoặc âm thanh bạn thật sự nghe được. Nếu không nghe thấy gì, ghi rõ là không nghe thấy gì và đã chờ bao lâu. Nếu nghe câu khác, chép lại câu đó càng đúng nguyên văn càng tốt. Nếu chỉ nghe được một phần, ghi phần nghe được và ghi rõ phần nào không nghe được.]

- Câu hoặc âm thanh thực tế: [điền]
- Có thông báo lỗi nào được đọc không: [có — chép lại / không]
- Màn hình còn dùng tiếp được sau lỗi không: [có / không]
- Bạn thoát khỏi tình trạng lỗi bằng cách nào: [điền, ví dụ dùng mục cứu hộ nào trong bảng điều khiển bộ đọc, bật TalkBack, hoặc nhờ người khác]

## 8. Mức độ

Chọn đúng một mức và xóa hai mức còn lại:

- [ ] Chặn chơi — không đi tiếp được, không tự thoát được, mất dữ liệu, hoặc mất đường nghe hoàn toàn.
- [ ] Khó chịu — vẫn chơi tiếp được nhưng phải đi đường vòng, nghe sai, nghe thiếu, phải lặp lại nhiều lần, hoặc tốn thêm nhiều thao tác.
- [ ] Nhỏ — sai sót không ngăn chơi và không làm mất thông tin chính, ví dụ lỗi diễn đạt hoặc một câu đọc chưa tự nhiên.

Lý do chọn mức này: [điền một hoặc hai câu]

## 9. Có cần người sáng mắt giúp không

- Có cần người sáng mắt giúp ở bước nào không: [không / có]
- Nếu có, giúp ở bước nào và giúp việc gì: [điền đúng bước trong mục 5 và việc đã nhờ, ví dụ nhìn tên nút trên màn hình vì không nghe được nhãn]
- Nếu không có người sáng mắt, bạn có tự hoàn thành được hành trình không: [có / không — ghi rõ dừng ở đâu]

## 10. Chẩn đoán đính kèm

Đường chẩn đoán hiện có trong mã là báo cáo đọc thành lời trong màn hình Thiết lập bộ đọc, mục Chẩn đoán, nút Đọc báo cáo chẩn đoán. Xem `USER-GUIDE.vi.md`, mục 12, và `R21-SPEECH-AUDIO-VI-DIAGNOSTICS.vi.md`, mục R21.4.

Báo cáo chỉ chứa mã và số. Báo cáo không chứa nội dung câu đã đọc, không chứa nội dung thông báo của game, và không chứa tên bản lưu, tên thế giới hay tên nhân vật. Báo cáo chỉ sống trong lần chạy hiện tại của ứng dụng và không ghi xuống đĩa, nên hãy nghe và ghi lại trước khi đóng ứng dụng.

Điền phần đã ghi lại được:

- Đã nghe báo cáo chẩn đoán trong cùng lần chạy với lỗi chưa: [đã nghe và ghi lại / chưa nghe / đã đóng ứng dụng nên mất báo cáo]
- Thời gian tiến trình đã chạy, theo báo cáo: [điền số giây đã nghe, hoặc ghi không ghi được]
- Lời đọc, theo báo cáo: đã đọc xong [ ], bị ngắt [ ], bị bỏ vì trùng [ ], bị bỏ vì hết hạn [ ], bị đẩy ra khi hàng đợi đầy [ ]
- Độ trễ bắt đầu đọc gần nhất và chậm nhất, theo báo cáo: [điền hai số mili giây đã nghe, hoặc ghi chưa đo được]. Lưu ý nguyên văn của báo cáo: đây chỉ là số đo; không có ngưỡng nào được tuyên bố.
- Thông báo trò chơi, theo báo cáo: thường [ ], khẩn cấp [ ]
- Ý định bị từ chối, theo báo cáo: đã cũ [ ], bị vô hiệu [ ], giá trị không hợp lệ [ ], không hỗ trợ [ ]
- Cue và sự cố cue, theo báo cáo: [điền các số đã nghe, hoặc ghi không dùng cue]
- Ba mươi sự kiện gần nhất, chép lại các mã sự kiện đã nghe: [điền các mã, giữ nguyên chữ tiếng Anh của mã]
- Hình thức gửi kèm: [chép tay vào báo lỗi này / ghi âm báo cáo / khác — ghi rõ]

## 11. Dữ liệu đính kèm và cam kết riêng tư

- Danh sách mọi thứ gửi kèm báo lỗi này: [liệt kê từng mục]
- Cam kết bắt buộc, giữ nguyên câu này trong báo lỗi đã gửi:

> Tôi không đính kèm bản lưu cá nhân trong báo lỗi này. Nếu người nhận cần bản lưu để tái hiện lỗi, người nhận phải yêu cầu riêng, nói rõ cần bản lưu nào và dùng vào việc gì, và tôi chỉ gửi sau khi đồng ý riêng cho yêu cầu đó.

- Nếu có tệp nào khác ngoài chẩn đoán ở mục 10 được gửi kèm, nó là gì và vì sao cần: [điền, hoặc ghi không có tệp nào khác]
- Trong báo lỗi này có chứa tên nhân vật, tên thế giới hoặc nội dung riêng tư nào không: [không / có — ghi rõ nằm ở mục nào để người nhận xử lý cẩn thận]

## 12. Thông tin bổ sung

[Bất kỳ điều gì khác giúp tái hiện lỗi mà các mục trên chưa có chỗ ghi. Nếu không có, ghi không có.]
