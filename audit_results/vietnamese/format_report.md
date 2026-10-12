# Vietnamese translation: technical-format audit

This report checks compressed catalog integrity, gettext validation,
Unicode validation, and common runtime placeholder/markup tokens.
Possible mismatches need contextual review and are not automatically errors.

## Compile and Unicode checks

| Test | Result | Diagnostic |
|---|---|---|
| gzip_integrity | PASS |  |
| gettext_format | PASS | 116272 translated messages. |
| unicode_check | PASS |  |

## Potential structural mismatches

| Category | Count |
|---|---:|
| issues | 275 |
| markup_tag_mismatch | 44 |
| printf_argument_mismatch | 231 |
| strings_checked | 116272 |

## markup_tag_mismatch: up to 25 examples

- **English**: '<empty rule>'
  - **Vietnamese**: '<quy tắc trống>'
  - **Reference**: src/auto_pickup.cpp:441, src/safemode_ui.cpp:251
  - **Tokens**: {'english': {'open_empty': 1}, 'vietnamese': {'open_quy': 1}}
- **English**: '[<Global>]'
  - **Vietnamese**: '[<Toàn cục>]'
  - **Reference**: src/auto_pickup.cpp:576, src/safemode_ui.cpp:198
  - **Tokens**: {'english': {'open_Global': 1}, 'vietnamese': {}}
- **English**: '[<Character>]'
  - **Vietnamese**: '[<Nhân vật>]'
  - **Reference**: src/auto_pickup.cpp:578, src/safemode_ui.cpp:200
  - **Tokens**: {'english': {'open_Character': 1}, 'vietnamese': {}}
- **English**: '<no name>'
  - **Vietnamese**: '<chưa đặt tên>'
  - **Reference**: src/clzones.cpp:763
  - **Tokens**: {'english': {'open_no': 1}, 'vietnamese': {}}
- **English**: '<Arrow Keys> To navigate'
  - **Vietnamese**: '<Phím mũi tên> Để điều hướng'
  - **Reference**: src/color.cpp:815
  - **Tokens**: {'english': {'open_Arrow': 1}, 'vietnamese': {}}
- **English**: "Don't move a <freaking> muscle…"
  - **Vietnamese**: 'Đừng có nhúc nhích dù chỉ một cơ bắp…'
  - **Reference**: src/dialogue_chatbin.h:125
  - **Tokens**: {'english': {'open_freaking': 1}, 'vietnamese': {}}
- **English**: 'Are you <freaking> insane!?'
  - **Vietnamese**: 'Mày bị điên rồi à!?'
  - **Reference**: src/dialogue_chatbin.h:154
  - **Tokens**: {'english': {'open_freaking': 1}, 'vietnamese': {}}
- **English**: '<No longer valid construction>'
  - **Vietnamese**: '<Công trình không còn hợp lệ>'
  - **Reference**: src/faction_camp.cpp:6034
  - **Tokens**: {'english': {'open_No': 1}, 'vietnamese': {}}
- **English**: ' <Unsupported recipe>'
  - **Vietnamese**: ' <Công thức không được hỗ trợ>'
  - **Reference**: src/faction_camp.cpp:6048
  - **Tokens**: {'english': {'open_Unsupported': 1}, 'vietnamese': {}}
- **English**: '<spacebar or 5> toggle lights'
  - **Vietnamese**: '<phím cách hoặc 5> bật/tắt đèn'
  - **Reference**: src/iuse_software_lightson.cpp:151
  - **Tokens**: {'english': {'open_spacebar': 1}, 'vietnamese': {}}
