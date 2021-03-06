// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <string>
#include <iostream>
#include <vector>
#include <boost/filesystem.hpp>
#include "Win32Lib.h"
#include "Process_.h"

namespace fusion {

Process::Process(const std::wstring& pathName, const std::vector<std::wstring>& args)
{
	std::wstring commandLine;
	auto it = args.begin();
	while (it != args.end())
	{
		if (it != args.begin())
			commandLine += L" ";
		commandLine += *it;
		++it;
	}

	Run(pathName, commandLine);
}

Process::Process(const std::wstring& pathName, const std::wstring& args)
{
	Run(pathName, args);
}

void Process::Run(const std::wstring& pathName, const std::wstring& args)
{
	m_name = boost::filesystem::wpath(pathName).filename().c_str();

	std::wstring commandLine;
	commandLine += L"\"";
	commandLine += pathName;
	commandLine += L"\"";

	if (!args.empty())
	{
		commandLine += L" ";
		commandLine += args;
	}

	SECURITY_ATTRIBUTES saAttr; 
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
	saAttr.bInheritHandle = true; 
	saAttr.lpSecurityDescriptor = nullptr; 

	HANDLE stdInRd, stdInWr;
	if (!CreatePipe(&stdInRd, &stdInWr, &saAttr, 0))
		ThrowLastError("CreatePipe");
	Handle stdInRd2(stdInRd);
	m_stdIn.reset(stdInWr);
	if (!SetHandleInformation(stdInWr, HANDLE_FLAG_INHERIT, 0))
		ThrowLastError("SetHandleInformation");

	HANDLE stdOutRd, stdOutWr;
	if (!CreatePipe(&stdOutRd, &stdOutWr, &saAttr, 0))
		ThrowLastError("CreatePipe");
	Handle stdOutWr2(stdOutWr);
	m_stdOut.reset(stdOutRd);
	if (!SetHandleInformation(stdOutRd, HANDLE_FLAG_INHERIT, 0))
		ThrowLastError("SetHandleInformation");

	HANDLE stdErrRd, stdErrWr;
	if (!CreatePipe(&stdErrRd, &stdErrWr, &saAttr, 0))
		ThrowLastError("CreatePipe");
	Handle stdErrWr2(stdErrWr);
	m_stdErr.reset(stdErrRd);
	if (!SetHandleInformation(stdErrRd, HANDLE_FLAG_INHERIT, 0))
		ThrowLastError("SetHandleInformation");

	STARTUPINFO startupInfo;
	startupInfo.cb = sizeof(startupInfo);
	startupInfo.lpReserved = nullptr;
	startupInfo.lpDesktop = nullptr;
	startupInfo.lpTitle = nullptr;
	startupInfo.dwX = 0;
	startupInfo.dwY = 0;
	startupInfo.dwXSize = 0;
	startupInfo.dwYSize = 0;
	startupInfo.dwXCountChars = 0;
	startupInfo.dwYCountChars = 0;
	startupInfo.dwFillAttribute = 0;
	startupInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	startupInfo.wShowWindow = SW_HIDE;
	startupInfo.cbReserved2 = 0;
	startupInfo.lpReserved2 = nullptr;
	startupInfo.hStdInput = stdInRd2.get();
	startupInfo.hStdOutput = stdOutWr2.get();
	startupInfo.hStdError = stdErrWr2.get();

	PROCESS_INFORMATION processInformation;

	if (!CreateProcess(
		nullptr,
		const_cast<wchar_t*>(commandLine.c_str()),
		nullptr,
		nullptr,
		true,
		0,
		nullptr,
		nullptr,
		&startupInfo,
		&processInformation))
		ThrowLastError("SetHandleInformation");

	m_hProcess.reset(processInformation.hProcess);
	m_hThread.reset(processInformation.hThread);
	m_processId = processInformation.dwProcessId;
	m_threadId = processInformation.dwThreadId;
}

std::wstring Process::GetName() const
{
	return m_name;
}

HANDLE Process::GetStdIn() const
{
	return m_stdIn.get();
}

HANDLE Process::GetStdOut() const
{
	return m_stdOut.get();
}

HANDLE Process::GetStdErr() const
{
	return m_stdErr.get();
}

HANDLE Process::GetProcessHandle() const
{
	return m_hProcess.get();
}

HANDLE Process::GetThreadHandle() const
{
	return m_hThread.get();
}

unsigned Process::GetProcessId() const
{
	return m_processId;
}

unsigned Process::GetThreadId() const
{
	return m_threadId;
}

bool Process::IsRunning() const
{
	DWORD exitCode;
	if (!GetExitCodeProcess(m_hProcess.get(), &exitCode))
		ThrowLastError("process exit");

	return exitCode == STILL_ACTIVE;
}

void Process::Wait() const
{
	if (WaitForSingleObject(m_hProcess.get(), INFINITE) != WAIT_OBJECT_0)
		ThrowLastError("process exit");
}

} // namespace fusion
