/*  Written By ied206 (aka Joveler)
 *    https://github.com/ied206
 *  Launches program and exit
 *  You can change LAUNCH_INI macro to set ini file name
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

#define ENCODING_ANSI		0
#define ENCODING_UTF16_LE	1
#define ENCODING_UTF16_BE	2
#define ENCODING_UTF8		3

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

int main(int argc, char* argv[])
{
	HANDLE hFile = NULL;
	uint8_t rawBuf[MAX_BUF_LEN_EX] = {0};
	wchar_t convBuf[MAX_BUF_LEN_EX] = {0};
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
	ConvertToUTF16(rawBuf, readByte, convBuf, MAX_BUF_LEN_EX, encoding);

	// Launch
	ShellExecuteW(NULL, L"open", (LPCWSTR) convBuf, NULL, NULL, SW_SHOW);
	return 0;
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

int ConvertToUTF16(uint8_t* src, size_t srcLen, wchar_t* dest, size_t destMaxLen, int encoding)
{
	int destLen = 0;

	switch (encoding)
	{
	case ENCODING_ANSI:
		destLen = MultiByteToWideChar(CP_ACP, 0, (char*) src, srcLen, dest, destMaxLen);
		if (destLen == 0)
		{
			StringCchPrintfW((WCHAR*) errMsg, MAX_MSG_BUF, L"[ERR] MultiByteToWideChar() failed!\nError Code : %lu\n\n", GetLastError());
			MessageBoxW(NULL, errMsg, NULL, MB_OK | MB_ICONERROR);
			fprintf(stderr, "[ERR] MultiByteToWideChar() failed!\nError Code : %lu\n\n", GetLastError());
			exit(ERR_MULTIBYTETOWIDECHAR_FAIL);
		}
		break;
	case ENCODING_UTF16_LE:
		memcpy(dest, src+2, srcLen-2); // 2B for UTF16 BOM
		destLen = srcLen - 2;
		break;
	case ENCODING_UTF16_BE:
		StringCchPrintfW((WCHAR*) errMsg, MAX_MSG_BUF, L"[ERR] This program do not support UTF-16-BE.\nRecommend use of UTF-16-LE.\n\n");
		MessageBoxW(NULL, errMsg, NULL, MB_OK | MB_ICONERROR);
		fprintf(stderr, "[ERR] This program do not support UTF-16-BE.\nRecommend use of UTF-16-LE.\n\n");
		exit(ERR_UNSUPPORTED_ENCODING);
		break;
	case ENCODING_UTF8:
		destLen = MultiByteToWideChar(CP_UTF8, 0, (char*) (src+3), srcLen-3, dest, destMaxLen); // 3B for UTF-8 BOM
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
