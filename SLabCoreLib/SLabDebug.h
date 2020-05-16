/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-15
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "SLabException.h"

#ifdef _DEBUG

inline void Verify(bool expression)
{
    if (!expression)
    {
        throw SLabException("Verification failed!");
    }
}

#endif