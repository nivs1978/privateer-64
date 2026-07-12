10 REM KAPTAJN KAPER - MAIN GAME
12 POKE 52,48:POKE 56,48:CLR
15 GOSUB 100
17 GOSUB 8000
20 GOSUB 2000
25 IX=100:IY=100:GOSUB 8300:REM SET INITIAL SHIP POSITION
30 GOTO 3010

41 REM ===================== MEMORY MAP (VIC BANK 0: $0000-$3FFF) =====================
42 REM LINE 12 SETS TOP OF BASIC RAM TO $3000 (48*256), PROTECTING $3000-$3FFF
43 REM $0400-$07FF ... DEFAULT SCREEN RAM (SPRITE PTRS AT $07F8=2040..2047)
44 REM $0801-$2FFF ... BASIC PROGRAM + VARIABLES + STRINGS (DO NOT PUT DATA HERE)
45 REM $3000-$37FF ... CUSTOM CHARSET (2KB, SELECTED VIA POKE 53272)
46 REM $3800-$383F ... SPRITE 0 SHIP DATA (64 BYTES, PTR=224 -> 224*64=14336)
47 REM $3840-$3FFF ... FREE PROTECTED RAM FOR MORE SPRITES/DATA (PTR 225..255)
48 REM $C000-$C044 ... MACHINE CODE CHARSET COPY ROUTINE (49152, RUN VIA SYS)
49 REM ================================================================================

100 REM INIT CUSTOM CHARSET AND REDEFINE CHAR 97, 115 AND 122 TO DANISH LETTER
110 REM RAM ALREADY RESERVED BEFORE GOSUB
101 REM === MACHINE CODE: COPY CHAR ROM TO RAM AT $3000 (DATA BLOCK) ===
102 DATA 120,173,1,0,72,41,251,141,1,0,162,0
103 DATA 189,0,208,157,0,48,189,0,209,157,0,49
104 DATA 189,0,210,157,0,50,189,0,211,157,0,51
105 DATA 189,0,212,157,0,52,189,0,213,157,0,53
106 DATA 189,0,214,157,0,54,189,0,215,157,0,55
107 DATA 232,208,205,104,141,1,0,88,96
108 REM === DANISH AE, OE and AA (8 BYTES EACH) ===
109 DATA 63,108,108,127,108,108,111,0
111 DATA 60,102,110,126,118,102,60,0
112 DATA 24,36,24,60,102,126,102,0
113 REM GRID CELL
114 DATA 1,1,1,1,1,1,1,255
120 FOR I=0 TO 68:READ B:POKE 49152+I,B:NEXT I
130 SYS 49152
140 POKE 53272,(PEEK(53272)AND240)OR12
150 REM Load Danish characters from DATA into custom charset at $3000
151 REM AE (Æ) -> spade ({97}), charset index 65
152 CA=12288+((97-32)*8)
153 FOR I=0 TO 7:READ D:POKE CA+I,D:NEXT I
154 REM OE (Ø) -> heart ({115}), charset index 83
155 CA=12288+((115-32)*8)
156 FOR I=0 TO 7:READ D:POKE CA+I,D:NEXT I
157 REM AA / Å -> diamond ({122}), charset index 90
158 CA=12288+((122-32)*8)
159 FOR I=0 TO 7:READ D:POKE CA+I,D:NEXT I
160 REM GRID CELL ({186}), charset index 90
161 CA=12288+((186-32)*8)
162 FOR I=0 TO 7:READ D:POKE CA+I,D:NEXT I

199 RETURN

2000 REM PLAYER NAME
2010 PRINT CHR$(147)
2020 PRINT CHR$(5)
2105 IX=240:IY=72:GOSUB 8300
2106 GOSUB 8100
2030 PRINT
2035 PRINT "{97}{115}{122}S{122}DAN SER DIT SKIB UD:"
2040 PRINT
2090 PRINT "HVAD ER DIT NAVN";
2100 INPUT N$
2110 IF N$="" THEN N$="KAPER"
2160 RETURN

