# Kế hoạch phần mã còn lại — bản khôi phục 2.1

Ngày 24/10/2026. Bản này được dựng lại từ 96 đầu việc trong `REMAINING-CODE-BACKLOG.json`; không tuyên bố khớp byte với bản 2.0 chỉ từng lưu cục bộ. Kế hoạch gốc và mục tiêu nghiệm thu nằm trong `PLAN.vi.md`.

Quy trình hiện tại: sửa các phát hiện của đợt rà 24/10, chạy kiểm thử có bằng chứng cho từng lớp, sửa lỗi phát sinh rồi tạo APK đầy đủ. APK hoặc test host đạt chưa thay cho thử TalkBack/giọng thật và người khiếm thị.

Các điều kiện chưa đóng phải giữ trạng thái mở: nguồn local thứ ba không còn trong môi trường; cấu hình mod/phát hành chưa chốt; hành trình Android/người dùng chưa có bằng chứng. Không dùng tổng 96 đầu việc làm phần trăm tiếp cận.

## R00 — Hòa giải nguồn, bảo toàn thay đổi cục bộ

Phụ thuộc: Nguồn và bản sao đã xác định.

- **R00.1**: so sánh ba phía từng tệp local với `f40dcdd` và `ad5ecf0`; ghi quyết định giữ/thay/ghép cho từng hunk, không áp dụng toàn patch cũ lên nguồn mới.
- **R00.2**: giữ cờ cho phép vuốt di chuyển xuyên C++ → JSON → Java; bổ sung điều kiện cho đi bộ, đủ hướng, lái/remote, deathcam và trạng thái editor/modal/hoạt động.
- **R00.3**: hòa giải hai phiên bản bài luyện cử chỉ, controller, chuỗi Việt/Anh và action focus/activate; tránh trùng preference, hai bài tập cạnh tranh hoặc quay lùi adapter mới.
- **R00.4**: chốt commit tích hợp mới, ghi nguồn từng phần; giữ các xóa local như quyết định riêng, không đưa xóa asset vào thay đổi trợ năng nếu chưa rõ mục đích.

Bài liên quan: T07, T09, T10, T11, T12, T19, T28, T50, T53, T54, T55, T62.

## R01 — Đóng phạm vi và sổ hành trình

Phụ thuộc: R00.

- **R01.1**: ghi manifest commit game/nội dung, mods và thứ tự nạp, ngôn ngữ, flavor/ABI/API, chế độ lượt/realtime và cách đọc; SDK trong repo chỉ là cấu hình build, chưa phải dải máy đã chứng nhận.
- **R01.2**: phân loại từng điểm input, raw-key, ImGui, dialog Java và constructor động theo luồng thực; bổ sung các điểm mà công cụ dò cú pháp bỏ sót.
- **R01.3**: tạo hồ sơ từng hành trình với đường vào, trường, tham số, xác nhận/hủy, kết quả, tri thức, adapter dùng chung/riêng, file/hàm và test ID; ghi riêng chưa kiểm kê/chưa viết/đã viết/chưa thử.
- **R01.4**: giữ toàn bộ hệ thống gameplay bắt buộc trong ma trận; công cụ debug/editor có thể tách phạm vi nhưng mọi đường hiện ra vẫn phải có nhãn và đường thoát; nội dung/mod thay đổi thì mở lại đối soát.

Bài liên quan: T08, T59, T63, T64.

## R02 — Hợp đồng bảng, cây và biểu mẫu

Phụ thuộc: R01.

- **R02.1**: thêm cây với parent ID, độ sâu, có con/mở/thu gọn, thứ tự và tổng của tập hiện hành; xác định ID logic và chính sách focus khi cha bị thu hoặc mục bị xóa.
- **R02.2**: thêm bảng với ID cột/header, ô và đơn vị/giá trị so sánh; truy cập theo hàng hoặc thuộc tính, không biến mỗi ô thành hành động gameplay.
- **R02.3**: thêm trường form: kiểu, miền số/bước, lựa chọn, giá trị nháp/đã áp dụng, lỗi và ràng buộc giữa trường; ý định riêng cho đặt giá trị, chọn, mở cây, xác nhận và hủy.
- **R02.4**: nối serialize/parse/validate/render, khả năng tương thích có chủ ý, giới hạn kích thước/phân trang; từ chối ID trùng, cây vòng/cha không hợp lệ và tham số ngoài miền tại game thread.

