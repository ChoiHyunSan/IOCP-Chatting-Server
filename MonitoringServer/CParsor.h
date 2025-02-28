#pragma once
#include <Windows.h>
#include <iostream>

#define FILE_BUFFER_MAX 1000

class CParsor
{
public:
	CParsor();
	~CParsor();

public:
	bool LoadFile(const char* fileName);
	bool GetValue(const char* valName, int* value, int charFlag = 0);
	bool GetNextWord(char* buf, int bufSize);
	bool SkipNoneCommand(void);

private:

	// 파일 관련
	FILE*	_pFile;							// 파일 포인터
	char	_fileName[100];					// 파일 이름
	bool	_isConnect = false;				// 현재 연결중이 상황   

	char    _fileBuffer[FILE_BUFFER_MAX];	// 파일 내용이 저장될 공간
	unsigned long	_fileSize = 0;			// 실제 받은 파일의 크기

	char* _pValue;							// 현재 가르키는 값의 주소 위치
};

