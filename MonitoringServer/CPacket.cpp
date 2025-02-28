#include "CPacket.h"
#include <iostream>
using namespace std;

#define LOG_COUNT 200000

INT64 CPacket::_allocCount = 0;
INT64 CPacket::_freeCount = 0;

CPacket* CPacket::Alloc()
{
	CPacket* packet = tlsPacketPool.Alloc();
	packet->_refCnt = 1;
	packet->Clear();
	packet->_headerFlag = false;

	InterlockedIncrement64(&_allocCount);

	return packet;
}

bool CPacket::Free(CPacket* packet)
{
	if (packet == nullptr) 
		return false;

	int refCnt = InterlockedDecrement(&packet->_refCnt);
	if (refCnt == 0)
	{
		tlsPacketPool.Free(packet);
		InterlockedIncrement64(&_freeCount);
	}
	return false;
}

INT64 CPacket::GetAllocCount()
{
	return _allocCount;
}

INT64 CPacket::GetFreeCount()
{
	return _freeCount;
}

DWORD CPacket::GetTotalSize()
{
	return tlsPacketPool.GetTotalSize();
}

CPacket::CPacket()
	: _buffer(nullptr), _dataSize(0), _readIndex(0), _bufferSize(eBUFFER_DEFAULT)
{
	_buffer = (char*)malloc(_bufferSize);
}

CPacket::CPacket(int iBufferSize)
	: _buffer(nullptr), _dataSize(0), _readIndex(0), _bufferSize(iBufferSize)
{
	_buffer = (char*)malloc(iBufferSize);
}

CPacket::~CPacket()
{
	free(_buffer);
}

void CPacket::Clear(void)
{
	_dataSize = 0;
	_readIndex = 0;
}

int CPacket::MoveWritePos(int iSize)
{
	if (_dataSize + iSize < 0 || _dataSize + iSize > _bufferSize - 1)
		return 0;

	_dataSize += iSize;

	return iSize;
}

int CPacket::MoveReadPos(int iSize)
{
	if (_readIndex + iSize < 0 || _readIndex + iSize > _bufferSize - 1)
		return 0;

	_readIndex += iSize;
	return iSize;
}

CPacket& CPacket::operator=(CPacket& clSrcPacket)
{
	if (_buffer != nullptr)
	{
		free(_buffer);
	}

	_buffer = (char*)malloc(clSrcPacket.GetBufferSize());
	if (_buffer != nullptr);
	{
		memcpy(_buffer, clSrcPacket.GetBufferPtr(), clSrcPacket.GetBufferSize());
	}

	_refCnt = clSrcPacket._refCnt;
	AddRefCount();

	return *this;
}

CPacket& CPacket::operator<<(unsigned char byValue)
{
	if (_bufferSize < _dataSize + sizeof(unsigned char)) return *this;

	memcpy((_buffer + _dataSize), &byValue, sizeof(unsigned char));
	_dataSize += sizeof(unsigned char);

	return *this;
}

CPacket& CPacket::operator<<(char chValue)
{
	if (_bufferSize < _dataSize + sizeof(char)) return *this;

	memcpy((_buffer + _dataSize), &chValue, sizeof(char));
	_dataSize += sizeof(char);

	return *this;
}

CPacket& CPacket::operator<<(short shValue)
{
	if (_bufferSize < _dataSize + sizeof(short)) return *this;

	memcpy((_buffer + _dataSize), &shValue, sizeof(short));
	_dataSize += sizeof(short);

	return *this;
}

CPacket& CPacket::operator<<(unsigned short wValue)
{
	if (_bufferSize < _dataSize + sizeof(unsigned short)) return *this;

	memcpy((_buffer + _dataSize), &wValue, sizeof(unsigned short));
	_dataSize += sizeof(unsigned short);

	return *this;
}

CPacket& CPacket::operator<<(int iValue)
{
	if (_bufferSize < _dataSize + sizeof(int)) return *this;

	memcpy((_buffer + _dataSize), &iValue, sizeof(int));
	_dataSize += sizeof(int);

	return *this;
}

CPacket& CPacket::operator<<(DWORD iValue)
{
	if (_bufferSize < _dataSize + sizeof(DWORD)) return *this;

	memcpy((_buffer + _dataSize), &iValue, sizeof(DWORD));
	_dataSize += sizeof(DWORD);

	return *this;
}

