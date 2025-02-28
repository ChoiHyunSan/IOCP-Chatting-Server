#pragma once
#include <Windows.h>
#include <iostream>
#define out

/***************************

	   CLockFreeQueue

***************************/
// - TLS MemoryPool에서 사용하기 위해 락프리가 적용되지 않은 메모리 풀 (NL : No Lock)
//   그 외에 모니터링 등에 필요한 기능들이 추가로 구현되어있다.

template<typename T>
class NLMemoryPool
{
public:
	struct st_BLOCK_NODE
	{
		st_BLOCK_NODE() : _next(nullptr) {}

		T data;
		st_BLOCK_NODE* _next;
	};


public:
	NLMemoryPool(int iBlockNum, bool bPlacementNew = false);
	virtual	~NLMemoryPool();


public:
	T*		Alloc(void);
	bool	Free(T* pData);
	void	FreeToSub(T* pData);

	int		GetCapacityCount(void)	{ return _capacity; }
	int		GetUseCount(void)		{ return _size; }
	int		GetSubUseCount(void)	{ return _subSize; }

	void	TakeSubList(NLMemoryPool<T>* objectPool);
	void	SwapSubList();

public:
	st_BLOCK_NODE*	_pFreeNode		= nullptr;
	st_BLOCK_NODE*	_pSubFreeNode	= nullptr;
		
	LONG			_subSize = 0;
	LONG			_size = 0;

private:
	int _useCount;
	int _capacity;

	const bool	_placementNew;
	SRWLOCK		_subLock;
};

template<typename T>
inline NLMemoryPool<T>::NLMemoryPool(int iBlockNum /* 초기 생성 개수 */, bool bPlacementNew /*꺼낼 때 생성자 호출 여부*/)
	: _placementNew(bPlacementNew)
{
	InitializeSRWLock(&_subLock);

	// 초기 생성 오브젝트가 있을 시, 개수만큼 만든다.
	if (iBlockNum > 0)
	{
		for (int cnt = 0; cnt < iBlockNum; cnt++)
		{
			st_BLOCK_NODE* newNode;
			if (_placementNew)
			{
				newNode = (st_BLOCK_NODE*)malloc(sizeof(st_BLOCK_NODE));
				newNode->_next = nullptr;
			}
			else
			{
				newNode = new st_BLOCK_NODE;
			}

			if (_pFreeNode == nullptr)
			{
				_pFreeNode = newNode;
				InterlockedIncrement(&_size);

				continue;
			}

			newNode->_next = _pFreeNode->_next;
			_pFreeNode->_next = newNode;

			InterlockedIncrement(&_size);
		}
	}
}

template<typename T>
inline NLMemoryPool<T>::~NLMemoryPool()
{
	while (_pFreeNode != nullptr)
	{
		st_BLOCK_NODE* oldNode = _pFreeNode;
		_pFreeNode = oldNode->_next;

		if (_placementNew)
		{
			free(oldNode);
		}
		else
		{
			delete oldNode;
		}
	}

	delete _pFreeNode;
}

template<typename T>
inline T* NLMemoryPool<T>::Alloc(void)
{
	if (_pFreeNode == nullptr)
	{
		for (int i = 0; i < 300; i++)
		{
			st_BLOCK_NODE* newNode;
			if (_placementNew)
			{
				newNode = (st_BLOCK_NODE*)malloc(sizeof(st_BLOCK_NODE));
			}
			else
			{
				newNode = new st_BLOCK_NODE;
			}

			newNode->_next = _pFreeNode;
			_pFreeNode = newNode;
		}
		_size += 300;
	}

	st_BLOCK_NODE* topNode = _pFreeNode;
	_pFreeNode = topNode->_next;

	if (_placementNew)
	{
		st_BLOCK_NODE* placementNode = new(topNode) st_BLOCK_NODE;
		return (T*)&(placementNode->data);
	}

	_size -= 1;

	return (T*)&(topNode->data);
}

template<typename T>
inline bool NLMemoryPool<T>::Free(T* pData)
{
	st_BLOCK_NODE* topNode = _pFreeNode;

	_pFreeNode = (st_BLOCK_NODE*)(pData);
	_pFreeNode->_next = topNode;

	if (_placementNew)
	{
		(_pFreeNode)->~st_BLOCK_NODE();
	}

	_size += 1;

	return true;
}

template<typename T>
inline void NLMemoryPool<T>::FreeToSub(T* pData)
{
	AcquireSRWLockExclusive(&_subLock);

	st_BLOCK_NODE* topNode = _pSubFreeNode;
	_pSubFreeNode = (st_BLOCK_NODE*)(pData);
	_pSubFreeNode->_next = topNode;

	if (_placementNew)
	{
		(_pSubFreeNode)->~st_BLOCK_NODE();
	}

	_subSize += 1;

	ReleaseSRWLockExclusive(&_subLock);
}

template<typename T>
inline void NLMemoryPool<T>::TakeSubList(NLMemoryPool<T>* objectPool)
{
	AcquireSRWLockExclusive(&_subLock);

	objectPool->_pFreeNode	= _pSubFreeNode;
	objectPool->_size		= _subSize;

	_pSubFreeNode			= nullptr;
	_subSize				= 0;

	ReleaseSRWLockExclusive(&_subLock);
}

template<typename T>
inline void NLMemoryPool<T>::SwapSubList()
{
	AcquireSRWLockExclusive(&_subLock);

	_pFreeNode		= _pSubFreeNode;
	_size			= _subSize;

	_pSubFreeNode	= nullptr;
	_subSize		= 0;

	ReleaseSRWLockExclusive(&_subLock);
}

