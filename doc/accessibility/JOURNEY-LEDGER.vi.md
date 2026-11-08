# Sổ hành trình R01 — bản tóm tắt đọc cho người

Ngày: 2026-10-23. Mục kế hoạch: R01.2 và R01.3 trong `REMAINING-CODE-PLAN.vi.md` bản 2.0.
Bản máy đọc được đầy đủ (đường vào, trường/tham số, xác nhận/hủy, kết quả, quyền biết,
adapter, file/hàm, test ID cho từng hành trình) là `JOURNEY-LEDGER.json` cùng thư mục.
Tài liệu này là bản tóm tắt của chính sổ đó, sinh từ cùng dữ liệu để không lệch nhau.

**Đối chiếu cuối giai đoạn 1 (R23.1, 2026-10-23):** toàn bộ bảng trong tài liệu này đã
được sinh lại từ `JOURNEY-LEDGER.json` tại commit ứng viên
`bc4f807c853e16909199578c4d68218ae50415b4` — xem trường `doi_chieu_R23` trong sổ JSON.
Số chốt từ dữ kiện tệp: **110 hành trình đã viết, chờ kiểm thử / 5 chưa viết** trong 115.

**Giai đoạn 1 — chỉ bằng chứng nguồn.** Sổ này không build, không chạy test, không giả lập.
Mọi hành trình đều ở trạng thái kiểm thử **chưa thử**; kiểm thử thuộc giai đoạn 2 (T01–T64
và các bài bổ sung TX), nghiệm thu người khiếm thị trên máy thật thuộc giai đoạn 3.

Ghi chú định dạng: nội dung mô tả trong `JOURNEY-LEDGER.json` dùng tiếng Việt không dấu
cho các trường tự do (tên nhóm vẫn đối chiếu được với COVERAGE.md theo thứ tự và ngữ cảnh);
mọi mã định danh, trạng thái và test ID đều là ASCII. Bản tóm tắt này giữ nguyên các tên đó
để khớp với sổ máy đọc được.

## 1. Quy ước trạng thái viết

- **đã viết, chờ kiểm thử**: có bằng chứng file/hàm adapter thật trong nguồn tại commit
  ứng viên `bc4f807c` đáp ứng đường đi chính của hành trình. Không đồng nghĩa đã nghiệm thu,
  và không phải phần trăm hoàn thành của nhóm hay của game.
- **chưa viết**: chưa có adapter riêng cho hành trình hoặc một nhánh bắt buộc của nó chưa có.
  Việc được phục vụ một phần qua menu/fallback dùng chung được ghi rõ trong sổ và **không**
  tự nó nâng trạng thái — đúng quy định của kế hoạch (fallback danh sách lệnh tự ghi rằng
  nội dung màn có thể cần bộ đọc riêng).
- **chưa kiểm kê**: mục tiêu bằng 0 trong phạm vi đã ghim của đợt này (mục 3 dưới đây).

## 2. Tổng quan theo 30 nhóm bề mặt của COVERAGE.md

| Nhóm bề mặt | Số hành trình | Đã viết, chờ kiểm thử | Chưa viết |
| --- | ---: | ---: | ---: |
| Khoi dong Java | 7 | 7 | 0 |
| Menu chinh | 4 | 4 | 0 |
| The gioi/mod | 9 | 7 | 2 |
| Tao nhan vat | 5 | 5 | 0 |
| Menu chung | 5 | 5 | 0 |
| Popup/nhap | 5 | 5 | 0 |
| Ban do cuc bo | 5 | 5 | 0 |
| Di chuyen/hanh dong | 5 | 5 | 0 |
| Hanh trang | 5 | 5 | 0 |
| Kho nang cao | 2 | 2 | 0 |
| Y te/trang thai | 6 | 6 | 0 |
| Thong bao | 2 | 2 | 0 |
| Chien dau | 4 | 4 | 0 |
| Che tao/xay | 4 | 4 | 0 |
| Ban do lon | 5 | 5 | 0 |
| NPC/thuong mai | 2 | 2 | 0 |
| Nhiem vu/trai | 3 | 3 | 0 |
| Phe/nguoi theo | 1 | 1 | 0 |
| Phuong tien | 4 | 4 | 0 |
| Dieu khien dong co | 1 | 1 | 0 |
| Vung/tu dong hoa | 3 | 3 | 0 |
| Bionics/dot bien/phep | 4 | 4 | 0 |
| Ky nang/kien thuc | 2 | 2 | 0 |
| May tinh trong game | 1 | 1 | 0 |
| Nhat ky/diem/ket thuc | 4 | 4 | 0 |
| Cau hinh | 3 | 3 | 0 |
| Minigame thiet bi | 5 | 5 | 0 |
| Tro giup/phim | 2 | 2 | 0 |
| Luu/tai/sao luu | 4 | 4 | 0 |
| Cong cu phat trien | 3 | 0 | 3 |
| **Tổng** | **115** | **110** | **5** |

Đính chính nguồn test: danh sách T01–T64 nằm ở `PLAN.vi.md` mục 15.4, **không** nằm trong
`TESTING.vi.md`. Các nhóm không có bài riêng trong T01–T64 — điều khiển động cơ, kỹ năng/kiến thức, máy tính,
minigame, công cụ phát triển — đã được gán mã bài bổ sung TX-* trong sổ; danh sách TX chính
thức chốt ở R22.3 (xem `TESTING.vi.md`).

