/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-15
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <picojson.h>

#include <cassert>
#include <cstdint>
#include <limits>
#include <sstream>
#include <string>

////////////////////////////////////////////////////////////////////////////////////////////////
// Data Structures
////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * These types define the cardinality of elements in the ElementContainer.
 *
 * Indices are equivalent to pointers in OO terms. Given that we don't believe
 * we'll ever have more than 4 billion elements, a 32-bit integer suffices.
 *
 * This also implies that where we used to store one pointer, we can now store two indices,
 * resulting in even better data locality.
 */
using ElementCount = std::uint32_t;
using ElementIndex = std::uint32_t;
static constexpr ElementIndex NoneElementIndex = std::numeric_limits<ElementIndex>::max();
