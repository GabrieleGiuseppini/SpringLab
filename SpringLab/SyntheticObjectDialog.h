/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2023-06-11
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/valnum.h>

#include <memory>
#include <optional>

class SyntheticObjectDialog : public wxDialog
{
public:

    SyntheticObjectDialog(
        wxWindow * parent);

    std::optional<size_t> AskNumSprings();

private:

    void OnDirty();

private:

    wxWindow * const mParent;

    wxTextCtrl * mNumSpringsTextCtrl;
    std::unique_ptr<wxIntegerValidator<size_t>> mNumSpringsTextCtrlValidator;
    wxButton * mOkButton;    
};
