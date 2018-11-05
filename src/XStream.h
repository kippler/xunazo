////////////////////////////////////////////////////////////////////////////////////////////////////
/// 
/// 파일/메모리 를 공통으로 읽기/쓰기 해주는 스트림 클래스. 파일 I/O 시 버퍼링 처리도 해준다.
/// 
/// @author   park
/// @date     10/08/15 17:30:07
/// 
/// Copyright(C) 2015 Bandisoft, All rights reserved.
/// 
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef ASSERT
# include <atlbase.h>
# define ASSERT(x)  ATLASSERT(x)
#endif 

#ifndef PURE
# define PURE = 0
#endif

#if (_MSC_VER == 1500 )		// 1500 == vs2008
#	define final			
#	define noexcept
#endif

#define EOF    (-1)
class XBuffer;

#pragma warning(push)
#pragma warning(disable: 26408)		// Avoid malloc() and free(), prefer the nothrow version of new with delete (r.10).



////////////////////////////////////////////////////////////////////////////////////////////////////
///         읽기 스트림 인터페이스
/// @param  
/// @return 
/// @date   Thursday, October 08, 2015  5:31:15 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
struct XReadStream
{
	virtual ~XReadStream() {};
	virtual BOOL	Read(LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead) PURE;
	virtual INT64	SetPos(INT64 pos) PURE;
	virtual INT64	GetPos() const PURE;
	virtual INT64	GetSize() PURE;

	/////////////////////////////////////////////////////////////////
	//
	// 유틸 함수
	//