Bài liên quan: T12, T13, T14, T15, T16, T17, T18, T31, T32, T33, T34, T49, T51.

## R03 — Dispatcher, quyền đầu vào và modal

Phụ thuộc: R02.

- **R03.1**: kiểm quyền theo session/frame/generation/revision/request và action/giá trị trước khi thực thi; tra lại đối tượng hiện tại và điều kiện engine ngay lúc nhận.
- **R03.2**: đóng mọi đường input kép, tap dư, pointer cancel, phím giả và callback quá hạn; đổi scope xóa hàng input cũ, Back/hủy chỉ tác động đúng chủ sở hữu.
- **R03.3**: nối adapter khi bật reader giữa màn đã mở, kể cả builder chỉ dựng một lần; modal lồng, popup Java/ImGui và nhánh native trả về sớm phải có đường nhận/hủy/kết quả.
- **R03.4**: giữ backend không Android và build-list nguồn mới; định nghĩa suspend/resume, mất focus/TalkBack/cài đặt hệ thống, lỗi TTS và tạo lại Activity mà không phát lệnh muộn.

Bài liên quan: T06, T09, T10, T11, T12, T13, T14, T15, T28, T53, T54, T55, T62.

## R04 — Menu, form, nhập và trung tâm lệnh dùng chung

Phụ thuộc: R03.

- **R04.1**: hoàn thiện filter/category/tab, tổng tập lọc, empty/disabled, multi-select và số lượng; giữ đúng `retval`, không dùng chỉ số sau lọc thay giá trị logic.
- **R04.2**: nối checkbox/radio/slider/số/text theo R02, lỗi tại trường, áp dụng/hủy/đặt lại và draft; nhập Việt/grapheme/IME/paste, min/max/bước và bảo toàn cú pháp tìm kiếm.
- **R04.3**: trung tâm lệnh có nhóm, mô tả tác động/chi phí, điều kiện/lý do vô hiệu, tham số và trợ giúp; phân biệt duyệt lệnh với gọi lệnh, không coi danh sách action ID là hoàn thiện.
- **R04.4**: tài liệu dài có từng thuộc tính/đoạn, đọc tiếp/quay đoạn/câu cuối/đánh vần, bookmark theo phiên và lịch sử tìm kiếm; cảnh báo ngắt không tự đọc lại toàn văn.

Bài liên quan: T13, T14, T15, T16, T17, T18, T31, T33, T38, T59, T60.

## R05 — Cử chỉ, luyện tập và cứu hộ

Phụ thuộc: R04.

- **R05.1**: giữ gán hai trục và mặc định 45 giây; thay deadline bằng thời gian đơn điệu, lưu riêng cấu hình đã xác nhận; restart/reboot/hết hạn/TalkBack trả cấu hình đã xác nhận.
- **R05.2**: đóng bài tập duyệt/chọn/đọc/hủy/tám hướng trong ngữ cảnh độc lập, không gửi gameplay; đọc từng hướng và tiến độ, có đường thoát hoặc học lại.
- **R05.3**: chuyển di chuyển nhanh qua quyền engine ở R00/R03; hủy gesture khi đổi chế độ; định nghĩa hướng bản đồ/isometric và không dùng cử chỉ đi bộ cho tăng ga hoặc remote.
- **R05.4**: cung cấp trợ giúp một ngón, tôi đang ở đâu, dừng/đọc lại/hủy/khôi phục; nếu giữ yêu cầu đổi tám hướng thì thêm bảng hoán vị hợp lệ và vùng thử riêng trên cùng cơ chế, không viết một hệ preference thứ hai.

Bài liên quan: T07, T09, T10, T11, T12, T19, T50, T64.

## R06 — Giọng, khởi động và cài dữ liệu

Phụ thuộc: R05.

- **R06.1**: thêm bộ chọn engine/voice/ngôn ngữ, dữ liệu offline/network, thử giọng theo lựa chọn nháp rồi xác nhận/lưu; giữ thanh tốc độ đang có và không tự chấp nhận giọng mạng.
- **R06.2**: nối health/timeout/retry có giới hạn, đổi engine/voice bị mất và phục hồi sau binder/process; thống nhất chủ sở hữu backend, không tạo TTS mỗi câu hoặc giữ Activity cũ.
- **R06.3**: thêm đường cứu hộ bằng âm Việt đóng gói, không phụ thuộc TTS đang lỗi; điều khiển retry/chọn nguồn đọc/cài đặt/quay lại bằng thao tác nghe được; không giả định app tự điều khiển được màn cài đặt Android.
- **R06.4**: cài/nâng dữ liệu bằng staging, kiểm tra hoàn tất rồi chuyển đổi, journal/rollback sau lỗi hoặc kill; đọc thiếu dung lượng/chép/giải nén, retry và thoát mà không báo game sẵn sàng sớm.

