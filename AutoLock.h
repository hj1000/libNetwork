#pragma once

class NonCopyable
{
private:
	NonCopyable(const NonCopyable&);
	const NonCopyable& operator=(const NonCopyable&);
public:
	NonCopyable(){}
};

class SyncObj : public NonCopyable
{
public:
	virtual bool Lock() = 0;
	virtual bool Unlock() = 0;
};

class CriticalSection : public SyncObj
{
private:
	CRITICAL_SECTION _cs;

public:
	CriticalSection()
	{
		InitializeCriticalSectionAndSpinCount(&_cs, 4000);
	}
	explicit CriticalSection(unsigned int spincount)
	{
		InitializeCriticalSectionAndSpinCount(&_cs, spincount);
	}
	virtual ~CriticalSection()
	{
		DeleteCriticalSection(&_cs);
	}

	bool Lock()		{ __try{EnterCriticalSection(&_cs);}__except(1){return false;}return true; }
	bool Unlock()	{ __try{LeaveCriticalSection(&_cs);}__except(1){return false;}return true; }
	bool TryEnter()	{ return TryEnterCriticalSection(&_cs) != FALSE; }
};

template<typename T>
class AutoLock : public NonCopyable
{
private:
	T& _lockobj;
	bool _locked;

public:
	explicit AutoLock(T* lockobj, bool initLock = true)	: _lockobj(*lockobj), _locked(false)
	{
		if(initLock)	Lock();
	}
	explicit AutoLock(T& lockobj, bool initLock = true)	: _lockobj(lockobj), _locked(false)
	{
		if(initLock)	Lock();
	}

	~AutoLock()
	{
		if(_locked)	Unlock();
	}

	void Lock()
	{
		if(!_locked)
		{
			_lockobj.Lock();
			_locked = true;
		}						
	}

	void Unlock()
	{
		if(_locked)
		{
			_locked = false;
			_lockobj.Unlock();
		}
	}
};

#define AUTOLOCK(CS)		AutoLock<CriticalSection> _cs(CS);

/* ����ϱ� ///////////////////////////////////////////////////////////
// class ��� ������ CriticalSection�� ����Ѵ� (�����ÿ� �ʱ�ȭ���� �Ϸ�)
class BLABLA
{
private:
	CriticalSection		m_csManager;
}

// �Լ��� ���� ������ lock�� �����ϱ� ���� ������ ���� ����Ѵ�.
BLABLA::function()
{
	AUTOLOCK( m_csManager );	// ���������� ����� ���ÿ� �������� _cs ������ lock�� �۵��Ѵ�.
	
	...do someting...
}	// �Լ��� ���������� ���� ���������� ����� AUTOLOCK�� unlock�� �Բ� �ı��ȴ�.

// Ư�� ��ġ������ lock�� �����ϱ� ���ؼ��� �߰�ȣ�� �����ش�.
BLABLA::function2()
{
	{
		AUTOLOCK( m_csManager );

		...do someting...
	}	// ��ȣ�� �������� ���� ���������� ����� AUTOLOCK�� unlock�� �Բ� �ı��ȴ�.

	...do someting...
}

/////////////////////////////////////////////////////////////////////*/


#define SAFE_DELETE(p) {if(p){delete p; p=0;}}