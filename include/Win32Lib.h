// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <string>
#include <boost/utility.hpp>
#include <boost/system/system_error.hpp>
#include <windows.h>
#include <memory>
#include <vector>

#pragma comment(lib, "Win32Lib.lib")

namespace fusion {

struct GlobalAllocDeleter
{
	typedef HGLOBAL pointer;

	void operator()(pointer p) const;
};

typedef std::unique_ptr<void, GlobalAllocDeleter> HGlobal;

struct HandleDeleter
{
	typedef HANDLE pointer;

	void operator()(pointer p) const;
};

typedef std::unique_ptr<void, HandleDeleter> Handle;

struct ChangeNotificationHandleDeleter
{
	typedef HANDLE pointer;

	void operator()(pointer p) const;
};

typedef std::unique_ptr<void, ChangeNotificationHandleDeleter> ChangeNotificationHandle;

template <typename T>
struct GdiObjectDeleter
{
	typedef T pointer;

	void operator()(pointer p) const
	{
		if (p != nullptr)
			DeleteObject(p);
	}
};

template <>
struct GdiObjectDeleter<HICON>
{
	typedef HICON pointer;

	void operator()(pointer p) const
	{
		if (p != nullptr)
			DestroyIcon(p);
	}
};

typedef std::unique_ptr<std::remove_pointer<HGDIOBJ>::type, GdiObjectDeleter<HGDIOBJ>> GdiObject;
typedef std::unique_ptr<std::remove_pointer<HPEN>::type, GdiObjectDeleter<HPEN>> HPen;
typedef std::unique_ptr<std::remove_pointer<HBRUSH>::type, GdiObjectDeleter<HBRUSH>> HBrush;
typedef std::unique_ptr<std::remove_pointer<HFONT>::type, GdiObjectDeleter<HFONT>> HFont;
typedef std::unique_ptr<std::remove_pointer<HBITMAP>::type, GdiObjectDeleter<HBITMAP>> HBitmap;
typedef std::unique_ptr<std::remove_pointer<HRGN>::type, GdiObjectDeleter<HRGN>> HRegion;
typedef std::unique_ptr<std::remove_pointer<HPALETTE>::type, GdiObjectDeleter<HPALETTE>> HPalette;
typedef std::unique_ptr<std::remove_pointer<HICON>::type, GdiObjectDeleter<HICON>> HIcon;

template <typename T>
class GlobalLock
{
public:
	explicit GlobalLock(const HGlobal& hg) :
		m_hg(hg.get()),
		m_ptr(::GlobalLock(m_hg))
	{
	}

	~GlobalLock()
	{
		::GlobalUnlock(m_hg);
	}

	T* Ptr() const
	{
		return static_cast<T*>(m_ptr);
	}

private:
	HGLOBAL m_hg;
	void* m_ptr;
};

class GdiObjectSelection
{
public:
	GdiObjectSelection(HDC hdc, HGDIOBJ hObject);
	~GdiObjectSelection();

private:
	HDC m_hdc;
	HGDIOBJ m_hObject;
};

class Win32Error : public boost::system::system_error
{
public:
	Win32Error(DWORD error, const std::string& what);
};

std::wstring MultiByteToWideChar(const char* str, int len);
std::wstring MultiByteToWideChar(const char* str);
std::wstring MultiByteToWideChar(const std::string& str);

std::string WideCharToMultiByte(const wchar_t* str, int len);
std::string WideCharToMultiByte(const wchar_t* str);
std::string WideCharToMultiByte(const std::wstring& str);

void ThrowWin32Error(DWORD error, const std::string& what);
void ThrowLastError(const std::string& what);
void ThrowLastError(const std::wstring& what);

SYSTEMTIME GetSystemTime();
SYSTEMTIME GetLocalTime();
FILETIME GetSystemTimeAsFileTime();
FILETIME FileTimeToLocalFileTime(const FILETIME& ft);
SYSTEMTIME FileTimeToSystemTime(const FILETIME& ft);
FILETIME SystemTimeToFileTime(const SYSTEMTIME& st);

Handle CreateFileMapping(HANDLE hFile, const SECURITY_ATTRIBUTES* pAttributes, DWORD protect, DWORD maximumSizeHigh, DWORD maximumSizeLow, const wchar_t* pName);
Handle OpenProcess(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId);
Handle CreateEvent(const SECURITY_ATTRIBUTES* pEventAttributes, bool manualReset, bool initialState, const wchar_t* pName);
Handle CreateMutex(const SECURITY_ATTRIBUTES* pMutexAttributes, bool initialOwner, const wchar_t* pName);
void SetPrivilege(const wchar_t* privilege, bool enablePrivilege);
void SetPrivilege(HANDLE hToken, const wchar_t* privilege, bool enablePrivilege);

struct WaitResult
{
	WaitResult(bool signaled = false, int index = 0);
	bool signaled;
	int index;
};

void WaitForSingleObject(HANDLE hObject);
bool WaitForSingleObject(HANDLE hObject, DWORD milliSeconds);
WaitResult WaitForAnyObject(const std::vector<HANDLE>& handles, DWORD milliSeconds);

class MutexLock : boost::noncopyable
{
public:
	explicit MutexLock(HANDLE hMutex);
	~MutexLock();

	void Release();

private:
	HANDLE m_hMutex;
};

class MappedViewOfFile : boost::noncopyable
{
public:
	MappedViewOfFile(HANDLE hFileMappingObject, DWORD access, DWORD offsetHigh, DWORD offsetLow, size_t bytesToMap);
	~MappedViewOfFile();

	void* Ptr();
	const void* Ptr() const;

private:
	void* m_ptr;
};


std::wstring RegGetStringValue(HKEY hKey, const wchar_t* valueName = nullptr);
std::wstring RegGetStringValue(HKEY hKey, const wchar_t* valueName, const wchar_t* defaultValue);

DWORD RegGetDWORDValue(HKEY hKey, const wchar_t* valueName = nullptr);
DWORD RegGetDWORDValue(HKEY hKey, const wchar_t* valueName, DWORD defaultValue);

ULONG_PTR GetParentProcessId();


} // namespace fusion
