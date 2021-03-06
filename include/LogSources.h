// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <boost/thread.hpp>
#include "Win32Lib.h"
#include "LogSource.h"

#pragma comment(lib, "DebugView++Lib.lib")

namespace fusion {
namespace debugviewpp {

struct LogSourceInfo
{
	LogSourceInfo(HANDLE handle, LogSource& logsource);		
	HANDLE handle;
	LogSource& logsource;
};

typedef std::vector<LogSourceInfo> LogSourcesVector;
typedef std::vector<HANDLE> LogSourcesHandles;

class LogSources
{
public:
	LogSources(bool startListening = false);
	~LogSources();

	void Add(std::unique_ptr<LogSource> source);
	void Listen();
	void Abort();
	Lines GetLines();

	std::vector<std::unique_ptr<LogSource>>& GetSources();		//todo: return iterator instead of internal vector

private:
	LogSourcesHandles GetWaitHandles();
	void Process(int index);

	boost::mutex m_mutex;
	std::vector<std::unique_ptr<LogSource>> m_sources;
	Handle m_updateEvent;
	bool m_end;
	bool m_sourcesDirty;
	LogSourcesHandles m_waitHandles;
	// make sure the thread is last to initialize
	boost::thread m_thread;
};

} // namespace debugviewpp 
} // namespace fusion