CPacket& CPacket::operator<<(long lValue)
{
	if (_bufferSize < _dataSize + sizeof(long)) return *this;

	memcpy((_buffer + _dataSize), &lValue, sizeof(long));
	_dataSize += sizeof(long);

	return *this;
}

CPacket& CPacket::operator<<(float fValue)
{
	if (_bufferSize < _dataSize + sizeof(float)) return *this;

	memcpy((_buffer + _dataSize), &fValue, sizeof(float));
	_dataSize += sizeof(float);

	return *this;
}

CPacket& CPacket::operator<<(__int64 iValue)
{
	if (_bufferSize < _dataSize + sizeof(__int64)) return *this;

	memcpy((_buffer + _dataSize), &iValue, sizeof(__int64));
	_dataSize += sizeof(__int64);

	return *this;
}

CPacket& CPacket::operator<<(double dValue)
{
	if (_bufferSize < _dataSize + sizeof(double)) return *this;

	memcpy((_buffer + _dataSize), &dValue, sizeof(double));
	_dataSize += sizeof(double);

	return *this;
}

CPacket& CPacket::operator>>(BYTE& byValue)
{
	if ((_dataSize - _readIndex) < sizeof(BYTE)) return *this;

	byValue = (BYTE) * (BYTE*)(_buffer + _readIndex);
	_readIndex += sizeof(BYTE);

	return *this;
}

CPacket& CPacket::operator>>(char& chValue)
{
	if ((_dataSize - _readIndex) < sizeof(char)) return *this;

	chValue = (char)*(char*)(_buffer + _readIndex);
	_readIndex += sizeof(char);

	return *this;
}

CPacket& CPacket::operator>>(short& shValue)
{
	if ((_dataSize - _readIndex) < sizeof(short)) return *this;

	shValue = (short)*(short*)(_buffer + _readIndex);
	_readIndex += sizeof(short);

	return *this;
}

CPacket& CPacket::operator>>(WORD& wValue)
{
	if ((_dataSize - _readIndex) < sizeof(WORD)) return *this;

	wValue = (WORD) * (WORD*)(_buffer + _readIndex);
	_readIndex += sizeof(WORD);

	return *this;
}

CPacket& CPacket::operator>>(int& iValue)
{
	if ((_dataSize - _readIndex) < sizeof(int)) return *this;

	iValue = (int)*(int*)(_buffer + _readIndex);
	_readIndex += sizeof(int);

	return *this;
}

CPacket& CPacket::operator>>(DWORD& dwValue)
{
	if ((_dataSize - _readIndex) < sizeof(DWORD)) return *this;

	dwValue = (DWORD) * (DWORD*)(_buffer + _readIndex);
	_readIndex += sizeof(DWORD);

	return *this;
}

CPacket& CPacket::operator>>(float& fValue)
{
	if ((_dataSize - _readIndex) < sizeof(float)) return *this;

	fValue = (float)*(float*)(_buffer + _readIndex);
	_readIndex += sizeof(float);

	return *this;
}

CPacket& CPacket::operator>>(__int64& iValue)
{
	if ((_dataSize - _readIndex) < sizeof(__int64)) return *this;

	iValue = (__int64)*(__int64*)(_buffer + _readIndex);
	_readIndex += sizeof(__int64);

	return *this;
}

CPacket& CPacket::operator>>(double& dValue)
{
	if ((_dataSize - _readIndex) < sizeof(double)) return *this;

	dValue = (double)*(double*)(_buffer + _readIndex);
	_readIndex += sizeof(double);

	return *this;
}

int CPacket::GetData(char* chpDest, int iSize)
{
	iSize = min(iSize, (_dataSize - _readIndex));

	memcpy(chpDest, (_buffer + _readIndex), iSize);
	_readIndex += iSize;

	return iSize;
}

int CPacket::PutData(char* chpSrc, int iSrcSize)
{
	iSrcSize = min(iSrcSize, (_bufferSize - _dataSize));

	memcpy((_buffer + _dataSize), chpSrc, iSrcSize);
	_dataSize += iSrcSize;

	return iSrcSize;
}

BOOL CPacket::AddHeader(void* header, int size)
{
	if (_headerFlag) return false;

	// 헤더 길이만큼 버퍼에 있는 내용을 옮기기
	char temp[eBUFFER_DEFAULT];
	memcpy(temp, header, size);
	memcpy(temp + size, GetBufferPtr(), GetDataSize());

	memcpy(GetBufferPtr(), temp, size + GetDataSize());
	MoveWritePos(size);
	_headerFlag = true;

	return true;
}