Bài liên quan: T01, T02, T03, T04, T05, T06, T08, T53, T54, T55, T56, T57, T63.

## R07 — Thế giới, mod và toàn bộ tạo nhân vật

Phụ thuộc: R06.

- **R07.1**: đóng tạo/chọn/đổi tên/sao chép/xóa world, danh sách save và xác nhận phá hủy; mod/phụ thuộc/xung đột/thứ tự/tùy chọn và lỗi dữ liệu có kết quả nghe được.
- **R07.2**: rà mọi tab nhân vật, chi phí/điểm/xung đột/điều kiện, nghề/nền tảng/kịch bản/proficiency/bionic/phép/vị trí/xe bắt đầu; giữ chi tiết đã có, thêm dữ liệu còn thiếu theo UI thực.
- **R07.3**: tên Việt/tên ngẫu nhiên có thể sửa, mẫu lưu/tải/xóa, summary cuối, quay tab sửa lựa chọn và hủy; luồng lỗi không làm mất draft vô ý.
- **R07.4**: nối chuỗi tạo → vào game → đi/mở cửa/nhặt/ăn/uống → trạng thái → lưu/thoát/tải, và quyết định realtime theo ý người chơi; không tự đổi luật hoặc option âm thầm.

Bài liên quan: T01, T02, T17, T18, T19, T20, T28, T31, T35, T53, T57, T64.

## R08 — Thăm dò, tri thức và kết quả hành động

Phụ thuộc: R07.

- **R08.1**: thăm dò con trỏ nhiều hướng/tầng, recenter, lọc/nhảy theo đối tượng đã biết; gắn nguồn thấy/nhớ/nghe/cảm nhận/chưa biết cho từng thuộc tính, không đọc sống qua memory.
- **R08.2**: giữ bộ lọc/quét memory/special perception đã có; bổ sung chi tiết hợp lệ của cửa/địa hình/item/xe/nguy cơ và vùng âm gần đúng; không tăng độ chính xác vượt quyền biết.
- **R08.3**: thêm kết quả hẹp tại điểm engine xử lý bước đi/tương tác: đi, mở, đánh, bị chặn, đổi tư thế hoặc chưa xác định; phản hồi sau sự kiện thật, không suy từ tọa độ không đổi.
- **R08.4**: đóng kéo/đẩy/leo/bơi/lên xuống, chọn hướng/ô và tương tác công tắc/cửa/lửa/chất lỏng; tách con trỏ quan sát khỏi điều khiển avatar/xe/camera và xóa dữ liệu theo nhân vật.

Bài liên quan: T19, T20, T21, T22, T23, T24, T25, T26, T27, T28, T29, T30, T35, T44, T50.

## R09 — Hành trang, túi, kho hai phía và lớp mặc

Phụ thuộc: R08.

- **R09.1**: giữ selector/số lượng/disabled đã có; đóng multi-select, chia/gộp stack, favorite/filter/sort/compare, nạp/tháo và trường hợp vật phẩm biến mất hoặc trùng tên.
- **R09.2**: dựng cây vật chứa/pocket với nguồn/đích, khả năng truy cập, đồ trong túi, giới hạn và lý do từ chối; chỉ mô tả contents khi game cho phép biết, không lộ container đóng.
- **R09.3**: adapter advanced inventory có pane đang thao tác, nguồn/đích, vị trí, tổng khối lượng/thể tích, số lượng, chuyển một/nhiều và hủy; tra lại sức chứa/quyền sở hữu khi chốt.
- **R09.4**: lớp mặc/thứ tự/phủ cơ thể/encumbrance, cầm/mặc/cởi/dùng, lựa chọn vật chứa và cảnh báo gốc; các thao tác move/toggle/select phân biệt với hành động tiêu lượt.

Bài liên quan: T18, T24, T31, T32, T33, T34, T35, T41, T47, T50.

