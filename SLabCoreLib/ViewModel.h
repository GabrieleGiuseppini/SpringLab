/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Vectors.h"

#include <algorithm>

/*
 * This class encapsulates the management of view and projection parameters.
 */
class ViewModel
{
public:

    using ProjectionMatrix = float[4][4];

public:

    ViewModel(
        float zoom,
        vec2f cameraWorldPosition,
        int canvasWidth,
        int canvasHeight)
        : mZoom(zoom)
        , mCam(cameraWorldPosition)
        , mCanvasWidth(std::max(canvasWidth, 1))
        , mCanvasHeight(std::max(canvasHeight, 1))
    {
        //
        // Initialize ortho matrix
        //

        constexpr float ZFar = 1000.0f;
        constexpr float ZNear = 1.0f;

        std::fill(
            &(mOrthoMatrix[0][0]),
            &(mOrthoMatrix[0][0]) + sizeof(mOrthoMatrix) / sizeof(float),
            0.0f);

        mOrthoMatrix[2][2] = -2.0f / (ZFar - ZNear);
        mOrthoMatrix[3][2] = -(ZFar + ZNear) / (ZFar - ZNear);
        mOrthoMatrix[3][3] = 1.0f;

        //
        // Recalculate calculated attributes
        //

        RecalculateAttributes();
    }

    float const & GetZoom() const
    {
        return mZoom;
    }

    void SetZoom(float zoom)
    {
        mZoom = Clamp(zoom, MinZoom, MaxZoom);

        RecalculateAttributes();
    }

    vec2f const & GetCameraWorldPosition() const
    {
        return mCam;
    }

    void SetCameraWorldPosition(vec2f const & pos)
    {
        mCam = pos;

        RecalculateAttributes();
    }

    int GetCanvasWidth() const
    {
        return mCanvasWidth;
    }

    int GetCanvasHeight() const
    {
        return mCanvasHeight;
    }

    float GetAspectRatio() const
    {
        return static_cast<float>(GetCanvasWidth()) / static_cast<float>(GetCanvasHeight());
    }

    void SetCanvasSize(int width, int height)
    {
        mCanvasWidth = width;
        mCanvasHeight = height;

        RecalculateAttributes();
    }

    float GetVisibleWorldWidth() const
    {
        return mVisibleWorldWidth;
    }

    float GetVisibleWorldHeight() const
    {
        return mVisibleWorldHeight;
    }

    vec2f GetVisibleWorldTopLeft() const
    {
        return mVisibleWorldTopLeft;
    }

    vec2f GetVisibleWorldBottomRight() const
    {
        return mVisibleWorldBottomRight;
    }

    float GetCanvasToVisibleWorldHeightRatio() const
    {
        return mCanvasToVisibleWorldHeightRatio;
    }

    float GetCanvasWidthToHeightRatio() const
    {
        return mCanvasWidthToHeightRatio;
    }

    //
    // Coordinate transformations
    //

    /*
     * Equivalent of the transformation we usually perform in vertex shaders.
     */
    inline vec2f WorldToNdc(vec2f const & worldCoordinates) const
    {
        return vec2f(
            worldCoordinates.x * mOrthoMatrix[0][0] + mOrthoMatrix[3][0],
            worldCoordinates.y * mOrthoMatrix[1][1] + mOrthoMatrix[3][1]);
    }

    inline vec2f ScreenToWorld(vec2f const & screenCoordinates) const
    {
        return vec2f(
            (screenCoordinates.x / static_cast<float>(mCanvasWidth) - 0.5f) * mVisibleWorldWidth + mCam.x,
            (screenCoordinates.y / static_cast<float>(mCanvasHeight) - 0.5f) * -mVisibleWorldHeight + mCam.y);
    }

    inline vec2f ScreenOffsetToWorldOffset(vec2f const & screenOffset) const
    {
        return vec2f(
            screenOffset.x / static_cast<float>(mCanvasWidth) * mVisibleWorldWidth,
            -screenOffset.y / static_cast<float>(mCanvasHeight) * mVisibleWorldHeight);
    }

