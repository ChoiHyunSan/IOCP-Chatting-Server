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

	// ���� ����
	FILE*	_pFile;							// ���� ������
	char	_fileName[100];					// ���� �̸�
	bool	_isConnect = false;				// ���� �������� ��Ȳ   

	char    _fileBuffer[FILE_BUFFER_MAX];	// ���� ������ ����� ����
	unsigned long	_fileSize = 0;			// ���� ���� ������ ũ��

	char* _pValue;							// ���� ����Ű�� ���� �ּ� ��ġ
};

