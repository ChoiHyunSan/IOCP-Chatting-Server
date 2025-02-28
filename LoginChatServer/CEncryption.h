#pragma once
#include <Windows.h>

class CEncryptionHelper
{
public:
	CEncryptionHelper();
	~CEncryptionHelper();

private:
	inline static void EncodeByKey(BYTE* msg, const DWORD len, const BYTE key);
	inline static void DecodeByKey(BYTE* msg, const DWORD len, const BYTE key);
public:
	static BOOL EncodePacket(BYTE* msg, const DWORD len, const BYTE fixedKey, const BYTE randKey);
	static BOOL DecodePacket(BYTE* msg, const DWORD payloadLen, const BYTE fixedKey, const BYTE randKey);
	static BYTE GetCheckSum(BYTE* msg, const DWORD len);
};

