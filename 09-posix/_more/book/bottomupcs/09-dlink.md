# 第9章。動態鏈接


## 代碼共享


我們知道，對於操作系統代碼只被認為是只讀的，並且與數據分離。那麼，如果程序不能修改代碼並具有大量的公共代碼，而不是為每個可執行程序復制代碼，那麼應該在許多可執行程序之間共享它，這似乎是合乎邏輯的。

有了虛擬內存，這很容易做到。加載庫代碼的物理內存頁可以被任意數量的地址空間中的任意數量的虛擬頁輕松引用。因此，當您在系統內存中只有一個庫代碼的物理副本時，每個進程都可以訪問該庫代碼的任意虛擬地址。


因此，人們很快就想到了共享庫的概念，顧名思義，這個庫是由多個可執行文件共享的。每個可執行文件都包含一個引用，實際上是說“我需要庫foo”。當程序被加載,系統檢查如果有其他程序已加載的代碼庫foo到內存,從而分享它的頁映射到可執行的物理內存,或者可執行庫加載到內存中。


這個過程稱為動態鏈接，因為當程序在系統中執行時，它會“動態地”執行鏈接過程的一部分。


### 動態庫詳細信息

庫很像一個永遠不會啟動的程序。它們有代碼和數據部分(函數和變量)，就像每個可執行文件一樣;但是沒有地方開始跑步。它們只是提供了一個函數庫供開發人員調用。


因此ELF可以像表示可執行程序一樣表示動態庫。有一些基本的區別，例如沒有指向執行應該從哪裡開始的指針，但是所有共享庫都是ELF對像，就像任何其他可執行文件一樣。


ELF頭有兩個互斥的標志，ET_EXEC和ET_DYN，用於將ELF文件標記為可執行文件或共享對像文件。

### 在可執行文件中包含庫

### 編譯

當您編譯使用動態庫的程序時，對像文件會留下對庫函數的引用，就像其他任何外部引用一樣。


您需要包含庫的標頭，以便編譯器知道您調用的函數的特定類型。注意，編譯器只需要知道與函數關聯的類型(例如，它接受int並返回char *)，這樣它就可以正確地為函數調用.[28]分配空間

### 鏈接

盡管動態鏈接器為共享庫做了大量的工作，但是傳統的鏈接器在創建可執行文件時仍然可以發揮作用。


傳統的鏈接器需要在可執行文件中留下一個指針，以便動態鏈接器知道在運行時滿足依賴項的庫是什麼。


可執行文件的動態部分需要每個可執行文件所依賴的共享庫的必要條目。


同樣，我們可以使用readelf程序檢查這些字段。下面我們來看一個非常標准的二進制文件，/bin/ls


> 9.1的例子。指定動態庫

```
  1 
    $ readelf --dynamic /bin/ls 
    
    Dynamic segment at offset 0x22f78 contains 27 entries:
  5   Tag        Type                         Name/Value
     0x0000000000000001 (NEEDED)             Shared library: [librt.so.1]
     0x0000000000000001 (NEEDED)             Shared library: [libacl.so.1]
     0x0000000000000001 (NEEDED)             Shared library: [libc.so.6.1]
     0x000000000000000c (INIT)               0x4000000000001e30
 10  ... snip ...
```

您可以看到它指定了三個庫。大多數(如果不是全部的話)系統上的程序共享的最常見的庫是libc。還有一些程序需要正確運行的其他庫。


直接讀取ELF文件有時很有用，但是檢查動態鏈接的可執行文件的通常方法是通過ldd。ldd為您“遍歷”庫的依賴關系;也就是說，如果一個庫依賴於另一個庫，它會向您顯示它。


> 9.2的例子。查看動態庫

```
  1 
    $ ldd /bin/ls
            librt.so.1 => /lib/tls/librt.so.1 (0x2000000000058000)
            libacl.so.1 => /lib/libacl.so.1 (0x2000000000078000)
  5         libc.so.6.1 => /lib/tls/libc.so.6.1 (0x2000000000098000)
            libpthread.so.0 => /lib/tls/libpthread.so.0 (0x20000000002e0000)
            /lib/ld-linux-ia64.so.2 => /lib/ld-linux-ia64.so.2 (0x2000000000000000)
            libattr.so.1 => /lib/libattr.so.1 (0x2000000000310000)
    $ readelf --dynamic /lib/librt.so.1
 10 
    Dynamic segment at offset 0xd600 contains 30 entries:
      Tag        Type                         Name/Value
     0x0000000000000001 (NEEDED)             Shared library: [libc.so.6.1]
     0x0000000000000001 (NEEDED)             Shared library: [libpthread.so.0]
 15  ... snip ...
    
```

我們可以看到，在上面的某個地方需要libpthread。如果我們進行一些挖掘，我們可以看到需求來自於librt。


這在C標准中並不總是如此。以前，編譯器會假設它不知道的任何函數都會返回一個int，在32位系統中，指針的大小與int相同，所以沒有問題。但是，對於64位系統，指針的大小通常是int類型的兩倍，因此如果函數返回指針，它的值將被銷毀。這顯然是不可接受的，因為指針不會指向有效內存。C99標准發生了變化，要求您指定所包含函數的類型.

## 動態鏈接器

動態鏈接器是代表可執行文件管理共享動態庫的程序。它將庫加載到內存中，並在運行時修改程序以調用庫中的函數。


ELF允許可執行程序指定一個解釋器，這個程序應該用於運行可執行程序。編譯器和靜態鏈接器將依賴於動態庫的可執行程序的解釋器設置為動態鏈接器。


> 9.3的例子。檢查程序解釋器

```
  1 
    ianw@lime:~/programs/csbu$ readelf --headers /bin/ls
    
    Program Headers:
  5   Type           Offset             VirtAddr           PhysAddr
                     FileSiz            MemSiz              Flags  Align
      PHDR           0x0000000000000040 0x4000000000000040 0x4000000000000040
                     0x0000000000000188 0x0000000000000188  R E    8
      INTERP         0x00000000000001c8 0x40000000000001c8 0x40000000000001c8
 10                  0x0000000000000018 0x0000000000000018  R      1
          [Requesting program interpreter: /lib/ld-linux-ia64.so.2]
      LOAD           0x0000000000000000 0x4000000000000000 0x4000000000000000
                     0x0000000000022e40 0x0000000000022e40  R E    10000
      LOAD           0x0000000000022e40 0x6000000000002e40 0x6000000000002e40
 15                  0x0000000000001138 0x00000000000017b8  RW     10000
      DYNAMIC        0x0000000000022f78 0x6000000000002f78 0x6000000000002f78
                     0x0000000000000200 0x0000000000000200  RW     8
      NOTE           0x00000000000001e0 0x40000000000001e0 0x40000000000001e0
                     0x0000000000000020 0x0000000000000020  R      4
 20   IA_64_UNWIND   0x0000000000022018 0x4000000000022018 0x4000000000022018
                     0x0000000000000e28 0x0000000000000e28  R      8
    
```

上面可以看到，解釋器被設置為/lib/ld-linux-ia64.so。2，它是動態鏈接器。當內核加載要執行的二進制文件時，它將檢查PT_INTERP字段是否存在，如果存在，則將它指向的內容加載到內存中並啟動它。


我們提到過，動態鏈接的可執行程序會留下一些引用，這些引用需要用直到運行時才可用的信息來修復，比如共享庫中的函數地址。留下的引用稱為重定位。


### 搬遷

