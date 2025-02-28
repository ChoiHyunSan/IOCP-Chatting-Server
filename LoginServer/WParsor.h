#pragma once
#include <Windows.h>
#include <iostream>

#define FILE_BUFFER_MAX 1000 * sizeof(WCHAR)

// UTF-16FE ������ �о���� ���� Ŭ����
// �ٸ� ���ڵ����� ������� ������ ���� �ʴ´�.

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
	FILE*	_pFile;								// ���� ������
	WCHAR	_fileName[100 * sizeof(WCHAR)];		// ���� �̸�
	bool	_isConnect = false;					// ���� �������� ��Ȳ

	WCHAR	_fileBuffer[FILE_BUFFER_MAX];		// ���� ������ ����� ����
	unsigned long _fileSize = 0;				// ���� ���� ������ ũ��
		
	WCHAR*	_pValue;							// ���� ����Ű�� ���� �ּ� ��ġ
};