## 3. Phân loại điểm đầu vào (R01.2)

Công cụ `tools/accessibility/inventory.py` đã được đọc mã trước khi chạy: nó chỉ quét nguồn
(đọc `src/*.cpp`, `src/*.h` và Java phía Android; lệnh ngoài duy nhất là `git rev-parse HEAD`),
không import hay chạy test nào, không nạp mã game. Kết quả tại HEAD là **đếm cú pháp**:
982 tệp, 1.133 điểm `register_action`, 87 biểu thức tạo `input_context`, 300 điểm gọi ImGui,
15 điểm nhập cũ, 12 đăng ký phím thủ công, 10 điểm dialog native. Các số này gồm cả nhánh
compile-time không chạy trên cấu hình đích, và chính công cụ cũng tự khai không phải chứng
nhận phủ — không dùng chúng làm số màn, số tính năng hay tỷ lệ tiếp cận.

Mọi điểm đã quét và mọi điểm bổ sung thủ công được phân vào 6 lớp trong sổ (trường
`phan_loai_diem_dau_vao`): A — có adapter riêng; B — chỉ qua adapter dùng chung (uilist,
query_popup, fallback danh sách lệnh của `input_context::handle_input`); C — vòng lặp bypass
chưa nối (`debug.cpp` DEBUG_MSG, `animation.cpp`, gamepad, thanh shortcut cảm ứng/joystick
cảm ứng, `imgui_demo`); D — backend curses không thuộc đường Android; E — bề mặt Java
(dialog native khi bộ đọc tắt và các bề mặt Java riêng); F — constructor/factory động
phân loại theo cơ chế hội tụ. **Số điểm chưa phân loại bằng 0** trong phạm vi đã ghim, với
một giới hạn ghi rõ: số màn runtime thật do JSON/mod sinh ra không đếm được từ nguồn tĩnh,
nên lớp F phân loại theo cơ chế (màn con mở ra thuộc lớp nào thì điểm động thuộc lớp đó),
và phải mở lại khi danh sách mod phát hành được ghim.

Điểm bổ sung quan trọng ngoài công cụ dò (chi tiết và bằng chứng ở trường
`diem_bo_sung_ngoai_cong_cu` trong sổ): context `MAIN_MENU` khai bằng member initializer
(`main_menu.h:21`); context `STUDY_ZONE_UI` gán bằng phép gán (`study_zone_ui.cpp:46`);
15 thành viên `input_context` trần trong header nhận category muộn; context theo biến
`input_category` của uilist (wish, overmap notes, editmap, npctalk, popup); 24
`register_action` động chỉ biết ở runtime; toàn bộ vòng lặp sự kiện SDL, gamepad và thanh
shortcut cảm ứng; hàng trăm call site `query_yn/query_int/popup` hội tụ về `popup.cpp`/
`NativeUI`; các bề mặt Java ngoài dialog (`ReaderSetupActivity`, `BootstrapSelfVoiceSurface`,
lớp nhập của bộ đọc, `ReaderTextEditor`, `CDDADocumentsProvider`); factory động (iuse
actors, activity actors, 188 loader của `DynamicDataLoader`, talk topic JSON); widget ImGui
ngoài mẫu dò và tầng khung `cata_imgui` chưa có a11y. Công cụ cũng đếm nhầm định nghĩa hàm
của chính `register_action` thành call site — đã ghi nhận trong sổ.

57 tên context literal đã được gán nhóm trong sổ (54 từ công cụ + MAIN_MENU, STUDY_ZONE_UI và COMPUTER_CONTINUE
bổ sung thủ công); 20 biểu thức context còn lại của công cụ là dạng theo biến, đã phân loại
ở lớp B/F.

**Đối chiếu lại ở R23.2 (tại commit ứng viên `bc4f807c`):** quét lại toàn bộ diff
R00–R22 — các `register_action` cộng thêm là action nội bộ của adapter trong các
context đã phân loại ở các lớp trên (họ `A11Y_INTERNAL_*` ở color/panels/ranged và
các đăng ký trong chính màn đã có scope), cộng đúng một context mới `COMPUTER_CONTINUE`
của R17 — context này có `screen_scope` riêng (lớp A), đã được ghi ở hành trình
`may-tinh-trong-game` và được bổ sung vào danh sách literal trong sổ JSON ở R23.2;
các binding động của xe trong `data/json/vehicle.json` thuộc lớp đã phân loại từ
R01. **Số điểm chưa phân loại về thực chất vẫn bằng 0** trong phạm vi đã ghim.

## 4. Danh sách hành trình theo nhóm

### Khoi dong Java

