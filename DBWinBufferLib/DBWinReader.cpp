// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "DBWinBuffer.h"
#include "DBWinReader.h"
#include "ProcessInfo.h"
#include "LineBuffer.h"

namespace fusion {
namespace debugviewpp {

const double HandleCacheTimeout = 15.0; //seconds

std::wstring GetDBWinName(bool global, const std::wstring& name)
{
	return global ? L"Global\\" + name : name;
}

Handle CreateDBWinBufferMapping(bool global)
{
	Handle hMap(CreateFileMapping(nullptr, nullptr, PAGE_READWRITE, 0, sizeof(DbWinBuffer), GetDBWinName(global, L"DBWIN_BUFFER").c_str()));
	if (GetLastError() == ERROR_ALREADY_EXISTS)
		throw std::runtime_error("CreateDBWinBufferMapping");
	return hMap;
}

DBWinReader::DBWinReader(bool global, bool startlistening) :
	m_autoNewLine(true),
	m_end(false),
	m_hBuffer(CreateDBWinBufferMapping(global)),
	m_dbWinBufferReady(CreateEvent(nullptr, false, true, GetDBWinName(global, L"DBWIN_BUFFER_READY").c_str())),
	m_dbWinDataReady(CreateEvent(nullptr, false, false, GetDBWinName(global, L"DBWIN_DATA_READY").c_str())),
	m_mappedViewOfFile(m_hBuffer.get(), PAGE_READONLY, 0, 0, sizeof(DbWinBuffer)),
	m_dbWinBuffer(static_cast<const DbWinBuffer*>(m_mappedViewOfFile.Ptr())),
	m_handleCacheTime(0.0)
{
	m_lines.reserve(4000);
	m_backBuffer.reserve(4000);
	SetEvent(m_dbWinBufferReady.get());

	if (startlistening)
	{
		m_thread = boost::thread(&DBWinReader::Run, this);
	}
}

DBWinReader::~DBWinReader()
{
	Abort();
}

boost::signals2::connection DBWinReader::Connect(const std::function<OnDBWinMessage>& onDBWinMessage)
{
	return m_onDBWinMessage.connect(onDBWinMessage);
}

bool DBWinReader::AtEnd() const
{
	return m_end;
}

HANDLE DBWinReader::GetHandle() const 
{
	return m_dbWinDataReady.get();
}

void DBWinReader::Notify()
{
	HANDLE handle = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, m_dbWinBuffer->processId);

#ifdef OPENPROCESS_DEBUG
	if (handle == 0)
	{
		Win32Error error(GetLastError(), "OpenProcess");
		std::string s = stringbuilder() << error.what() << " " <<  m_dbWinBuffer->data;
		Add(m_dbWinBuffer->processId, s.c_str(), handle);
		continue;
	}
#endif
	m_onDBWinMessage(m_timer.Get(), GetSystemTimeAsFileTime(), m_dbWinBuffer->processId, handle, m_dbWinBuffer->data);
	SetEvent(m_dbWinBufferReady.get());
}

bool DBWinReader::AutoNewLine() const
{
	return m_autoNewLine;
}

void DBWinReader::AutoNewLine(bool value)
{
	m_autoNewLine = value;
}

void DBWinReader::Abort()
{
	m_end = true;
	SetEvent(m_dbWinDataReady.get());	// will this not interfere with other DBWIN listers? There can be only one DBWIN client..
	m_thread.join();
}

void DBWinReader::Run()
{
	for (;;)
	{
		SetEvent(m_dbWinBufferReady.get());
		WaitForSingleObject(m_dbWinDataReady.get());
		if (m_end)
			break;

		HANDLE handle = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, m_dbWinBuffer->processId);

#ifdef OPENPROCESS_DEBUG
		if (handle == 0)
		{
			Win32Error error(GetLastError(), "OpenProcess");
			std::string s = stringbuilder() << error.what() << " " <<  m_dbWinBuffer->data;
			Add(m_dbWinBuffer->processId, s.c_str(), handle);
			continue;
		}
#endif
		Add(m_dbWinBuffer->processId, m_dbWinBuffer->data, handle);
	}
}

void DBWinReader::Add(DWORD pid, const char* text, HANDLE handle)
{
	DBWinMessage line;
	line.time = m_timer.Get();
	line.systemTime = GetSystemTimeAsFileTime();
	line.pid = pid;
	line.handle = handle;
	line.message = text;
	AddLine(line);
}

void DBWinReader::AddLine(const DBWinMessage& DBWinMessage)
{
	boost::unique_lock<boost::mutex> lock(m_linesMutex);
	m_lines.push_back(DBWinMessage);
}

Lines DBWinReader::GetLines()
{
	m_backBuffer.clear();
	{
		boost::unique_lock<boost::mutex> lock(m_linesMutex);
		m_lines.swap(m_backBuffer);
	}
	return ProcessLines(m_backBuffer);
}

Lines DBWinReader::ProcessLines(const DBWinMessages& DBWinMessages)
{
	Lines resolvedLines = CheckHandleCache();
	for (auto i = DBWinMessages.begin(); i != DBWinMessages.end(); ++i)
	{
		std::string processName; 
		if (i->handle)
		{
			Handle processHandle(i->handle);
			processName = Str(ProcessInfo::GetProcessName(processHandle.get())).str();
			m_handleCache.Add(i->pid, std::move(processHandle));
		}

		auto lines = ProcessLine(Line(i->time, i->systemTime, i->pid, processName, i->message));
		for (auto line = lines.begin(); line != lines.end(); ++line)
			resolvedLines.push_back(*line);
	}

	return resolvedLines;
}

Lines DBWinReader::ProcessLine(const Line& line)
{
	Lines lines;
	if (m_lineBuffers.find(line.pid) == m_lineBuffers.end())
	{
		std::string message;
		message.reserve(4000);
		m_lineBuffers[line.pid] = std::move(message);
	}
	std::string& message = m_lineBuffers[line.pid];

	Line outputLine = line;
	for (auto i = line.message.begin(); i != line.message.end(); i++)
	{
		if (*i == '\r')
			continue;

		if (*i == '\n')
		{
			outputLine.message = std::move(message);
			message.clear();
			lines.push_back(outputLine);
		}
		else
		{
			message.push_back(char(*i));
		}
	}

	if (message.empty())
	{
		m_lineBuffers.erase(line.pid);
	}
	else if (m_autoNewLine || message.size() > 8192)	// 8k line limit prevents stack overflow in handling code 
	{
		outputLine.message = std::move(message);
		message.clear();
		lines.push_back(outputLine);
	}
	return lines;
}

Lines DBWinReader::CheckHandleCache()
{
	if ((m_timer.Get() - m_handleCacheTime) < HandleCacheTimeout)
		return Lines();

	Lines lines;
	Pids removedPids = m_handleCache.Cleanup();
	for (auto i = removedPids.begin(); i != removedPids.end(); i++)
	{
		DWORD pid = *i;
		if (m_lineBuffers.find(pid) != m_lineBuffers.end())
		{
			if (!m_lineBuffers[pid].empty())
				lines.push_back(Line(m_timer.Get(), GetSystemTimeAsFileTime(), pid, "<flush>", m_lineBuffers[pid]));
			m_lineBuffers.erase(pid);
		}
	}
	m_handleCacheTime = m_timer.Get();
	return lines;
}

} // namespace debugviewpp 
} // namespace fusion
