#pragma once
/*************************

        CRingBuffer

**************************/
// 1����Ʈ ������ �����͸� �����ϴ� ���� ť

#define	RINGBUFFER_SIZE	 1024 * 5
#define	RINGBUFFER_SIZEMAX  1024 * 10 

class CRingBuffer
{
public:
    CRingBuffer();
    CRingBuffer(int size);
    ~CRingBuffer();

public:
    void Resize(const int size);      // ũ�� ������
    int  GetBufferSize()              // ����ũ�� ��ȯ
    {
        return _bufferSize;
    }

    /////////////////////////////////////////////////////////////////////////
    // ���� ������� �뷮 ���.
    //
    // Parameters: ����.
    // Return: (int)������� �뷮.
    /////////////////////////////////////////////////////////////////////////
    // - �������� -
    // if ((_rear + 1) % _bufferSize == _front) ���� ��, _bufferSize -> _bufferSize - 1�� ����
    int	GetUseSize(void)
    {

        int front = _front;
        int rear = _rear;

        if (front == rear) return 0;
        else if ((rear + 1) % _bufferSize == front)  return _bufferSize - 1;
        else if (front < rear)  return rear - front;
        else return (_bufferSize - front + rear);
    }

    /////////////////////////////////////////////////////////////////////////
    // ���� ���ۿ� ���� �뷮 ���. 
    //
    // Parameters: ����.
    // Return: (int)�����뷮.
    /////////////////////////////////////////////////////////////////////////
    int	GetFreeSize(void)
    {
        return (_bufferSize - 1) - GetUseSize();
    }

    /////////////////////////////////////////////////////////////////////////
    // WritePos �� ����Ÿ ����.
    //
    // Parameters: (char *)����Ÿ ������. (int)ũ��. 
    // Return: (int)���� ũ��.
    /////////////////////////////////////////////////////////////////////////
    int	Enqueue(char* chpData, int iSize);

    /////////////////////////////////////////////////////////////////////////
    // ReadPos ���� ����Ÿ ������. ReadPos �̵�.
    //
    // Parameters: (char *)����Ÿ ������. (int)ũ��.
    // Return: (int)������ ũ��.
    /////////////////////////////////////////////////////////////////////////
    int	Dequeue(char* chpDest, int iSize);

    /////////////////////////////////////////////////////////////////////////
    // ReadPos ���� ����Ÿ �о��. ReadPos ����.
    //
    // Parameters: (char *)����Ÿ ������. (int)ũ��.
    // Return: (int)������ ũ��.
    /////////////////////////////////////////////////////////////////////////
    int	Peek(char* chpDest, int iSize);

    /////////////////////////////////////////////////////////////////////////
    // ������ ��� ����Ÿ ����.
    //
    // Parameters: ����.
    // Return: ����.
    /////////////////////////////////////////////////////////////////////////
    void ClearBuffer(void)
    {
        _front = _rear = 0;
    }

    /////////////////////////////////////////////////////////////////////////
    // ���� �����ͷ� �ܺο��� �ѹ濡 �а�, �� �� �ִ� ����.
    // (������ ���� ����)
    //
    // ���� ť�� ������ ������ ���ܿ� �ִ� �����ʹ� �� -> ó������ ���ư���
    // 2���� �����͸� ��ų� ���� �� ����. �� �κп��� �������� ���� ���̸� �ǹ�
    //
    // Parameters: ����.
    // Return: (int)��밡�� �뷮.
    ////////////////////////////////////////////////////////////////////////
    int	DirectEnqueueSize(void)
    {
        int front = _front;
        int rear = _rear;

        if (front <= rear) return _bufferSize - rear - 1;
        else return front - rear - 1;
    }
    int	DirectDequeueSize(void)
    {
        int front = _front;
        int rear = _rear;

        if (front == rear) return 0;
        else if (front < rear) return rear - front;
        else return _bufferSize - front - 1;
    }

    /////////////////////////////////////////////////////////////////////////
    // ���ϴ� ���̸�ŭ �б���ġ ���� ���� / ���� ��ġ �̵�
    //
    // Parameters: ����.
    // Return: (int)�̵�ũ��
    /////////////////////////////////////////////////////////////////////////
    int	MoveRear(int iSize);
    int	MoveFront(int iSize);

    /////////////////////////////////////////////////////////////////////////
    // ������ Front ������ ����.
    //
    // Parameters: ����.
    // Return: (char *) ���� ������.
    /////////////////////////////////////////////////////////////////////////
    char* GetFrontBufferPtr(void);

    /////////////////////////////////////////////////////////////////////////
    // ������ RearPos ������ ����.
    //
    // Parameters: ����.
    // Return: (char *) ���� ������.
    /////////////////////////////////////////////////////////////////////////
    char* GetRearBufferPtr(void);

    char* GetWritePtr(void);
    char* GetBufferPtr()
    {
        return _buf;
    }

    // ���� �������� ���� �� �ε���
    char* GetFirstIndex()
    {
        return &_buf[(_front + 1) % _bufferSize];
    }

    // ���� �����Ͱ� ���� �ε��� ������ġ
    char* GetNextIndex()
    {
        return &_buf[(_rear + 1) % _bufferSize];
    }
private:
    void Init();

private:
    int _bufferSize;

    char* _buf;
    int _front;       // �� �� ������ �ٷ� �� �ε���
    int _rear;        // �� �� ������ �ε���

    bool deleteFlag = false;
};

