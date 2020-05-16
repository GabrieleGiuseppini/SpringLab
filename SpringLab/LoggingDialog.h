/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-15
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <wx/dialog.h>
#include <wx/textctrl.h>

#include <memory>

class slLogMessageEvent : public wxEvent
{
public:

    slLogMessageEvent(
        wxEventType eventType,
        int winid,
        std::string const & message)
        : wxEvent(winid, eventType)
        , mMessage(message)
    {
    }

    slLogMessageEvent(slLogMessageEvent const & other)
        : wxEvent(other)
        , mMessage(other.mMessage)
    {
    }

    virtual wxEvent *Clone() const override
    {
        return new slLogMessageEvent(*this);
    }

    std::string const & GetMessage() const
    {
        return mMessage;
    }

private:
    std::string const mMessage;
};

wxDECLARE_EVENT(slEVT_LOG_MESSAGE, slLogMessageEvent);


class LoggingDialog : public wxDialog
{
public:

	LoggingDialog(wxWindow* parent);

	virtual ~LoggingDialog();

	void Open();

private:

    void OnKeyDown(wxKeyEvent& event);
	void OnClose(wxCloseEvent& event);
    void OnLogMessage(slLogMessageEvent & event);

private:

	wxWindow * const mParent;

	wxTextCtrl * mTextCtrl;

	DECLARE_EVENT_TABLE()
};