3000 REM MAP VIEW: blank screen, hide sprite, draw map, restore
3010 PRINT CHR$(147)
3020 PRINT CHR$(5)
3030 REM POKE 53265,PEEK(53265) AND 239:REM screen off (DEN bit)
3040 GOSUB 8200:REM hide ship sprite
3050 GOSUB 11020:REM draw map (PRINT block with curly tokens)
3060 POKE 53265,PEEK(53265) OR 16:REM screen on
3070 GOSUB 8300:GOSUB 8100:REM show ship sprite
3100 GET A$: IF A$="" THEN 3100
3150 GOTO 3100:REM MAIN MAP LOOP

5000 REM HARBOR TEXT SCREEN
5010 PRINT CHR$(147)
5020 PRINT CHR$(5)
5030 PRINT "YOU ENTER A HARBOR."
5040 PRINT
5050 PRINT "THIS WILL LATER BECOME A MENU"
5060 PRINT "WITH BUY, SELL, REPAIR AND CREW"
5070 PRINT "OPTIONS."
5080 PRINT
5090 PRINT "PRESS ANY KEY TO RETURN TO SEA"
5100 GOSUB 9000
5110 RETURN

6000 REM MIST EVENT SCREEN
6010 PRINT CHR$(147)
6020 PRINT CHR$(5)
6030 PRINT "A MIST ROLLS IN ACROSS THE SEA."
6040 PRINT
6050 PRINT "THIS WILL LATER BE USED FOR"
6060 PRINT "RANDOM EVENTS, TREASURE, OR"
6070 PRINT "ENEMY ENCOUNTERS."
6080 PRINT
6090 PRINT "PRESS ANY KEY TO CONTINUE"
6100 GOSUB 9000
6110 RETURN

8000 REM SPRITE INIT - LOAD 3 SINGLE-COLOR SHIP SPRITES (0,1,2)
8005 REM DATA POINTER ALREADY ADVANCED BY CHARSET INIT; READ 3*64 BYTES
8006 REM LOAD TO $3800-$38BF (14336-14527): PTR 224,225,226
8007 FOR X=14336 TO 14336+191:READ Y:POKE X,Y:NEXT X
8020 POKE 2040,224:POKE 2041,225:POKE 2042,226:REM SPRITE POINTERS 0..2
8030 POKE 53287,10:POKE 53288,15:POKE 53289,3:REM COLORS: S0 LRED, S1 WHITE, S2 CYAN
8060 POKE 53276,PEEK(53276)AND 248:REM SPRITES 0..2 SINGLE-COLOR
8070 POKE 53277,PEEK(53277)AND 248:REM SPRITES 0..2 NORMAL WIDTH
8080 POKE 53271,PEEK(53271)AND 248:REM SPRITES 0..2 NORMAL HEIGHT
8090 RETURN

8100 REM SHOW SHIP - SPRITES 0,1,2
8110 POKE 53269,PEEK(53269)OR 7
8120 RETURN

8200 REM HIDE SHIP - SPRITES 0,1,2
8210 POKE 53269,PEEK(53269)AND 248
8220 RETURN

8300 REM SET SHIP POSITION (IX,IY) FOR SPRITES 0,1,2 OVERLAID
8310 IF IX>255 THEN POKE 53264,PEEK(53264)OR 7:SX=IX-256:GOTO 8330
8320 POKE 53264,PEEK(53264)AND 248:SX=IX
8330 POKE 53248,SX:POKE 53250,SX:POKE 53252,SX
8340 POKE 53249,IY:POKE 53251,IY:POKE 53253,IY
8350 RETURN

9000 REM WAIT FOR KEY PRESS
9010 GET A$: IF A$="" THEN 9010
9020 RETURN

9490 REM === MACHINE CODE: COPY CHAR ROM TO RAM AT $3000 ===
9491 REM 69 BYTES, READ BY LINE 120 (I=0 TO 68), RUN VIA SYS 49152

