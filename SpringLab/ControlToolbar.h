/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-21
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <wx/panel.h>

#include <memory>
#include <string>
#include <unordered_map>

class ControlToolbar final : public wxPanel
{
public:

    static long const ID_SIMULATION_CONTROL_PLAY;
    static long const ID_SIMULATION_CONTROL_FAST_PLAY;
    static long const ID_SIMULATION_CONTROL_PAUSE;
    static long const ID_SIMULATION_CONTROL_STEP;

public:

    ControlToolbar(wxWindow* parent);

    virtual ~ControlToolbar();


private:


private:


};