## R10 — Sức khỏe, sinh hoạt và hoạt động dài

Phụ thuộc: R09.

- **R10.1**: hoàn thiện trạng thái/cơ thể/effect/nhiệt/morale/nhu cầu/thuốc, đơn vị và điều kiện đã biết; giữ mô tả sức khỏe theo UI, không thay bằng HP hay cân nặng sống chưa được biết.
- **R10.2**: đóng ăn/uống/chọn container/chuyển chất lỏng/thuốc/băng bó, bộ phận, vật tư, hiệu quả/điều kiện/cảnh báo và kết quả sau hoạt động.
- **R10.3**: đọc/học/ngủ/đợi/hoạt động có tiến độ tiết chế, lý do bị ngắt, tiếp tục/hủy và kết quả hủy thật; cảnh báo thời gian tiếp tục trôi trong mode tương ứng.
- **R10.4**: mở rộng cảnh báo có ngưỡng, chống lặp và lịch sử cho trạng thái/nguy cơ chưa có log; reset baseline khi load/đổi phiên và gộp với log để không đọc kép.

Bài liên quan: T16, T35, T36, T37, T38, T53, T54, T60.

## R11 — Chiến đấu, nhắm ô/tầng và vùng ảnh hưởng

Phụ thuộc: R10.

- **R11.1**: đóng melee/reach/võ thuật/đổi mục tiêu/rút lui và safe mode; đọc trạng thái/kết quả nhân vật biết, không đọc HP bí mật hoặc tên kẻ ẩn.
- **R11.2**: mở rộng chọn mục tiêu hiện có bằng chọn ô trống/tầng, tầm/che khuất/aim/đường bắn; giữ engine là thẩm quyền khả năng tấn công, focus/chọn điểm không FIRE.
- **R11.3**: đóng fire mode/loại đạn/reload/unload/ném/turret và xác nhận mục tiêu trung lập/đồng đội; revalidate mục tiêu/đạn khi chốt và xử lý danh sách rỗng/>256 hàng.
- **R11.4**: mô tả footprint/AOE theo dữ liệu game công khai, hướng/tầng và nguy cơ đồng minh đã biết; cho sửa/hủy riêng, không đưa thực thể ẩn vào tổng số/cue.

Bài liên quan: T20, T22, T23, T24, T30, T39, T40, T41, T42, T52.

## R12 — Chế tạo, sửa chữa, xây và tháo

Phụ thuộc: R11.

- **R12.1**: giữ recipe/batch/yêu cầu; bổ sung công thức lồng/nhóm/filter/favorite, ánh sáng/crafter/điều kiện và tổng tài nguyên dùng chung, không suy đủ từ từng nhóm riêng lẻ.
- **R12.2**: chọn nguyên liệu/dụng cụ thay thế và đồ chứa bằng selector có số lượng; xác nhận tiêu hao/bắt đầu, gián đoạn/tiếp tục, byproduct/kết quả thật mà không tạo preview ngẫu nhiên mới.
- **R12.3**: đóng sửa đồ/tháo rã/disassembly/repair, yêu cầu/rủi ro đã biết, vật tư, mục tiêu và kết quả; tận dụng selector chung, nối modal riêng còn thiếu.
- **R12.4**: công trình/blueprint/giai đoạn/vị trí/hướng, ô hợp lệ/không hợp lệ và xác nhận phe phái; khởi công/hủy/hoàn thành vẫn qua logic gốc, không mô tả điều kiện ô ẩn bằng dữ liệu sống.

Bài liên quan: T21, T24, T31, T32, T37, T43, T44.

## R13 — Bản đồ lớn, tuyến, ghi chú và nhiệm vụ

Phụ thuộc: R12.

- **R13.1**: giữ con trỏ/tuyến/note/search đã có; đóng tầng/legend/filter và từng nhánh kết quả tìm kiếm; nêu rõ độ chính xác tọa độ/vị trí theo quyền biết.
- **R13.2**: tạo/xem/sửa/xóa ghi chú, vùng nguy hiểm và điểm quan tâm; form tên/nội dung/biểu tượng có lời tương đương, xác nhận xóa và quay đúng focus.
- **R13.3**: lập tuyến/chọn điểm/khởi hành/hủy, lý do không thể đi và dừng nguy hiểm gốc; duyệt tuyến chỉ quan sát, chọn điểm không tự khởi hành.
- **R13.4**: đóng nhận/theo dõi/đổi mục tiêu/nộp/thất bại nhiệm vụ, hạn/phần thưởng/điều kiện/kết quả được biết và liên kết vị trí; giữ adapter mission/calendar đã có.

