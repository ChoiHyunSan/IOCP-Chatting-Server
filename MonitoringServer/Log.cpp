#pragma once
#include "Log.h"

// 해당 변수를 변경하여 런타임으로 로그 정보를 남긴다.
int	g_iLogLevel = dfLOG_LEVEL_ERROR;

void ChangeLogLevel(const int newLevel)
{
	g_iLogLevel = newLevel;
}

void Log(const WCHAR* msg, int errCode, int logLevel, const WCHAR* file)
{
	if (g_iLogLevel > logLevel) return;

	FILE* pFile;
	bool result = _wfopen_s(&pFile, file, L"a");
	if (result == 0)
	{
		const char* date_char = __DATE__;
		wchar_t date_wchar[12];
		size_t converted_chars = 0;

		// char 배열을 wchar_t 배열로 변환
		mbstowcs_s(&converted_chars, date_wchar, sizeof(date_wchar) / sizeof(wchar_t), date_char, _TRUNCATE);

		const char* time_char = __TIME__;
		wchar_t time_wchar[12];
		converted_chars = 0;

		mbstowcs_s(&converted_chars, time_wchar, sizeof(time_wchar) / sizeof(wchar_t), time_char, _TRUNCATE);

		// TODO : : LOG TIME 찍기
		fwprintf(pFile, L"[ %s | %s ] ", date_wchar, time_wchar);
		fwprintf(pFile, L"%s", msg);
		if (errCode != 0)
		{
			fwprintf(pFile, L" , errCode : %d", errCode);
		}
		fwprintf(pFile, L"\n");

		fclose(pFile);
	}
}