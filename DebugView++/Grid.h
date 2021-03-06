// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <string>

#pragma warning(push, 3)
#include "PropertyGrid.h"
#pragma warning(pop)
#include "PropertyColorItem.h"

namespace fusion {
namespace debugviewpp {

std::wstring GetGridItemText(const CPropertyGridCtrl& grid, int iItem, int iSubItem);

template <typename ItemType>
ItemType& GetGridItem(const CPropertyGridCtrl& grid, int iItem, int iSubItem)
{
	return dynamic_cast<ItemType&>(*grid.GetProperty(iItem, iSubItem));
}

template <typename Enum>
CPropertyListItem* CreateEnumTypeItem(const wchar_t* name, const Enum* types, size_t count, Enum value)
{
	std::vector<const wchar_t*> items(count + 1);
	int index = 0;
	for (size_t i = 0; i < count; ++i)
	{
		items[i] = EnumToWCharPtr(types[i]);
		if (types[i] == value)
			index = i;
	}
	items[count] = nullptr;
	auto pItem = PropCreateList(name, items.data());
	pItem->SetValue(CComVariant(index));
	return pItem;
}

template <typename Enum, size_t N>
CPropertyListItem* CreateEnumTypeItem(const wchar_t* name, const Enum (&types)[N], Enum value)
{
	return CreateEnumTypeItem<Enum>(name, types, N, value);
}

} // namespace debugviewpp 
} // namespace fusion