| Hành trình | Trạng thái viết | Adapter | Test | Fixture |
| --- | --- | --- | --- | --- |
| Cai/nang du lieu lan dau va tien do khoi dong (`cai-dat-du-lieu-lan-dau`) | đã viết, chờ kiểm thử | rieng (Java) | T01, T05 | — |
| Cai/nang du lieu co staging, journal va rollback (`cai-dat-staging-rollback`) | đã viết, chờ kiểm thử | rieng (Java) | T05, T53 | F-15 |
| Thieu giong Viet: huong dan chon/cai giong va quay lai (`chon-va-cai-giong-doc`) | đã viết, chờ kiểm thử | rieng (Java) | T03 | — |
| Am cuu ho tieng Viet dong goi khi TTS loi/thieu giong (`am-cuu-ho-khong-phu-thuoc-tts`) | đã viết, chờ kiểm thử | rieng (Java) | T03, T04 | — |
| Thu giong, chinh toc do 0,5-3,0 va xac nhan bat bo doc (`thu-giong-va-bat-bo-doc`) | đã viết, chờ kiểm thử | rieng (Java) | T01, T02, T04 | — |
| Chon engine/voice/ngon ngu doc, thu theo nhap roi luu (`chon-engine-va-voice`) | đã viết, chờ kiểm thử | rieng (Java) | T03, T04, T56 | — |
| Bat/tat bo doc noi bo va phoi hop TalkBack/touch exploration (`bat-tat-bo-doc-va-phoi-hop-talkback`) | đã viết, chờ kiểm thử | rieng (Java) | T06, T62 | — |

### Menu chinh

| Hành trình | Trạng thái viết | Adapter | Test | Fixture |
| --- | --- | --- | --- | --- |
| Duyet va chon muc o menu chinh (`menu-chinh-duyet-chon`) | đã viết, chờ kiểm thử | rieng | T01, T02 | — |
| Tiep tuc/tai game tu menu chinh (`menu-chinh-tiep-tuc-va-tai`) | đã viết, chờ kiểm thử | rieng | T28, T53 | F-15 |
| Doc tai lieu/van ban o menu chinh (`menu-chinh-tai-lieu`) | đã viết, chờ kiểm thử | rieng | T16 | — |
| Canh bao va lua chon realtime khi vao game/tai/tutorial (`canh-bao-realtime-khi-vao-game`) | đã viết, chờ kiểm thử | rieng | T08 | — |

### The gioi/mod

| Hành trình | Trạng thái viết | Adapter | Test | Fixture |
| --- | --- | --- | --- | --- |
| Chon the gioi va thay so ban luu (`chon-the-gioi`) | đã viết, chờ kiểm thử | rieng | T28 | F-15 |
| Tao the gioi co ban (ten, muc tuy chon, hoan tat/dat lai/ngau nhien) (`tao-the-gioi-co-ban`) | đã viết, chờ kiểm thử | rieng | T01, T64 | — |
| Tuy chon nang cao cua the gioi (tab/nhom/gia tri/dieu kien) (`tuy-chon-nang-cao-cua-the-gioi`) | đã viết, chờ kiểm thử | rieng | T17, T18 | — |
| Chon mod khi tao/sao chep the gioi (nhom, mo ta, xung dot, thu tu nap) (`chon-mod-khi-tao-the-gioi`) | đã viết, chờ kiểm thử | rieng | T63 | — |
| Xem mod va thu tu nap cua the gioi hien co (chi doc) (`xem-mod-cua-the-gioi-hien-co`) | đã viết, chờ kiểm thử | rieng | — | — |
| Doi ten the gioi (`doi-ten-the-gioi`) | chưa viết | chua-co | — | — |
| Sao chep the gioi (sao cai dat sang the gioi moi) (`sao-chep-the-gioi`) | đã viết, chờ kiểm thử | rieng | — | — |
| Xoa the gioi kem xac nhan pha huy (`xoa-the-gioi-va-xac-nhan`) | đã viết, chờ kiểm thử | rieng | T57 | — |
| Cai dat/thay du lieu mod cho the gioi (`cai-thay-du-lieu-mod`) | chưa viết | chua-co | — | — |

### Tao nhan vat

| Hành trình | Trạng thái viết | Adapter | Test | Fixture |
| --- | --- | --- | --- | --- |
| Tao nhan vat qua tat ca tab (kich ban/nghe/nen tang/thuoc tinh/ky nang/dac tinh) (`tao-nhan-vat-cac-tab`) | đã viết, chờ kiểm thử | rieng | T17, T64 | — |
| Nhap/sua ten nhan vat tieng Viet va ten ngau nhien (`nhap-ten-nhan-vat`) | đã viết, chờ kiểm thử | rieng | T17 | — |
| Tong ket nhan vat truoc khi bat dau (`tong-ket-nhan-vat-truoc-bat-dau`) | đã viết, chờ kiểm thử | rieng | T64 | — |
| Luu/tai/xoa mau (template) nhan vat (`mau-nhan-vat-luu-tai-xoa`) | đã viết, chờ kiểm thử | rieng | — | — |
| Chon thanh pho/vi tri/xe bat dau trong tao nhan vat (`chon-diem-bat-dau`) | đã viết, chờ kiểm thử | rieng | — | — |

### Menu chung

