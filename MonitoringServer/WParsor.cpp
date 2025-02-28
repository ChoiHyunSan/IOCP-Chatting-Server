#include "WParsor.h"
#include <wchar.h>

WParsor::WParsor()
	: _isConnect(false)
{
}

WParsor::~WParsor()
{
	if (_isConnect)
	{
		fclose(_pFile);
	}
}

bool WParsor::LoadFile(const WCHAR* fileName)
{
	_wfopen_s(&_pFile, fileName, L"r");
	if (_pFile == nullptr)
	{
		return false;
	}

	_isConnect = true;

	// BOM 코드 체크 후, UTF-16LE 인코딩이 확인되면 BOM 코드만큼을 건너뛰기
	WCHAR bomCode;
	fread(&bomCode, sizeof(WCHAR), 1, _pFile);
	if (bomCode != 0xFEFF)
	{
		return false;
	}

	fread(_fileBuffer, sizeof(WCHAR), FILE_BUFFER_MAX, _pFile);
	_pValue = _fileBuffer;

	return true;
}

bool WParsor::GetValue(const WCHAR* valName, int* value, int size)
{
	if (!_isConnect)
	{
		return false;
	}

	_pValue = _fileBuffer;

	const int bufSize = 100;
	WCHAR buf[bufSize];

	while (GetNextWord(buf, bufSize))
	{
		// Value Name
		if (0 == wcscmp(buf, valName))
		{
			if (GetNextWord(buf, bufSize))
			{
				// : 
				if (0 == wcscmp(buf, L":"))
				{
					// Value
					if (GetNextWord(buf, bufSize))
					{
						if (size != 0)
						{
							memcpy(value, buf, size);
						}
						else
						{
							*value = _wtoi(buf);
						}
						return true;
					}
					return false;
				}
				return false;
			}
			return false;
		}
	}

	return false;
}	

bool WParsor::GetNextWord(WCHAR* readBuf, int bufSize)
{
	WCHAR* start = _pValue;
	WCHAR* last = _pValue;
	int length = 0;
	int skip = 0;
	while (1)
	{
		if (*last == ',' || *last == '"' || *last == 0x20 || *last == 0x00 ||
			*last == 0x08 || *last == 0x09 || *last == 0x0a || *last == 0x0d)
		{
			if (length == 0)
			{
				start += 1;
				last += 1;
				skip++;
			}
			else
			{
				break;
			}
		}
		else
		{
			length += 1;
			last += 1;
		}

		// 파일의 끝에 도달하거나 bufSize를 초과하는 경우 false
		if (_fileBuffer + FILE_BUFFER_MAX <= last || length >= bufSize - 1)
		{
			return false;
		}
	}

	memcpy_s(readBuf, length * sizeof(WCHAR), start, length * sizeof(WCHAR));
	readBuf[length] = 0;

	_pValue += (length + 1 + skip);

	return true;
}

