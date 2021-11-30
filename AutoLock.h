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

/* 사용하기 ///////////////////////////////////////////////////////////
// class 멤버 변수로 CriticalSection을 등록한다 (생성시에 초기화까지 완료)
class BLABLA
{
private:
	CriticalSection		m_csManager;
}

// 함수가 끝날 때까지 lock을 유지하기 위해 다음과 같이 사용한다.
BLABLA::function()
{
	AUTOLOCK( m_csManager );	// 지역변수로 선언과 동시에 지역변수 _cs 생성과 lock이 작동한다.
	
	...do someting...
}	// 함수를 빠져나오는 순간 지역변수로 선언된 AUTOLOCK은 unlock과 함께 파괴된다.

// 특정 위치까지만 lock을 유지하기 위해서는 중괄호로 감싸준다.
BLABLA::function2()
{
	{
		AUTOLOCK( m_csManager );

		...do someting...
	}	// 괄호를 빠져나온 순간 지역변수로 선언된 AUTOLOCK은 unlock과 함께 파괴된다.

	...do someting...
}

/////////////////////////////////////////////////////////////////////*/


#define SAFE_DELETE(p) {if(p){delete p; p=0;}}