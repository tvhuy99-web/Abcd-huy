# Asset âm cứu hộ R06.3 — transcript, nguồn và kiểm chứng tệp

Các tệp trong `android/app/src/main/res/raw/`, phát qua
`RescueAudioPlayer` khi TTS lỗi/thiếu giọng. Đây là âm ghi sẵn thật
(có tiếng nói, đo được thời lượng), **không phải tệp câm/placeholder**.

## Nguồn giọng — nói đúng sự thật

- Kế hoạch đầu tiên là giọng trực tuyến vi-VN của Microsoft qua
  `edge-tts` (đã cài vào `~/workspace/tts-venv`). **Không dùng được
  trong môi trường này**: kết nối trực tiếp tới
  `speech.platform.bing.com:443` lỗi `SSL: WRONG_VERSION_NUMBER`, đi
  qua proxy egress thì treo tới hết thời gian chờ (đã thử cả hai,
  tệp ra 0 byte). Ghi lại để giai đoạn sau khỏi thử lại mò.
- Các tệp hiện tại được sinh **ngoại tuyến bằng eSpeak NG 1.51**
  (`espeak-ng -v vi -s 140`, giọng `vi` — Vietnamese (Northern)), xuất
  WAV rồi nén MP3 mono 48 kbit/s bằng ffmpeg, ngày 2026-10-23.
- Hệ quả trung thực: giọng eSpeak NG là giọng formant, nghe máy móc,
  **không** bằng giọng thần kinh của Microsoft. Độ rõ khi nghe thật
  phải nghiệm thu ở giai đoạn 2/3 trên máy thật; khi có đường sinh
  giọng tự nhiên (edge-tts ở môi trường mạng khác, hoặc thu âm
  người), chỉ cần thay tệp cùng tên `R.raw.*` — mã không đổi.
- Quyền dùng: eSpeak NG là phần mềm nguồn mở (GPL-3.0). Âm sinh ra
  bằng công cụ này để đóng gói trong APK là quyết định phát hành;
  ghi ở đây để người phát hành xác nhận ở giai đoạn phát hành, không
  tự khẳng định thay.

## Danh sách tệp (đo bằng ffprobe tại máy này, 2026-10-23)

| Tệp | Hằng RescueAudioPlayer | Thời lượng | Dung lượng | Codec |
| --- | --- | --- | --- | --- |
| `a11y_rescue_tts_failed.mp3` | `CLIP_TTS_FAILED` | 24,99 giây | 150.535 byte | mp3, mono 48 kbit/s |
| `a11y_rescue_retrying.mp3` | `CLIP_RETRYING` | 3,91 giây | 24.050 byte | mp3, mono 48 kbit/s |
| `a11y_rescue_install_failed.mp3` | `CLIP_INSTALL_FAILED` | 16,13 giây | 97.402 byte | mp3, mono 48 kbit/s |
| `a11y_rescue_no_space.mp3` | `CLIP_NO_SPACE` | 16,56 giây | 99.910 byte | mp3, mono 48 kbit/s |
| `a11y_rescue_welcome.mp3` | `CLIP_WELCOME` | 11,30 giây | 68.406 byte | mp3, mono 48 kbit/s |
| `a11y_rescue_setup_failed.mp3` | `CLIP_SETUP_FAILED` | 17,87 giây | 107.747 byte | mp3, mono 48 kbit/s |

`CLIP_WELCOME` đã có asset và hằng số nhưng **chưa được nối** vào
luồng phát nào: màn chào hiện tại đi qua self-voice/TTS khi khả dụng,
và phát clip chào vô điều kiện có thể chồng tiếng; điểm nối dự kiến
là cùng màn cứu hộ (chỉ phát khi TTS lỗi) — để mở ở R06.3 cho tới khi
có quyết định nối, không ghi là đã dùng.

## Transcript đầy đủ

- **a11y_rescue_tts_failed:** "Trò chơi chưa đọc được bằng giọng tổng
  hợp. Giọng tiếng Việt có thể bị thiếu hoặc dịch vụ đọc đang lỗi.
  Bạn vẫn có thể tiếp tục bằng các nút trên màn hình. Theo thứ tự từ
  trên xuống: nút một, nghe lại hướng dẫn này. Nút hai, thử lại giọng
  đọc. Nút ba, mở cài đặt giọng đọc của máy. Nút bốn, mở thiết lập bộ
  đọc. Nút cuối, quay lại."
  (Khớp đúng thứ tự nút của màn cứu hộ ở `SplashScreen`:
  nghe lại → thử lại giọng → cài đặt giọng → thiết lập bộ đọc → quay lại.)
- **a11y_rescue_retrying:** "Đang thử lại giọng đọc. Vui lòng chờ
  trong giây lát."
- **a11y_rescue_install_failed:** "Cài đặt dữ liệu trò chơi chưa
  hoàn tất. Dữ liệu cũ vẫn được giữ nguyên. Theo thứ tự từ trên
  xuống: nút một, nghe lại hướng dẫn này. Nút hai, thử cài lại. Nút
  ba, mở cài đặt của máy. Nút cuối, thoát."
  (Khớp thứ tự nút màn lỗi cài đặt: nghe lại → thử cài lại → cài đặt
  → thoát.)
- **a11y_rescue_no_space:** "Máy không còn đủ dung lượng trống để
  cài dữ liệu trò chơi. Hãy xóa bớt tệp hoặc ứng dụng không dùng rồi
  thử lại. Nút một, nghe lại hướng dẫn này. Nút hai, thử cài lại.
  Nút ba, mở cài đặt của máy. Nút cuối, thoát."
- **a11y_rescue_welcome:** "Chào mừng bạn đến với trò chơi. Nếu bạn
  không nghe được giọng đọc này, hãy nhờ người hỗ trợ bật trình đọc
  màn hình, hoặc mở cài đặt giọng đọc của máy để cài giọng tiếng
  Việt."
- **a11y_rescue_setup_failed:** "Giọng đọc đang lỗi hoặc thiếu giọng
  tiếng Việt. Màn hình thiết lập vẫn có các nút có nhãn chữ. Bạn có
  thể mở cài đặt giọng đọc của máy để cài hoặc chọn giọng tiếng Việt,
  rồi quay lại đây và thử lại. Nếu máy có trình đọc màn hình, hãy bật
  nó để nghe nhãn từng nút."
  (Cố ý không nêu thứ tự nút: màn thiết lập có nhiều điều khiển,
  nêu thứ tự sẽ sai; clip chỉ dẫn đường đi an toàn.)
