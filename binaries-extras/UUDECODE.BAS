10 REM KEY OFF  - on windows uuencoded file requires \r\n terminators 
15 rem linux   uuencode  binFile suggestedName >  outFile.uu        uuencode uudecod  uudecode > uudecode.uu
16 rem  processes 33 lines/min on pc xt ~1h for a normal 150K file
20 DEFINT A-Z
30 REM Trap error opening input file
40 ON ERROR GOTO 650
50 REM CLS
60 REM LOCATE 5,11
70 PRINT STRING$(40," ")
80 REM LOCATE 5,11
90 INPUT "Enter name of input file: ", INFILE$
100 REM OPEN "I",#1,INFILE$
110 OPEN INFILE$ FOR INPUT AS #1
120 REM LOCATE 8,10
130 PRINT STRING$(40," ")
140 REM Trap error opening output file
150 ON ERROR GOTO 700
160 REM LOCATE 8,10
170 INPUT "Enter name of output file: ", OUTFILE$
180 REM OPEN "R",#2,OUTFILE$,1
190 OPEN OUTFILE$ AS #2 LEN=1
200 FIELD #2,1 AS N$
210 REM Trap error at end of file
220 ON ERROR GOTO 750
230 REM Search for header line
240 LINE INPUT #1,A$
250 IF LEFT$(A$,5) <>"begin" THEN 240
260 REM LOCATE 11,10
270 PRINT "Header = ";A$
280 SP = ASC(" ")
290 DIM BUF(100)
300 RECNO# = 1
310 prog = 0
320 REM Main loop
330 LINE INPUT #1, A$
340 P = 0
350 IF prog MOD 100 = 0 THEN PRINT prog
360 prog = prog+1
370 BYTES = ASC(LEFT$(A$,1)) - SP
380 IF BYTES = 64 THEN BYTES = 0
390 IF BYTES = 0 THEN 600
400 COUNT% = INT(BYTES/3+.9): COUNT%=COUNT%*4
410 FOR I = 2 TO COUNT% STEP 4
420    X1 = ASC(MID$(A$,I,I)) - SP
430    IF X1 = 64 THEN X1 = 0
440    X2 = ASC(MID$(A$,I+1,I+1)) - SP
450    IF X2 = 64 THEN X2 = 0
460    X3 = ASC(MID$(A$,I+2,I+2)) - SP
470    IF X3 = 64 THEN X3 = 0
480    X4 = ASC(MID$(A$,I+3,I+3)) - SP
490    IF X4 = 64 THEN X4 = 0
500    IF P<BYTES THEN P = P + 1: BUF(P) = (X2\16) + (X1*4)
510    IF P<BYTES THEN P = P + 1: BUF(P) = (X3\4) + ((X2 MOD 16) * 16)
520    IF P<BYTES THEN P = P + 1: BUF(P) = X4 + ((X3 MOD 4) * 64)
530 NEXT I
540 FOR I = 1 TO P
550   LSET N$ = CHR$(BUF(I))
560   PUT #2, RECNO#
570   RECNO# = RECNO# + 1
580 NEXT I
590 GOTO 330
600 PRINT "done ..."
610 END
620 REM
630 REM Error trapping routines
640 REM
650 REM LOCATE 22,20
660 PRINT "Can't open input file"
670 END
680 GOSUB 810
690 RESUME 40
700 REM LOCATE 22,20
710 PRINT "Can't open output file"
720 END
730 GOSUB 810
740 RESUME 120
750 REM LOCATE 22,20
760 PRINT "Header line not found"
770 END
780 GOSUB 810
790 REM LOCATE 24,1
800 END
810 FOR I = 1 TO 5000: NEXT I
820 REM LOCATE 22,20
830 PRINT STRING$(30," ")
840 RETURN
