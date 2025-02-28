#pragma once
#include <Windows.h>
#include <iostream>

#define FILE_BUFFER_MAX 1000 * sizeof(WCHAR)

// UTF-16FE 파일을 읽어오기 위한 클래스
// 다른 인코딩으로 만들어진 파일은 읽지 않는다.

class WParsor
{
public:
	WParsor();
	~WParsor();

public:
	bool LoadFile(const WCHAR* fileName);
	bool GetValue(const WCHAR* valName, int* value, int charFlag = 0);
	bool GetNextWord(WCHAR* buf, int bufSize);

private:
	FILE*	_pFile;								// 파일 포인터
	WCHAR	_fileName[100 * sizeof(WCHAR)];		// 파일 이름
	bool	_isConnect = false;					// 현재 연결중인 상황

	WCHAR	_fileBuffer[FILE_BUFFER_MAX];		// 파일 내용이 저장될 공간
	unsigned long _fileSize = 0;				// 실제 받은 파일의 크기
		
	WCHAR*	_pValue;							// 현재 가르키는 값의 주소 위치
};

