/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2023-03-19
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "SLabTypes.h"

#include <algorithm>
#include <cassert>
#include <deque>

/*
 * Model for a very simplistic FIFO cache. 
 * The cache is comprised of a number of lines, each holding B bytes.
 * We assume the content of a cache line is memory-aligned to the cache line size.
 *
 * Used to evaluate goodness of element re-ordering strategies.
 */
template<size_t NLines, size_t BLine, typename TElement>
class CacheModel
{

	static ElementCount constexpr LineElementCount = BLine / sizeof(TElement);

public:

	// Visits the element; returns true if this was a cache hit
	bool Visit(ElementIndex elementIndex)
	{
		assert(mLines.size() <= NLines);

		for (ElementIndex lineStartElementIndex : mLines)
		{
			if (lineStartElementIndex <= elementIndex && elementIndex < lineStartElementIndex + LineElementCount)
			{
				// Cache hit
				return true;
			}
		}

		// Cache miss

		// Cache it

		if (mLines.size() == NLines)
		{
			mLines.pop_front();
		}

		mLines.push_back(elementIndex - (elementIndex % LineElementCount));

		return false;
	}

	void Reset()
	{
		mLines.clear();
	}

private:

	// A FIFO list of lines; each line indicates the index of the first element
	// in it
	std::deque<ElementIndex> mLines;
};