Bài liên quan: T25, T28, T29, T30, T45, T46.

## R14 — NPC, giao dịch, đồng đội và trại

Phụ thuộc: R13.

- **R14.1**: giữ câu trả lời/điều kiện/lịch sử; đóng người nói/nội dung dài/đổi phản hồi/xác nhận tác động và danh sách thay đổi mà không chọn nhầm câu qua callback cũ.
- **R14.2**: giao dịch hai phía có lượng từng món, giá/đơn vị/tổng/nợ/tín dụng/giới hạn, tiền và chia stack; quyết định trao đổi riêng, nghe kết quả bị từ chối/thành công.
- **R14.3**: ra lệnh đồng đội, trang bị/liên hệ/khả năng biết được, giao việc/trả người, ETA và trạng thái; xử lý người mất liên hệ/đổi danh sách/không đủ điều kiện.
- **R14.4**: trại có công việc/vùng/nguồn lực/người/thời gian/điều kiện và kết quả, mọi menu con thực tế; không bỏ qua xây trại hoặc chuỗi hậu cần bằng cách chỉ hỗ trợ tab phe.

Bài liên quan: T31, T32, T33, T34, T46, T47, T48.

## R15 — Xe, sơ đồ bộ phận, cargo và lái/remote

Phụ thuộc: R14.

- **R15.1**: dựng sơ đồ dạng cây/bảng theo ô xe/vị trí tương đối/nhóm chức năng, tên bộ phận/tình trạng và kết nối; duyệt ô/part không tự sửa hoặc di chuyển xe.
- **R15.2**: lắp/sửa/tháo/thay bộ phận, vị trí, phụ thuộc, dụng cụ/linh kiện, thời gian và kết quả; tra lại xe/part khi chốt vì cấu trúc có thể đổi.
- **R15.3**: cây cargo theo từng bộ phận/vật chứa/món, lượng nhiên liệu/pin/sức chứa mà UI được biết; chuyển vật phẩm qua adapter R09, không chỉ đọc tên kho.
- **R15.4**: lên/xuống, nổ/tắt máy, lái/phanh/tốc độ/hướng, cruise/remote và ngưỡng smart controller bằng form có miền; công bố mode riêng và khóa cử chỉ đi bộ khỏi các lệnh tăng ga.

Bài liên quan: T32, T33, T34, T44, T49, T50, T54.

## R16 — Vùng, tự động hóa và quy tắc

Phụ thuộc: R15.

- **R16.1**: tạo vùng bằng vị trí hiện tại/con trỏ/tọa độ, chọn hai góc/tầng, đọc phạm vi/kích thước/loại trước khi xác nhận; giữ đúng giới hạn tạo vùng của game.
- **R16.2**: xem/đổi tên/sửa/xóa/bật/tắt/thứ tự vùng, nguồn/đích/điều kiện tác vụ; đóng thực thi/tạm dừng/kết quả tác vụ mà không tự khởi chạy khi duyệt.
- **R16.3**: Study Zone và cấu hình theo loại vùng dùng form/bảng R02, đọc trường/đơn vị/giới hạn/điều kiện, áp dụng/hủy và dữ liệu trước/sau.
- **R16.4**: bảng Safe Mode/auto-pickup và bộ lọc có điều kiện/ưu tiên/nguồn, thêm/sửa/xóa/kiểm quy tắc/đặt lại; cho nghe kết quả khớp bằng dữ liệu mà UI gốc cung cấp.

Bài liên quan: T14, T18, T20, T31, T51.

## R17 — Bionics, đột biến, phép và máy tính

Phụ thuộc: R16.

- **R17.1**: bionics có chủ động/bị động/nguồn điện/chi phí/trạng thái/điều kiện, kích hoạt và phẫu thuật với vật tư/rủi ro đã biết, xác nhận/hủy/kết quả.
- **R17.2**: mutation có nhóm/điều kiện/trạng thái/chi phí, dùng/tắt/đổi phím và phản hồi; helper gán phím đã có không được tính là adapter toàn màn.
- **R17.3**: phép/khả năng có level/cost/tầm/yêu cầu/chọn ô hoặc mục tiêu/AOE, học/chuẩn bị/dùng và kết quả; dùng bộ chọn R11, không làm lộ nội dung chưa được cấp.
- **R17.4**: máy tính trong game có màn lựa chọn/văn bản/trạng thái và các nhánh YESNOQUIT/phím thô, nhập/xác nhận/hủy; truy vấn không tự chạy lệnh máy tính.