10000 REM === SPRITE 0-3 to make a combined ship sprite DATA: SHIP (3x64 BYTES) ===
10005 :: rem sprite1 / singlecolor / color: 10
10010 data 0,0,0,46,0,0,110,0,0,110,0,0,224,0,0,0
10020 data 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
10030 data 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
10040 data 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,10
10050 :: rem sprite2 / singlecolor / color: 15
10060 data 16,0,0,16,0,0,16,0,0,16,0,0,16,0,0,16
10070 data 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
10080 data 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
10090 data 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,15
10100 :: rem sprite3 / singlecolor / color: 3
10110 data 0,0,0,0,0,0,0,0,0,0,0,0,3,128,0,239
10120 data 128,0,127,128,0,127,0,0,0,0,0,0,0,0,0,0
10130 data 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
10140 data 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3

11000 REM === MAP PRINTED ===
11020 POKE 53280,14:POKE 53281,6
11030 PRINT '{clear}';
11040 PRINT '{lgrn}{rvs on}{2 32}{lred}{rvs off}POINT{58} 0 {lgrn}{rvs on}{5 32}{lred}{rvs off}MAL{58} 1{3 48} PA 3{2 48} TR{97}K{lgrn}{rvs on}{3 32}{rvs off}';
11050 PRINT '{lgrn}{rvs on}{40 32}{rvs off}';
11060 PRINT '{lgrn}{rvs on}{40 32}{rvs off}';
11070 PRINT '{lgrn}{rvs on}{6 32}{lred}{8 32}{red}{rvs off}{209}{lgrn}{15 186}{lred}{223}{rvs on}{3 32}{lgrn}{6 32}{rvs off}';
11080 PRINT '{lgrn}{rvs on}{6 32}{lred}{8 32}{rvs off}{169}{lgrn}{13 186}{lred}{205}{lgrn}{2 186}{lred}{223}{rvs on}{2 32}{lgrn}{6 32}{rvs off}';
11090 PRINT '{lgrn}{rvs on}{6 32}{lred}{3 32}{lgrn}{rvs off}{186}{lred}{rvs on}{3 32}{rvs off}{169}{lgrn}{14 186}{red}{209}{lred}{rvs on}{223}{lgrn}{rvs off}{186}{lred}{rvs on}{169}{2 32}{lgrn}{6 32}{rvs off}';
11100 PRINT '{lgrn}{rvs on}{6 32}{lred}{2 32}{rvs off}{169}{rvs on}{169} {lgrn}{rvs off}{186}{red}{209}{lgrn}{16 186}{lred}{rvs on}{5 32}{lgrn}{6 32}{rvs off}';
11110 PRINT '{lgrn}{rvs on}{6 32}{lred}{2 32}{lgrn}{rvs off}{186}{lred}{rvs on} {rvs off}{223}{lgrn}{186}{lred}{rvs on} {lgrn}{rvs off}{16 186}{lred}{223}{rvs on}{4 32}{lgrn}{6 32}{rvs off}';
11120 PRINT '{lgrn}{rvs on}{6 32}{lred}{2 32}{rvs off}{165}{lgrn}{16 186}{lred}{rvs on}{169}{3 32}{rvs off}{187}{223}{rvs on}{3 32}{lgrn}{6 32}{rvs off}';
11130 PRINT '{lgrn}{rvs on}{6 32}{lred}{2 32}{rvs off}{161}{lgrn}{15 186}{lred}{rvs on}{169}{4 32}{red}{rvs off}{209}{lgrn}{186}{lred}{rvs on}{3 32}{lgrn}{6 32}{rvs off}';
11140 PRINT '{lgrn}{rvs on}{6 32}{lred}{2 32}{rvs off}{161}{lgrn}{186}{lred}{rvs on}{161}{rvs off}{181}{lgrn}{5 186}{lred}{162}{185}{lgrn}{186}{lred}{185}{lgrn}{186}{red}{209}{lred}{rvs on} {187}{4 32}{lgrn}{rvs off}{2 186}{lred}{rvs on}{3 32}{lgrn}{6 32}{rvs off}';
11150 PRINT '{lgrn}{rvs on}{6 32}{lred}{2 32}{lgrn}{rvs off}{3 186}{lred}{rvs on} {rvs off}{187}{lgrn}{3 186}{lred}{205}{lgrn}{186}{lred}{184}{rvs on}{3 32}{lgrn}{rvs off}{2 186}{lred}{rvs on}{181}{4 32}{lgrn}{rvs off}{186}{lred}{rvs on}{223}{rvs off}{223}{rvs on}{2 32}{lgrn}{6 32}{rvs off}';
11160 PRINT '{lgrn}{rvs on}{6 32}{lred}{rvs off}{184}{169}{lgrn}{3 186}{lred}{rvs on} {rvs off}{169}{lgrn}{6 186}{lred}{rvs on}{2 32}{rvs off}{161}{lgrn}{186}{lred}{rvs on} {161}{4 32}{lgrn}{rvs off}{3 186}{lred}{223}{rvs on} {lgrn}{6 32}{rvs off}';
11170 PRINT '{lgrn}{rvs on}{6 32}{lred}{rvs off}{181}{lgrn}{4 186}{lred}{rvs on} {lgrn}{rvs off}{6 186}{lred}{rvs on}{4 32}{lgrn}{rvs off}{186}{lred}{rvs on} {rvs off}{181}{rvs on}{4 32}{223}{lgrn}{rvs off}{3 186}{lred}{rvs on} {lgrn}{6 32}{rvs off}';
11180 PRINT '{lgrn}{rvs on}{6 32}{lred}{rvs off}{181}{lgrn}{186}{lred}{rvs on} {lgrn}{rvs off}{5 186}{lred}{162}{rvs on}{2 32}{223}{6 32}{lgrn}{rvs off}{186}{lred}{rvs on}{4 32}{red}{rvs off}{209}{lgrn}{3 186}{lred}{223}{lgrn}{rvs on}{6 32}{rvs off}';
11190 PRINT '{lgrn}{rvs on}{6 32}{rvs off}{10 186}{red}{209}{lred}{rvs on}{10 32}{rvs off}{169}{rvs on}{161} {lgrn}{rvs off}{4 186}{rvs on}{6 32}{rvs off}';
11200 PRINT '{lgrn}{rvs on}{6 32}{rvs off}{9 186}{lred}{183}{184}{rvs on}{9 32}{rvs off}{169}{lgrn}{186}{lred}{rvs on}{165} {lgrn}{rvs off}{4 186}{rvs on}{6 32}{rvs off}';
11210 PRINT '{lgrn}{rvs on}{40 32}{rvs off}';
11220 PRINT '{lgrn}{rvs on}{40 32}{rvs off}';
11230 PRINT '{lgrn}{rvs on}{40 32}{rvs off}';
11240 PRINT '{lred}{176}{11 195}{178}{14 195}{178}{11 195}{174}';
11250 PRINT '{lred}{221}TR{97}K{58}{2 32}0 {lblu} {lred} {194}RGSDL{58} 6{2 48}{3 32}{lblu} {lred}{194}KORN{58} 30 {lblu}{2 32}{lred}{221}';
11260 PRINT '{lred}{171}{11 195}{219}{14 195}{219}{11 195}{179}';
11270 PRINT '{lred}{221}M{97}ND{58} 2{2 48} {blu}{rvs on} {lred}{rvs off}{194}SKYTS{58} 20{4 32}{blu}{rvs on} {lred}{rvs off}{194} REP{58} 2{2 48}{lblu}{2 32}{lred}{221}';
11280 PRINT '{lred}{173}{11 195}{177}{14 195}{177}{11 195}';
11290 POKE 2023,125:POKE 56295,10:REM LOWER RIGHT CHAR + LRED
11300 RETURN