| Hành trình | Trạng thái viết | Adapter | Test | Fixture |
| --- | --- | --- | --- | --- |
| Menu danh sach dung chung: loc, category, tab, tong tap loc, muc rong/khong dung duoc (`menu-danh-sach-loc-va-category`) | đã viết, chờ kiểm thử | rieng (adapter dung chung) | T09, T13, T14, T15 | — |
| Menu chon nhieu/checkbox/radio qua uilist (`menu-chon-nhieu-va-checkbox`) | đã viết, chờ kiểm thử | rieng (adapter dung chung) | T31 | F-03 |
| Danh sach lenh da dang ky cua man hien tai (fallback ngu canh) (`danh-sach-lenh-theo-ngu-canh`) | đã viết, chờ kiểm thử | rieng (adapter dung chung) | T59, T62 | — |
| Trung tam lenh day du: nhom, mo ta tac dong/chi phi, dieu kien/ly do vo hieu, tham so, tro giup (`trung-tam-lenh-day-du`) | đã viết, chờ kiểm thử | rieng (adapter dung chung) | T59 | — |
| Luyen tap cu chi: duyet/chon/doc/huy va tam huong trong ngu canh doc lap (`luyen-tap-cu-chi`) | đã viết, chờ kiểm thử | rieng (Java) | T07, T09, T10, T11, T12, T19, T50, T64 | F-06 |

### Popup/nhap

| Hành trình | Trạng thái viết | Adapter | Test | Fixture |
| --- | --- | --- | --- | --- |
| Popup thong bao va hoi Yes/No/Quit (`popup-thong-bao-va-yes-no`) | đã viết, chờ kiểm thử | rieng | T09, T15 | — |
| Nhap van ban tieng Viet (ten, ghi chu, tim kiem) qua trinh sua cua bo doc (`nhap-van-ban-tieng-viet`) | đã viết, chờ kiểm thử | rieng | T17, T18 | — |
| Nhap so/so luong co mien va loi tai truong (`nhap-so-va-so-luong`) | đã viết, chờ kiểm thử | rieng | T18, T31 | F-03 |
| Nhap/chon toa do qua input_popup (`nhap-toa-do`) | đã viết, chờ kiểm thử | rieng | — | — |
| Popup chon nhieu muc va popup tien do/thong bao dai (`popup-chon-va-tien-do`) | đã viết, chờ kiểm thử | rieng | T16 | — |

### Ban do cuc bo

| Hành trình | Trạng thái viết | Adapter | Test | Fixture |
| --- | --- | --- | --- | --- |
| Doc o hien tai va xung quanh (dia hinh/do dac/vat pham/sinh vat theo truong) (`doc-o-hien-tai-va-xung-quanh`) | đã viết, chờ kiểm thử | rieng | T19, T21, T22, T23, T24 | F-01 |
| Quet ban do voi bo loc da biet/dang chu y/dau am thanh (`quet-ban-do-va-bo-loc`) | đã viết, chờ kiểm thử | rieng | T26, T29, T30 | F-01 |
| Quet tri nho o cac tang lan can (chi tu memory) (`quet-memory-tang-khac`) | đã viết, chờ kiểm thử | rieng | T27, T29 | — |
| Cam nhan dac biet (mui/nhiet/dien tu...) theo mo ta game cho phep (`cam-nhan-dac-biet`) | đã viết, chờ kiểm thử | rieng | T23, T30 | F-01 |
| Tham do bang con tro doc lap: nhieu huong/tang, recenter, nhay toi doi tuong da biet (`tham-do-bang-con-tro`) | đã viết, chờ kiểm thử | chua-co | T29 | — |

### Di chuyen/hanh dong

| Hành trình | Trạng thái viết | Adapter | Test | Fixture |
| --- | --- | --- | --- | --- |
| Di chuyen tam huong (ke ca vuot di chuyen nhanh khi engine cho phep) va thay doi tu the/tang (`di-chuyen-tam-huong-va-vuot`) | đã viết, chờ kiểm thử | rieng | T10, T19, T20 | — |
| Ket qua hep tai diem engine xu ly buoc di/tuong tac (di/mo/danh/bi chan/doi tu the) (`ket-qua-hanh-dong-tai-diem-engine`) | đã viết, chờ kiểm thử | chua-co | T20 | — |
| Chon huong/o ke (choose_direction/choose_adjacent) cho tuong tac va cong trinh (`tuong-tac-huong-ke`) | đã viết, chờ kiểm thử | rieng | T20, T44 | — |
| Hoat dong dang chay: tien do, canh bao realtime/thoi gian troi, scope cho rieng (`hoat-dong-dang-chay-va-canh-bao`) | đã viết, chờ kiểm thử | rieng | T37 | — |
| Keo/day, leo, boi, len/xuong va tuong tac cong tac/cua/lua/chat long theo huong (`keo-day-leo-boi-len-xuong`) | đã viết, chờ kiểm thử | chua-co | T20 | — |

### Hanh trang

| Hành trình | Trạng thái viết | Adapter | Test | Fixture |
| --- | --- | --- | --- | --- |
| Mo hanh trang, chon vat pham va mo menu hanh dong (dung/cam/mac/coi/examine/nap-thao) (`hanh-trang-chon-va-hanh-dong`) | đã viết, chờ kiểm thử | rieng | T09, T31, T33 | F-03 |
| Chon so luong/ mot phan stack khi nhat/dung/bo (`chon-so-luong-va-chia-stack`) | đã viết, chờ kiểm thử | rieng | T31 | F-03 |
| Doc thong tin vat pham (item info) dang tai lieu (`thong-tin-vat-pham`) | đã viết, chờ kiểm thử | rieng | T16 | — |
| Cay vat chua/pocket: xem va cat/lay do trong tui long nhau (`cay-tui-va-vat-chua`) | đã viết, chờ kiểm thử | rieng | T32 | F-02 |
| So sanh vat pham (compare) trong hanh trang (`so-sanh-vat-pham`) | đã viết, chờ kiểm thử | rieng | — | — |

