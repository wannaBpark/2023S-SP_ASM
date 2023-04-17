main    
        LDA     #gap
        +JSUB   stinit
READ    LDX     #0
RLOOP   TD       INDEV   .TEST INPUT DEVICE
        JEQ       RLOOP   .LOOP UNTIL DEVICE IS READY
        RD       INDEV   .READ ONE BYTE FROM INPUT DEVICE
        STCH   BUFFER,X    .STORE IT TO Buffer[x]
        LDA       #0
        LDCH   BUFFER,X    .DATA -> A Register
        TIXR   T       .x++
        COMP   EOL     .check A Register == '\n' ?
        STX       BUFLEN
        JEQ     CMPINST   .If end of line '\n', goto write
        J       RLOOP

WRITE   LDX       #0
WLOOP   TD      OUTDEV  .TEST OUTPUT DEVICE
        JEQ     WLOOP   .LOOP UNTIL OUTPUT DEVICE is READY
        LDCH   BUFFER,X    .LOAD DATA -> A Register
        WD       OUTDEV  .WRITE ONE BYTE -> OUTPUT DEVICE
        TIX     BUFLEN
        JLT       WLOOP

WRITEF  LDX     #0
FLOOP   TD      OUTDEV
        JEQ     WLOOP   .LOOP UNTIL OUTPUT DEVICE is READY
        LDCH    cFULL,X .LOAD cFULL -> A Register
        WD      OUTDEV  .WRITE ONE BYTE -> OUTPUT DEVICE
        TIX     #4
        JLT     FLOOP
        LDA     EOL
        WD      OUTDEV  .WRITE '\n'
        J       READ

WRITEN  LDX     #0
NLOOP   TD      OUTDEV
        JEQ     WLOOP    .LOOP UNTIL OUTPUT DEVICE is READY
        LDCH    cNONE,X  .LOAD cNONE -> A Register
        WD      OUTDEV  .WRITE ONE BYTE -> OUTPUT DEVICE
        TIX     #4
        JLT     NLOOP
        LDA     EOL
        WD      OUTDEV  .WRITE '\n'
        J       READ

CMPINST LDX     #0
        LDCH    BUFFER,X    .Buffer[x] -> A register - >TMP
        STA     TMP         
CHKINP  LDCH    cINPUT,X    .cINPUT[x] -> A register
        COMP    TMP         . if Buffer[x] == 'I'
        JEQ     CMPINP      . goto Compare 'INPUT'

        LDCH    cLIST,X
        COMP    TMP
        JEQ     CMPLIST
        
        LDCH    cDELETE, X
        COMP    TMP
        JEQ     CMPDLT

        LDCH	cFIND, X
        COMP	TMP
        JEQ	CMPFIND

CMPINP  LDCH   BUFFER,X    .Buffer[x] -> A register
        STA     TMP         .A register ->TMP
        LDCH   cINPUT,X    .Input[x] -> A register
        COMP   TMP         . if Buffer[x] == 'I'
        JLT     READ        . If it doesn't match, Jump to READ
        JGT     READ
        TIX     #6
        JLT     CMPINP      . not COMPARED ALL YET -> loop again
        
        LDA     TSIZE       . if TSIZE == 15 ???
        COMP   MAXSIZE        
        JEQ     WRITEF      . WRITE "FULL"

        LDCH   BUFFER,X    . A regsiter = Buffer[++x]
        LDX     TSIZE       . TMP -> Tree[X = Tsize]
        STCH   TREE,X
        TIX     TSIZE       . Tsize++
        STX     TSIZE
        J       READ

CMPLIST LDCH   BUFFER,X    .Buffer[x] -> A register
        STA     TMP         .A register ->TMP
        LDCH   cLIST,X     .Input[x] -> A register
        COMP   TMP         . if Buffer[x] == 'L'
        JLT     READ        . If it doesn't match, Jump to READ
        JGT     READ
        TIX     #4
        JLT     CMPLIST      . not COMPARED ALL YET -> loop again
        
        LDA     TSIZE       . if TSIZE == 0 ???
        COMP   #0        
        JEQ     READ        . Null tree -> goto READ
        LDA     #0
        JSUB   porder      . call POST_ORDER

        LDA	#0
        STA	CNT
        J       READ        . back to main