- **English**: "That was the first thing to really shake me out of it.  I didn't really have any very close friends, but there were people back in town I cared about a bit.  I had sent some texts, but I hadn't really twigged that they hadn't replied for days.  I got in my truck and tried to get back to town.  Didn't get far before I hit a <zombie> infested pileup blocking the highway, and that's when I started to"
  - **Vietnamese**: 'Đó là điều đầu tiên thực sự kéo tôi tỉnh giấc. Tôi không có bạn bè thân thiết nào, nhưng vẫn có vài người trong thị trấn mà tôi hơi quan tâm. Tôi đã gửi vài tin nhắn, nhưng không để ý rằng suốt mấy ngày trời họ chẳng buồn hồi đáp. Tôi leo lên xe bán tải và cố chạy về thị trấn. Chẳng đi được bao xa thì đụng phải một vụ tông xe liên hoàn đầy rẫy <zombies> chắn ngang đường cao tốc, và đó là lúc tôi b'
  - **Reference**: data/json/npcs/Backgrounds/rural_1.json
  - **Tokens**: {'english': {'open_zombie': 1, 'open_zombies': 1}, 'vietnamese': {'open_zombies': 2}}
- **English**: "That was the first thing to really shake me out of it.  I didn't really have any very close friends, but there were people back in town I cared about a bit.  I had sent some texts, but I hadn't really twigged that they hadn't replied for days.  I got in my truck and tried to get back to town.  Didn't get far before I hit a <zombie> infested pileup blocking the highway, and that's when I started to"
  - **Vietnamese**: 'Đó là điều đầu tiên thực sự kéo tôi tỉnh giấc. Tôi không có bạn bè thân thiết nào, nhưng vẫn có vài người trong thị trấn mà tôi hơi quan tâm. Tôi đã gửi vài tin nhắn, nhưng không để ý rằng suốt mấy ngày trời họ chẳng buồn hồi đáp. Tôi leo lên xe bán tải và cố chạy về thị trấn. Chẳng đi được bao xa thì đụng phải một vụ tông xe liên hoàn đầy rẫy <zombies> chắn ngang đường cao tốc, và đó là lúc tôi b'
  - **Reference**: data/json/npcs/Backgrounds/rural_1.json
  - **Tokens**: {'english': {'open_zombie': 1, 'open_zombies': 1}, 'vietnamese': {'open_zombies': 2}}
- **English**: "That was the first thing to really shake me out of it.  I didn't really have any very close friends, but there were people back in town I cared about a bit.  I had sent some texts, but I hadn't really twigged that they hadn't replied for days.  I got in my truck and tried to get back to town.  Didn't get far before I hit a <zombie> infested pileup blocking the highway, and that's when I started to"
  - **Vietnamese**: 'Đó là điều đầu tiên thực sự kéo tôi tỉnh giấc. Tôi không có bạn bè thân thiết nào, nhưng vẫn có vài người trong thị trấn mà tôi hơi quan tâm. Tôi đã gửi vài tin nhắn, nhưng không để ý rằng suốt mấy ngày trời họ chẳng buồn hồi đáp. Tôi leo lên xe bán tải và cố chạy về thị trấn. Chẳng đi được bao xa thì đụng phải một vụ tông xe liên hoàn đầy rẫy <zombies> chắn ngang đường cao tốc, và đó là lúc tôi b'
  - **Reference**: data/json/npcs/Backgrounds/rural_1.json
  - **Tokens**: {'english': {'open_zombie': 1, 'open_zombies': 1}, 'vietnamese': {'open_zombies': 2}}
- **English**: "That was the first thing to really shake me out of it.  I didn't really have any very close friends, but there were people back in town I cared about a bit.  I had sent some texts, but I hadn't really twigged that they hadn't replied for days.  I got in my truck and tried to get back to town.  Didn't get far before I hit a <zombie> infested pileup blocking the highway, and that's when I started to"
  - **Vietnamese**: 'Đó là điều đầu tiên thực sự kéo tôi tỉnh giấc. Tôi không có bạn bè thân thiết nào, nhưng vẫn có vài người trong thị trấn mà tôi hơi quan tâm. Tôi đã gửi vài tin nhắn, nhưng không để ý rằng suốt mấy ngày trời họ chẳng buồn hồi đáp. Tôi leo lên xe bán tải và cố chạy về thị trấn. Chẳng đi được bao xa thì đụng phải một vụ tông xe liên hoàn đầy rẫy <zombies> chắn ngang đường cao tốc, và đó là lúc tôi b'
  - **Reference**: data/json/npcs/Backgrounds/rural_1.json
  - **Tokens**: {'english': {'open_zombie': 1, 'open_zombies': 1}, 'vietnamese': {'open_zombies': 2}}
