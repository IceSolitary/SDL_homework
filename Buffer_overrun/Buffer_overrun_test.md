# 软件项目安全开发生命周期 缓冲区溢出实验报告
### 17 软工 葛启明 201711143018
***
## 实验目的：
>通过实验构造缓冲区溢出的实例，进而理解缓冲区溢出对于软件安全带来的危害。
##实验思路：
>通过给main函数输入一个较大的参数，使用`strcpy(y,x)`函数，将大参数输入给x，并给y开辟一个小于x的空间，进而复制便会造成缓冲区溢出，并使溢出的代码段覆盖原有的函数返回地址，造成程序异常。
## 实验代码：
``` C
#define CRT_SECURE_NO_WARNING

#include<stdlib.h>
#include<stdio.h>
#include<string.h>

int sub(char* x) {
    char y[10];
    strcpy(y, x);
    return 0;
}

int main(int argc, char** argv) {
    if (argc > 1) {
        sub(argv[1]);
    }
    printf("exit");
    system("pause");
}
```
## 实验过程：
1. 在Visual Studio中建立项目，输入代码，编译运行前，在项目属性中禁用C++异常,禁用安全检查和SDL检查,并在属性>调试中，设置命令参数为一个较大的参数。<img src='img\2.png'>
2. 在` strcpy(y, x);`和`sub(argv[1]);`两行设置断点，编译运行项目,进行反汇编查看寄存器与内存中的数据
3. 在汇编代码中，ebp为栈指针寄存器，指向一个栈帧的栈顶，esp为基址指针寄存器，指向一个栈帧的底部，而对于C语言的函数调用返回，简而言之就是入栈出栈的过程。`push ebp`压入ebp的地址，`mov ebp,esp`,将esp的值赋给ebp，也就是上一个栈帧的顶部是下一个栈帧的底部，后续`sub esp,4Ch`以及后续操作用于给函数所需的局部变量开辟空间，call指令用于将当前ip入栈并且跳转，用于函数调用，并记录返回地址。
4. 主函数反汇编后代码如下
``` C
int main(int argc, char** argv) {
00A41600 55                   push        ebp  
00A41601 8B EC                mov         ebp,esp  
00A41603 83 EC 40             sub         esp,40h  
00A41606 53                   push        ebx  
00A41607 56                   push        esi  
00A41608 57                   push        edi  
00A41609 B9 06 90 A4 00       mov         ecx,0A49006h  
00A4160E E8 91 FB FF FF       call        00A411A4  
	if (argc > 1) {
00A41613 83 7D 08 01          cmp         dword ptr [ebp+8],1  
00A41617 7E 17                jle         00A41630  
		sub(argv[1]);
00A41619 B8 04 00 00 00       mov         eax,4  
00A4161E C1 E0 00             shl         eax,0  
00A41621 8B 4D 0C             mov         ecx,dword ptr [ebp+0Ch]  
00A41624 8B 14 01             mov         edx,dword ptr [ecx+eax]  
00A41627 52                   push        edx  
00A41628 E8 BD FB FF FF       call        00A411EA  
00A4162D 83 C4 04             add         esp,4  
	}
	printf("exit");
00A41630 68 30 5B A4 00       push        0A45B30h  
00A41635 E8 02 FA FF FF       call        00A4103C  
00A4163A 83 C4 04             add         esp,4  
	system("pause");
00A4163D 68 40 5B A4 00       push        0A45B40h  
00A41642 FF 15 18 81 A4 00    call        dword ptr ds:[00A48118h]  
00A41648 83 C4 04             add         esp,4  
}
00A4164B 33 C0                xor         eax,eax  
00A4164D 5F                   pop         edi  
00A4164E 5E                   pop         esi  
00A4164F 5B                   pop         ebx  
}
00A41650 8B E5                mov         esp,ebp  
00A41652 5D                   pop         ebp  
```
在 `00A41628 E8 BD FB FF FF       call        00A411EA`处调用的sub函数。
<img src='img\3.png'>
我们可以看到命令参数在进入函数被存储在了edx中，接下来我们进入sub函数
5. sub反汇编后的代码如下：<img src='img\1.png'>
在函数执行完`00A416DB E8 F1 FA FF FF       call        00A411D1  `即从`strcpy(x,y)`返回后我们观察ebp中存储的数据<img src='img\4.png'>,此时由于缓冲区溢出，导致ebp中存储的栈帧底部变为多个39，也就是说函数的返回地址被覆盖了，无法寻找主函数的地址，在ret指令后ide会报异常。
<img src='img\5.png'>
6. 实验结束