porder  
        COMP   TSIZE       . if A >= TSIZE, return;
        JEQ   exitP
        JGT   exitP

        STA     tmpA
        STL     tmpL

        .+JSUB   print       .printf(A)
        .if this is delete node T register ->1
        +JSUB   push        .push A
        LDA     tmpL
        +JSUB   push        .push L
        LDA     tmpA        . A = tmpA

        MUL     #2
        ADD   #1
        +JSUB   porder

        +JSUB   pop         .finished left child, pop parent
        STA   tmpL
        +JSUB   pop
        STA   tmpA        .real parent -> tmpA

        +JSUB   push        .push parent itself so that load parent after traversing right child
        LDA     tmpL
        +JSUB   push        .push L
        LDA     tmpA        . A = tmpA

        MUL     #2
        ADD   #2

        +JSUB   porder

        +JSUB   pop         .pop it self so we can print it out parent(current)
        STA   tmpL
        +JSUB   pop
        STA   tmpA        .real parent -> tmpA

        . delete 
        . T register -> 0 if this is Delete Node
        +JSUB   print

        LDL     tmpL
exitP   RSUB


print   LDX     tmpA
printL  TD      OUTDEV
        JEQ     printL  .LOOP UNTIL OUTPUT DEVICE is READY
        LDA	FLAG    . if FLAG == 1 (after founded), dont print
        COMP	#1
        JEQ	exitPR  
        LDCH    TREE,X  .LOAD TREE[x] -> A 
        COMP    NULL
        JEQ     exitPR   .If Tree[x] == NULL, return;
        

        WD      OUTDEV  .WRITE ONE BYTE
        LDA     EOL
        WD      OUTDEV  .WRITE '\n'

        LDA	CNT
        ADD	#1
        STA	CNT
exitPR  RSUB

CMPDLT  LDCH    BUFFER,X    .Buffer[x] -> A register
        STA     TMP         .A register ->TMP
        LDCH    cDELETE,X     .Input[x] -> A register
        COMP    TMP         . if Buffer[x] == 'L'
        JLT     READ        . If it doesn't match, Jump to READ
        JGT     READ
        TIX     #7
        JLT     CMPDLT      . not COMPARED ALL YET -> loop again
        
        LDCH    BUFFER,X       . store Delete node value
        STA     wDLT        
        LDA	#0
        JSUB    delete      . call POST_ORDER

        LDA	POS
        COMP	#0
        JEQ	WRITEN
        LDA	#0
        STA	POS         . POS = 0 , FLAG = 0 after using it
        STA	FLAG
        J       READ        . back to main

delete  
        COMP    TSIZE       . if A >= TSIZE, return;
        JEQ     exitD
        JGT     exitD

        STA     tmpA
        STL     tmpL

        LDX     tmpA
        LDCH    TREE,X   .if this is delete node FLAG = 1
        COMP    wDLT
        JEQ     setF
        J       nosetF

setF    LDA     #1
        STA     FLAG
nosetF
        LDA     tmpA
        +JSUB   push        .push A
        LDA     tmpL
        +JSUB   push        .push L
        LDA     tmpA        . A = tmpA

        MUL     #2
        ADD     #1
        +JSUB   delete

        +JSUB   pop         .finished left child, pop parent
        STA     tmpL
        +JSUB   pop
        STA     tmpA        .real parent -> tmpA

        +JSUB   push        .push parent itself so that load parent after traversing right child
        LDA     tmpL
        +JSUB   push        .push L
        LDA     tmpA        . A = tmpA

        MUL     #2
        ADD     #2

        +JSUB   delete

        +JSUB   pop         .pop it self so we can print it out parent(current)
        STA     tmpL
        +JSUB   pop
        STA     tmpA        .real parent -> tmpA

        LDX     tmpA        . if Tree[x] == wDLT (node we are looking for)
        LDCH	TREE,X      . POS = tmpA(x)
        COMP	wDLT
        JEQ	setPOS
        J	CHKFLG

setPOS
        LDA	tmpA
        STA	POS

CHKFLG
        LDA     FLAG        .if FLAG == 1, setNULL (Tree[x])
        COMP    #1
        JEQ     setNULL
        J       nosetFZ
setNULL
        LDX     tmpA        .Tree[x] = NULL
        LDA     NULL
        STCH    TREE,X
        LDA	#2        

restrF  LDA     tmpA       .A = tmpA, restore A value
        COMP    POS        .if A == POS (Parent Position)
        JEQ     setFZ
        J       nosetFZ
setFZ   LDA     #0
        STA     FLAG
nosetFZ
        LDA     tmpA            .restore tmpA -> A reg , POS = tmpA if this is Delete Node

        LDL     tmpL
exitD   RSUB