- **English**: "That was the first thing to really shake me out of it.  I didn't really have any very close friends, but there were people back in town I cared about a bit.  I had sent some texts, but I hadn't really twigged that they hadn't replied for days.  I got in my truck and tried to get back to town.  Didn't get far before I hit a <zombie> infested pileup blocking the highway, and that's when I started to"
  - **Vietnamese**: 'Đó là điều đầu tiên thực sự kéo tôi tỉnh giấc. Tôi không có bạn bè thân thiết nào, nhưng vẫn có vài người trong thị trấn mà tôi hơi quan tâm. Tôi đã gửi vài tin nhắn, nhưng không để ý rằng suốt mấy ngày trời họ chẳng buồn hồi đáp. Tôi leo lên xe bán tải và cố chạy về thị trấn. Chẳng đi được bao xa thì đụng phải một vụ tông xe liên hoàn đầy rẫy <zombies> chắn ngang đường cao tốc, và đó là lúc tôi b'
  - **Reference**: data/json/npcs/Backgrounds/rural_1.json
  - **Tokens**: {'english': {'open_zombie': 1, 'open_zombies': 1}, 'vietnamese': {'open_zombies': 2}}
- **English**: "That was the first thing to really shake me out of it.  I didn't really have any very close friends, but there were people back in town I cared about a bit.  I had sent some texts, but I hadn't really twigged that they hadn't replied for days.  I got in my truck and tried to get back to town.  Didn't get far before I hit a <zombie> infested pileup blocking the highway, and that's when I started to"
  - **Vietnamese**: 'Đó là điều đầu tiên thực sự kéo tôi tỉnh giấc. Tôi không có bạn bè thân thiết nào, nhưng vẫn có vài người trong thị trấn mà tôi hơi quan tâm. Tôi đã gửi vài tin nhắn, nhưng không để ý rằng suốt mấy ngày trời họ chẳng buồn hồi đáp. Tôi leo lên xe bán tải và cố chạy về thị trấn. Chẳng đi được bao xa thì đụng phải một vụ tông xe liên hoàn đầy rẫy <zombies> chắn ngang đường cao tốc, và đó là lúc tôi b'
  - **Reference**: data/json/npcs/Backgrounds/rural_1.json
  - **Tokens**: {'english': {'open_zombie': 1, 'open_zombies': 1}, 'vietnamese': {'open_zombies': 2}}
- **English**: "It's less like a well, and more like a covered pit currently.  I'll need you to find a mechanical pump, one that will make this <freaking> thing work."
  - **Vietnamese**: 'Hiện giờ nó chẳng khác gì một cái hố có nắp đậy cả. Tôi sẽ cần bạn tìm một cái máy bơm cơ học, thứ giúp cái của nợ này hoạt động được.'
  - **Reference**: data/json/npcs/godco/godco_missions.json
  - **Tokens**: {'english': {'open_freaking': 1}, 'vietnamese': {}}
- **English**: "I'm not gonna go and fix this <freaking> mess."
  - **Vietnamese**: 'Tôi sẽ không đi dọn cái đống hỗn độn chết tiệt này đâu.'
  - **Reference**: data/json/npcs/godco/godco_missions.json
  - **Tokens**: {'english': {'open_freaking': 1}, 'vietnamese': {}}
- **English**: "I do what I can, but he's mostly beyond my skillset.  Coke's a <freaking> <name_b>, really messes a person up.  Arturo was on it for several years before he tried to get help, now he has auditory hallucinations; hears voices from nowhere.  Traumatic experiences have a harsher effect on recovering addicts, but <the_cataclysm> is worse than anything we've gone through before."
  - **Vietnamese**: 'Tôi làm những gì có thể, nhưng anh ấy gần như vượt quá khả năng chuyên môn của tôi rồi. Cocaine là thứ <freaking> khốn kiếp, nó hủy hoại một con người ghê gớm. Arturo dính vào nó nhiều năm trước khi chịu tìm sự giúp đỡ, giờ anh ấy bị ảo giác thính giác; nghe thấy những giọng nói từ hư không. Những sang chấn tâm lý gây tác động tồi tệ hơn nhiều lên những người đang cai nghiện, mà <the_cataclysm> th'
  - **Reference**: data/json/npcs/godco/members/NPC_Jane_Schulz.json
  - **Tokens**: {'english': {'open_freaking': 1, 'open_name_b': 1, 'open_the_cataclysm': 1}, 'vietnamese': {'open_freaking': 1, 'open_the_cataclysm': 1}}
