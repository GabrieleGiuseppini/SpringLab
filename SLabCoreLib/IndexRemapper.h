/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2023-03-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "SLabTypes.h"

#include <cassert>
#include <vector>

class IndexRemapper final
{
public:

	explicit IndexRemapper(size_t elementCount)
		: mOldToNew(elementCount, NoneElementIndex)
	{
		mNewToOld.reserve(elementCount);
	}

	static IndexRemapper MakeIdempotent(size_t elementCount)
	{
		IndexRemapper remapper(elementCount);
		for (size_t i = 0; i < elementCount; ++i)
		{
			remapper.mNewToOld.emplace_back(static_cast<ElementIndex>(i));
			remapper.mOldToNew[i] = static_cast<ElementIndex>(i);
		}

		return remapper;
	}

	auto const & GetOldIndices() const
	{
		return mNewToOld;
	}

	/*
	 * Adds a oldIndex -> <current size> mapping.
	 */
	void AddOld(ElementIndex oldIndex)
	{
		ElementIndex const newIndex = static_cast<ElementIndex>(mNewToOld.size());
		mNewToOld.emplace_back(oldIndex);
		mOldToNew[oldIndex] = newIndex;
	}

	ElementIndex OldToNew(ElementIndex oldIndex) const
	{
		assert(mOldToNew[oldIndex] != NoneElementIndex);
		return mOldToNew[oldIndex];
	}

	ElementIndex NewToOld(ElementIndex newIndex) const
	{
		assert(newIndex < mNewToOld.size());
		return mNewToOld[newIndex];
	}

private:

	std::vector<ElementIndex> mNewToOld;
	std::vector<ElementIndex> mOldToNew;	
};