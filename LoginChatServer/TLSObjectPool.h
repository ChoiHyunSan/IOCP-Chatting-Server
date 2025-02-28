#pragma once
#include <iostream>
#include <Windows.h>
#include "NLObjectPool.h"
#include "CProfiler.h"

#define dfTHREAD_COUNT  100

#define dfINIT_SIZE		200
#define dfMAX_SIZE		3000
#define dfMIN_SUBSIZE   1000

extern LONG subCnt;

template<typename T>
class TLSObjectPool
{
public:
	TLSObjectPool();

	T*					Alloc(int i = 0);
	bool				Free(T* pData);

	NLObjectPool<T>*	GetObjectPool();
	void				RequestFreeList(NLObjectPool<T>* objectPool);
	DWORD	            GetTotalSize();
private:
	NLObjectPool<T>*	_objectPoolArr[dfTHREAD_COUNT];
	DWORD				_tlsIndex;

	DWORD				_maxSize = dfMAX_SIZE;
};

template<typename T>
inline TLSObjectPool<T>::TLSObjectPool()
{
	ZeroMemory(_objectPoolArr, sizeof(_objectPoolArr));

	_tlsIndex = TlsAlloc();
	if (_tlsIndex == TLS_OUT_OF_INDEXES)
	{
		DebugBreak();
	}
}

template<typename T>
inline T* TLSObjectPool<T>::Alloc(int i)
{
	NLObjectPool<T>* objectPool = GetObjectPool();
	if (objectPool->GetUseCount() <= 0)
	{
		RequestFreeList(objectPool);
		return objectPool->Alloc();
	}
	else
	{
		return objectPool->Alloc();
	}
}
template<typename T>
inline void TLSObjectPool<T>::RequestFreeList(NLObjectPool<T>* objectPool)
{
	int	maxSize = 0;
	int maxIndex;
	if (objectPool->GetSubUseCount() > 0)
	{
		objectPool->SwapSubList();
		return;
	}
	for (int i = 0; i < dfTHREAD_COUNT; i++)
	{
		if (_objectPoolArr[i] == nullptr)
			break;

		// 서브 리스트의 크기를 비교하여 maxSize & maxIndex 갱신
		int subListSize = _objectPoolArr[i]->GetSubUseCount();
		if (subListSize > maxSize)
		{
			maxIndex = i;
			maxSize = subListSize;
		}
	}
	if (maxSize != 0)
	{
		_objectPoolArr[maxIndex]->TakeSubList(objectPool);
	}
}

template<typename T>
inline bool TLSObjectPool<T>::Free(T* pData)
{
	NLObjectPool<T>* objectPool = GetObjectPool();

	// 1) 사이즈 체크 -> 많은 경우엔 Sub 리스트에 Free
	if (objectPool->GetUseCount() > _maxSize)
	{
		objectPool->FreeToSub(pData);
	}
	else
	{
		objectPool->Free(pData);
	}
	return true;
}

template<typename T>
inline DWORD TLSObjectPool<T>::GetTotalSize()
{
	DWORD sum = 0;
	for (int i = 0; i < dfTHREAD_COUNT; i++)
	{
		if (_objectPoolArr[i] == nullptr)
			break;

		sum += ((_objectPoolArr[i]->GetSubUseCount()) + (_objectPoolArr[i]->GetUseCount()));
	}
	return sum;
}

template<typename T>
inline NLObjectPool<T>* TLSObjectPool<T>::GetObjectPool()
{
	NLObjectPool<T>* objectPool = static_cast<NLObjectPool<T>*>(TlsGetValue(_tlsIndex));
	if (objectPool == nullptr)
	{
		objectPool = new NLObjectPool<T>(dfINIT_SIZE, false);

		TlsSetValue(_tlsIndex, static_cast<PVOID>(objectPool));

		static LONG _threadArrIndex = -1;
		LONG threadIndex = InterlockedIncrement(&_threadArrIndex);

		_objectPoolArr[threadIndex] = objectPool;
	}

	return objectPool;
}