- **English**: "Do I have a <freaking> exclamation sign over my <freaking> head, like I'm a <freaking> quest-giver from some <freaking> video game?"
  - **Vietnamese**: 'Trên đầu tao có cắm cái dấu chấm than <freaking> chết tiệt nào à, làm như tao là một NPC giao nhiệm vụ <freaking> trong mấy cái trò chơi điện tử <freaking> không bằng?'
  - **Reference**: data/json/npcs/island_prison/prisoners.json
  - **Tokens**: {'english': {'open_freaking': 4}, 'vietnamese': {'open_freaking': 3}}
- **English**: '&This visitor seems way more watchful than normal.  You notice zip bags peeking out from <mypossesivepronoun> overcoat, they are clearly pushing something.  It doesn\'t take much longer for them to notice you in kind.\n"You want something?  Or just gawking?"'
  - **Vietnamese**: '&Vị khách này có vẻ cảnh giác hơn hẳn bình thường. Bạn nhận thấy những chiếc túi zip thò ra từ áo khoác của cô ấy, rõ ràng là đang buôn bán thứ gì đó lén lút. Chẳng mất bao lâu để cô ấy cũng nhận ra ánh mắt của bạn.\n"Muốn mua gì không? Hay chỉ đứng ngó trừng trừng thế?"'
  - **Reference**: data/json/npcs/robofac/robofac_ancilla_npcs/BAR_ENCOUNTER_MERCENARIES/BEM_drugs.json
  - **Tokens**: {'english': {'open_mypossesivepronoun': 1}, 'vietnamese': {}}
- **English**: '&This visitor seems way more watchful than normal.  You notice zip bags peeking out from <mypossesivepronoun> overcoat, they are clearly pushing something.  It doesn\'t take much longer for them to notice you in kind.\n"You want something?  Or just gawking?"'
  - **Vietnamese**: '&Vị khách này có vẻ cảnh giác hơn hẳn bình thường. Bạn nhận thấy những chiếc túi zip thò ra từ áo khoác của anh ấy, rõ ràng là đang buôn bán thứ gì đó lén lút. Chẳng mất bao lâu để anh ấy cũng nhận ra ánh mắt của bạn.\n"Muốn mua gì không? Hay chỉ đứng ngó trừng trừng thế?"'
  - **Reference**: data/json/npcs/robofac/robofac_ancilla_npcs/BAR_ENCOUNTER_MERCENARIES/BEM_drugs.json
  - **Tokens**: {'english': {'open_mypossesivepronoun': 1}, 'vietnamese': {}}
- **English**: '&This visitor seems way more watchful than normal.  You notice zip bags peeking out from <mypossesivepronoun> overcoat, they are clearly pushing something.  It doesn\'t take much longer for them to notice you in kind.\n"You want something?  Or just gawking?"'
  - **Vietnamese**: '&Vị khách này có vẻ cảnh giác hơn hẳn bình thường. Bạn nhận thấy những chiếc túi zip thò ra từ áo khoác của họ, rõ ràng là đang buôn bán thứ gì đó lén lút. Chẳng mất bao lâu để họ cũng nhận ra ánh mắt của bạn.\n"Muốn mua gì không? Hay chỉ đứng ngó trừng trừng thế?"'
  - **Reference**: data/json/npcs/robofac/robofac_ancilla_npcs/BAR_ENCOUNTER_MERCENARIES/BEM_drugs.json
  - **Tokens**: {'english': {'open_mypossesivepronoun': 1}, 'vietnamese': {}}
