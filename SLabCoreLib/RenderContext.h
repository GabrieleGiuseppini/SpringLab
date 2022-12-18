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

#include <optional>
#include <vector>

class RenderContext
{
public:

    RenderContext(
        int canvasWidth,
        int canvasHeight);

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
        mIsViewModelDirty = true;
    }

    vec2f const & GetCameraWorldPosition() const
    {
        return mViewModel.GetCameraWorldPosition();
    }

    void SetCameraWorldPosition(vec2f const & pos)
    {
        mViewModel.SetCameraWorldPosition(pos);
        mIsViewModelDirty = true;
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
        mIsViewModelDirty = true;
        mIsCanvasSizeDirty = true;
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

    inline vec2f WorldToScreen(vec2f const & worldCoordinates) const
    {
        return mViewModel.WorldToScreen(worldCoordinates);
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
        float const * pointNormRadii,
        float const * pointHighlights,
        float const * pointFrozenCoefficients);

    void UploadSpringsStart(size_t springCount);

    void UploadSpring(
        vec2f const & springEndpointAPosition,
        vec2f const & springEndpointBPosition,
        vec4f const & springColor,
        float springNormThickness,
        float springHighlight);

    void UploadSpringsEnd();

    void RenderEnd();

private:

    std::unique_ptr<ShaderManager> mShaderManager;

    ViewModel mViewModel;

private:

    ////////////////////////////////////////////////////////////////
    // Settings
    ////////////////////////////////////////////////////////////////

    void ProcessSettingChanges();

    bool mIsCanvasSizeDirty;
    void OnCanvasSizeUpdated();

    bool mIsViewModelDirty;
    void OnViewModelUpdated();

private:

    ////////////////////////////////////////////////////////////////
    // Points
    ////////////////////////////////////////////////////////////////

#pragma pack(push)

    struct PointVertex
    {
        vec2f Position;
        vec2f VertexSpacePosition;
        vec4f Color;
        float Highlight;
        float FrozenCoefficient;

        PointVertex(
            vec2f const & position,
            vec2f const & vertexSpacePosition,
            vec4f const & color,
            float highlight,
            float frozenCoefficient)
            : Position(position)
            , VertexSpacePosition(vertexSpacePosition)
            , Color(color)
            , Highlight(highlight)
            , FrozenCoefficient(frozenCoefficient)
        {}
    };

#pragma pack(pop)

    size_t mPointVertexCount;

    SLabOpenGLVAO mPointVAO;

    SLabOpenGLMappedBuffer<PointVertex, GL_ARRAY_BUFFER> mPointVertexBuffer;
    SLabOpenGLVBO mPointVertexVBO;

    ////////////////////////////////////////////////////////////////
    // Springs
    ////////////////////////////////////////////////////////////////

#pragma pack(push)

    struct SpringVertex
    {
        vec2f Position;
        vec2f VertexSpacePosition;
        vec4f Color;
        float Highlight;

        SpringVertex(
            vec2f const & position,
            vec2f const & vertexSpacePosition,
            vec4f const & color,
            float highlight)
            : Position(position)
            , VertexSpacePosition(vertexSpacePosition)
            , Color(color)
            , Highlight(highlight)
        {}
    };

#pragma pack(pop)

    SLabOpenGLVAO mSpringVAO;

    std::vector<SpringVertex> mSpringVertexBuffer;
    SLabOpenGLVBO mSpringVertexVBO;
};