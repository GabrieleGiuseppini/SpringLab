/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-15
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <SLabCoreLib/CircularList.h>

#include <wx/wx.h>

#include <memory>

class ScalarTimeSeriesProbeControl : public wxPanel
{
public:

    ScalarTimeSeriesProbeControl(
        wxWindow * parent,
        int width,
        int height);

    virtual ~ScalarTimeSeriesProbeControl();

    void RegisterSample(float value);

    void UpdateSimulation();

    void Reset();

private:

    void OnMouseClick(wxMouseEvent & event);
    void OnPaint(wxPaintEvent & event);
    void OnEraseBackground(wxPaintEvent & event);

    void Render(wxDC& dc);

    inline int MapValueToY(float value) const;

private:

    int const mWidth;
    int const mHeight;

    std::unique_ptr<wxBitmap> mBufferedDCBitmap;
    wxPen const mTimeSeriesPen;
    wxPen mGridPen;

    float mMaxValue;
    float mMinValue;
    float mGridValueSize;

    CircularList<float, 200> mSamples;
};