- **English**: '&You find the gun runner intently assessing the swirling smoke of a stale cigarette.  <mypronoun> barely acknowledges your approach.  "Let\'s cut it short, I have come upon an excess supply of exotic weaponry and let\'s say that it would be expedient for it to disappear and you seem like just the kind of person I\'m looking for."\n\n<mypronoun> hands you an inventory list, loosely annotated in a piece '
  - **Vietnamese**: '&Bạn thấy kẻ buôn vũ khí đang chăm chú nhìn làn khói xoáy từ một điếu thuốc tàn. Cô ấy hầu như chẳng thèm để ý đến việc bạn tiến lại gần. "Nói ngắn gọn thôi, tôi vừa vớ được một lượng lớn vũ khí ngoại lai dư thừa và cứ coi như sẽ rất tiện nếu mớ hàng này biến mất đi, mà trông bạn có vẻ chính là kiểu người tôi đang tìm kiếm đấy."\n\nCô ấy đưa cho bạn một danh sách kiểm kê, được ghi chú nguệch ngoạc t'
  - **Reference**: data/json/npcs/robofac/robofac_ancilla_npcs/BAR_ENCOUNTER_MERCENARIES/BEM_gunrunner.json
  - **Tokens**: {'english': {'open_mypronoun': 2}, 'vietnamese': {}}
- **English**: '&<mypronoun> quickly removes the offending piece of meat from your hand and places two golden coins on the table, apparently unshaken by the prospect of meeting someone who casually visits bars while carrying cuts of human flesh.  "Now if you please, I have a meal to get rid of." <mypronoun> says, between poorly masked bouts of nausea.'
  - **Vietnamese**: '&Người đó nhanh chóng giật lấy miếng thịt chướng mắt khỏi tay bạn và đặt hai đồng tiền vàng lên bàn, bề ngoài có vẻ không hề nao núng trước cảnh tượng gặp một kẻ thản nhiên đi bar mà mang theo từng tảng thịt người. "Bây giờ xin phép, tôi có một bữa ăn cần phải tống khứ đi đây." Người đó nói, cố giấu đi những cơn buồn nôn đang chực trào.'
  - **Reference**: data/json/npcs/robofac/robofac_ancilla_npcs/BAR_ENCOUNTER_MERCENARIES/BEM_human_sample.json
  - **Tokens**: {'english': {'open_mypronoun': 2}, 'vietnamese': {}}

## printf_argument_mismatch: up to 25 examples

- **English**: '% stamina to swing.'
  - **Vietnamese**: '% thể lực để vung.'
  - **Reference**: src/item_info.cpp:3637
  - **Tokens**: {'english': [(None, 's', 's')], 'vietnamese': []}
- **English**: '0.0 % Failure Chance'
  - **Vietnamese**: '0.0 % Tỉ lệ thất bại'
  - **Reference**: src/ranged.cpp:4446
  - **Tokens**: {'english': [(None, 'float', 'F')], 'vietnamese': []}
- **English**: 'Damaged power circuits cause short-circuiting inside your muscles when your batteries are above 75% capacity, causing greatly reduced strength.  This has no effect if you have no internal batteries.'
  - **Vietnamese**: 'Các mạch điện bị hỏng gây ra đoản mạch bên trong cơ bắp của bạn khi pin trên 75% dung lượng, khiến sức mạnh bị giảm đi đáng kể. Điều này không có tác dụng nếu bạn không có pin cấy ghép bên trong.'
  - **Reference**: data/json/bionics.json
  - **Tokens**: {'english': [(None, 'c', 'c')], 'vietnamese': [(None, 'integer', 'd')]}
- **English**: '50% of trees/trunks at the forest position will be cut down.'
  - **Vietnamese**: '50% số cây/thân cây tại vị trí rừng sẽ bị đốn hạ.'
  - **Reference**: data/json/faction_missions.json
  - **Tokens**: {'english': [(None, 'integer', 'o')], 'vietnamese': [(None, 's', 's')]}