    inline vec2f WorldToScreen(vec2f const & worldCoordinates) const
    {
        return vec2f(
            ((worldCoordinates.x - mCam.x) / mVisibleWorldWidth + 0.5f) * static_cast<float>(mCanvasWidth),
            ((worldCoordinates.y - mCam.y) / -mVisibleWorldHeight + 0.5f) * static_cast<float>(mCanvasHeight));
    }

    inline float PixelWidthToWorldWidth(float pixelWidth) const
    {
        // Width in NDC coordinates (between 0 and 2.0)
        float const ndcW = 2.0f * pixelWidth / static_cast<float>(mCanvasWidth);

        // An NDC width of 2 is the entire visible world width
        return (ndcW / 2.0f) * mVisibleWorldWidth;
    }

    inline float PixelHeightToWorldHeight(float pixelHeight) const
    {
        // Height in NDC coordinates (between 0 and 2.0)
        float const ndcH = 2.0f * pixelHeight / static_cast<float>(mCanvasHeight);

        // An NDC height of 2 is the entire visible world height
        return (ndcH / 2.0f) * mVisibleWorldHeight;
    }

    /*
     * Calculates the zoom required to ensure that the specified world
     * width is fully visible in the canvas.
     */
    inline float CalculateZoomForWorldWidth(float worldWidth) const
    {
        assert(worldWidth > 0.0f);
        return ZoomHeightConstant * GetAspectRatio() / worldWidth;
    }

    /*
     * Calculates the zoom required to ensure that the specified world
     * height is fully visible in the canvas.
     */
    inline float CalculateZoomForWorldHeight(float worldHeight) const
    {
        assert(worldHeight > 0.0f);
        return ZoomHeightConstant / worldHeight;
    }

    //
    // Projection matrixes
    //

    inline ProjectionMatrix const & GetOrthoMatrix() const
    {
        return mOrthoMatrix;
    }

private:

    float CalculateVisibleWorldWidth(float zoom) const
    {
        return CalculateVisibleWorldHeight(zoom) * GetAspectRatio();
    }

    float CalculateVisibleWorldHeight(float zoom) const
    {
        assert(zoom != 0.0f);

        return ZoomHeightConstant / zoom;
    }

    void RecalculateAttributes()
    {
        mVisibleWorldWidth = CalculateVisibleWorldWidth(mZoom);
        mVisibleWorldHeight = CalculateVisibleWorldHeight(mZoom);

        mVisibleWorldTopLeft = vec2f(
            mCam.x - (mVisibleWorldWidth / 2.0f),
            mCam.y + (mVisibleWorldHeight / 2.0f));
        mVisibleWorldBottomRight = vec2f(
            mCam.x + (mVisibleWorldWidth /2.0f),
            mCam.y - (mVisibleWorldHeight / 2.0f));

        mCanvasToVisibleWorldHeightRatio = static_cast<float>(mCanvasHeight) / mVisibleWorldHeight;
        mCanvasWidthToHeightRatio = static_cast<float>(mCanvasWidth) / static_cast<float>(mCanvasHeight);

        // Recalculate kernel Ortho Matrix cells
        mOrthoMatrix[0][0] = 2.0f / mVisibleWorldWidth;
        mOrthoMatrix[1][1] = 2.0f / mVisibleWorldHeight;
        mOrthoMatrix[3][0] = -2.0f * mCam.x / mVisibleWorldWidth;
        mOrthoMatrix[3][1] = -2.0f * mCam.y / mVisibleWorldHeight;
    }

private:

    // Constants
    static float constexpr MinZoom = 0.02f;
    static float constexpr MaxZoom = 50.0f;
    static float constexpr ZoomHeightConstant = 10.0f; // World height at zoom=1.0

    // Primary inputs
    float mZoom;
    vec2f mCam;
    int mCanvasWidth;
    int mCanvasHeight;

    // Calculated attributes
    float mVisibleWorldWidth;
    float mVisibleWorldHeight;
    vec2f mVisibleWorldTopLeft;
    vec2f mVisibleWorldBottomRight;
    float mCanvasToVisibleWorldHeightRatio;
    float mCanvasWidthToHeightRatio;
    ProjectionMatrix mOrthoMatrix;
};
