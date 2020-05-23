/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-15
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <filesystem>

class StandardSystemPaths
{
public:

    static StandardSystemPaths & GetInstance()
    {
        if (nullptr == mSingleInstance)
        {
            // Note: no real need to lock
            mSingleInstance = new StandardSystemPaths();
        }

        return *mSingleInstance;
    }

    std::filesystem::path GetUserPicturesSimulatorFolderPath() const;

    std::filesystem::path GetUserSimulatorSettingsRootFolderPath() const;

private:

    StandardSystemPaths()
    {}

    static StandardSystemPaths * mSingleInstance;
};
