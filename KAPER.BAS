5 poke 53265,peek(53265) and 239 : rem screen off (DEN bit)
10 rem created with petmate
15 DIM MF(63),MD(63):SP=8:SR=5
20 poke 53280,14
30 poke 53281,6
40 poke 53272,21
45 GOSUB 5000
50 DATA 63
60 DATA 4455,29,4205,29,4455,57,2228,29,0,29,2228,57,3338,29,2973,29
70 DATA 2807,29,3338,29,4455,29,0,29,4455,29,5613,29,5001,29,4455,29
80 DATA 5001,57,2500,57,2500,57,5001,29,4455,29,4205,29,2500,29,3338,29
85 DATA 2500,29,3338,57,3746,29,4205,29,4455,7,5001,7,4455,14,4205,29
86 DATA 3746,29,3338,29,3746,7,4205,7,3746,14,3338,29,2973,29,2807,29
87 DATA 2973,7,3338,7,2973,14,2807,29,2500,29,2228,29,2500,29,2228,29
88 DATA 2103,29,1873,29,1669,29,2228,29,2103,29,2500,29,2228,29,2807,29
89 DATA 2500,29,2973,29,2807,14,2973,14,2807,29,2228,57,2228,57
90 SK=0:PRINT CHR$(147);:GOTO 600
150 GET K$: IF K$<>"" THEN 160
152 IF TI=T THEN 150
154 T=TI:FOR J=1 TO SR:GOSUB 5100:NEXT:GOTO 150
160 POKE 54276,0:POKE 53265,PEEK(53265)AND239:POKE 198,0:PRINT
164 PRINT CHR$(147);
166 PRINT
167 PRINT
165 PRINT "LOAD"CHR$(34)"MAIN"CHR$(34)",8"
169 PRINT
170 PRINT
171 PRINT
172 PRINT
173 PRINT "RUN"
174 PRINT CHR$(19);
175 POKE 631,13:POKE 632,13:POKE 198,2
176 END
600 PRINT '{gry3}{rvs on}{172}{162}{162}{162}{162}{162}{162}{162}{162}{162}{162}{187}{lred}                            {rvs off}';
602 PRINT '{gry3}{161}     {red}{rvs on}{123}{gry3}{rvs off}    {rvs on}{161}{lred} {gry3}{rvs off}                          {lred}{rvs on} {rvs off}';
604 PRINT '{gry3}{161} {lblu} {gry3}{rvs on}{169}{127} {188}{rvs off}{187}{lblu}  {gry3} {rvs on}{161}{lred} {gry3}{rvs off}{127}{161}{rvs on} {rvs off} {lblu}   {gry3}        {lblu} {gry3}   {lblu}  {gry3} {lblu}  {gry3} {lblu} {lred}{rvs on} {rvs off}';
606 PRINT '{gry3}{161} {rvs on}{169}  {127}{172}{rvs off}{190}{lblu}  {gry3} {rvs on}{161}{lred} {gry3}{rvs off} {rvs on}{188}{rvs off}{169} {184}{184}{rvs on}{127}{lblu}{rvs off} {gry3}{rvs on}{127}{162}{127}{lblu}{rvs off} {gry3}{rvs on}{169}{rvs off}{162} {184}{184}{rvs on}{127}{rvs off} {lblu}  {gry3}{rvs on}{162}{lblu}{rvs off} {gry3}{rvs on}  {127}{lred} {rvs off}';
608 PRINT '{gry3}{161} {rvs on}{162}{162}{172}{162}{rvs off}{161}{lred}{rvs on}  {lblu}{rvs off} {gry3} {rvs on}{161}{lred} {lblu}{rvs off} {gry3}{rvs on}{172}{127}{white}{rvs off} {gry3}{rvs on}{169}{162} {white}{rvs off} {gry3}{rvs on} {rvs off}{162}{169}{lblu} {gry3}{rvs on}{161}{rvs off}{161}{white} {gry3}{rvs on}{169}{162} {rvs off}  {white} {gry3}{rvs on} {lblu}{rvs off} {gry3}{rvs on} {rvs off} {rvs on} {lred} {rvs off}';
610 PRINT '{gry3}{rvs on}{188}{rvs off}{162}{162}{lred}{rvs on}      {gry3}{rvs off}{162}{162}{rvs on}{190}{lred} {gry3}{169}{rvs off}{161}{rvs on} {rvs off} {188}{162}{127} {rvs on} {rvs off} {lblu}  {gry3}{188}{rvs on}{188}{white}{rvs off} {gry3}{188}{162}{127} {127}{162}{169}{white} {gry3}{rvs on} {lblu}{rvs off} {gry3}{rvs on} {lred} {rvs off}';
612 PRINT '{gry3}{161} {lblu} {lred}{rvs on}{162}{162}{162}{162}{162}{lblu}{rvs off}  {gry3} {rvs on}{161}{lred} {gry3}{rvs off}       {188}{rvs on}{162}{rvs off}{190}            {white} {gry3} {lblu} {gry3} {lred}{rvs on} {rvs off}';
614 PRINT '{gry3}{161}   u{174}     {rvs on}{161}{lred} {lblu}{rvs off} {white} {lblu} {gry3}{127}{161}{rvs on} {rvs off}     {white} {gry3}   {lblu} {gry3}   {white} {gry3}   {lblu}   {lred}{rvs on} {rvs off}';
616 PRINT '{gry3}{161}{lblu}   {gry3}gb{lblu} {gry3}    {rvs on}{161}{lred} {gry3}{rvs off} {white} {lblu}  {gry3}{rvs on}{188}{rvs off}{169}{white} {gry3}{184}{184}{rvs on}{127}{rvs off} {rvs on}{127}{162}{127}{rvs off} {rvs on}{169}{162}{127}{rvs off}{188}{162}{rvs on}{172}{127}{rvs off} {lblu}   {lred}{rvs on} {rvs off}';
618 PRINT '{gry3}{161}      u{174}  {rvs on}{161}{lred} {white}{rvs off} {lblu}   {gry3}{rvs on}{172}{127}{rvs off} {rvs on}{169}{162} {rvs off} {rvs on} {rvs off}{162}{169} {rvs on} {rvs off}{162}{rvs on} {rvs off} {rvs on} {rvs off} {rvs on}{162}{rvs off} {lblu}   {lred}{rvs on} {rvs off}';
620 PRINT '{gry3}{161}{lblu}      {gry3}gb  {rvs on}{161}{lred} {lblu}{rvs off}   {gry3}{rvs on}{169}{rvs off}{161}{rvs on} {lblu}{rvs off} {gry3}{188}{162}{127} {rvs on} {rvs off}  {lblu} {gry3}{127}{162}{187}{172}{rvs on} {rvs off}{187}  {lblu}   {lred}{rvs on} {rvs off}';
622 PRINT '{gry3}{161}{lblu}         {gry3} {rvs on}{161}{lred} {gry3}{rvs off}          {188}{rvs on}{162}{rvs off}{190}             {lred}{rvs on} {rvs off}';
624 PRINT '{gry3}{rvs on}{188}{rvs off}{162}{162}{162}{162}{162}{162}{162}{162}{162}{162}{rvs on}{190}{lred}                            {rvs off}';
626 PRINT '{lblu}          {gry3} {lblu}                  {gry3}{172}{rvs on}    {rvs off}{187}{lblu}     ';
628 PRINT '{lblu}               {lred}{162}{162}{rvs on}    {209}{lblu}{rvs off}     {gry3} {172}{rvs on}      {rvs off}{187}{lblu}    ';
630 PRINT '{lblu}            {lred} {rvs on}     {cyn} {lred}{162}{lblu}{rvs off}      {gry3}  {rvs on}        {rvs off} {lblu}   ';
632 PRINT '{lblu}            {lred}q{rvs on}   {cyn}   {lblu}{rvs off}       {gry3}  {rvs on}  {rvs off} {rvs on}  {rvs off} {rvs on}  {rvs off}  {lblu}  ';
634 PRINT '{lblu}              {cyn}{rvs on}{162}{162}{162}{162}{162}{lblu}{rvs off}       {gry3}  {rvs on}        {lblu}{rvs off} {gry3} {lblu}  ';
636 PRINT '{lblu}                          {gry3} {lblu} {gry3}{188}{rvs on}      {rvs off}{190}{lblu} {gry3} {lblu}  ';
638 PRINT '{lblu}  {gry3} {162}{162}{162}{162}{162}{162}{162}{162}{lblu}              {gry3}{162}{187}{lblu}  {gry3}{188}{rvs on}    {rvs off}{190}{lblu}   {gry3}{162}{lblu} ';
640 PRINT '{lblu} {gry3} {160}{161}{rvs on}{lred}      {rvs off}{gry3}{rvs on}{161}{lblu}{rvs off}             {gry3}{rvs on}{225}  {rvs off}{187}{lblu}  {gry3}{rvs on} {rvs off}{162}{162}{rvs on} {rvs off}{lblu}  {gry3}{172}{rvs on}  {rvs off}{161}';
642 PRINT '{lblu} {gry3} {160}{rvs on}{188}{rvs off}{162}{162}{162}{162}{162}{162}{rvs on}{190}{lblu}{rvs off}              {gry3}{184}{184}{rvs on} {rvs off}{162}{lblu} {gry3}{188}{rvs on}  {rvs off}{190}{lblu} {gry3}{162}{rvs on} {rvs off}{184}{184}{lblu} ';
644 PRINT '{lblu} {gry3}  {161}RIGS- {lblu}{gry3}{rvs on}{161}{lblu}{rvs off}                 {gry3}{184}{rvs on} {rvs off}{162} {lblu} {gry3}{162}{rvs on} {rvs off}{184}{lblu}    ';
646 PRINT '{lblu} {gry3}  {161}DALERE{rvs on}{161}{lblu}{rvs off}                  {gry3}{184}{rvs on}   {rvs off}{184}{lblu}      ';
648 PRINT '{lblu}   {gry3}{rvs on}{188}{rvs off}{162}{162}{162}{162}{162}{162}{rvs on}{190}{lblu}{rvs off}                  {gry3}{162}{rvs on} {rvs off}{184}{184}{rvs on} {rvs off}{162}{lblu}';
649 POKE 53265,PEEK(53265) OR 16
650 PRINT '{home}{24 down}{gry3}TRYK PAA EN TAST FOR AT FORTSAETTE!';
651 PRINT '{home}'
652 T=TI:GOTO 150