Bài liên quan: T16, T17, T18, T40, T41, T42, T52.

## R18 — Minigame và màn điều khiển riêng

Phụ thuộc: R17.

- **R18.1**: mỗi trò có luật nghe được, trạng thái, bàn chơi/con trỏ, phím ngữ nghĩa, khởi động lại/hủy/kết quả; scope riêng không nhận cử chỉ thế giới.
- **R18.2**: Minesweeper/Lights On xuất ô/trạng thái/láng giềng hợp lệ; chỉ đọc số đã lộ/cờ và công tắc được biết, không lộ mìn hoặc lời giải.
- **R18.3**: Sokoban/Kitten mô tả vị trí/vật cản/vật thể và thao tác theo luật, không dùng giả định “mọi vật trên sàn đều chặn đường”; chạm duyệt không đi nhân vật.
- **R18.4**: Snake/trò realtime có chính sách đọc/cue và điều khiển đáp ứng thời gian; không tự làm chậm hoặc đổi luật mà không công bố. Nếu chưa chơi được thì mục này còn mở, không gọi là hoàn thiện.

Bài liên quan: .

## R19 — Kiến thức, trợ giúp và mọi cấu hình còn lại

Phụ thuộc: R18.

- **R19.1**: kỹ năng/proficiency/sách/công thức đã học, tiến độ và điều kiện theo UI; chuyển biểu đồ/thanh/màu thành thuộc tính nghe được, không suy kiến thức ẩn.
- **R19.2**: trợ giúp tìm theo chủ đề/lệnh, giải thích điều khiển và đường quay lại; phần screen_reader cũ của help được rà lại với bộ đọc nội bộ, không mặc định tương đương.
- **R19.3**: keybinding/shortcut có danh mục, gán/xóa/va chạm/mặc định/lưu/hủy, hoạt động cả khi không có phím cứng; giữ helper chọn ký tự đã có.
- **R19.4**: hoàn thiện options Android/game, panels/color/layout và mọi tab điều kiện, giá trị/miền/mô tả/draft/apply/cancel/reset; màn xuất hiện do mod hoặc cấu hình vẫn được kiểm kê.

Bài liên quan: T13, T14, T15, T16, T17, T18, T38, T59, T62 và bài riêng từng trang.

## R20 — Lưu, phục hồi, vòng đời và kết thúc

Phụ thuộc: R19.

- **R20.1**: nhiều world/nhân vật/save, tên/thời điểm/chi tiết đúng; lưu/tải/xóa/xác nhận/lỗi bản lưu, không tự ghi đè khi xử lý lỗi và không giữ tri thức từ nhân vật cũ.
- **R20.2**: backup/export/import/restore qua DocumentsProvider/file picker, quyền truy cập, tên trùng, thiếu dung lượng, staging và lỗi; chuyển quyền đọc hệ thống có chỉ dẫn và đường trở về.
- **R20.3**: process death/relaunch, Activity recreate/khóa/cuộc gọi/mất audio focus/xoay, cập nhật APK giữ save/config; không khôi phục lệnh hoặc xác nhận đang chờ từ phiên cũ.
- **R20.4**: giữ adapter deathcam/nhật ký/điểm đã có, đóng chấp nhận tử vong/nghỉ hưu/lời cuối/xuất/xóa/xử lý thế giới/trở menu; mọi màn hậu game có hủy hoặc bước tiếp hợp lệ.

Bài liên quan: T28, T53, T54, T55, T56, T57, T58, T63.

## R21 — Lời đọc, âm hiệu, tiếng Việt và chẩn đoán

Phụ thuộc: R20.