動態鏈接器的關鍵部分是在運行時修復地址，這是唯一可以確定內存中加載位置的時間。可以簡單地將重新定位看作是在加載時需要固定某個特定地址的說明。在代碼准備運行之前，您需要檢查並讀取所有的重定位，並修復它所引用的地址，以指向正確的位置。


>表9.1。搬遷的例子


Address | Action
--------|------------------
0x123456 | Address of symbol "x"
0x564773 | Function X

每個體系結構都有多種類型的重新定位，每種類型的確切行為都作為系統的ABI的一部分進行記錄。搬遷的定義相當直接。


> 9.4的例子。按照ELF的定義進行重新定位

```
  1 
    typedef struct {
      Elf32_Addr    r_offset;  <--- address to fix
      Elf32_Word    r_info;    <--- symbol table pointer and relocation type
  5 }
    
    typedef struct {
      Elf32_Addr    r_offset;
      Elf32_Word    r_info;
 10   Elf32_Sword   r_addend;
    } Elf32_Rela
    
```

r_offset字段指需要修復的文件中的偏移量。r_info字段指定了重新定位的類型，該類型描述了修復這段代碼需要做什麼。通常為架構定義的最簡單的重新定位就是符號的值。在這種情況下，只需在指定的位置替換符號的地址，重新定位就“固定”了。



這兩種類型，一種帶有一個加數，另一種沒有指定不同的重定位操作方式。addend是應該添加到固定地址以找到正確地址的簡單內容。例如，如果對符號i進行重新定位，因為原始代碼正在執行類似於i[8]的操作，那麼addend將被設置為8。意思是“找到我的地址，然後通過它”。

這個addend值需要存儲在某處。這兩種形式涵蓋了這兩種解決方案。在REL表單中，addend實際上存儲在程序代碼中固定地址的位置。這意味著要正確地修復地址，您需要首先讀取您將要修復的內存，以獲得任何addend、存儲它、找到“真正的”地址、將addend添加到其中，然後(在addend之上)將其寫回。RELA格式指定了重新定位中的addend。


每種方法的權衡應該是明確的。使用REL時，您需要進行額外的內存引用，以便在修復之前找到加數，但是您不會在二進制文件中浪費空間，因為您使用了重新定位目標內存。使用RELA時，您保留了已重新定位的加數，但浪費了磁盤上二進制文件中的空間。大多數現代系統使用RELA重新定位。

### 行動中的重新定位

下面的示例展示了重新定位的工作方式。我們創建了兩個非常簡單的共享庫，並從另一個中引用其中一個。


> 9.5的例子。指定動態庫

```
  1 
    $ cat addendtest.c
    extern int i[4];
    int *j = i + 2;
  5 
    $ cat addendtest2.c
    int i[4];
    
    $ gcc -nostdlib -shared -fpic -s -o addendtest2.so addendtest2.c
 10 $ gcc -nostdlib -shared -fpic -o addendtest.so addendtest.c ./addendtest2.so
    
    $ readelf -r ./addendtest.so
    
    Relocation section '.rela.dyn' at offset 0x3b8 contains 1 entries:
 15   Offset          Info           Type           Sym. Value    Sym. Name + Addend
    0000000104f8  000f00000027 R_IA64_DIR64LSB   0000000000000000 i + 8
    
```

因此，我們在addendtest中有一個重定位。類型R_IA64_DIR64LSB。如果您在IA64 ABI中查找這個，可以將縮寫分解為


R_IA64:所有重定位都以這個前綴開始。


DIR64 : 一個64位的直接類型重定位


LSB:因為IA64可以在大端和小端模式下運行，所以這個重定位是小端(最小有效字節)。


ABI繼續說，這種重新定位意味著“重新定位所指向的符號的價值，加上任何加數”。我們可以看到我們有一個加數為8,因為sizeof(int)= = 4,我們已經把兩個整數的數組(* j =我+ 2),所以在運行時,為了解決這個問題重新定位你需要找到符號的地址我並把它的值,+ 8 0 x104f8。

### 位置獨立性

在可執行文件中，在虛擬內存中給代碼和數據段指定一個基本地址。可執行代碼不是共享的，每個可執行代碼都有自己的新地址空間。這意味著編譯器知道數據部分的確切位置，並可以直接引用它。


函式庫沒有這樣的保證。他們可以知道，他們的數據部分將是一個指定的偏移基址;但是，基本地址的確切位置只能在運行時才能知道。


因此，所有庫都必須使用代碼生成，這些代碼無論放在內存的什麼地方都可以執行，稱為位置無關代碼(或簡稱PIC)。注意，數據部分仍然是代碼部分的固定偏移量;但要真正找到數據地址，偏移量需要添加到加載地址。

## 全局偏移表

在考慮共享庫的目標時，您可能已經注意到遷移的一個關鍵問題。我們之前提到過，具有虛擬內存的共享庫的最大優點是多個程序可以通過共享頁面來使用內存中的代碼。


這個問題源於這樣一個事實，即庫不能保證它們將被放在內存中的位置。動態鏈接器將在虛擬內存中為所需的每個庫找到最方便的位置並將其放在那裡。如果這種情況沒有發生，想想另一種選擇;系統中的每個庫都需要自己的虛擬內存塊，這樣就不會有兩個庫重疊。每次向系統添加新庫時，都需要分配。有些人可能亂寫一個巨大的函式庫，沒有留下足夠的空間給其他函式庫!很有可能，你的程序根本就不想使用這個庫。


因此，如果您通過重新定位來修改共享庫的代碼，那麼該代碼就不再是可共享的了。我們失去了共享庫的優勢。


下面我們將解釋這樣做的機制。

### 全局偏移表

假設我們取一個符號的值。只需要重定位，動態鏈接器就會查找該符號的內存地址，然後重寫代碼以加載該地址。


一個相當直接的改進是在二進制文件中留出空間來保存該符號的地址，並讓動態鏈接器將地址放在那裡，而不是直接放在代碼中。這樣我們就不需要觸及二進制代碼部分。


為這些地址預留的區域稱為全局偏移表或GOT。GOT活在一個名為 .GOT 的 ELF 文件中。


> 圖9.1。通過GOT訪問內存