- **English**: '100% of total material will be brought back.'
  - **Vietnamese**: '100% tổng số vật liệu sẽ được mang về.'
  - **Reference**: data/json/faction_missions.json
  - **Tokens**: {'english': [(None, 'integer', 'o')], 'vietnamese': []}
- **English**: '95% of trees/trunks at the forest position will be cut down.'
  - **Vietnamese**: '95% số cây/thân cây tại vị trí rừng sẽ bị đốn hạ.'
  - **Reference**: data/json/faction_missions.json
  - **Tokens**: {'english': [(None, 'integer', 'o')], 'vietnamese': [(None, 's', 's')]}
- **English**: '0% of total material will be brought back.'
  - **Vietnamese**: '0% tổng số vật liệu sẽ được mang về.'
  - **Reference**: data/json/faction_missions.json
  - **Tokens**: {'english': [(None, 'integer', 'o')], 'vietnamese': []}
- **English**: 'As a weapon, this item needs considerable space to use properly and does 70% of its normal damage to adjacent enemies.'
  - **Vietnamese**: 'Khi dùng làm vũ khí, vật phẩm này cần không gian đáng kể để sử dụng hiệu quả và gây 70% lượng sát thương thông thường lên những kẻ địch ở liền kề.'
  - **Reference**: data/json/flags.json
  - **Tokens**: {'english': [(None, 'integer', 'o')], 'vietnamese': []}
- **English**: 'You can craft at 100% speed in any level of light.'
  - **Vietnamese**: 'Bạn có thể chế tạo ở 100% tốc độ trong bất kỳ mức ánh sáng nào.'
  - **Reference**: data/json/flags.json
  - **Tokens**: {'english': [(None, 's', 's')], 'vietnamese': []}
- **English**: "You've learned the fundamentals of Aikido.  Your movement would be familiar to any practitioners in a pre-Cataclysm dojo.\n\nBlocked damage reduced by 25% of Dexterity, +1 blocking effectiveness, +1 dodge attempt."
  - **Vietnamese**: 'Bạn đã học được những nguyên lý cơ bản của Aikido. Chuyển động của bạn sẽ rất quen thuộc với bất kỳ môn sinh nào trong võ đường trước Thảm họa.\n\nSát thương chặn được giảm đi 25% Khéo léo, +1 hiệu quả chặn, +1 lần thử né đòn.'
  - **Reference**: data/json/martialarts.json
  - **Tokens**: {'english': [(None, 'integer', 'o')], 'vietnamese': []}
- **English**: 'An agile stance that allows your blocks to reduce incoming damage.\n\nBlocked damage reduced by 100% of Dexterity.'
  - **Vietnamese**: 'Một thế đứng linh hoạt cho phép các đòn chặn của bạn giảm bớt sát thương nhận vào.\n\nSát thương chặn được giảm đi 100% Khéo léo.'
  - **Reference**: data/json/martialarts.json
  - **Tokens**: {'english': [(None, 'integer', 'o')], 'vietnamese': []}
- **English**: 'Sliding footwork enhances the crushing continuity of your attacks.\n\nArmor penetration increased by 75% of Strength.'
  - **Vietnamese**: 'Bộ pháp trượt giúp tăng cường tính liên tục đầy uy lực cho các đòn tấn công của bạn.\n\nXuyên giáp tăng thêm 75% Sức mạnh.'
  - **Reference**: data/json/martialarts.json
  - **Tokens**: {'english': [(None, 'integer', 'o')], 'vietnamese': []}
- **English**: 'You switch or sturdy your grip mid-step to aid your blocking.\n\nBlocked damage reduced by 40% of Strength.\nLasts 3 turns.  Stacks 3 times.'
  - **Vietnamese**: 'Bạn đổi hoặc siết chặt tay cầm ngay giữa bước di chuyển để hỗ trợ việc đỡ đòn.\n\nSát thương chặn được giảm đi 40% Sức mạnh.\nKéo dài 3 lượt. Cộng dồn 3 lần.'
  - **Reference**: data/json/martialarts.json
  - **Tokens**: {'english': [(None, 'integer', 'o')], 'vietnamese': []}
