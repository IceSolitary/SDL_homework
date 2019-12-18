# DLL的创建与使用
## 实验目的：
>   学会创建与使用DLL
## 实验步骤：
### 1、会编写dll。把.c文件编译为obj文件，把obj文件和lib文件链接为新的dll和lib文件。注意使用def文件定义导出函数。
#### 具体过程：
首先设置项目的配置类型为动态库，如下图： 
<img src='img\1.png'>  
编写一个C函数用于呼出messageBox
``` c
#include<Windows.h>

int lib_func(char *msg) {
	MessageBox(NULL,msg,"nya",MB_OK);
	return 0;
}

```
编写def文件定义导出函数，先做如下配置：<img src='img\2.png'>  
编写def文件，文件名为exp.def：
``` 
LIBRARY BaseLib
EXPORTS 
	lib_func @1
```
将BaseLib.c文件编译成base.obj文件 在cmd中输入`cl -c BaseLib.c`  
再对base.obj进行链接导出为dll，在cmd中输入`link -dll -def:exp.def BaseLib.obj User32.lib`
此时工程文件夹中已经产生了相应的dll文件  
使用`dumpbin /exports BaseLib.dll`查看导出函数，如下图  
<img src='img\3.png'>  
### 2、编写一个exe，调用第一步生成的dll文件中的导出函数。方法是（1）link是，将第一步生成的lib文件作为输入文件。（2）保证dll文件和exe文件在同一个目录，或者dll文件在系统目录。
#### 具体过程：
创建一个exe
编写BaseLib.h头文件
``` c
#pragma once

int lib_func(char *msg);
```
编写test.c作为主函数
``` c
#include"BaseLib.h"

int main()
{
	lib_func("call a dll");
	return 0;
}
```
编译test.c`cl -c test.c`，将之前生成的dll和lib放在同一目录下，链接`link test.obj BaseLib.lib`
运行test.exe  
<img src='img\4.png'>  

### 3、第二步调用方式称为load time 特点是exe文件导入表中会出先需要调用的dll文件名及函数名，并且在link 生成exe是，需明确输入lib文件。还有一种调用方式称为 run time。参考上面的链接，使用run time的方式，调用dll的导出函数。包括系统API和第一步自行生成的dll，都要能成功调用。
#### 具体过程：
编写 RunTimeTest.c用于测试
``` c
// A simple program that uses LoadLibrary and 
// GetProcAddress to access myPuts from Myputs.dll. 

#include <windows.h> 
#include <stdio.h> 

typedef int(__cdecl *MYPROC)(LPWSTR);

int main(void)
{
	HINSTANCE hinstLib;
	MYPROC ProcAdd;
	BOOL fFreeResult, fRunTimeLinkSuccess = FALSE;

	// Get a handle to the DLL module.

	hinstLib = LoadLibrary(TEXT("BaseLib.dll"));

	// If the handle is valid, try to get the function address.

	if (hinstLib != NULL)
	{
		ProcAdd = (MYPROC)GetProcAddress(hinstLib, "lib_func");

		// If the function address is valid, call the function.

		if (NULL != ProcAdd)
		{
			fRunTimeLinkSuccess = TRUE;
			(ProcAdd)("call a dll!!!");
		}
		// Free the DLL module.

		fFreeResult = FreeLibrary(hinstLib);
	}

	// If unable to call the DLL function, use an alternative.
	if (!fRunTimeLinkSuccess)
		printf("Message printed from executable\n");

	return 0;

}
```
在cmd中输入`cl RunTimeTest.c`直接编译链接,将dll放在同一目录下直接运行RunTimeTest.exe，成功，结果如下图：  
<img src='img\5.png'>