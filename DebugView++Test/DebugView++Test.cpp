// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"

#define BOOST_TEST_MODULE DebugView++Lib Unit Test
#include <boost/test/unit_test_gui.hpp>
#include <random>

#include "Utilities.h"
#include "ProcessInfo.h"
#include "IndexedStorage.h"
#include "Win32Lib.h"
#include "DBWinBuffer.h"

namespace fusion {
namespace debugviewpp {

BOOST_AUTO_TEST_SUITE(DebugViewPlusPlusLib)

std::string GetTestString(int i)
{
	return stringbuilder() << "BB_TEST_ABCDEFGHI_EE_" << i;
}

BOOST_AUTO_TEST_CASE(IndexedStorageRandomAccess)
{
	using namespace indexedstorage;

	size_t testSize = 10000;
	auto testMax = testSize - 1;

	std::default_random_engine generator;
	std::uniform_int_distribution<int> distribution(0, testMax);
	SnappyStorage s;
	for (size_t i = 0; i < testSize; ++i)
		s.Add(GetTestString(i));

	for (size_t i = 0; i < testSize; ++i)
	{
		size_t j = distribution(generator);  // generates number in the range 0..testMax 
		BOOST_REQUIRE_EQUAL(s[j], GetTestString(j));
	}
}

BOOST_AUTO_TEST_CASE(IndexedStorageCompression)
{
	using namespace indexedstorage;

	size_t testSize = 10000;
	auto testMax = testSize - 1;

	std::default_random_engine generator;
	std::uniform_int_distribution<int> distribution(0, testMax);
	SnappyStorage s;
	VectorStorage v;

	size_t m0 = ProcessInfo::GetPrivateBytes();

	for (size_t i = 0; i < testSize; ++i)
		v.Add(GetTestString(i));

	size_t m1 = ProcessInfo::GetPrivateBytes();
	size_t required1 = m1 - m0;
	BOOST_MESSAGE("VectorStorage requires: " << required1/1024 << " bK");

	for (size_t i = 0; i < 100000; ++i)
		s.Add(GetTestString(i));

	size_t m2 = ProcessInfo::GetPrivateBytes();
	size_t required2 = m2 - m1;
	BOOST_MESSAGE("SnappyStorage requires: " << required2/1024 << " kB (" << (100*required2)/required1 << "%)");
	BOOST_REQUIRE_GT(0.50*required1, required2);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace debugviewpp 
} // namespace fusion
