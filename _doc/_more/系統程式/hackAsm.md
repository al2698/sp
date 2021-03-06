# Hack 組合語言 / 組譯器

## 轉換過程

* [從 C 到組合語言](./c2asm)
* [從組合語言到機器碼](./asm2bin)

## Hack 組譯器

```
PS D:\ccc\course\sp\code\c\03-asmVm\hack\c> gcc asm.c c6.c -o asm
PS D:\ccc\course\sp\code\c\03-asmVm\hack\c> ./asm ../test/Add    
======= SYMBOL TABLE ===========
0: R0, 0
1: R1, 1
2: R2, 2
3: R3, 3
4: R4, 4
5: R5, 5
6: R6, 6
7: R7, 7
8: R8, 8
9: R9, 9
10: R10, 10
11: R11, 11
12: R12, 12
13: R13, 13
14: R14, 14
15: R15, 15
16: SCREEN, 16384
17: KBD, 24576
18: SP, 0
19: LCL, 1
20: ARG, 2
21: THIS, 3
22: THAT, 4
============= PASS1 ================
00:@2
01:D=A
02:@3
03:D=D+A
04:@0
05:M=D
======= SYMBOL TABLE ===========
0: R0, 0
1: R1, 1
2: R2, 2
3: R3, 3
4: R4, 4
5: R5, 5
6: R6, 6
7: R7, 7
8: R8, 8
9: R9, 9
10: R10, 10
11: R11, 11
12: R12, 12
13: R13, 13
14: R14, 14
15: R15, 15
16: SCREEN, 16384
17: KBD, 24576
18: SP, 0
19: LCL, 1
20: ARG, 2
21: THIS, 3
22: THAT, 4
============= PASS2 ================
  @2                   0000000000000010 0002
  D=A                  1110110000010000 ec10
  @3                   0000000000000011 0003
  D=D+A                1110000010010000 e090
  @0                   0000000000000000 0000
  M=D                  1110001100001000 e308
```


## HackCPU 虛擬機的用法


```
$ gcc asm.c c6.c -o asm
$ ./asm ../add/Add
```

上面的組譯器指令會產生 ../add/Add.bin 檔，於是我們可以用虛擬機執行該機器碼檔案！


```
PS D:\ccc\course\sp\code\c\03-asmVm\hack\c> gcc vm.c -o vm
PS D:\ccc\course\sp\code\c\03-asmVm\hack\c> ./vm ../test/Add.bin
PC=0000 I=0002 A=0001 D=0002 m[A]=0000
PC=0001 I=EC10 A=0002 D=0002 m[A]=0002 a=0 c=30 d=2 j=0
PC=0002 I=0003 A=0003 D=0003 m[A]=0002
PC=0003 I=E090 A=0004 D=0003 m[A]=0005 a=0 c=02 d=2 j=0
PC=0004 I=0000 A=0005 D=0000 m[A]=0005
PC=0005 I=E308 A=0006 D=0000 m[A]=0005 a=0 c=0C d=1 j=0
exit program !
```