### Kho nang cao

| Hành trình | Trạng thái viết | Adapter | Test | Fixture |
| --- | --- | --- | --- | --- |
| Advanced inventory: chuyen do giua hai phia (nguon/dich, vi tri, tong khoi luong/the tich) (`kho-nang-cao-hai-phia`) | đã viết, chờ kiểm thử | rieng | T34 | F-03 |
| Lop mac (armor layers): thu tu, phu co the, encumbrance (`lop-mac-va-thu-tu`) | đã viết, chờ kiểm thử | rieng | — | — |

### Y te/trang thai

| Hành trình | Trạng thái viết | Adapter | Test | Fixture |
| --- | --- | --- | --- | --- |
| Y te: bo phan, vet thuong/hieu ung, chon dieu tri va vat tu (`y-te-va-dieu-tri`) | đã viết, chờ kiểm thử | rieng | T36 | F-04 |
| Man trang thai nhan vat day du (thuoc tinh/ky nang/hieu ung/nhu cau qua player_display) (`man-trang-thai-nhan-vat`) | đã viết, chờ kiểm thử | rieng (player_display) | T38 | — |
| Canh bao tu dong khi vuot nguong doi/khat/buon ngu/dau/suc ben (`canh-bao-nguong-sinh-ton`) | đã viết, chờ kiểm thử | rieng | T38 | — |
| An/uong va dung thuoc qua hanh trang (chuoi co ban) (`an-uong-co-ban`) | đã viết, chờ kiểm thử | rieng (qua adapter hanh trang + popup) | T35 | — |
| Chon container va do/chuyen/dung chat long (`chuoi-chat-long-va-container`) | đã viết, chờ kiểm thử | dung chung (uilist R04 + selector R09) | T35 | — |
| Ngu/doi/doc sach va hoat dong dai: tien do, ngat, tiep tuc/huy (`ngu-doi-doc-va-hoat-dong-dai`) | đã viết, chờ kiểm thử | rieng | T37 | — |

### Thong bao

| Hành trình | Trạng thái viết | Adapter | Test | Fixture |
| --- | --- | --- | --- | --- |
| Nghe thong bao game moi theo uu tien (bad/warning/critical) va coalescing/cooldown (`nghe-thong-bao-moi`) | đã viết, chờ kiểm thử | rieng | T16, T38, T60 | — |
| Mo lich su thong bao va doc lai theo thu tu (`lich-su-thong-bao`) | đã viết, chờ kiểm thử | rieng | T38 | — |

### Chien dau

| Hành trình | Trạng thái viết | Adapter | Test | Fixture |
| --- | --- | --- | --- | --- |
| Can chien/reach/vo thuat: chon dich, kieu danh, rut lui, safe mode (`can-chien-va-vo-thuat`) | đã viết, chờ kiểm thử | rieng (outcome) + dung chung (uilist/target_ui) | T39 | F-05 |
| Duyet muc tieu ban xa theo danh sach (huong/khoang cach/diem ngam), chon diem khong tu FIRE (`nham-muc-tieu-danh-sach`) | đã viết, chờ kiểm thử | rieng | T40, T41 | F-05 |
| Chon o trong/tang, tam/che khuat/duong ban va mo ta footprint AOE (`nham-o-trong-tang-va-aoe`) | đã viết, chờ kiểm thử | rieng | T40, T42 | F-05 |
| Fire mode, loai dan, reload/unload, nem va turret kem xac nhan muc tieu trung lap/dong doi (`che-do-ban-dan-nap-nem-turret`) | đã viết, chờ kiểm thử | rieng + dung chung | T41 | F-05 |

### Che tao/xay

| Hành trình | Trạng thái viết | Adapter | Test | Fixture |
| --- | --- | --- | --- | --- |
| Che tao: danh sach cong thuc/nhom, favorite, me 1-50, craftability (`che-tao-cong-thuc`) | đã viết, chờ kiểm thử | rieng | T43 | — |
| Doc yeu cau chi tiet va cac nhom lua chon dung cu/nguyen lieu khi che tao (`yeu-cau-va-nhom-lua-chon-che-tao`) | đã viết, chờ kiểm thử | rieng | T43 | — |
| Xay dung: nhom, bien the/cong doan, vi tri, yeu cau, trang thai o (`xay-dung-cong-trinh`) | đã viết, chờ kiểm thử | rieng | T44 | — |
| Sua do, thao ra/disassembly va repair qua iuse/repair actors (`sua-chua-va-thao-ra`) | đã viết, chờ kiểm thử | dung chung (selector R09 + uilist R04 + popup R03/R04) | T44 | — |

### Ban do lon

