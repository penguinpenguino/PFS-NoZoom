#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>

#define PROC_NAME "PirateFS-Win64-Shipping.exe"
#define HIT "\x1B[32m[+]\x1B[39m "
#define MISS "\x1B[31m[-]\x1B[39m "

#define LOC 0x20DC490
#define BYTES "\xF3\x0F\x11\x89\xA0\x02\x00\x00"
#define NOPS  "\x90\x90\x90\x90\x90\x90\x90\x90"
#define NUMBYTES 8

#define print(str) WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), str, strlen(str), NULL, NULL)

bool GetPIDByName();
bool GetProcBase();
void toggle(bool enabled);
void on_exit();
DWORD WINAPI Listen(void* args);

HANDLE proc;
void* base;
DWORD pid;
bool enabled = true;

int main() {
	// Enable colored output
	SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
	BYTE* bytes;
	BYTE* tbytes;

	// Find process
	if (!GetPIDByName()) {
		print(MISS "Couldn\'t find PFS\n");
		goto end;
	}
	print(HIT "Found PFS\n");

	// Open it
	proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (!proc) {
		print(MISS "Couldn\'t open PFS\n");
		goto end;
	}
	print(HIT "Opened PFS\n");

	// Find image base
	if (!GetProcBase()) {
		print(MISS "Couldn\'t get image base\n");
		goto end;
	}
	print(HIT "Got image base\n");
	
	// Verify bytes
	bytes = reinterpret_cast<BYTE*>(malloc(NUMBYTES));
	tbytes = reinterpret_cast<BYTE*>(malloc(NUMBYTES));
	memcpy(tbytes, BYTES, NUMBYTES);
	if (!ReadProcessMemory(proc, base + LOC, bytes, NUMBYTES, NULL)) {
		print(MISS "Couldn\'t read memory\n");
		goto end;
	}
	for (int i = 0; i < NUMBYTES; i++) {
		if (bytes[i] != tbytes[i]) {
			print(MISS "Sig is invalid, game probably updated. DM GoodUsername240 on Discord and I\'ll update it.\n");
			free(bytes);
			goto end;
		}
	}
	free(tbytes);
	free(bytes);

	
	toggle(enabled);
	CreateThread(0, 0, Listen, 0, 0, 0);
	atexit(on_exit);
end:
	while (1)
		Sleep(1000000);
	return 0;
}



DWORD WINAPI Listen(void* args) {
	while (1) {
		if (GetAsyncKeyState(VK_OEM_COMMA) & 0x8000) {
			enabled = !enabled;
			toggle(enabled);
			
			// Prevent it from spam toggling it
			while (GetAsyncKeyState(VK_OEM_COMMA) & 0x8000) {
				Sleep(10);
			}
		}
		Sleep(10);
	}
	return 0;
}

void toggle(bool enabled) {
	if (enabled) {
		if (WriteProcessMemory(proc, base + LOC, NOPS, NUMBYTES, NULL))
			print("\x1B[2J\x1B[0;0f\x1B[?25l" HIT "Enabled (, to toggle)");
	} else {
		if (WriteProcessMemory(proc, base + LOC, BYTES, NUMBYTES, NULL))
			print("\x1B[2J\x1B[0;0f\x1B[?25l" MISS "Disabled (, to toggle)");
	}
}

void on_exit() {
	toggle(false);
}

bool GetProcBase() {
	// Get HMODULE
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
	MODULEENTRY32 entry;
	entry.dwSize = sizeof(MODULEENTRY32);
	Module32First(snap, &entry);
	do {
		if (strcmp(entry.szModule, PROC_NAME) == 0)
			break;
	} while (Module32Next(snap, &entry));
	CloseHandle(snap);
	if (strcmp(entry.szModule, PROC_NAME) != 0)
		return false;

	// Get module base
	MODULEINFO info;
	if (!GetModuleInformation(proc, entry.hModule, &info, sizeof(MODULEINFO)))
		return false;
	base = info.lpBaseOfDll;
	return true;
}

bool GetPIDByName() {
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);
	Process32First(snap, &entry);
	do {
		if (strcmp(entry.szExeFile, PROC_NAME) == 0) {
			CloseHandle(snap);
			pid = entry.th32ProcessID;
			return true;
		}
	} while (Process32Next(snap, &entry));
	CloseHandle(snap);
	return false;
}