- **R21.1**: rà ưu tiên cảnh báo/focus/kết quả, coalescing/cooldown/hết hạn, bookmark/đọc tiếp và giới hạn queue; không tự đọc lại tài liệu màn cũ sau cảnh báo hoặc resume.
- **R21.2**: audio focus, loa/mono/tai nghe/Bluetooth, cue/rung tùy chọn có lời tương đương, âm lượng/hướng/lỗi phát; cue cũng tuân quyền biết, không che cảnh báo bằng nhạc.
- **R21.3**: hoàn thiện chuỗi Việt/Anh, đơn vị/hướng/số nhiều/dấu và thuật ngữ, fallback tên gốc/đánh vần; thêm nguồn vào quy trình đóng gói dịch mà chưa chạy build trong giai đoạn này.
- **R21.4**: log chẩn đoán có giới hạn và không thu save cá nhân mặc định, context/request/status/queue/độ trễ, xuất báo cáo nghe được; chuẩn bị đo hiệu năng và chế độ ít hình, chỉ tối ưu sâu sau phép đo ở giai đoạn 2.

Bài liên quan: T16, T22, T23, T24, T30, T38, T54, T55, T56, T60, T61, T62, T63.

## R22 — Chuẩn bị fixture, bài kiểm thử và tài liệu bàn giao

Phụ thuộc: R21.

- **R22.1**: viết fixture/scenario/save tái tạo được cho tri thức giống nhau-bí mật khác nhau, cây túi, kho, vết thương, chiến đấu, xe, vùng, trại, ability và minigame; không dùng save cá nhân làm điều kiện.
- **R22.2**: hoàn thiện bài có giá trị cho schema/Unicode/token/input/quyền biết, tích hợp gameplay và lỗi TTS/vòng đời; ghi oracle trạng thái game, không chỉ assertion tên hàm hoặc chuỗi nguồn.
- **R22.3**: mở rộng T01–T64 bằng bài riêng cho form/bảng/cây, engine/voice/cứu hộ, smart controller/Study Zone/quy tắc, máy tính, từng minigame và trại; gán fixture và loại máy/cấu hình cho từng bài.
- **R22.4**: viết hướng dẫn nghe từ người mới, lệnh/cử chỉ/phục hồi/backup, mẫu báo lỗi và hồ sơ candidate/checksum/signing/upgrade; đánh dấu tài liệu APK thử giọng cũ là lịch sử để không lẫn mã mới.

Bài liên quan: T01, T02, T03, T04, T05, T06, T07, T08, T09, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25, T26, T27, T28, T29, T30, T31, T32, T33, T34, T35, T36, T37, T38, T39, T40, T41, T42, T43, T44, T45, T46, T47, T48, T49, T50, T51, T52, T53, T54, T55, T56, T57, T58, T59, T60, T61, T62, T63, T64.

## R23 — Đóng giai đoạn viết mã và ghim ứng viên

Phụ thuộc: R00, R01, R02, R03, R04, R05, R06, R07, R08, R09, R10, R11, R12, R13, R14, R15, R16, R17, R18, R19, R20, R21, R22.

- **R23.1**: đối chiếu lại mọi hàng ma trận với đủ đường tìm/dữ liệu/tham số/xác nhận/hủy/kết quả/quyền biết; cập nhật file/hàm/commit của từng nhánh và mọi phần chưa viết.
- **R23.2**: loại TODO/stub/fallback chặn dùng trong phạm vi; rà đường input chưa phân loại, nguồn mới từ mod/config và các xung đột còn treo; mở lại mục liên quan nếu còn thiếu.
- **R23.3**: ghim commit code/nội dung/manifest, lưu bản sao thay đổi, danh sách bài/fixture và chỗ cần chứng minh runtime; kiểm đủ file/resource/build-list bằng đọc nguồn, không gọi là biên dịch thành công.
- **R23.4**: bàn giao báo cáo kết thúc giai đoạn 1 với trạng thái đã viết/chờ kiểm thử cho từng hành trình và mọi ràng buộc còn tồn tại; sau đó mới bắt đầu giai đoạn 2 theo thứ tự ở phần 6.

Bài liên quan: .

## Điều kiện đóng giai đoạn

Mỗi hành trình phải đủ đường tìm, thông tin, tham số, xác nhận/hủy, kết quả thật và ranh giới tri thức; ghi rõ file/hàm/commit và bài sẽ/chạy đã chạy. Còn lỗi chặn, asset thiếu hoặc luồng bắt buộc chỉ có stub thì mục liên quan chưa đóng.

Kết quả kiểm thử phải gắn với commit và cấu hình thực. Lỗi rollback, journal, phiên game và điều khiển biểu mẫu phải có kiểm tra hồi quy. Không làm yếu kiểm tra hoặc bỏ tính năng để tạo báo cáo xanh.
