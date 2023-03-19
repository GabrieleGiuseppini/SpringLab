/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2023-03-19
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <algorithm>
#include <cassert>
#include <deque>

/*
 * Model for a very simplistic FIFO cache. 
 *
 * Used to evaluate goodness of element re-ordering strategies.
 */
template<typename TElement, size_t CacheSize>
class CacheModel
{
public:

	// Visits the element; returns true if this was a cache hit
	bool Visit(TElement element)
	{
		assert(mWorkingSet.size() <= CacheSize);

		if (std::find(
			mWorkingSet.cbegin(),
			mWorkingSet.cend(),
			element) == mWorkingSet.cend())
		{
			// Cache miss

			// Cache it
			if (mWorkingSet.size() == CacheSize)
			{
				mWorkingSet.pop_front();
			}
			mWorkingSet.push_back(element);			

			return false;
		}
		else
		{
			// Cache hit
			return true;
		}
	}

	void Reset()
	{
		mWorkingSet.clear();
	}

private:

	std::deque<TElement> mWorkingSet;
};