| Hành trình | Trạng thái viết | Adapter | Test | Fixture |
| --- | --- | --- | --- | --- |
| Overmap: con tro, o ke can theo muc da kham pha, toa do/tang con tro (`overmap-con-tro-va-o-ke-can`) | đã viết, chờ kiểm thử | rieng | T45 | — |
| Tao/xem/sua/xoa ghi chu va danh dau nguy hiem tren overmap (`overmap-ghi-chu-va-nguy-hiem`) | đã viết, chờ kiểm thử | rieng | T46 | — |
| Lap tuyen, chon diem, bat dau travel va huy (`overmap-tuyen-va-travel`) | đã viết, chờ kiểm thử | rieng | T45 | — |
| Tim kiem dia diem tren overmap va duyet ket qua (`overmap-tim-kiem`) | đã viết, chờ kiểm thử | rieng | T45 | — |
| Lich: doc gia tri thoi gian, tang/giam/dat lai va ly do gioi han do chinh xac (`lich-va-chinh-thoi-gian`) | đã viết, chờ kiểm thử | rieng (qua menu chung + callback rieng) | — | — |

### NPC/thuong mai

| Hành trình | Trạng thái viết | Adapter | Test | Fixture |
| --- | --- | --- | --- | --- |
| Hoi thoai NPC: nguoi noi, cau tra loi/dieu kien, lich su dai (`hoi-thoai-npc`) | đã viết, chờ kiểm thử | rieng | T47 | — |
| Giao dich hai phia: gia/don vi/tong, so du/tin dung/no, gioi han, pane dang thao tac (`giao-dich-hai-phia`) | đã viết, chờ kiểm thử | rieng (qua adapter hanh trang - trade_selector) | T47 | — |

### Nhiem vu/trai

| Hành trình | Trạng thái viết | Adapter | Test | Fixture |
| --- | --- | --- | --- | --- |
| Nhiem vu: bon tab, hang va chi tiet (han/phan thuong/dieu kien/muc tieu hien hanh, vi tri) (`nhiem-vu-danh-sach-va-chi-tiet`) | đã viết, chờ kiểm thử | rieng | T46 | — |
| Giao viec/tra dong doi va tab nhiem vu trai (companion missions) (`giao-viec-va-tra-dong-doi`) | đã viết, chờ kiểm thử | rieng | T48 | F-08 |
| Trai: cong viec/vung/nguon luc/nguoi/thoi gian va moi menu con cua faction_camp (`trai-xay-dung-va-hau-can`) | đã viết, chờ kiểm thử | rieng (2 man tu ve) + dung-chung (uilist/popup/selector) | T48 | F-08 |

### Phe/nguoi theo

| Hành trình | Trạng thái viết | Adapter | Test | Fixture |
| --- | --- | --- | --- | --- |
| Phe va nguoi theo: bon tab, danh sach trai/nguoi theo/nhom/sinh vat, chi tiet phe/sinh vat (`phe-va-nguoi-theo`) | đã viết, chờ kiểm thử | rieng | T48 | F-08 |

### Phuong tien

| Hành trình | Trạng thái viết | Adapter | Test | Fixture |
| --- | --- | --- | --- | --- |
| So do xe dang cay/bang: o xe, vi tri tuong doi, nhom chuc nang, ten/tinh trang bo phan (`so-do-xe-va-bo-phan`) | đã viết, chờ kiểm thử | rieng (veh_interact::reader_session, scope "vehicle-interact") | T49 | F-06 |
| Lap/sua/thao/thay bo phan xe: vi tri, phu thuoc, dung cu/linh kien, thoi gian, ket qua (`lap-sua-thao-bo-phan-xe`) | đã viết, chờ kiểm thử | rieng (veh_interact::reader_session cac mode install/repair/remove/select_part) | T49 | F-06 |
| Cay cargo theo bo phan/vat chua/mon; nhien lieu/pin/suc chua UI biet; chuyen vat pham (`cargo-xe`) | đã viết, chờ kiểm thử | rieng (reader_cargo_data trong veh_interact) + dung chung (advanced inventory R09 cho chuyen do) | T49 | F-06 |
| Len/xuong xe, no/tat may, lai/phanh/toc do/huong, cruise va remote (`lai-xe-remote-va-cruise`) | đã viết, chờ kiểm thử | dung chung (menu veh_menu/uilist R04 + lenh di chuyen the gioi) + rieng (trang thai lai trong player_status, form smart controller) | T50 | F-06 |

### Dieu khien dong co

| Hành trình | Trạng thái viết | Adapter | Test | Fixture |
| --- | --- | --- | --- | --- |
| Smart engine controller: dieu chinh nguong va huy (`smart-controller`) | đã viết, chờ kiểm thử | rieng | T50 | F-06 |

### Vung/tu dong hoa

| Hành trình | Trạng thái viết | Adapter | Test | Fixture |
| --- | --- | --- | --- | --- |
| Tao vung bang vi tri hien tai/con tro/toa do: hai goc/tang, pham vi/kich thuoc/loai truoc xac nhan (`tao-vung-hai-goc`) | đã viết, chờ kiểm thử | rieng (zone-manager + look-around) + dung-chung (uilist/popup nhap) | T51 | F-07 |
| Xem/doi ten/sua/xoa/bat-tat/thu tu vung va thuc thi/tam dung/ket qua tac vu vung (`quan-ly-va-chay-vung`) | đã viết, chờ kiểm thử | rieng (zone-manager) + dung-chung (uilist sua) | T51 | F-07 |
| Study Zone: cau hinh theo loai vung bang form/bang (truong/don vi/gioi han, ap dung/huy, truoc/sau) (`study-zone`) | đã viết, chờ kiểm thử | rieng (study-zone) | T51 | F-07 |