	// 원하는 만큼 다 읽으면 TRUE
	BOOL	Read_(LPVOID buf, DWORD len)
	{
		DWORD read;
		if(Read(buf, len, &read) && read==len) return TRUE;
		return FALSE;
	}
	// method 를 가지고 이동하기
	INT64	SetPos(INT64 pos, DWORD method)
	{
		INT64 newPos=0;
		if(method==FILE_BEGIN) newPos = pos;
		else if(method==FILE_CURRENT) newPos = GetPos() + pos;
		else if(method==FILE_END) newPos = GetSize() + pos;
		else {ASSERT(0); return -1;}
		return SetPos(newPos);
	}
	BOOL	IsEOF()
	{
		return GetPos()>=GetSize() ? TRUE : FALSE;
	}
	long GetC()
	{
		BYTE b;
		return Read_(&b, sizeof(BYTE))==FALSE ? EOF : b;
	}
	BOOL	ReadBYTE(BYTE& val)
	{
		return Read_(&val, sizeof(BYTE));
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////
///         쓰기 스트림 인터페이스
/// @param  
/// @return 
/// @date   Thursday, October 15, 2015  2:10:23 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
struct XWriteStream
{
	virtual ~XWriteStream() {};
	virtual BOOL	Write(LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite) PURE;
	virtual INT64	SetPos(INT64 pos) PURE;
	virtual INT64	GetPos() const PURE;
	virtual INT64	GetSize() PURE;

	/////////////////////////////////////////////////////////////////
	//
	// 유틸 함수
	//
	// method 를 가지고 이동하기
	INT64	SetPos(INT64 pos, DWORD method)
	{
		INT64 newPos=0;
		if(method==FILE_BEGIN) newPos = pos;
		else if(method==FILE_CURRENT) newPos = GetPos() + pos;
		else if(method==FILE_END) newPos = GetSize() + pos;
		else {ASSERT(0); return -1;}
		return SetPos(newPos);
	}
};


////////////////////////////////////////////////////////////////////////////////////////////////////
///         내부 버퍼링을 지원하는 파일 읽기 클래스
/// @param  
/// @return 
/// @date   Thursday, October 08, 2015  5:33:54 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
class XFileReadStream : public XReadStream
{
public :
	XFileReadStream();
	~XFileReadStream() final;
	BOOL	Read(LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead) final;
	INT64	SetPos(INT64 pos) final;
	INT64	GetPos() const final;
	INT64	GetSize() final ;

public :
	BOOL	Open(LPCWSTR filePathName);	// 파일 직접 열기
	void	Close();
	BOOL	ReadWhole(XBuffer& buffer, BOOL addNull=FALSE);	// 전체 다 읽어서 버퍼에 넣기
	BOOL	Attach(HANDLE hFile, BOOL autoCloseHandle=FALSE);

private :
	void	InitMembers();

public :
	enum { LEN_BUF = 8192 };
	HANDLE	m_handle;
	BYTE*	m_buf;
	BYTE*	m_left;					// m_buf 내 현재 데이타 시작 위치.
	BYTE*	m_right;				// m_buf 내 유효한 데이타 끝 위치-1

	INT64	m_virtPos;				// 가상의 파일 포인터
	INT64	m_size;

	BOOL	m_bAutoCloseHandle;		// 종료시 자동으로 핸들 닫을까 말까.
};




////////////////////////////////////////////////////////////////////////////////////////////////////
///         메모리 버퍼
/// @param  
/// @return 
/// @date   Thursday, October 08, 2015  5:33:54 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
class XMemoryReadStream : public XReadStream
{
public :
	XMemoryReadStream();
	~XMemoryReadStream() final;
	BOOL	Read(LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead) final;
	INT64	SetPos(INT64 pos) final;
	INT64	GetPos() const final;
	INT64	GetSize()final;

public :
	BOOL	Alloc(int size);
	BOOL	OpenFile(LPCWSTR filePathName);
	void	Free() noexcept;
	void	Attach(BYTE* buf, int size, BOOL bSelfFree=FALSE);

public :
	BYTE*	m_buf;			// 메모리 버퍼
	int		m_size;			// 버퍼 alloc 크기
	int		m_pos;			// 현재 읽는 위치
	BOOL	m_bSelfFree;	// 종료시 자동으로 메모리 해제할까 여부
};



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// 
/// 
/// WRITE STREAM
/// 
/// 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
///         버퍼링 처리하는 파일 쓰기 클래스
/// @param  
/// @return 
/// @date   Thursday, October 15, 2015  6:36:13 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
class XFileWriteStream : public XWriteStream
{
public :
	XFileWriteStream();
	~XFileWriteStream() final;
	BOOL	Write(LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite) final;
	INT64	SetPos(INT64 pos) final;
	INT64	GetPos() const final;
	INT64	GetSize()final;

public :
	BOOL	Open(LPCWSTR filePathName);
	void	Attach(HANDLE hFile, BOOL bAutoCloseHandle=FALSE);
	void	Close() noexcept;
	HANDLE	GetHandle() { return m_handle; }
	LPCWSTR GetFilePathName() { return m_filePathName; }
	BOOL	IsOpened() {return m_handle == INVALID_HANDLE_VALUE ? FALSE : TRUE;}

private :
	BOOL	FlushBuffer() noexcept;

private :
	enum { LEN_BUF = 8192 };
	HANDLE	m_handle;
	BYTE*	m_buf;			// 쓰기 버퍼
	DWORD	m_bufpos;				// 쓰기 버퍼에서 사용중인 크기
	INT64	m_virtPos;				// 파일 포인터의 가상 위치 (현재 위치 + m_bufpos)
	BOOL	m_bAutoCloseHandle;		// 종료시 자동으로 핸들 닫을까 말까.
	LPCWSTR	m_filePathName;			// 파일명 기억하기

};




////////////////////////////////////////////////////////////////////////////////////////////////////
///         메모리로 구현한 쓰기 스트림 클래스
/// @param  
/// @return 
/// @date   Thursday, October 15, 2015  6:41:45 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
class XMemoryWriteStream : public XWriteStream
{
public :
	XMemoryWriteStream();
	~XMemoryWriteStream() final;
	BOOL	Write(LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite) final;
	INT64	SetPos(INT64 pos) final;
	INT64	GetPos() const final;
	INT64	GetSize() final;

public :
	BOOL	Alloc(int size);			// 미리 쓸 크기를 아는 경우 사용한다.
	void	Free() noexcept;
	void	SetMaxAlloc(int size) { m_maxAllocSize = size; }
	BYTE*	GetBuffer() { return m_buf; }
	void	Attach(BYTE* buf, int len);

private :
	BOOL	IncreaseBuffer(int targetSize);

private :
	enum	{ INITIAL_ALLOC_SIZE = 4096	};		// 초기 alloc 크기
	enum	{ MAX_ALLOC_SIZE = 1024*1024*100 };	// 메모리 alloc 최대 크기 - 일단 100MB

public :
	BYTE*	m_buf;			// 메모리 버퍼
	int		m_allocSize;	// 버퍼 alloc 크기
	int		m_size;			// 버퍼에서 사용중인 크기; 파일 크기에 해당된다.
	int		m_pos;			// 현재 위치 ; m_size 보다 커질 수 있다.
	BOOL	m_bSelfFree;	// 종료시 자동으로 메모리 해제할까 여부
	int		m_maxAllocSize;	// 최대 alloc 할 크기
};




////////////////////////////////////////////////////////////////////////////////////////////////////
///         dummy output stream - 출력 파일의 크기를 계산만 하고자 할때 사용한다.
///			pos 와 size 를 구분하지 않으니 주의 필요.
/// @param  
/// @return 
/// @date   Friday, October 16, 2015  4:17:46 PM
////////////////////////////////////////////////////////////////////////////////////////////////////
#define _max(a,b) (((a) > (b)) ? (a) : (b))
class XDummyWriteStream : public XWriteStream
{
public :
	XDummyWriteStream() {m_size = 0; m_pos = 0;}
	~XDummyWriteStream() final {} 
	BOOL	Write(LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite) final { UNREFERENCED_PARAMETER(lpBuffer);  m_pos += nNumberOfBytesToWrite; m_size = _max(m_pos, m_size); return TRUE; }
	INT64	SetPos(INT64 pos) final { m_pos = pos; return m_pos ; }
	INT64	GetPos() const final { return m_pos; }
	INT64	GetSize() final { return m_size; }

	INT64 m_size;
	INT64 m_pos;

};



/////////////////////////////////////////////////////////////////////////////////
//
// XBUFFER - 간단하게 바이트 버퍼를 주고 받을 때 사용한다.
//
// XAutoByte 와 조금 다른점이라고 하면, size 와 alloc 사이즈가 따로 있다는점?
//



class XBuffer
{
public:
	XBuffer()
	{
		data = NULL;
		dataSize = allocSize = 0;
	}
	XBuffer(int size)
	{
		data = NULL;
		dataSize = allocSize = 0;
		Alloc(size);
	}
	~XBuffer() 
	{ 
		if (data) 
			free(data); 
	}
	BOOL	Alloc(int count)
	{
		if (data && allocSize == count) return TRUE;	// 동일하다. 할꺼 없다.
		if (data) free(data);
		data = (BYTE*)malloc(count*sizeof(BYTE));
		allocSize = count;
		dataSize = 0;
		return data ? TRUE : FALSE;
	}
	BOOL	Calloc(int count)
	{
		if (data) free(data);
		data = (BYTE*)calloc(count, sizeof(BYTE));
		allocSize = count;
		dataSize = 0;
		return data ? TRUE : FALSE;
	}
	BOOL	Write(const BYTE* _data, int _size)
	{
		if (Alloc(_size) == FALSE) return FALSE;
		memcpy(data, _data, _size);
		dataSize = _size;
		return TRUE;
	}
	operator BYTE*() { return data; }

public:
	BYTE*	data;
	int		dataSize;		// 버퍼의 유효한 데이타 크기 - 서로 크기를 주고받을때 사용한다.
	int		allocSize;		// 버퍼의 alloc된 크기
};



#pragma warning(pop)
