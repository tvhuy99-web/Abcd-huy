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
| issues | 35 |
| markup_tag_mismatch | 35 |
| strings_checked | 116272 |

## markup_tag_mismatch: up to 25 examples

- **English**: "Don't move a <freaking> muscle…"
  - **Vietnamese**: 'Đừng có nhúc nhích dù chỉ một cơ bắp…'
  - **Reference**: src/dialogue_chatbin.h:125
  - **Tokens**: {'english': {'open_freaking': 1}, 'vietnamese': {}}
- **English**: 'Are you <freaking> insane!?'
  - **Vietnamese**: 'Mày bị điên rồi à!?'
  - **Reference**: src/dialogue_chatbin.h:154
  - **Tokens**: {'english': {'open_freaking': 1}, 'vietnamese': {}}
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
- **English**: "&<mypronoun> seems completely lost to the present, but you can't help but notice <mypossesivepronoun> gear is caked with mud and broken twigs.  It doesn't look like you'll be getting much conversation from this one."
  - **Vietnamese**: '&Cô ấy dường như hoàn toàn mất kết nối với thực tại, nhưng bạn không thể không để ý rằng trang bị của cô ấy bám đầy bùn đất và cành cây gãy. Có vẻ bạn sẽ chẳng moi được mấy câu chuyện từ người này đâu.'
  - **Reference**: data/json/npcs/robofac/robofac_ancilla_npcs/BAR_ENCOUNTER_MERCENARIES/BEM_jabberwock.json
  - **Tokens**: {'english': {'open_mypronoun': 1, 'open_mypossesivepronoun': 1}, 'vietnamese': {}}
- **English**: '&Continued prodding manages to extract a baffling response from your interlocutor "I- I\'ve seen it, the god of our new world, a god of flesh and bone." <mypronoun> says with a maddened glare, as though this were some sort of shocking revelation.'
  - **Vietnamese**: '&Sự gặng hỏi liên tục cuối cùng cũng moi được một câu trả lời khó hiểu từ người đối thoại: "T-tôi đã thấy nó rồi, vị thần của thế giới mới của chúng ta, một vị thần bằng máu thịt và xương cốt." Cô ấy nói với ánh mắt điên dại, như thể đây là một phát hiện gây sốc tột cùng.'
  - **Reference**: data/json/npcs/robofac/robofac_ancilla_npcs/BAR_ENCOUNTER_MERCENARIES/BEM_jabberwock.json
  - **Tokens**: {'english': {'open_mypronoun': 1}, 'vietnamese': {}}
- **English**: '&Continued prodding manages to extract a baffling response from your interlocutor "I- I\'ve seen it, the god of our new world, a god of flesh and bone." <mypronoun> says with a maddened glare, as though this were some sort of shocking revelation.'
  - **Vietnamese**: '&Sự gặng hỏi liên tục cuối cùng cũng moi được một câu trả lời khó hiểu từ người đối thoại: "T-tôi đã thấy nó rồi, vị thần của thế giới mới của chúng ta, một vị thần bằng máu thịt và xương cốt." Anh ấy nói với ánh mắt điên dại, như thể đây là một phát hiện gây sốc tột cùng.'
  - **Reference**: data/json/npcs/robofac/robofac_ancilla_npcs/BAR_ENCOUNTER_MERCENARIES/BEM_jabberwock.json
  - **Tokens**: {'english': {'open_mypronoun': 1}, 'vietnamese': {}}
- **English**: '&Continued prodding manages to extract a baffling response from your interlocutor "I- I\'ve seen it, the god of our new world, a god of flesh and bone." <mypronoun> says with a maddened glare, as though this were some sort of shocking revelation.'
  - **Vietnamese**: '&Sự gặng hỏi liên tục cuối cùng cũng moi được một câu trả lời khó hiểu từ người đối thoại: "T-tôi đã thấy nó rồi, vị thần của thế giới mới của chúng ta, một vị thần bằng máu thịt và xương cốt." Họ nói với ánh mắt điên dại, như thể đây là một phát hiện gây sốc tột cùng.'
  - **Reference**: data/json/npcs/robofac/robofac_ancilla_npcs/BAR_ENCOUNTER_MERCENARIES/BEM_jabberwock.json
  - **Tokens**: {'english': {'open_mypronoun': 1}, 'vietnamese': {}}
- **English**: '&<mypronoun> silently points at your maps with a trembling hand.'
  - **Vietnamese**: '&Người đó im lặng chỉ tay vào bản đồ của bạn bằng một bàn tay run rẩy.'
  - **Reference**: data/json/npcs/robofac/robofac_ancilla_npcs/BAR_ENCOUNTER_MERCENARIES/BEM_jabberwock.json
  - **Tokens**: {'english': {'open_mypronoun': 1}, 'vietnamese': {}}
- **English**: '&The mercenary seems uncharacteristically upbeat, or perhaps just drunk.  "Found this neat gizmo in a lab run a few days ago."  <mypronoun> blurts while procuring a small cube of etched glass from <mypossesivepronoun> pocket.  "Now, I have no use for such a thing, but perhaps you do.  What do you say?  It\'s yours for 3 coins."'
  - **Vietnamese**: '&Nữ lính đánh thuê có vẻ phấn chấn một cách khác thường, hoặc có lẽ chỉ là đang say rượu. "Tìm thấy món đồ chơi xịn sò này trong một lần đột kích phòng lab vài ngày trước." Cô ấy buột miệng trong khi móc ra một khối kính nhỏ khắc hoa văn từ trong túi áo của mình. "Giờ thì, tôi chẳng có việc gì cần dùng đến thứ này cả, nhưng biết đâu bạn lại cần. Thấy sao hả? Để lại cho bạn với giá 3 đồng xu."'
  - **Reference**: data/json/npcs/robofac/robofac_ancilla_npcs/BAR_ENCOUNTER_MERCENARIES/BEM_template_seller.json
  - **Tokens**: {'english': {'open_mypronoun': 1, 'open_mypossesivepronoun': 1}, 'vietnamese': {}}
- **English**: "Freaking storm's brewing, better brace yourself."
  - **Vietnamese**: 'Cơn bão <freaking> đang nổi lên, lo mà thủ thế đi.'
  - **Reference**: data/json/npcs/snippets/talk_tags_combat.json
  - **Tokens**: {'english': {}, 'vietnamese': {'open_freaking': 1}}
- **English**: '<mypronoun> will recharge power CBMs until <mypronoun> has 90% of total power.'
  - **Vietnamese**: '<mypronoun> sẽ nạp CBM năng lượng cho đến khi đạt 90% tổng năng lượng.'
  - **Reference**: data/json/npcs/snippets/talk_tags_follower.json
  - **Tokens**: {'english': {'open_mypronoun': 2}, 'vietnamese': {'open_mypronoun': 1}}

Full findings in `format_findings.jsonl`.
False positives are possible, especially for punctuation and colors.
