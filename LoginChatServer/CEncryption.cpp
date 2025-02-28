#include "CEncryption.h"

CEncryptionHelper::CEncryptionHelper()
{
}

CEncryptionHelper::~CEncryptionHelper()
{
}

inline void CEncryptionHelper::EncodeByKey(BYTE* msg, const DWORD len, const BYTE key)
{
    BYTE prevRet = 0;
    for (int i = 0; i < len; i++)
    {
        msg[i] = msg[i] ^ (key + prevRet + i + 1);
        prevRet = msg[i];
    }
}

inline void CEncryptionHelper::DecodeByKey(BYTE* msg, const DWORD len, const BYTE key)
{
    for (int i = len - 1; i >= 1; i--)
    {
        msg[i] = msg[i] ^ (key + msg[i - 1] + i + 1);
    }
    msg[0] = msg[0] ^ (key + 1);
}

BYTE CEncryptionHelper::GetCheckSum(BYTE* msg, const DWORD len)
{
    INT64 sum = 0;
    for (int i = 0; i < len; i++)
    {
        sum += msg[i];
    }
    return (BYTE)(sum % 256);
}

BOOL CEncryptionHelper::EncodePacket(BYTE* msg, const DWORD len, const BYTE fixedKey, const BYTE randKey)
{
    if (msg == nullptr || len == 0) 
        return false;

    EncodeByKey(msg, len, randKey);
    EncodeByKey(msg, len, fixedKey);

    return true;
}

BOOL CEncryptionHelper::DecodePacket(BYTE* msg, const DWORD len, const BYTE fixedKey, const BYTE randKey)
{
    if (msg == nullptr || len == 0) 
        return false;

    DecodeByKey(msg, len, fixedKey);
    DecodeByKey(msg, len, randKey);

    return true;
}
