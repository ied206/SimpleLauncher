/* SimpleLauncher v1.1
 *   Very Simple Executable Launcher for Win32
 *   Useful in PE envrionment which contains minimum dependency
 *
 *  Written By ied206 (aka Joveler)
 *    https://github.com/ied206
 *    https://joveler.kr
 *
 *  Launches program and exit.
 *  You can change LAUNCH_INI macro to set ini file name.
 *
 *  Distributed under MIT License.
 */

#define UNICODE
#define _UNICODE

#define ERR_FILE_DO_NOT_EXIST			1
#define ERR_CREATEFILE_FAIL				2
#define ERR_READFILE_FAIL				3
#define ERR_UNSUPPORTED_ENCODING		4
#define ERR_UNKNOWN_ENCODING			5
#define ERR_MULTIBYTETOWIDECHAR_FAIL	6
#define ERR_MALLOC_FAIL					7
#define ERR_ENV_NAME_TOO_LONG			8

#define ENCODING_ANSI		0
#define ENCODING_UTF16_LE	1
#define ENCODING_UTF16_BE	2
#define ENCODING_UTF8		3

// You can customize LAUNCH_INI value
#define LAUNCH_INI		L"PrecCalcPath.ini"
#define MAX_PATH_LEN_EX	32768
#define MAX_BUF_LEN_EX	(32768 * 4)
#define MAX_MSG_BUF 256


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <shlwapi.h>
#if defined(__MINGW32__) && defined(_DEBUG)
#undef __CRT__NO_INLINE
#endif // __MINGW32__
#include <strsafe.h>
#if defined(__MINGW32__) && defined(_DEBUG)
#define __CRT__NO_INLINE
#endif // __MINGW32__
#include <windows.h>


wchar_t errMsg[MAX_MSG_BUF] = {0};
int DetectBOM(uint8_t* buf);
int ConvertToUTF16(uint8_t* src, size_t srcLen, wchar_t* dest, size_t destMaxLen, int encoding);
size_t ExpandLaunchPath(wchar_t* srcBuf, wchar_t* destBuf, size_t destMaxLen);
wchar_t* GetParameters();

int main(int argc, char* argv[])
{
	HANDLE hFile = NULL;
	int hRes = 0;
	uint8_t rawBuf[MAX_BUF_LEN_EX] = {0};
	wchar_t convBuf[MAX_BUF_LEN_EX] = {0};
	wchar_t expandBuf[MAX_BUF_LEN_EX] = {0};
	wchar_t fileName[MAX_PATH_LEN_EX] = {0};
	uint32_t readByte = 0;
	int encoding = 0;


	// Get Path Name
	GetModuleFileNameW(NULL, fileName, MAX_PATH_LEN_EX);
	StringCchCopyW((WCHAR*) StrRChrW(fileName, NULL, L'\\')+1, MAX_PATH_LEN_EX, LAUNCH_INI);
	if (!PathFileExistsW(fileName))
	{
		StringCchPrintfW((WCHAR*) errMsg, MAX_MSG_BUF, L"[ERR] %s do not exists.\n\n", fileName);
		MessageBoxW(NULL, errMsg, NULL, MB_OK | MB_ICONERROR);
		fprintf(stderr, "[ERR] %S do not exists.\n\n", fileName);
		exit(ERR_FILE_DO_NOT_EXIST);
	}

	// Open File
	hFile = CreateFileW((WCHAR*) fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		StringCchPrintfW((WCHAR*) errMsg, MAX_MSG_BUF, L"[ERR] CreateFile() failed!\nError Code : %lu\n\n", GetLastError());
		MessageBoxW(NULL, errMsg, NULL, MB_OK | MB_ICONERROR);
		fprintf(stderr, "[ERR] CreateFile() failed!\nError Code : %lu\n\n", GetLastError());
		exit(ERR_CREATEFILE_FAIL);
	}

	// Read ini file
	if (!ReadFile(hFile, rawBuf, MAX_BUF_LEN_EX, (DWORD*) &readByte, NULL))
	{
		StringCchPrintfW((WCHAR*) errMsg, MAX_MSG_BUF, L"[ERR] ReadFile() failed!\nError Code : %lu\n\n", GetLastError());
		MessageBoxW(NULL, errMsg, NULL, MB_OK | MB_ICONERROR);
		fprintf(stderr, "[ERR] ReadFile() failed!\nError Code : %lu\n\n", GetLastError());
		exit(ERR_READFILE_FAIL);
	}

	// Detect Encoding
	encoding = DetectBOM(rawBuf);
	// Convert to UTF-16-LE
	ConvertToUTF16(rawBuf, readByte, convBuf, MAX_BUF_LEN_EX * sizeof(wchar_t), encoding);
	// Expand Environment Variables
	ExpandLaunchPath(convBuf, expandBuf, MAX_BUF_LEN_EX * sizeof(wchar_t));
	// Launch with ShellExecute API
	// According to MSDN, ShellExecute's return value can be casted only to int.
	// size_t casting is used to evade gcc's [-Wpointer-to-int-cast] warning.
	hRes = (int) ((size_t) ShellExecuteW(NULL, L"open", (LPCWSTR) expandBuf, (LPCWSTR) GetParameters(), NULL, SW_SHOWNORMAL));

	// Return and Exit
	if (32 < hRes)
		return 0;
	else
		return 1;
}



// 0xEF,0xBB,0xBF - UTF8
// 0xFE,0xFF - UTF-16 BE
// 0xFF,0xFE - UTF-16 LE
int DetectBOM(uint8_t* buf)
{
	if (buf[0] == 0xEF && buf[1] == 0xBB && buf[2] == 0xBF)
		return ENCODING_UTF8;
	else if (buf[0] == 0xFF && buf[1] == 0xFE)
		return ENCODING_UTF16_LE;
	else if (buf[0] == 0xFE && buf[1] == 0xFF)
		return ENCODING_UTF16_BE;
	else
		return ENCODING_ANSI;
}