5000 REM INIT MUSIC (VOICE 1 SQUARE WAVE)
5010 POKE 54296,15
5020 POKE 54277,9: POKE 54278,240
5030 POKE 54274,0: POKE 54275,8
5040 RESTORE
5050 READ MC
5060 FOR I=1 TO MC: READ MF(I),MD(I): NEXT
5070 MI=1: MT=0: MG=1: MP=0: PH=1
5080 POKE 54276,0
5090 RETURN
5100 REM ADVANCE MUSIC ONE TICK
5110 IF MG=0 THEN RETURN
5120 IF MT>0 THEN MT=MT-1: IF MT=0 AND PH=0 AND MP>0 THEN POKE 54276,0: MT=MP: PH=1
5122 IF MT>0 THEN RETURN
5130 IF MI>MC THEN MG=0: POKE 54276,0: RETURN
5140 F=MF(MI): D=MD(MI): MI=MI+1
5150 TT=INT((D+SP-1)/SP): IF TT<1 THEN TT=1
5160 IF F=0 THEN POKE 54276,0: MT=TT: MP=0: PH=1: RETURN
5170 MT=INT((TT*3)/4): IF MT<1 THEN MT=1
5172 MP=TT-MT: IF MP<0 THEN MP=0
5180 F2=F+F+F+F
5190 H=INT(F2/256): L=F2-H*256
5192 IF H<0 THEN H=0
5194 IF H>255 THEN H=255
5196 IF L<0 THEN L=0
5198 IF L>255 THEN L=255
5199 POKE 54272,L: POKE 54273,H
5200 POKE 54276,65: PH=0
5210 RETURN