- **English**: 'You prepare for the final crushing strike.\n\nAccuracy increased by 15% of Strength, armor penetration increased by 225% of Strength.\nLasts 1 turn.'
  - **Vietnamese**: 'Bạn chuẩn bị cho đòn nghiền nát quyết định cuối cùng.\n\nĐộ chính xác tăng thêm 15% Sức mạnh, xuyên giáp tăng thêm 225% Sức mạnh.\nKéo dài 1 lượt.'
  - **Reference**: data/json/martialarts.json
  - **Tokens**: {'english': [(None, 'integer', 'o'), (None, 'integer', 'o')], 'vietnamese': []}
- **English**: 'A solid stance allows you to block more damage than normal and deliver better punches.\n\n+1 accuracy, blocked damage reduced by 50% of Strength.'
  - **Vietnamese**: 'Thế đứng vững chắc cho phép bạn chặn được nhiều sát thương hơn bình thường và tung ra những cú đấm tốt hơn.\n\n+1 độ chính xác, sát thương chặn được giảm đi 50% Sức mạnh.'
  - **Reference**: data/json/martialarts.json
  - **Tokens**: {'english': [(None, 'integer', 'o')], 'vietnamese': []}
- **English**: "You've seen your chance.  Now strike back!\n\n+25% damage.\nLasts 1 turn."
  - **Vietnamese**: 'Bạn đã nhìn thấy cơ hội của mình. Giờ hãy phản công!\n\n+25% sát thương.\nKéo dài 1 lượt.'
  - **Reference**: data/json/martialarts.json
  - **Tokens**: {'english': [(None, 'integer', 'd')], 'vietnamese': [(None, 's', 's')]}
- **English**: "Hit or miss, it's just part of the dance!  And the best part is about to start!\n\n+5% damage.\nLasts 1 turn.  Stacks 3 times."
  - **Vietnamese**: 'Trúng hay trượt, đó chỉ là một phần của điệu nhảy! Và phần hay nhất sắp bắt đầu!\n\n+5% sát thương.\nKéo dài 1 lượt. Cộng dồn 3 lần.'
  - **Reference**: data/json/martialarts.json
  - **Tokens**: {'english': [(None, 'integer', 'd')], 'vietnamese': [(None, 's', 's')]}
- **English**: "Your attacks strike at your opponents' weak spots with speed and precision instead of brute force.\n\nBash damage increased by 75% of Dexterity but decreased by 75% of Strength."
  - **Vietnamese**: 'Các đòn tấn công của bạn nhắm vào điểm yếu của đối thủ bằng tốc độ và sự chính xác thay vì sức mạnh thô bạo.\n\nSát thương đập tăng thêm 75% Khéo léo nhưng giảm đi 75% Sức mạnh.'
  - **Reference**: data/json/martialarts.json
  - **Tokens**: {'english': [(None, 'integer', 'o'), (None, 'integer', 'o')], 'vietnamese': []}
- **English**: 'You plan your attacks far in advance, relying on your intuition instead of your speed to strike true.\n\nAccuracy increased by 25% of Intelligence but decreased by 25% of Dexterity, bash damage increased by 75% of Intelligence but decreased by 75% of Strength.'
  - **Vietnamese**: 'Bạn tính toán trước các đòn tấn công từ rất sớm, dựa vào trực giác thay vì tốc độ để đánh trúng đích.\n\nĐộ chính xác tăng thêm 25% Trí tuệ nhưng giảm đi 25% Khéo léo, sát thương đập tăng thêm 75% Trí tuệ nhưng giảm đi 75% Sức mạnh.'
  - **Reference**: data/json/martialarts.json
  - **Tokens**: {'english': [(None, 'integer', 'o'), (None, 'integer', 'o'), (None, 'integer', 'o'), (None, 'integer', 'o')], 'vietnamese': []}
