/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-15
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ImageData.h"

#include <filesystem>
#include <functional>

/*
 * Image standards:
 *  - Coordinates have origin at lower-left
 */
class ImageFileTools
{
public:

    static ImageSize GetImageSize(std::filesystem::path const & filepath);

    static RgbaImageData LoadImageRgba(std::filesystem::path const & filepath);
    static RgbImageData LoadImageRgb(std::filesystem::path const & filepath);

    static void SaveImage(
        std::filesystem::path filepath,
        RgbaImageData const & image);

    static void SaveImage(
        std::filesystem::path filepath,
        RgbImageData const & image);

private:

    static void CheckInitialized();

    static unsigned int InternalLoadImage(std::filesystem::path const & filepath);

    template <typename TColor>
    static ImageData<TColor> InternalLoadImage(
        std::filesystem::path const & filepath,
        int targetFormat,
        int targetOrigin);

    static void InternalSaveImage(
        ImageSize imageSize,
        void const * imageData,
        int bpp,
        int format,
        std::filesystem::path filepath);

private:

    static bool mIsInitialized;
};
