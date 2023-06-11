/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2023-06-11
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "SyntheticObjectDialog.h"

#include <SLabCoreLib/Utils.h>

#include <wx/gbsizer.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

static int constexpr CellBorder = 8;

SyntheticObjectDialog::SyntheticObjectDialog(
    wxWindow * parent)
    : mParent(parent)
{
    Create(
        parent,
        wxID_ANY,
        _("New Object"),
        wxDefaultPosition,
        wxSize(250, 200),
        wxCAPTION | wxFRAME_SHAPED);

    SetBackgroundColour(GetDefaultAttributes().colBg);

    //
    // Lay out controls
    //

    wxBoxSizer * dialogVSizer = new wxBoxSizer(wxVERTICAL);

    dialogVSizer->AddSpacer(20);

    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    // Num springs
    {
        {
            auto label = new wxStaticText(this, wxID_ANY, _("Number of Springs:"), wxDefaultPosition, wxDefaultSize,
                wxALIGN_LEFT);

            gridSizer->Add(
                label,
                wxGBPosition(0, 0),
                wxGBSpan(1, 1),
                wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL,
                CellBorder);
        }

        {
            mNumSpringsTextCtrlValidator = std::make_unique<wxIntegerValidator<size_t>>();
            mNumSpringsTextCtrlValidator->SetRange(1, 1000000);

            mNumSpringsTextCtrl = new wxTextCtrl(
                this,
                wxID_ANY,
                "1000",
                wxDefaultPosition,
                wxDefaultSize,
                wxTE_RIGHT | wxTE_PROCESS_ENTER,
                *mNumSpringsTextCtrlValidator);

            mNumSpringsTextCtrl->Bind(
                wxEVT_TEXT_ENTER,
                [this](wxCommandEvent &)
                {
                    mNumSpringsTextCtrl->Navigate();
                });

            gridSizer->Add(
                mNumSpringsTextCtrl,
                wxGBPosition(0, 1),
                wxGBSpan(1, 1),
                wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL,
                CellBorder);
        }
    }

    dialogVSizer->Add(
        gridSizer,
        0, 
        wxALIGN_CENTER_HORIZONTAL, 
        0);

    dialogVSizer->AddSpacer(20);

    // Buttons
    {
        wxBoxSizer * buttonsSizer = new wxBoxSizer(wxHORIZONTAL);

        buttonsSizer->AddSpacer(20);

        {
            mOkButton = new wxButton(this, wxID_OK, _("OK"));
            buttonsSizer->Add(mOkButton, 0);
        }

        buttonsSizer->AddSpacer(20);

        {
            auto button = new wxButton(this, wxID_CANCEL, _("Cancel"));
            buttonsSizer->Add(button, 0);
        }

        buttonsSizer->AddSpacer(20);

        dialogVSizer->Add(buttonsSizer, 0, wxALIGN_CENTER_HORIZONTAL);
    }

    dialogVSizer->AddSpacer(20);

    //
    // Finalize dialog
    //

    SetSizerAndFit(dialogVSizer);

    Centre(wxCENTER_ON_SCREEN | wxBOTH);
}

std::optional<size_t> SyntheticObjectDialog::AskNumSprings()
{
    int const result = wxDialog::ShowModal();
    if (result == wxID_OK)
    {
        std::string const strValue = mNumSpringsTextCtrl->GetValue().ToStdString();

        size_t value;
        if (Utils::LexicalCast<size_t>(strValue, &value))
        {
            return value;
        }
    }

    return std::nullopt;
}

