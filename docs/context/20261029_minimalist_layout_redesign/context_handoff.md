## Handoff Summary
Skill completed: ka-think
Next skill: ka-execute

## Locked Decisions (DO NOT reopen)
- [D1] [Overall Layout & Style]:
  - Triet ly toi gian cong thai hoc (Analogue Pocket / Panic Playdate style).
  - Toi uu man hinh E-Ink don sac 300 ppi tren Kindle Paperwhite 3 (1072x1448).
  - Man hinh game 6x (960x864) can giua (y = 72 px).
- [D2] [Top Bar & Navigation]:
  - An toan bo thanh nut chuc nang cong kenh cu.
  - Su dung bieu tuong Hamburger (☰) mong thanh lich o goc tren cung ben trai (x = 24 px, y = 16 px, hitbox 64x64 px).
  - Bam vao Hamburger mo popup / drawer menu toi gian chua cac chuc nang: Open ROM, Save, Load, Theme, Quit.
- [D3] [D-Pad Geometry & Aesthetic]:
  - Cum phim D-pad ngu giac huong tam bo goc theo dung hinh anh mau nguoi dung cung cap.
  - Tam rong (deadzone tron 36px) chong bam nham.
  - 4 canh ngu giac co vien manh 2.5px va bieu tuong chevron mui ten (^, v, <, >) stroke manh thanh lich.
  - Tam cum dat tai X = 220 px, Y = 1170 px.
- [D4] [Action Buttons (A/B) & Auxiliary (Select/Start)]:
  - Cum A/B: 2 nut tron duong kinh 92px dat cheo goc 28° theo ban kinh xoay ngon cai phai (B tai 740, 1200; A tai 890, 1120).
  - Select/Start: Dang vien nang ngang (capsule) 84x28px bo tron tai day trung tam (Y = 1380 px).
- [D5] [Touch Hitbox & E-Ink Visual Feedback]:
  - Hitbox mo rong 16px moi phia de bam khong can nhin.
  - Phan hoi thi giac tuc thi bang co che dao mau tuong phan cao (High-Contrast Inversion: nen chuyen den dac, icon/chu dao sang trang tinh khiet), triet tieu ghosting tren E-Ink.
  - Giu nguyen luong xu ly multi-touch 2 ngon muot ma tren CFS scheduler.

## Documented Assumptions
- [A1]: Ve vector toan bo cac nut bam bang Cairo 2D trong GTK2 de dam bao do sac net 300 ppi tuyet doi.
- [A2]: Menu Hamburger duoc render dang popover / overlay nhe nhang, khong chiem cho co dinh cua man hinh choi game.

## Open Questions
- Khong con cau hoi mo nao. Toan bo thiet ke da duoc khoa.

## Files to be created/modified in execution
- main.ui: Cap nhat cau truc widget GTK, thay thanh nut tren cung bang nut Hamburger o goc trai va khu vuc joystick_area.
- gambatte-k2.c: Cap nhat ham ve vector gb_controls_draw (Cairo drawing logic cho ngu giac D-pad, nut A/B, capsule Select/Start, Hamburger icon, va hieu ung dao mau tuong phan cao).