- **English**: 'Life and combat are a circle.  An attack leads to a counter and to an attack once again.  Seek to complete this loop.\n\n+10% damage.\nLasts 3 turns.'
  - **Vietnamese**: 'Cuộc sống và chiến đấu là một vòng tuần hoàn. Một đòn tấn công dẫn đến phản đòn rồi lại dẫn đến tấn công. Hãy cố gắng hoàn thành vòng lặp này.\n\n+10% sát thương.\nKéo dài 3 lượt.'
  - **Reference**: data/json/martialarts.json
  - **Tokens**: {'english': [(None, 'integer', 'd')], 'vietnamese': [(None, 's', 's')]}
- **English**: 'You take a moment to reflect on battles past and to come.  This insight will protect you from future harm.\n\n+2 blocking effectiveness, blocked damage reduced by 100% of Intelligence, +1.0 Dodging skill.\nLasts 2 turns.'
  - **Vietnamese**: 'Bạn dành một chút thời gian để suy ngẫm về những trận chiến đã qua và sắp tới. Sự thấu hiểu này sẽ bảo vệ bạn khỏi bị tổn hại trong tương lai.\n\n+2 hiệu quả chặn, sát thương chặn được giảm đi 100% Trí tuệ, +1.0 kỹ năng Né đòn.\nKéo dài 2 lượt.'
  - **Reference**: data/json/martialarts.json
  - **Tokens**: {'english': [(None, 'integer', 'o')], 'vietnamese': []}
- **English**: 'You can follow up a critical hit with a stronger attack if the opportunity presents itself.\n\n+15% damage.\nEnables "Combination Strike" technique.\nLasts 3 turns.  Stacks 3 times.'
  - **Vietnamese**: 'Bạn có thể bồi thêm sau một đòn chí mạng bằng một đòn tấn công mạnh hơn nếu cơ hội xuất hiện.\n\n+15% sát thương.\nKích hoạt kỹ thuật "Đòn đánh liên hoàn" (Combination Strike).\nKéo dài 3 lượt. Cộng dồn 3 lần.'
  - **Reference**: data/json/martialarts.json
  - **Tokens**: {'english': [(None, 'integer', 'd')], 'vietnamese': [(None, 's', 's')]}
- **English**: 'Your side stance reduces the chance that you will be harmed in combat.\n\nBlocked damage reduced by 50% of Dexterity.'
  - **Vietnamese**: 'Thế đứng nghiêng người giúp giảm nguy cơ bạn bị thương trong chiến đấu.\n\nSát thương chặn được giảm đi 50% Khéo léo.'
  - **Reference**: data/json/martialarts.json
  - **Tokens**: {'english': [(None, 'integer', 'o')], 'vietnamese': []}
- **English**: 'You are stalwart and will not budge against any threat.\n\n+2 blocking effectiveness, +1 block attempt, -1.0 Dodging skill, blocked damage reduced by 50% of Strength.'
  - **Vietnamese**: 'Bạn kiên định và sẽ không lùi bước trước bất kỳ mối đe dọa nào.\n\n+2 hiệu quả chặn, +1 lần thử chặn, -1.0 kỹ năng Né đòn, sát thương chặn được giảm đi 50% Sức mạnh.'
  - **Reference**: data/json/martialarts.json
  - **Tokens**: {'english': [(None, 'integer', 'o')], 'vietnamese': []}
- **English**: 'You moved and nullified the effects of Stand Your Ground!\n\n-2 blocking effectiveness, -1 block attempt, +1.0 Dodging skill, blocked damage increased by 50% of Strength.\nPrevents "High Round Strike" technique.\nLasts 2 turns.'
  - **Vietnamese**: 'Bạn đã di chuyển và làm mất tác dụng của Trụ Vững Vị Trí!\n\n-2 hiệu quả chặn, -1 lần thử chặn, +1.0 kỹ năng Né đòn, sát thương chặn tăng thêm 50% Sức mạnh.\nNgăn chặn kỹ thuật "Đòn quét tầm cao" (High Round Strike).\nKéo dài 2 lượt.'
  - **Reference**: data/json/martialarts.json
  - **Tokens**: {'english': [(None, 'integer', 'o')], 'vietnamese': []}

Full findings in `format_findings.jsonl`.
False positives are possible, especially for punctuation and colors.