### Bionics/dot bien/phep

| Hành trình | Trạng thái viết | Adapter | Test | Fixture |
| --- | --- | --- | --- | --- |
| Bionics: chu dong/bi dong, nguon dien/chi phi/trang thai, kich hoat va phau thuat (`bionics`) | đã viết, chờ kiểm thử | rieng (man bionics) + dung-chung (uilist/popup/inventory selector cho phau thuat) | T52 | F-05, F-09 |
| Dot bien: nhom/dieu kien/trang thai/chi phi, dung/tat va phan hoi (`dot-bien`) | đã viết, chờ kiểm thử | rieng (man mutations) | T52 | F-05, F-09 |
| Phep/kha nang: level/cost/tam/yeu cau, chon o/muc tieu/AOE, hoc/chuan bi/dung va ket qua (`phep-va-kha-nang`) | đã viết, chờ kiểm thử | dung-chung (uilist SPELL_MENU + spellbook) + rieng (du lieu entry/callback trong magic.cpp; selector nham R11) | T52 | F-05, F-09 |
| Gan doi phim cho phep/bionics/dot bien bang danh sach ky tu hop le (`gan-phim-kha-nang`) | đã viết, chờ kiểm thử | rieng (diem noi dung chung) | T59 | — |

### Ky nang/kien thuc

| Hành trình | Trạng thái viết | Adapter | Test | Fixture |
| --- | --- | --- | --- | --- |
| Ky nang/proficiency: tien do va dieu kien dang nghe duoc thay bieu do/thanh/mau (`ky-nang-va-proficiency`) | đã viết, chờ kiểm thử | rieng | TX-ky-nang-proficiency | — |
| Doc sach de hoc: yeu cau doc, hoc cong thuc/ky nang, tien do va ngat/tiep tuc (`doc-sach-va-hoc`) | đã viết, chờ kiểm thử | dung chung (hanh trang + uilist yeu cau doc trong avatar::read + activity R10) | T37 | — |

### May tinh trong game

| Hành trình | Trạng thái viết | Adapter | Test | Fixture |
| --- | --- | --- | --- | --- |
| May tinh trong game: lua chon/van ban/trang thai va nhanh YESNOQUIT/phim tho (`may-tinh-trong-game`) | đã viết, chờ kiểm thử | rieng (computer-query/computer-continue + tai lieu terminal) + dung-chung (uilist menu chinh) | TX-may-tinh | F-09 |

### Nhat ky/diem/ket thuc

| Hành trình | Trạng thái viết | Adapter | Test | Fixture |
| --- | --- | --- | --- | --- |
| Nhat ky: pane trang/thay doi/noi dung, sua trang bang trinh nhap cua bo doc, tao/xoa/xuat (`nhat-ky`) | đã viết, chờ kiểm thử | rieng | T58 | — |
| Diem sau ket thuc: tab, thanh tich/quy tac/diem, tong ha guc, nhat ky tuong niem dang cay doc dai (`diem-va-thanh-tich`) | đã viết, chờ kiểm thử | rieng | T58 | — |
| Man ket thuc: noi dung bo sung dang hien thi va nhap loi cuoi (`man-ket-thuc-va-loi-cuoi`) | đã viết, chờ kiểm thử | rieng | T58 | — |
| Deathcam: bao che do va quet theo tam camera thay vi quanh nhan vat (`deathcam`) | đã viết, chờ kiểm thử | rieng | T58 | — |

### Cau hinh

| Hành trình | Trạng thái viết | Adapter | Test | Fixture |
| --- | --- | --- | --- | --- |
| Tuy chon game/Android: tab/nhom/gia tri/mo ta, chinh tang/giam, draft/ap dung/huy/dat lai (`tuy-chon-game`) | đã viết, chờ kiểm thử | rieng | T02, T62 | — |
| Safe Mode va auto-pickup: bang quy tac co dieu kien/uu tien/nguon, them/sua/xoa/kiem quy tac/dat lai (`safemode-va-auto-pickup`) | đã viết, chờ kiểm thử | rieng (safe-mode-manager, auto-pickup-manager + 2 man test) | T51 | F-07 |
| Panels/color/layout: cau hinh bang dieu khien va mau (thong tin khong chi bang mau) (`panels-va-mau-sac`) | đã viết, chờ kiểm thử | rieng | — | — |

### Minigame thiet bi

| Hành trình | Trạng thái viết | Adapter | Test | Fixture |
| --- | --- | --- | --- | --- |
| Robot tim kitten (iuse_software_kitten) (`minigame-kitten`) | đã viết, chờ kiểm thử | rieng (screen_scope "robot-finds-kitten") | TX-minigame-kitten | F-10 |
| Lights On (iuse_software_lightson) (`minigame-lightson`) | đã viết, chờ kiểm thử | rieng (screen_scope "lightson") | TX-minigame-lightson | F-13 |
| Minesweeper (iuse_software_minesweeper) (`minigame-minesweeper`) | đã viết, chờ kiểm thử | rieng (screen_scope "minesweeper") | TX-minigame-minesweeper | F-11 |
| Snake (iuse_software_snake) - tro realtime (`minigame-snake`) | đã viết, chờ kiểm thử | rieng (screen_scope "snake") | TX-minigame-snake | F-14 |
| Sokoban (iuse_software_sokoban) (`minigame-sokoban`) | đã viết, chờ kiểm thử | rieng (screen_scope "sokoban") | TX-minigame-sokoban | F-12 |