CMPFIND LDCH    BUFFER,X      .Buffer[x] -> A register
        STA     TMP           .A register ->TMP
        LDCH    cFIND,X       .Input[x] -> A register
        COMP    TMP           . if Buffer[x] == 'L'
        JLT     READ          . If it doesn't match, Jump to READ
        JGT     READ
        TIX     #5
        JLT     CMPFIND       . not COMPARED ALL YET -> loop again
        
        LDCH    BUFFER,X      . store FIND node value
        STA     wFIND        
        LDA	#0
        JSUB    find          . call IN_ORDER FIND
RETFOUND
        LDA	FLAG            . FLAG == 1, goto FOUND
        COMP	#1
        JEQ	FOUND

        LDA	#0
        STA     CNT
        J	WRITEN          .NOT FOUND -> print NONE
FOUND
        LDA	#0              . FLAG = 0
        STA	FLAG

        J	WRITEC          . WRITE C

WRITEC  LDX     #0
CLOOP   TD      OUTDEV
        JEQ     CLOOP    .LOOP UNTIL OUTPUT DEVICE is READY
        LDA     CNT       .LOAD cNONE -> A Register
        COMP	#10
        JLT	ONEDEC

        LDA	#49            .if doo zari soo, print '1'
        WD      OUTDEV
        LDA	CNT             .calculate first zari
        SUB	#10

ONEDEC
        ADD	#48
        WD      OUTDEV  .WRITE ONE BYTE -> OUTPUT DEVICE
        LDA     EOL
        WD      OUTDEV  .WRITE '\n'
        LDA	#0      . CNT = 0
        STA	CNT
        J       READ

find  
        COMP   TSIZE       . if A >= TSIZE, return;
        JEQ   exitF
        JGT   exitF

        STA     tmpA
        STL     tmpL

        .+JSUB   print       .printf(A)
        .if this is delete node T register ->1
        +JSUB   push        .push A
        LDA     tmpL
        +JSUB   push        .push L
        LDA     tmpA        . A = tmpA   PARENT

        MUL     #2
        ADD   #1
        +JSUB   find

        +JSUB   pop         .finished left child, pop parent
        STA   tmpL
        +JSUB   pop
        STA   tmpA        .real parent -> tmpA

        .if it equals, goto FOUND

        +JSUB   print 
        
        LDX	tmpA            . Tree[x] == wFIND?
        LDCH	TREE,X
        COMP	wFIND
        JEQ	FLAGONE
        J       NXT
FLAGONE
        J	FOUND
        LDA	#1              . if FOUND, FLAG = TRUE  
        STA	FLAG
NXT
        LDA	tmpA
        +JSUB   push        .push parent itself so that load parent after traversing right child
        LDA     tmpL
        +JSUB   push        .push L
        LDA     tmpA        . A = tmpA

        MUL     #2
        ADD   #2

        +JSUB   find

        +JSUB   pop         .pop it self so we can print it out parent(current)
        STA   tmpL
        +JSUB   pop
        STA   tmpA        .real parent -> tmpA

        . delete 
        . T register -> 0 if this is Delete Node     

        LDL     tmpL
exitF   RSUB

.STACK 
stinit  STA     stackp    . inicializira sklad na naslovu iz A
   RSUB
push    STA     @stackp   . spravi vrednost iz A na sklad
   LDA     stackp
   ADD     #3
   STA     stackp
   RSUB    
pop     LDA     stackp    . spravi vrednost s sklada v A
   SUB     #3
   STA     stackp
   LDA     @stackp
   RSUB
   
stackp  RESW    1      . stackptr

INDEV   BYTE    0
OUTDEV  BYTE    1
DATA    RESB    1
BUFFER  RESB    10
BUFLEN  WORD    0
EOL     WORD    10
cINPUT  BYTE    C'INPUT '
cFIND   BYTE    C'FIND '
cDELETE BYTE    C'DELETE '
cLIST   BYTE    C'LIST'
cFULL   BYTE    C'FULL'
cNONE   BYTE    C'NONE'
TREE    BYTE    C'000000000000000'
TSIZE   WORD    0                   .STORE CURRENT TREE INDEX
MAXSIZE WORD    15
TWO     WORD    2
TMP     RESW    1                   .STORE TMP of Buffer[x]

tmpA    WORD    1
tmpL    RESW    1
gap     RESW    64
NULL    WORD    0
wDLT    WORD    0
wFIND   WORD    0
FLAG    WORD    0
POS     WORD    0
CNT     WORD    0