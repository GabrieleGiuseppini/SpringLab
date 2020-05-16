/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-16
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ImageData.h"
#include "ShaderManager.h"
#include "Vectors.h"
#include "ViewModel.h"

#include <functional>

class RenderContext
{
public:

    RenderContext(
        std::function<void()> makeRenderContextCurrentFunction,
        std::function<void()> swapRenderBuffersFunction);

    ////////////////////////////////////////////////////////////////
    // View properties
    ////////////////////////////////////////////////////////////////

    float const & GetZoom() const
    {
        return mViewModel.GetZoom();
    }

    void SetZoom(float zoom)
    {
        mViewModel.SetZoom(zoom);

        OnViewModelUpdated();
    }

    vec2f const & GetCameraWorldPosition() const
    {
        return mViewModel.GetCameraWorldPosition();
    }

    void SetCameraWorldPosition(vec2f const & pos)
    {
        mViewModel.SetCameraWorldPosition(pos);

        OnViewModelUpdated();
    }

    int GetCanvasWidth() const
    {
        return mViewModel.GetCanvasWidth();
    }

    int GetCanvasHeight() const
    {
        return mViewModel.GetCanvasHeight();
    }

    void SetCanvasSize(int width, int height)
    {
        mViewModel.SetCanvasSize(width, height);

        OnCanvasSizeUpdated();
        OnViewModelUpdated();
    }

    float GetVisibleWorldWidth() const
    {
        return mViewModel.GetVisibleWorldWidth();
    }

    float GetVisibleWorldHeight() const
    {
        return mViewModel.GetVisibleWorldHeight();
    }

    float GetVisibleWorldLeft() const
    {
        return mViewModel.GetVisibleWorldTopLeft().x;
    }

    float GetVisibleWorldRight() const
    {
        return mViewModel.GetVisibleWorldBottomRight().x;
    }

    float GetVisibleWorldTop() const
    {
        return mViewModel.GetVisibleWorldTopLeft().y;
    }

    float GetVisibleWorldBottom() const
    {
        return mViewModel.GetVisibleWorldBottomRight().y;
    }

    float CalculateZoomForWorldWidth(float worldWidth) const
    {
        return mViewModel.CalculateZoomForWorldWidth(worldWidth);
    }

    float CalculateZoomForWorldHeight(float worldHeight) const
    {
        return mViewModel.CalculateZoomForWorldHeight(worldHeight);
    }

    inline vec2f ScreenToWorld(vec2f const & screenCoordinates) const
    {
        return mViewModel.ScreenToWorld(screenCoordinates);
    }

    inline vec2f ScreenOffsetToWorldOffset(vec2f const & screenOffset) const
    {
        return mViewModel.ScreenOffsetToWorldOffset(screenOffset);
    }

    ////////////////////////////////////////////////////////////////
    // Interactions
    ////////////////////////////////////////////////////////////////

    RgbImageData TakeScreenshot();

    ////////////////////////////////////////////////////////////////
    // Rendering
    ////////////////////////////////////////////////////////////////

    void RenderStart();

    void UploadPoints(
        size_t pointCount,
        vec2f const * pointPositions,
        vec4f const * pointColors,
        float const * pointNormRadii);

    void RenderEnd();

private:

    void OnCanvasSizeUpdated();
    void OnViewModelUpdated();

private:

    ////////////////////////////////////////////////////////////////
    // Points
    ////////////////////////////////////////////////////////////////

#pragma pack(push)

    struct PointVertex
    {
        vec2f QuadCorner;
        vec2f Center;
        vec4f Color;

        PointVertex(
            vec2f const & quadCorner,
            vec2f const & center,
            vec4f const & color)
            : QuadCorner(quadCorner)
            , Center(center)
            , Color(color)
        {}
    };

#pragma pack(pop)

    size_t mPointVertexCount;

    SLabOpenGLVAO mPointVAO;

    SLabOpenGLMappedBuffer<PointVertex, GL_ARRAY_BUFFER> mPointVertexBuffer;
    SLabOpenGLVBO mPointVertexVBO;

private:

    std::function<void()> const mMakeRenderContextCurrentFunction;
    std::function<void()> const mSwapRenderBuffersFunction;

    std::unique_ptr<ShaderManager> mShaderManager;

    ViewModel mViewModel;
};