### Tro giup/phim

| Hành trình | Trạng thái viết | Adapter | Test | Fixture |
| --- | --- | --- | --- | --- |
| Tro giup: tim theo chu de/lenh, giai thich dieu khien, duong quay lai (`tro-giup-va-tim-lenh`) | đã viết, chờ kiểm thử | rieng | T59 | — |
| Keybinding: danh muc, gan/xoa/va cham/mac dinh/luu/huy, dung duoc khi khong co phim cung (`gan-phim-keybinding`) | đã viết, chờ kiểm thử | rieng (diem noi dung chung) | T59 | — |

### Luu/tai/sao luu

| Hành trình | Trạng thái viết | Adapter | Test | Fixture |
| --- | --- | --- | --- | --- |
| Luu/tai/xoa ban luu cho nhieu world/nhan vat: ten/thoi diem/chi tiet dung (`luu-tai-xoa-ban-luu`) | đã viết, chờ kiểm thử | rieng | T28, T53 | F-15 |
| Backup/export/import/restore qua DocumentsProvider/file picker (`backup-export-import`) | đã viết, chờ kiểm thử | rieng | T57 | — |
| Ban luu loi: phan hoi va duong phuc hoi khong ghi de, khong dat lai du lieu nguoi choi (`loi-ban-luu-va-phuc-hoi`) | đã viết, chờ kiểm thử | rieng | T57 | — |
| Nghi huu nhan vat va cac man hau game: chap nhan tu vong/nghi huu, xu ly the gioi, tro menu (`nghi-huu-va-hau-game`) | đã viết, chờ kiểm thử | dung-chung | T58 | — |

### Cong cu phat trien

| Hành trình | Trạng thái viết | Adapter | Test | Fixture |
| --- | --- | --- | --- | --- |
| Debug menu/console va DEBUG_MSG (pham vi rieng) (`debug-menu-va-console`) | chưa viết | chua-co | TX-debug-menu | — |
| Trinh sua ban do editmap (pham vi rieng) (`editmap`) | chưa viết | chua-co | TX-editmap | — |
| Wish menu (pham vi rieng): chon vat pham/sinh vat de wish (`wish-menu`) | chưa viết | chua-co (co callback bo tro) | TX-wish | — |

## 5. Trạng thái còn lại sau đối chiếu R23

Năm hành trình chưa viết, theo lý do (chi tiết từng dòng ở sổ JSON):

- **Không tồn tại trong UI thật** — không có luồng để nối adapter; chỉ mở lại nếu bản game
  sau bổ sung tính năng: đổi tên thế giới hiện có (`doi-ten-the-gioi`), cài/thay dữ liệu
  mod trong game (`cai-thay-du-lieu-mod`).
- **Phạm vi riêng — công cụ phát triển** (không phải tính năng chơi; nghĩa vụ là nhãn
  giới hạn và đường thoát khi chúng hiện ra): debug menu/console (`debug-menu-va-console`),
  editmap (`editmap`), wish menu (`wish-menu`).

Điểm mở theo bề mặt (không phải cả hành trình — hành trình liên quan vẫn đã viết qua
đường còn lại): marker `partial_con` chưa được công bố ngữ nghĩa trong snapshot thế
giới (đường hiện có là menu `prompt_partial_construction`); tệp `src/veh_appliance.cpp`
(thiết bị dạng xe) chưa có điểm trợ năng nào — header ImGui của nó chưa được công bố;
ô lọc của màn gán phím không có setter nên bộ lọc của bộ đọc là lọc trình bày; chặn
cứng import backup khi phiên game đang sống cần cờ C++/JNI; snapshot trên 4096 node
chưa có phương án phân trang. Quyết định cho từng điểm: xem
`PHASE1-CLOSING-REPORT.vi.md`.

Điểm đứt của cây nền (không thuộc adapter): dispatch `game::cast_spell` không tồn tại
trong cây — `handle_action.cpp` gọi `cast_spell()`/`cast_spell(true)` nhưng không có
khai báo/định nghĩa nào và `known_magic::select_spell` không có caller; thi triển phép
đầu-cuối bị chặn tới khi giai đoạn 2 khôi phục glue của engine (bài TX-21).

## 6. Quy tắc mở lại sổ

Mở lại sổ và các mục liên quan khi: danh sách mod phát hành được ghim hoặc đổi (mục 8 của
`SCOPE-MANIFEST-2026-10-23.vi.md`); một mục R02–R23 viết xong adapter cho hành trình đang
ghi chưa viết (cập nhật trạng thái kèm commit); phát hiện màn/constructor động mới ngoài
lớp F; hoặc R23 đối chiếu cuối giai đoạn 1.

Đối chiếu cuối giai đoạn 1 đã thực hiện ở R23 (2026-10-23) tại commit ứng viên
`bc4f807c853e16909199578c4d68218ae50415b4`: 110 đã viết, chờ kiểm thử / 5 chưa viết
trong 115 hành trình. Các lần mở lại tiếp theo thuộc giai đoạn 2/3.
