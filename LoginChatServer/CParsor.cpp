#include "CParsor.h"

CParsor::CParsor()
	: _isConnect(false)
{

}

CParsor::~CParsor()
{
	if (_isConnect)
	{
		fclose(_pFile);
	}
}

bool CParsor::LoadFile(const char* fileName)
{
	fopen_s(&_pFile, fileName, "r");
	if (_pFile == nullptr)
	{
		return false;
	}

	_isConnect = true;

	fread(_fileBuffer, 1, FILE_BUFFER_MAX, _pFile);
	_pValue = _fileBuffer;

	return true;
}

bool CParsor::GetValue(const char* valName, int* value, int size)
{
	if (!_isConnect)
	{
		return false;
	}

	_pValue = _fileBuffer;

	const int bufSize = 100;
	char buf[bufSize];
	while (GetNextWord(buf, bufSize))
	{
		// Value Name
		if (0 == strcmp(buf, valName))
		{
			if (GetNextWord(buf, bufSize))
			{
				// : 
				if (0 == strcmp(buf, ":"))
				{
					if (GetNextWord(buf, bufSize)) 
					{
						if (size != 0)
						{
							size = min(strlen(buf), size);
							memcpy(value, buf, size);
						}
						else
						{
							*value = atoi(buf);
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

// 탐색 시작 위치 / 
bool CParsor::GetNextWord(char* readBuf, int bufSize)
{

	char* start = _pValue;
	char* last = _pValue;
	int length = 0;
	int skip = 0;
	while (1)
	{
		if (*last == ',' || *last == '"' || *last == 0x20 || *last == 0x00 || 
			*last == 0x08 || *last == 0x09 || *last == 0x0a || *last == 0x0d)
		{
			if (length == 0)
			{
				start	+= 1;
				last	+= 1;
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

	memcpy_s(readBuf, length, start, length);
	readBuf[length] = 0;

	_pValue += (length + 1 + skip);

	return true;
}

bool CParsor::SkipNoneCommand(void)
{
	return false;
}