![](http://www.bottomupcs.com/chapter08/figures/got-plt.png)

> 為了保持代碼(綠色)的可共享性，我們定義了進程私有區域，我們可以將公共變量的地址存儲到其中。這允許我們在進程地址空間的任何地方加載代碼，同時仍然共享底層物理頁面。


GOT對每個進程都是私有的，進程必須具有對它的寫權限。相反，庫代碼是共享的，進程應該只對代碼具有讀取和執行權限;如果進程可以修改代碼，這將是一個嚴重的安全漏洞。


### GOT在行動


> 9.6的例子。使用GOT

```
  1 
    $ cat got.c
    extern int i;
    
  5 void test(void)
    {
            i = 100;
    }
    
 10 $ gcc -nostdlib  -shared -o got.so ./got.c
    
    $ objdump --disassemble ./got.so
    
    ./got.so:     file format elf64-ia64-little
 15 
    Disassembly of section .text:
    
    0000000000000410 <test>:
     410:   0d 10 00 18 00 21       [MFI]       mov r2=r12
 20  416:   00 00 00 02 00 c0                   nop.f 0x0
     41c:   81 09 00 90                         addl r14=24,r1;;
     420:   0d 78 00 1c 18 10       [MFI]       ld8 r15=[r14]
     426:   00 00 00 02 00 c0                   nop.f 0x0
     42c:   41 06 00 90                         mov r14=100;;
 25  430:   11 00 38 1e 90 11       [MIB]       st4 [r15]=r14
     436:   c0 00 08 00 42 80                   mov r12=r2
     43c:   08 00 84 00                         br.ret.sptk.many b0;;
    
    $ readelf --sections ./got.so
 30 There are 17 section headers, starting at offset 0x640:
    
    Section Headers:
      [Nr] Name              Type             Address           Offset
           Size              EntSize          Flags  Link  Info  Align
 35   [ 0]                   NULL             0000000000000000  00000000
           0000000000000000  0000000000000000           0     0     0
      [ 1] .hash             HASH             0000000000000120  00000120
           00000000000000a0  0000000000000004   A       2     0     8
      [ 2] .dynsym           DYNSYM           00000000000001c0  000001c0
 40        00000000000001f8  0000000000000018   A       3     e     8
      [ 3] .dynstr           STRTAB           00000000000003b8  000003b8
           000000000000003f  0000000000000000   A       0     0     1
      [ 4] .rela.dyn         RELA             00000000000003f8  000003f8
           0000000000000018  0000000000000018   A       2     0     8
 45   [ 5] .text             PROGBITS         0000000000000410  00000410
           0000000000000030  0000000000000000  AX       0     0     16
      [ 6] .IA_64.unwind_inf PROGBITS         0000000000000440  00000440
           0000000000000018  0000000000000000   A       0     0     8
      [ 7] .IA_64.unwind     IA_64_UNWIND     0000000000000458  00000458
 50        0000000000000018  0000000000000000  AL       5     5     8
      [ 8] .data             PROGBITS         0000000000010470  00000470
           0000000000000000  0000000000000000  WA       0     0     1
      [ 9] .dynamic          DYNAMIC          0000000000010470  00000470
           0000000000000100  0000000000000010  WA       3     0     8
 55   [10] .got              PROGBITS         0000000000010570  00000570
           0000000000000020  0000000000000000 WAp       0     0     8
      [11] .sbss             NOBITS           0000000000010590  00000590
           0000000000000000  0000000000000000   W       0     0     1
      [12] .bss              NOBITS           0000000000010590  00000590
 60        0000000000000000  0000000000000000  WA       0     0     1
      [13] .comment          PROGBITS         0000000000000000  00000590
           0000000000000026  0000000000000000           0     0     1
      [14] .shstrtab         STRTAB           0000000000000000  000005b6
           000000000000008a  0000000000000000           0     0     1
 65   [15] .symtab           SYMTAB           0000000000000000  00000a80
           0000000000000258  0000000000000018          16    12     8
      [16] .strtab           STRTAB           0000000000000000  00000cd8
           0000000000000045  0000000000000000           0     0     1
    Key to Flags:
 70   W (write), A (alloc), X (execute), M (merge), S (strings)
      I (info), L (link order), G (group), x (unknown)
      O (extra OS processing required) o (OS specific), p (processor specific)
    
```

在上面，我們創建了一個引用外部符號的簡單共享庫。我們不知道這個符號在編譯時的地址，所以我們把它留給動態鏈接器在運行時修復。


但我們希望我們的代碼保持可共享，以防其他進程也想使用我們的代碼。


分解顯示了我們如何處理.got。在IA64(為庫編譯的體系結構)上，寄存器r1被稱為全局指針，總是指向.got部分加載到內存中的位置。

如果我們查看readelf輸出，我們可以看到.got部分從裝載到內存的庫的0x10570字節開始。因此，如果在地址0x6000000000000000處將庫加載到內存中，那麼.got將位於0x6000000000010570處，而寄存器r1將始終指向這個地址。


通過反彙編工作，我們可以看到我們將值100存儲到寄存器r15中的內存地址中。如果我們回頭看，可以看到寄存器15保存了寄存器14中存儲的內存地址的值。再回頭看一步，我們看到我們加載這個地址是通過添加一個小的數字來注冊1。GOT只是一個長長的條目列表，每個外部變量對應一個條目。這意味著外部變量i的GOT條目存儲了24字節(即3個64位地址)。


> 9.7的例子。對GOT重新定位

```
  1 
    $ readelf --relocs ./got.so
    
    Relocation section '.rela.dyn' at offset 0x3f8 contains 1 entries:
  5   Offset          Info           Type           Sym. Value    Sym. Name + Addend
    000000010588  000f00000027 R_IA64_DIR64LSB   0000000000000000 i + 0
    
```

我們也可以查看這個條目的重新定位。重新定位表示“將偏移位置10588的值替換為符號i存儲在的內存位置”。


我們知道.got從前一個輸出的偏移量0x10570開始。我們還看到了代碼如何在這之後加載地址0x18(十進制為24)，給我們一個地址0x10570 + 0x18 = 0x10588……搬遷的地址!


因此，在程序開始之前，動態鏈接器將修復重新定位，以確保偏移0x10588處的內存值是全局變量i的地址!

## 庫


### 過程查找表


庫可能包含許多函數，一個程序可能最終包含許多庫來完成其工作。一個程序可能只使用一個或兩個函數，它們來自許多可用庫中的每個庫，並且取決於通過代碼的運行時路徑，可以使用一些函數而不是其他函數。


正如我們所看到的，動態鏈接的過程是一個計算量相當大的過程，因為它涉及到查找和搜索許多表。任何可以減少管理費用的方法都會提高性能。


過程查找表(PLT)促進了程序中所謂的延遲綁定。對於位於GOT中的變量，綁定與上面描述的修復過程是同義詞。當一個條目被“固定”後，它就被稱為“綁定”到它的真實地址。

正如我們所提到的，有時程序會包含來自庫的函數，但實際上從來不會調用該函數，這取決於用戶輸入。綁定這個函數的過程非常密集，包括加載代碼、搜索表和寫入內存。完成一個沒有使用的函數的綁定過程純粹是浪費時間。


延遲綁定將推遲此開銷，直到使用PLT調用實際的函數。


每個庫函數在PLT中都有一個條目，這個條目最初指向一些特殊的虛擬代碼。當程序調用函數時，它實際上調用PLT條目(通過GOT引用變量也是一樣的)。

這個虛擬函數將加載一些需要傳遞給動態鏈接器的參數，以便解析函數，然後調用動態鏈接器的特殊查找函數。動態鏈接器查找函數的實際地址，並在虛擬函數調用的頂部將該位置寫入調用二進制文件中。


因此，下次調用函數時，可以加載地址，而不必再次回到動態加載器中。如果一個函數從未被調用，那麼PLT條目將永遠不會被修改，但是不會有運行時開銷。

### 行動中的PLT

事情開始變得有點棘手了!如果沒有其他事情，您應該開始認識到，在解析動態符號方面還有大量工作要做!


讓我們考慮一下簡單的“hello World”應用程序。這將只對printf進行一個庫調用，以將字符串輸出給用戶。


> 9.8的例子。你好，世界PLT的例子

```
  1 
    $ cat hello.c
    #include <stdio.h>
    
  5 int main(void)
    {
            printf("Hello, World!\n");
            return 0;
    }
 10 
    $ gcc -o hello hello.c
    
    $ readelf --relocs ./hello
    
 15 Relocation section '.rela.dyn' at offset 0x3f0 contains 2 entries:
      Offset          Info           Type           Sym. Value    Sym. Name + Addend
    6000000000000ed8  000700000047 R_IA64_FPTR64LSB  0000000000000000 _Jv_RegisterClasses + 0
    6000000000000ee0  000900000047 R_IA64_FPTR64LSB  0000000000000000 __gmon_start__ + 0
    
 20 Relocation section '.rela.IA_64.pltoff' at offset 0x420 contains 3 entries:
      Offset          Info           Type           Sym. Value    Sym. Name + Addend
    6000000000000f10  000200000081 R_IA64_IPLTLSB    0000000000000000 printf + 0
    6000000000000f20  000800000081 R_IA64_IPLTLSB    0000000000000000 __libc_start_main + 0
    6000000000000f30  000900000081 R_IA64_IPLTLSB    0000000000000000 __gmon_start__ + 0
 25 
```

上面可以看到，printf符號有一個R_IA64_IPLTLSB重定位。這表示“將符號printf的地址放入內存地址0x6000000000000f10”。我們必須開始深入挖掘，找到得到函數的確切過程。


下面我們來看看程序 main() 函數的分解。


> 9.9的例子。Hello world main()

```
  1 
    4000000000000790 <main>:
    4000000000000790:       00 08 15 08 80 05       [MII]       alloc r33=ar.pfs,5,4,0
    4000000000000796:       20 02 30 00 42 60                   mov r34=r12
  5 400000000000079c:       04 08 00 84                         mov r35=r1
    40000000000007a0:       01 00 00 00 01 00       [MII]       nop.m 0x0
    40000000000007a6:       00 02 00 62 00 c0                   mov r32=b0
    40000000000007ac:       81 0c 00 90                         addl r14=72,r1;;
    40000000000007b0:       1c 20 01 1c 18 10       [MFB]       ld8 r36=[r14]
 10 40000000000007b6:       00 00 00 02 00 00                   nop.f 0x0
    40000000000007bc:       78 fd ff 58                         br.call.sptk.many b0=4000000000000520 <_init+0xb0>
    40000000000007c0:       02 08 00 46 00 21       [MII]       mov r1=r35
    40000000000007c6:       e0 00 00 00 42 00                   mov r14=r0;;
    40000000000007cc:       01 70 00 84                         mov r8=r14
 15 40000000000007d0:       00 00 00 00 01 00       [MII]       nop.m 0x0
    40000000000007d6:       00 08 01 55 00 00                   mov.i ar.pfs=r33
    40000000000007dc:       00 0a 00 07                         mov b0=r32
    40000000000007e0:       1d 60 00 44 00 21       [MFB]       mov r12=r34
    40000000000007e6:       00 00 00 02 00 80                   nop.f 0x0
 20 40000000000007ec:       08 00 84 00                         br.ret.sptk.many b0;;
    
```

對0x4000000000000520的調用一定是在調用printf函數。通過閱讀readelf一節，我們可以找到它的位置。


> 9.10的例子。Hello world部分

```
  1 
                $ readelf --sections ./hello
    There are 40 section headers, starting at offset 0x25c0:
    
  5 Section Headers:
      [Nr] Name              Type             Address           Offset
           Size              EntSize          Flags  Link  Info  Align
      [ 0]                   NULL             0000000000000000  00000000
           0000000000000000  0000000000000000           0     0     0
 10 ...
      [11] .plt              PROGBITS         40000000000004c0  000004c0
           00000000000000c0  0000000000000000  AX       0     0     32
      [12] .text             PROGBITS         4000000000000580  00000580
           00000000000004a0  0000000000000000  AX       0     0     32
 15   [13] .fini             PROGBITS         4000000000000a20  00000a20
           0000000000000040  0000000000000000  AX       0     0     16
      [14] .rodata           PROGBITS         4000000000000a60  00000a60
           000000000000000f  0000000000000000   A       0     0     8
      [15] .opd              PROGBITS         4000000000000a70  00000a70
 20        0000000000000070  0000000000000000   A       0     0     16
      [16] .IA_64.unwind_inf PROGBITS         4000000000000ae0  00000ae0
           00000000000000f0  0000000000000000   A       0     0     8
      [17] .IA_64.unwind     IA_64_UNWIND     4000000000000bd0  00000bd0
           00000000000000c0  0000000000000000  AL      12     c     8
 25   [18] .init_array       INIT_ARRAY       6000000000000c90  00000c90
           0000000000000018  0000000000000000  WA       0     0     8
      [19] .fini_array       FINI_ARRAY       6000000000000ca8  00000ca8
           0000000000000008  0000000000000000  WA       0     0     8
      [20] .data             PROGBITS         6000000000000cb0  00000cb0
 30        0000000000000004  0000000000000000  WA       0     0     4
      [21] .dynamic          DYNAMIC          6000000000000cb8  00000cb8
           00000000000001e0  0000000000000010  WA       5     0     8
      [22] .ctors            PROGBITS         6000000000000e98  00000e98
           0000000000000010  0000000000000000  WA       0     0     8
 35   [23] .dtors            PROGBITS         6000000000000ea8  00000ea8
           0000000000000010  0000000000000000  WA       0     0     8
      [24] .jcr              PROGBITS         6000000000000eb8  00000eb8
           0000000000000008  0000000000000000  WA       0     0     8
      [25] .got              PROGBITS         6000000000000ec0  00000ec0
 40        0000000000000050  0000000000000000 WAp       0     0     8
      [26] .IA_64.pltoff     PROGBITS         6000000000000f10  00000f10
           0000000000000030  0000000000000000 WAp       0     0     16
      [27] .sdata            PROGBITS         6000000000000f40  00000f40
           0000000000000010  0000000000000000 WAp       0     0     8
 45   [28] .sbss             NOBITS           6000000000000f50  00000f50
           0000000000000008  0000000000000000  WA       0     0     8
      [29] .bss              NOBITS           6000000000000f58  00000f50
           0000000000000008  0000000000000000  WA       0     0     8
      [30] .comment          PROGBITS         0000000000000000  00000f50
 50        00000000000000b9  0000000000000000           0     0     1
      [31] .debug_aranges    PROGBITS         0000000000000000  00001010
           0000000000000090  0000000000000000           0     0     16
      [32] .debug_pubnames   PROGBITS         0000000000000000  000010a0
           0000000000000025  0000000000000000           0     0     1
 55   [33] .debug_info       PROGBITS         0000000000000000  000010c5
           00000000000009c4  0000000000000000           0     0     1
      [34] .debug_abbrev     PROGBITS         0000000000000000  00001a89
           0000000000000124  0000000000000000           0     0     1
      [35] .debug_line       PROGBITS         0000000000000000  00001bad
 60        00000000000001fe  0000000000000000           0     0     1
      [36] .debug_str        PROGBITS         0000000000000000  00001dab
           00000000000006a1  0000000000000001  MS       0     0     1
      [37] .shstrtab         STRTAB           0000000000000000  0000244c
           000000000000016f  0000000000000000           0     0     1
 65   [38] .symtab           SYMTAB           0000000000000000  00002fc0
           0000000000000b58  0000000000000018          39    60     8
      [39] .strtab           STRTAB           0000000000000000  00003b18
           0000000000000479  0000000000000000           0     0     1
    Key to Flags:
 70   W (write), A (alloc), X (execute), M (merge), S (strings)
      I (info), L (link order), G (group), x (unknown)
      O (extra OS processing required) o (OS specific), p (processor specific)
    
```

上面可以看到，printf符號有一個R_IA64_IPLTLSB重定位。這表示“將符號printf的地址放入內存地址0x6000000000000f10”。我們必須開始深入挖掘，找到得到函數的確切過程。


下面我們來看看程序的主()函數的分解。


> 9.9 的例子。Hello world PLT

```
  1 
    40000000000004c0 <.plt>:
    40000000000004c0:       0b 10 00 1c 00 21       [MMI]       mov r2=r14;;
    40000000000004c6:       e0 00 08 00 48 00                   addl r14=0,r2
  5 40000000000004cc:       00 00 04 00                         nop.i 0x0;;
    40000000000004d0:       0b 80 20 1c 18 14       [MMI]       ld8 r16=[r14],8;;
    40000000000004d6:       10 41 38 30 28 00                   ld8 r17=[r14],8
    40000000000004dc:       00 00 04 00                         nop.i 0x0;;
    40000000000004e0:       11 08 00 1c 18 10       [MIB]       ld8 r1=[r14]
 10 40000000000004e6:       60 88 04 80 03 00                   mov b6=r17
    40000000000004ec:       60 00 80 00                         br.few b6;;
    40000000000004f0:       11 78 00 00 00 24       [MIB]       mov r15=0
    40000000000004f6:       00 00 00 02 00 00                   nop.i 0x0
    40000000000004fc:       d0 ff ff 48                         br.few 40000000000004c0 <_init+0x50>;;
 15 4000000000000500:       11 78 04 00 00 24       [MIB]       mov r15=1
    4000000000000506:       00 00 00 02 00 00                   nop.i 0x0
    400000000000050c:       c0 ff ff 48                         br.few 40000000000004c0 <_init+0x50>;;
    4000000000000510:       11 78 08 00 00 24       [MIB]       mov r15=2
    4000000000000516:       00 00 00 02 00 00                   nop.i 0x0
 20 400000000000051c:       b0 ff ff 48                         br.few 40000000000004c0 <_init+0x50>;;
    4000000000000520:       0b 78 40 03 00 24       [MMI]       addl r15=80,r1;;
    4000000000000526:       00 41 3c 70 29 c0                   ld8.acq r16=[r15],8
    400000000000052c:       01 08 00 84                         mov r14=r1;;
    4000000000000530:       11 08 00 1e 18 10       [MIB]       ld8 r1=[r15]
 25 4000000000000536:       60 80 04 80 03 00                   mov b6=r16
    400000000000053c:       60 00 80 00                         br.few b6;;
    4000000000000540:       0b 78 80 03 00 24       [MMI]       addl r15=96,r1;;
    4000000000000546:       00 41 3c 70 29 c0                   ld8.acq r16=[r15],8
    400000000000054c:       01 08 00 84                         mov r14=r1;;
 30 4000000000000550:       11 08 00 1e 18 10       [MIB]       ld8 r1=[r15]
    4000000000000556:       60 80 04 80 03 00                   mov b6=r16
    400000000000055c:       60 00 80 00                         br.few b6;;
    4000000000000560:       0b 78 c0 03 00 24       [MMI]       addl r15=112,r1;;
    4000000000000566:       00 41 3c 70 29 c0                   ld8.acq r16=[r15],8
 35 400000000000056c:       01 08 00 84                         mov r14=r1;;
    4000000000000570:       11 08 00 1e 18 10       [MIB]       ld8 r1=[r15]
    4000000000000576:       60 80 04 80 03 00                   mov b6=r16
    400000000000057c:       60 00 80 00                         br.few b6;;
    
 40           
 ```

讓我們一步一步地看一下說明書。首先，我們將r1中的值加80，並將其存儲在r15中。我們從前面知道r1將指向GOT，所以這表示“將r1580字節存儲到GOT”。接下來我們要做的是將存儲在GOT中這個位置的值加載到r16中，並將r15中的值增加8個字節。然後，我們將r1 (GOT的位置)存儲在r14中，並將r1設置為r15之後的8個字節中的值。然後轉移到r16。


在上一章中，我們討論了如何通過包含函數地址和全局指針地址的函數描述符調用函數。在這裡，我們可以看到PLT條目首先加載函數值，將8字節轉移到函數描述符的第二部分，然後在調用函數之前將該值加載到op寄存器中。


但是我們到底在裝載什麼呢?我們知道r1會指向GOT。我們經過了80字節的got (0x50)


> 9.12的例子。Hello world GOT

```
  1 
    $ objdump --disassemble-all ./hello 
    Disassembly of section .got:
    
  5 6000000000000ec0 <.got>:
            ...
    6000000000000ee8:       80 0a 00 00 00 00                   data8 0x02a000000
    6000000000000eee:       00 40 90 0a                         dep r0=r0,r0,63,1
    6000000000000ef2:       00 00 00 00 00 40       [MIB] (p20) break.m 0x1
 10 6000000000000ef8:       a0 0a 00 00 00 00                   data8 0x02a810000
    6000000000000efe:       00 40 50 0f                         br.few 6000000000000ef0 <_GLOBAL_OFFSET_TABLE_+0x30>
    6000000000000f02:       00 00 00 00 00 60       [MIB] (p58) break.m 0x1
    6000000000000f08:       60 0a 00 00 00 00                   data8 0x029818000
    6000000000000f0e:       00 40 90 06                         br.few 6000000000000f00 <_GLOBAL_OFFSET_TABLE_+0x40>
 15 Disassembly of section .IA_64.pltoff:
    
    6000000000000f10 <.IA_64.pltoff>:
    6000000000000f10:       f0 04 00 00 00 00       [MIB] (p39) break.m 0x0
    6000000000000f16:       00 40 c0 0e 00 00                   data8 0x03b010000
 20 6000000000000f1c:       00 00 00 60                         data8 0xc000000000
    6000000000000f20:       00 05 00 00 00 00       [MII] (p40) break.m 0x0
    6000000000000f26:       00 40 c0 0e 00 00                   data8 0x03b010000
    6000000000000f2c:       00 00 00 60                         data8 0xc000000000
    6000000000000f30:       10 05 00 00 00 00       [MIB] (p40) break.m 0x0
 25 6000000000000f36:       00 40 c0 0e 00 00                   data8 0x03b010000
    6000000000000f3c:       00 00 00 60                         data8 0xc000000000
    
```

0x6000000000000ec0 + 0x50 = 0x6000000000000f10，或。ia_64。pltoff部分。現在我們開始有所進展了!


我們可以解碼objdump輸出，這樣我們就能確切地看到這裡裝載的是什麼。交換前8個字節的字節順序f0 04 00000000 40最後是0x4000000000004f0。現在這個地址看起來很熟悉了!回頭看看PLT的組裝輸出，我們看到了那個地址。


在0x4000000000004f0處的代碼首先將一個零值放入r15，然後分支回到0x40000000000004c0。等一下!這是PLT部分的開始。

我們也可以跟蹤這段代碼。首先保存全局指針(r2)的值，然後將三個8字節的值加載到r16、r17和r1中。然後，我們將分支到r17中的地址。我們在這裡看到的是對動態鏈接器的實際調用!


我們需要深入研究ABI，以確切地了解此時裝載的是什麼。ABI說了兩件事——動態鏈接的程序必須有一個可以容納3個8字節值的特殊部分(稱為DT_IA_64_PLT_RESERVE部分)。在二進制文件的動態段中有一個指針，其中的保留區域。


> 9.13的例子。動態段

```
  1 
                
    Dynamic segment at offset 0xcb8 contains 25 entries:
      Tag        Type                         Name/Value
  5  0x0000000000000001 (NEEDED)             Shared library: [libc.so.6.1]
     0x000000000000000c (INIT)               0x4000000000000470
     0x000000000000000d (FINI)               0x4000000000000a20
     0x0000000000000019 (INIT_ARRAY)         0x6000000000000c90
     0x000000000000001b (INIT_ARRAYSZ)       24 (bytes)
 10  0x000000000000001a (FINI_ARRAY)         0x6000000000000ca8
     0x000000000000001c (FINI_ARRAYSZ)       8 (bytes)
     0x0000000000000004 (HASH)               0x4000000000000200
     0x0000000000000005 (STRTAB)             0x4000000000000330
     0x0000000000000006 (SYMTAB)             0x4000000000000240
 15  0x000000000000000a (STRSZ)              138 (bytes)
     0x000000000000000b (SYMENT)             24 (bytes)
     0x0000000000000015 (DEBUG)              0x0
     0x0000000070000000 (IA_64_PLT_RESERVE)  0x6000000000000ec0 -- 0x6000000000000ed8
     0x0000000000000003 (PLTGOT)             0x6000000000000ec0
 20  0x0000000000000002 (PLTRELSZ)           72 (bytes)
     0x0000000000000014 (PLTREL)             RELA
     0x0000000000000017 (JMPREL)             0x4000000000000420
     0x0000000000000007 (RELA)               0x40000000000003f0
     0x0000000000000008 (RELASZ)             48 (bytes)
 25  0x0000000000000009 (RELAENT)            24 (bytes)
     0x000000006ffffffe (VERNEED)            0x40000000000003d0
     0x000000006fffffff (VERNEEDNUM)         1
     0x000000006ffffff0 (VERSYM)             0x40000000000003ba
     0x0000000000000000 (NULL)               0x0
 30 
```

你注意到了嗎?它和GOT的值一樣。這意味著GOT中的前三個8字節條目實際上是保留區域;因此，全局指針總是指向它。


當動態鏈接器啟動時，填充這些值是它的職責。ABI說，給這個模塊一個唯一ID的動態鏈接器將填充第一個值，第二個值是動態鏈接器的全局指針值，第三個值是查找和修復符號的函數的地址。


> 9.14的例子。用於設置特殊值的動態鏈接器中的代碼(來自libc sysdeps/ia64/dl-machine.h)

```
  1 
    /* Set up the loaded object described by L so its unrelocated PLT
       entries will jump to the on-demand fixup code in dl-runtime.c.  */
    
  5 static inline int __attribute__ ((unused, always_inline))
    elf_machine_runtime_setup (struct link_map *l, int lazy, int profile)
    {
      extern void _dl_runtime_resolve (void);
      extern void _dl_runtime_profile (void);
 10 
      if (lazy)
        {
          register Elf64_Addr gp __asm__ ("gp");
          Elf64_Addr *reserve, doit;
 15 
          /*
           * Careful with the typecast here or it will try to add l-l_addr
           * pointer elements
           */
 20       reserve = ((Elf64_Addr *)
                     (l->l_info[DT_IA_64 (PLT_RESERVE)]->d_un.d_ptr + l->l_addr));
          /* Identify this shared object.  */
          reserve[0] = (Elf64_Addr) l;
    
 25       /* This function will be called to perform the relocation.  */
          if (!profile)
            doit = (Elf64_Addr) ((struct fdesc *) &_dl_runtime_resolve)->ip;
          else
            {
 30           if (GLRO(dl_profile) != NULL
                  && _dl_name_match_p (GLRO(dl_profile), l))
                {
                  /* This is the object we are looking for.  Say that we really
                     want profiling and the timers are started.  */
 35               GL(dl_profile_map) = l;
                }
              doit = (Elf64_Addr) ((struct fdesc *) &_dl_runtime_profile)->ip;
            }
    
 40       reserve[1] = doit;
          reserve[2] = gp;
        }
    
      return lazy;
 45 }
    
```

我們可以通過動態鏈接器看到它是如何被設置的通過查看為二進制函數做這個的函數。reserve變量是從二進制文件中的PLT_RESERVE部分指針中設置的。唯一值(放入reserve[0])是該對像的鏈接映射地址。鏈接映射是glibc中用於共享對像的內部表示。然後，我們將_dl_runtime_resolve的地址放入第二個值(假設我們沒有使用剖析)。備用[2]最終被設置為gp，這是通過“__asm__”調用從r2中找到的。


回頭看ABI，我們看到條目的重定位索引必須放在r15中，惟一標識符必須在r16中傳遞。

在我們跳到PLT的開始之前，r15已經在存根代碼中被設置了。查看條目，並注意到每個PLT條目如何用遞增的值加載r15 ?如果您查看重新定位，應該不會感到意外，printf重新定位是0號。


r16我們從動態鏈接器初始化的值加載，如前所述。一旦准備好了，我們就可以將函數地址和全局指針加載到函數中。

此時運行的是動態鏈接器函數_dl_runtime_resolve。它找到了搬遷;還記得重定位如何指定符號的名稱嗎?它使用這個名稱來查找正確的函數;如果庫不在內存中，那麼可能需要從磁盤加載庫，或者以其他方式共享代碼。


重新定位記錄為動態鏈接器提供它需要“修復”的地址;還記得它是在GOT中被初始PLT存根加載的嗎?這意味著在第一次調用函數之後，第二次加載函數時，它將得到函數的直接地址;使動態鏈接器短路。

### 總結

您已經看到了PLT背後的確切機制，以及動態鏈接器的內部工作方式。要記住的重點是


程序中的庫調用實際上是在二進制文件的PLT中調用一小段代碼。


這個存根代碼加載一個地址並跳轉到它。


最初，該地址指向動態鏈接器中的一個函數，該函數能夠查找“真實”函數，給定該函數的重定位條目中的信息。


動態鏈接器重寫存根代碼讀取的地址，以便下次調用該函數時，它將直接轉到正確的地址。

## 與庫和鏈接器一起工作

動態鏈接器的出現既提供了一些我們可以利用的優勢，也提供了一些需要解決的額外問題，以獲得一個功能系統。


### 庫版本

一個潛在的問題是不同版本的庫。由於只有靜態庫，出現問題的可能性就小得多，因為所有庫代碼都直接構建到應用程序的二進制文件中。如果您想使用新版本的庫，您需要將它重新編譯成一個新的二進制文件，替換舊的。

對於公共庫來說，這顯然是相當不切實際的，最常見的當然是libc，它包含在大多數應用程序中。如果它只能作為靜態庫使用，任何更改都需要重新構建系統中的每個應用程序。


然而，動態庫工作方式的改變可能會導致多個問題。在最好的情況下，修改是完全兼容的，外部可見的內容不會改變。另一方面，更改可能導致應用程序崩潰;例如，如果一個使用int的函數改變為使用int *。更糟糕的是，新的庫版本可能會改變語義，然後突然開始悄悄地返回不同的、可能是錯誤的值。這可能是一個非常令人討厭的bug;當應用程序崩潰時，您可以使用調試器來隔離錯誤發生的位置，而數據損壞或修改可能只出現在應用程序看似不相關的部分。

動態鏈接器需要一種方法來確定系統中庫的版本，以便能夠識別新的版本。現代動態鏈接器可以使用許多方案來找到正確的庫版本。

### sonames
使用sonames，我們可以向庫中添加一些額外的信息來幫助識別版本。


正如我們前面看到的，應用程序在二進制文件的動態部分列出了它在dt_required字段中需要的庫。實際的庫保存在磁盤上的文件中，通常保存在/lib中，用於核心系統庫，或/usr/lib中用於可選庫。


為了允許庫的多個版本存在於磁盤上，它們顯然需要不同的文件名。soname方案使用名稱和文件系統鏈接的組合來構建庫的層次結構。

這是通過引入主庫和次要庫修訂的概念來實現的。一個小的修訂是一個完全向後兼容以前版本的庫;這通常只包含bug修復。因此，主要修訂是不兼容的任何修訂;改變函數的輸入或函數的行為方式。


由於每個庫修訂(主要的或次要的)都需要保存在磁盤上的單獨文件中，這就形成了庫層次結構的基礎。庫的名稱由約定的libNAME.so.MAJOR.MINOR[29]命名。然而，如果每個應用程序都直接鏈接到這個文件上，我們就會遇到與靜態庫相同的問題;每次發生微小的更改時，我們都需要重新構建應用程序以指向新庫。

我們真正想要參考的是函式庫的主要數字。如果發生了更改，則需要合理地重新編譯應用程序，因為我們需要確保我們的程序仍然與新庫兼容。


因此soname是libNAME.so.MAJOR。soname應該設置在共享庫中動態部分的DT_SONAME字段中;庫作者可以在構建庫時指定這個版本。


因此，磁盤上的每個小版本庫文件都可以在它的DT_SONAME字段中指定相同的主版本號，以便動態鏈接器知道這個特定的庫文件實現了對庫API和ABI的一個特定的主要修訂。


為了跟蹤這一點，通常會運行一個名為ldconfig的應用程序，以創建系統上最新版本的符號鏈接，這些鏈接是以主版本命名的。ldconfig通過運行所有實現特定主要修訂號的庫來工作，然後選擇最小修訂號最高的庫。然後它從libNAME.so中創建一個符號鏈接。主要到光盤上的實際庫文件，即libNAME.so.MAJOR.MINOR。

<!-- XXX:談談libtool版本 -->


層次結構的最後一部分是庫的編譯名稱。在編譯程序時，要鏈接到庫，需要使用-lNAME標志，它會停止搜索libNAME。文件在庫搜索路徑中。但是請注意，我們沒有指定任何版本號;我們只是想鏈接到系統上最新的庫。在編譯libNAME之間創建符號鏈接取決於庫的安裝過程。所以命名和最新的庫代碼在系統上。通常由包管理系統(dpkg或rpm)處理。這不是一個自動化的過程，因為系統上的最新庫可能不是您希望始終針對它進行編譯的庫;例如，如果最新安裝的庫是不適合一般使用的開發版本。


一般過程如下圖所示。


> 圖9.2。soname

![](http://www.bottomupcs.com/chapter08/figures/libs.png)


描述soname系統

### 動態鏈接器如何查找庫

當應用程序啟動時，動態鏈接器查看dt_required字段以查找所需的庫。這個字段包含庫的soname，因此下一步是讓動態鏈接器遍歷其搜索路徑中查找它的所有庫。


這個過程在概念上包括兩個步驟。首先，動態鏈接器需要搜索所有庫，以找到實現給定soname的庫。其次，需要比較小版本的文件名，以找到最新版本，然後就可以加載了。

我們在前面提到過，ldconfig在庫soname和最新的小版本之間設置了一個符號鏈接。因此，動態鏈接器應該只需要跟隨該鏈接找到要加載的正確文件，而不必在每次需要應用程序時打開所有可能的庫並決定使用哪個庫。


由於文件系統訪問非常慢，ldconfig還創建了安裝在系統中的庫的緩存。這個緩存只是動態鏈接器可用的庫調用列表和指向磁盤上主要版本鏈接的指針，這樣動態鏈接器就不必讀取整個目錄中的文件來定位正確的鏈接。你可以用/sbin/ldconfig -p來分析;它實際上存在於文件/etc/ldconfig.so.cache中。如果在緩存中沒有找到庫，動態鏈接器將退回到遍歷文件系統的較慢選項，因此在安裝新庫時重新運行ldconfig非常重要。

### 發現符號

我們已經討論了動態鏈接器如何獲取庫函數的地址，並將其放入PLT中供程序使用。但到目前為止，我們還沒有討論動態鏈接器如何找到函數的地址。整個過程稱為綁定，因為符號名綁定到它表示的地址。


動態鏈接器有一些信息;首先是它正在搜索的符號，其次是該符號可能所在的庫列表，由二進制文件中的dt_required字段定義。


每個共享對像庫都有一個部分，標記為SHT_DYNSYM並稱為.dynsym，這是動態鏈接所需的最小符號集——這是庫中任何可以由外部程序調用的符號。

### 動態符號表

事實上，有三個部分在描述動態符號時都起了作用。首先，讓我們看看ELF規範中符號的定義


> 9.15的例子。ELF(小精靈)的符號定義

```
  1 
    typedef struct {
              Elf32_Word    st_name;
              Elf32_Addr    st_value;
  5           Elf32_Word    st_size;
              unsigned char st_info;
              unsigned char st_other;
              Elf32_Half    st_shndx;
    } Elf32_Sym;
 10 
```

> Table 9.2. ELF symbol fields

Field	Value | st_name	An index to the string table
------------|------------------------------------
st_value | Value - in a relocatable shared object this holds the offset from the section of index given in st_shndx
st_size | Any associated size of the symbol
st_info | Information on the binding of the symbol (described below) and what type of symbol this is (a function, object, etc).
st_other | Not currently used
st_shndx | Index of the section this symbol resides in (see st_value

如您所見，符號名稱的實際字符串保存在一個單獨的部分(.dynstr;dynsym部分中的條目只包含字符串部分的索引。這為動態鏈接器創建了一定程度的開銷;動態鏈接器必須讀取.dynsym部分中的所有符號條目，然後跟隨索引指針查找符號名稱以進行比較。


為了加快這一過程，我們引入了第三部分.hash，其中包含一個符號名到符號表條目的hash表。這個哈希表是在構建庫時預先計算的，允許動態鏈接器更快地查找符號條目，通常只有一兩個查找。

### 符號綁定

雖然我們通常說找到一個符號的地址的過程指的是綁定那個符號的過程，但符號綁定有一個單獨的含義。


符號的綁定決定了它在動態鏈接過程中的外部可見性。在其定義的對像文件之外，局部符號是不可見的。全局符號對其他對像文件可見，可以滿足其他對像中未定義的引用。


弱引用是低優先級全局引用的一種特殊類型。這意味著它被設計成被覆蓋，我們很快就會看到。


下面是一個示例C程序，我們分析它來檢查符號綁定。


> 9.16的例子。符號綁定示例

```
  1 
    $ cat test.c
    static int static_variable;
    
  5 extern int extern_variable;
    
    int external_function(void);
    
    int function(void)
 10 {
            return external_function();
    }
    
    static int static_function(void)
 15 {
            return 10;
    }
    
    #pragma weak weak_function
 20 int weak_function(void)
    {
            return 10;
    }
    
 25 $ gcc -c test.c
    $ objdump --syms test.o
    
    test.o:     file format elf32-powerpc
    
 30 SYMBOL TABLE:
    00000000 l    df *ABS*  00000000 test.c
    00000000 l    d  .text  00000000 .text
    00000000 l    d  .data  00000000 .data
    00000000 l    d  .bss   00000000 .bss
 35 00000038 l     F .text  00000024 static_function
    00000000 l    d  .sbss  00000000 .sbss
    00000000 l     O .sbss  00000004 static_variable
    00000000 l    d  .note.GNU-stack        00000000 .note.GNU-stack
    00000000 l    d  .comment       00000000 .comment
 40 00000000 g     F .text  00000038 function
    00000000         *UND*  00000000 external_function
    0000005c  w    F .text  00000024 weak_function
    
    $ nm test.o
 45          U external_function
    00000000 T function
    00000038 t static_function
    00000000 s static_variable
    0000005c W weak_function
 50 
    
```

注意使用#pragma來定義弱符號。語用是向編譯器傳遞額外信息的一種方式;它的使用並不常見，但有時需要編譯器做一些普通的操作


我們用兩種不同的工具檢查符號;在這兩種情況下，綁定都顯示在第二列;代碼應該相當直接(在工具手冊中有文檔說明)。

### 符號覆寫 Overriding symbols

對於程序員來說，能夠覆蓋庫中的符號通常是非常有用的;那就是用不同的定義顛覆普通的符號。


我們提到，庫搜索的順序是由庫中dt_required字段的順序給出的。但是，可以插入庫作為最後要搜索的庫;這意味著，它們中的任何符號都將作為最終引用。


這是通過一個名為LD_PRELOAD的環境變量完成的，該變量指定鏈接器應該最後加載的庫。


> 9.17的例子。LD_PRELOAD的例子

```
  1 
    $ cat override.c
    #define _GNU_SOURCE 1
    #include <stdio.h>
  5 #include <stdlib.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <dlfcn.h>
    
 10 pid_t getpid(void)
    {
            pid_t (*orig_getpid)(void) = dlsym(RTLD_NEXT, "getpid");
            printf("Calling GETPID\n");
    
 15         return orig_getpid();
    }
    
    $ cat test.c
    #include <stdio.h>
 20 #include <stdlib.h>
    #include <unistd.h>
    
    int main(void)
    {
 25         printf("%d\n", getpid());
    }
    
    $ gcc -shared -fPIC -o liboverride.so override.c -ldl
    $ gcc -o test test.c
 30 $ LD_PRELOAD=./liboverride.so ./test
    Calling GETPID
    15187
```

在上面的示例中，我們覆蓋getpid函數，在調用它時打印一個小語句。我們使用libc提供的dlysm函數和一個參數，告訴它繼續，並找到下一個名為getpid的符號。


### 隨著時間的推移，符號越來越弱

弱符號的概念是，符號被標記為較低的優先級，可以被另一個符號覆蓋。只有當沒有找到其他實現時，弱符號才會是它所使用的符號。


動態加載器的邏輯擴展是，應該加載所有庫，對於其他庫中的普通符號，應該忽略這些庫中的任何弱符號。這就是glibc最初在Linux中實現弱符號處理的方式。

但是，這實際上與Unix標准(SysVr4)中的文字不符。標准實際上規定弱符號只能由靜態鏈接器處理;它們應該與動態鏈接器無關(參見下面關於綁定順序的部分)。


當時，使動態鏈接器覆蓋弱符號的Linux實現與SGI的IRIX平台相匹配，但與Solaris和AIX等其他實現不同。當開發人員意識到這種行為違反了標准時，它被逆轉了，舊的行為降級為需要一個特殊的環境標志(LD_DYNAMIC_WEAK)。

### 指定綁定順序

我們已經看到了如何通過預加載另一個具有相同符號定義的共享庫來覆蓋另一個庫中的函數。按照動態加載器加載庫的順序，最後一個符號被解析為最後一個符號。


庫是按照二進制文件的dt_required標志中指定的順序加載的。這又取決於在構建對像時在命令行上傳遞庫的順序。當要定位符號時，動態鏈接器從最後加載的庫開始，向後工作，直到找到符號為止。


然而，一些共享庫需要一種方法來覆蓋這種行為。他們需要對動態鏈接器說:“首先在我的內部尋找這些符號，而不是從上次加載的庫向後工作”。庫可以在其動態部分標頭中設置dt_symbol標志來獲得這種行為(這通常通過在構建共享庫時在靜態鏈接器命令行上傳遞- bsymbol標志來設置)。

這個標志的作用是控制符號可見性。庫中的符號不能被重寫，因此可以認為是正在加載的庫的私有符號。


但是，由於庫不是為這種行為標記的，就是沒有標記的，因此這會丟失很多粒度。一個更好的系統將允許我們使一些符號私有，一些符號公開。

### 符號 Symbol 版本控制

更好的系統來自於符號版本控制。對於符號版本控制，我們為靜態鏈接器指定一些額外的輸入，以便為它提供關於共享庫中的符號的更多信息。


> 9.18的例子。符號版本控制的例子

```
  1 
    $ cat Makefile
    all: test testsym
    
  5 clean:
            rm -f *.so test testsym
    
    liboverride.so : override.c
            $(CC) -shared -fPIC -o liboverride.so override.c
 10 
    libtest.so : libtest.c
            $(CC) -shared -fPIC -o libtest.so libtest.c
    
    libtestsym.so : libtest.c
 15         $(CC) -shared -fPIC -Wl,-Bsymbolic -o libtestsym.so libtest.c
    
    test : test.c libtest.so liboverride.so
            $(CC) -L. -ltest -o test test.c
    
 20 testsym : test.c libtestsym.so liboverride.so
            $(CC) -L. -ltestsym -o testsym test.c
    
    $ cat libtest.c
    #include <stdio.h>
 25 
    int foo(void) {
            printf("libtest foo called\n");
            return 1;
    }
 30 
    int test_foo(void)
    {
            return foo();
    }
 35 
    $ cat override.c
    #include <stdio.h>
    
    int foo(void)
 40 {
            printf("override foo called\n");
            return 0;
    }
    
 45 $ cat test.c
    #include <stdio.h>
    
    int main(void)
    {
 50         printf("%d\n", test_foo());
    }
    
    $ cat Versions
    {global: test_foo;  local: *; };
 55 
    $ gcc -shared -fPIC -Wl,-version-script=Versions -o libtestver.so libtest.c
    
    $ gcc -L. -ltestver -o testver test.c
    
 60 $ LD_LIBRARY_PATH=. LD_PRELOAD=./liboverride.so ./testver
    libtest foo called
    
    100000574 l     F .text	00000054              foo
    000005c8 g     F .text	00000038              test_foo
 65 
```

在如上所述的最簡單的情況下，我們只是簡單地說明符號是全局的還是局部的。因此，在foo函數之上的情況下，最有可能是test_foo的支持函數;雖然我們很高興test_foo函數的整體功能被覆蓋，但是如果我們使用共享庫版本，它需要保持不變的訪問，沒有人應該修改支持函數。


這使我們能夠更好地組織名稱空間。許多庫可能希望實現一些可以命名為普通函數的東西，比如讀寫;然而，如果他們都這麼做了，給程序的實際版本可能是完全錯誤的。通過將符號指定為本地符號，只有開發人員才能確保不會與內部名稱衝突，相反，他選擇的名稱不會影響任何其他程序。

該方案的一個擴展是符號版本控制。這樣，您就可以在同一個庫中指定同一符號的多個版本。靜態鏈接器在符號名(類似於@VER)之後附加一些版本信息，這些信息描述了給出的符號的版本。


如果開發人員實現了一個名稱相同但可能是二進制或編程方式不同的實現的函數，他可以增加版本號。當根據共享庫構建新的應用程序時，它們將獲得符號的最新版本。但是，針對同一庫的早期版本構建的應用程序將請求舊版本(例如，在它們請求的符號名中包含較舊的@VER字符串)，從而獲得原始實現。

<!-- XXX:示例 -->

> [29] 您可以選擇將一個版本作為次要編號之後的最終標識符。通常，這足以區分所有不同的版本庫。