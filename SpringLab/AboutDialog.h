/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-15
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <wx/dialog.h>

class AboutDialog : public wxDialog
{
public:

    AboutDialog(wxWindow* parent);

	virtual ~AboutDialog();

    void Open();

private:

	wxWindow * const mParent;
};
