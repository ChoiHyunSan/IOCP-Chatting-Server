#pragma once
#include <Windows.h>
#include <vector>
#include "ObjectPool.h"
#include "TLSMemoryPool.h"

using namespace std;

#define out
#define KEY_BIT 47
#define POOL_NODE_INITCOUNT 0

template<typename T>
class CLockFreeQueue
{
	friend class TLSObjectPool<T>;

public:
	CLockFreeQueue()
	{
		InitializeSRWLock(&lock);

		_nodePool = new MemoryPool<Node>(POOL_NODE_INITCOUNT, false);

		// ���̳�� ����
		Node* dummyNode = _nodePool->Alloc();
		if (dummyNode == nullptr)
		{
			DebugBreak();
			return;
		}

		_head = dummyNode;
		_tail = dummyNode;

		_size = 0;
	}

	~CLockFreeQueue()
	{
		// delete _nodePool;
	}

public:
	bool Enqueue(T& data)
	{
		Node* newNode = _nodePool->Alloc();

		newNode->_data = data;
		newNode->_next = nullptr;

		LONGLONG uniqueBit = (InterlockedIncrement64(&_key) << KEY_BIT);
		Node* newTailNode = reinterpret_cast<Node*>((LONGLONG)newNode | uniqueBit);

		while (true)
		{
			Node* tailNode = _tail;																// ����ŷ �� Tail���
			Node* tailMaskNode = reinterpret_cast<Node*>((LONGLONG)tailNode & _addressMask);	// ����ŷ �� Tail���
			Node* nextNode = tailMaskNode->_next;

			if (nextNode != nullptr)
			{
				// �и� Tail�� �´µ�, �ڿ� ���� �޸��Ÿ� Tail�� �����ϰڴ�.
				InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_tail), nextNode, tailNode);
			}
			else
			{
				if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&tailMaskNode->_next), newTailNode, nextNode) == nextNode)
				{
					InterlockedIncrement(&_size);
					InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_tail), newTailNode, tailNode);
					break;
				}
			}	
		}
		return true;
	}

	bool Dequeue(out T& data)
	{
		while (true)
		{
			Node* headNode = _head;																	// Mask ����� �� head
			Node* headMaskNode = reinterpret_cast<Node*>((LONGLONG)headNode & _addressMask);		// Mask ����� �� head

			Node* nextNode = headMaskNode->_next;													// ���� _head�� �� ��� -> �����͸� ������ ���
			Node* tailNode = _tail;

			if (_size == 0)
			{
				return false;
			}

			if (reinterpret_cast<Node*>(((LONGLONG)nextNode & _addressMask)) == nullptr)
			{
				return false;
			}

			if (tailNode == headNode)
			{
				InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_tail), nextNode, tailNode);
				continue;
			}

			Node* dataNode = reinterpret_cast<Node*>((LONGLONG)nextNode & _addressMask);
			data = dataNode->_data;

			if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_head), nextNode, headNode) == headNode)
			{
				// tlsNodePool.Free(headMaskNode);
				_nodePool->Free(headMaskNode);

				InterlockedDecrement(&_size);				
				break;
			}
		}
		return true;
	}

	int  GetSize()
	{
		return _size;
	}

	int GetNodeAllocCount()
	{
		return _nodePool->GetAllocCount();
	}

	void Clear()
	{

	}

public:
	struct Node
	{
		Node() : _next(nullptr) {}

		T		_data;
		Node*	_next;
	};

	Node*				_head;
	Node*				_tail;
	MemoryPool<Node>*	_nodePool;
	
private:
	static TLSObjectPool<CLockFreeQueue<T>::Node> tlsNodePool;

private:
	LONGLONG			_key = 0;
	LONGLONG			_addressMask = 0x7FFFFFFFFFFF;

private:
	LONG _size;

	SRWLOCK lock;
};

template<typename T>
TLSObjectPool<typename CLockFreeQueue<T>::Node> CLockFreeQueue<T>::tlsNodePool;