int ConvertToUTF16(uint8_t* srcBuf, size_t srcLen, wchar_t* destBuf, size_t destMaxLen, int encoding)
{
	int destLen = 0;

	switch (encoding)
	{
	case ENCODING_ANSI:
		destLen = MultiByteToWideChar(CP_ACP, 0, (char*) srcBuf, srcLen, destBuf, destMaxLen);
		if (destLen == 0)
		{
			StringCchPrintfW((WCHAR*) errMsg, MAX_MSG_BUF, L"[ERR] MultiByteToWideChar() failed!\nError Code : %lu\n\n", GetLastError());
			MessageBoxW(NULL, errMsg, NULL, MB_OK | MB_ICONERROR);
			fprintf(stderr, "[ERR] MultiByteToWideChar() failed!\nError Code : %lu\n\n", GetLastError());
			exit(ERR_MULTIBYTETOWIDECHAR_FAIL);
		}
		break;
	case ENCODING_UTF16_LE:
		memcpy(destBuf, srcBuf+2, srcLen-2); // 2B for UTF16 BOM
		destLen = srcLen - 2;
		break;
	case ENCODING_UTF16_BE:
		StringCchPrintfW((WCHAR*) errMsg, MAX_MSG_BUF, L"[ERR] This program do not support UTF-16-BE.\nRecommend use of UTF-16-LE.\n\n");
		MessageBoxW(NULL, errMsg, NULL, MB_OK | MB_ICONERROR);
		fprintf(stderr, "[ERR] This program do not support UTF-16-BE.\nRecommend use of UTF-16-LE.\n\n");
		exit(ERR_UNSUPPORTED_ENCODING);
		break;
	case ENCODING_UTF8:
		destLen = MultiByteToWideChar(CP_UTF8, 0, (char*) (srcBuf+3), srcLen-3, destBuf, destMaxLen); // 3B for UTF-8 BOM
		if (destLen == 0)
		{
			StringCchPrintfW((WCHAR*) errMsg, MAX_MSG_BUF, L"[ERR] MultiByteToWideChar() failed!\nError Code : %lu\n\n", GetLastError());
			MessageBoxW(NULL, errMsg, NULL, MB_OK | MB_ICONERROR);
			fprintf(stderr, "[ERR] MultiByteToWideChar() failed!\nError Code : %lu\n\n", GetLastError());
			exit(ERR_MULTIBYTETOWIDECHAR_FAIL);
		}
		break;
	default:
		StringCchPrintfW((WCHAR*) errMsg, MAX_MSG_BUF, L"[ERR] Wrong encoding value in ConvertToUTF16()!\n\n");
		MessageBoxW(NULL, errMsg, NULL, MB_OK | MB_ICONERROR);
		fprintf(stderr, "[ERR] Wrong encoding value in ConvertToUTF16()!\n\n");
		exit(ERR_UNKNOWN_ENCODING);
		break;
	}

	return destLen;
}

size_t ExpandLaunchPath(wchar_t* srcBuf, wchar_t* destBuf, size_t destMaxLen)
{
	wchar_t* search = NULL;

	// 1. Interpret %EnvVar% from srcBuf to tmpBuf
	ExpandEnvironmentStringsW(srcBuf, destBuf, destMaxLen);

	// 2. Need only first line, just before \r\n
	search = StrStrW(destBuf, L"\r\n");
	if (search) // \r\n exists
		search[0] = L'\0';

	// Return buffer length of destBuf
	return wcslen(destBuf) * sizeof(wchar_t);
}

// Get Command line arguments and catch start point of argv[1]
wchar_t* GetParameters()
{
	wchar_t* cmdRawLine = GetCommandLineW();
	wchar_t* cmdParam = NULL;

	// Case 1 : Simplest form of 'single param', no space
	// Ex) calc.exe
	if (StrChrW(cmdRawLine, L' ') == NULL)
		cmdParam = NULL;
	else // It is 'multiple params' OR 'single param with quotes'
	{
		if (StrChrW(cmdRawLine, L'\"') == NULL)
			// Case 2 : 'multiple params' without quotes
			// Ex) notepad.exe Notepad-UTF8.txt
			cmdParam = StrChrW(cmdRawLine, L' ');
		else
		{
			// Detect if first parameter has quotes
			if (StrChrW(cmdRawLine, L'\"') == cmdRawLine)
			{
				wchar_t* cmdLeftQuote = NULL; // Start of first parameter
				wchar_t* cmdRightQuote = NULL; // End of first parameter
				cmdLeftQuote = StrChrW(cmdRawLine, L'\"');
				cmdRightQuote = StrChrW(cmdLeftQuote+1, L'\"');

				// Case 3 : Single param with quotes on first param
				// Ex) "Simple Browser.exe"
				if (StrChrW(cmdRightQuote+1, L' ') == NULL)
					cmdParam = NULL;
				// Case 4 : Multiple param with quotes on first param
				// Ex) "Simple Browser.exe" joveler.kr
				else
					cmdParam = StrChrW(cmdRightQuote+1, L' '); // Spaces between cmdLeftQuote and cmdRightQuote must be ignored
			}
			// Case 5 : Multiple param, but no quotes on first param
			// Ex) notepad.exe "Notepad UTF8.txt"
			else
				cmdParam = StrChrW(cmdRawLine, L' ');
		}
	}

	return cmdParam;
}

