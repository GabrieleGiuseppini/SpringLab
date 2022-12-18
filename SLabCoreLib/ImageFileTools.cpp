/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-15
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ImageFileTools.h"

#include "SLabException.h"

#include <IL/il.h>
#include <IL/ilu.h>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <regex>

bool ImageFileTools::mIsInitialized = false;

ImageSize ImageFileTools::GetImageSize(std::filesystem::path const & filepath)
{
    //
    // Load image
    //

    ILuint imgHandle = InternalLoadImage(filepath);

    //
    // Get size
    //

    int const width = ilGetInteger(IL_IMAGE_WIDTH);
    int const height = ilGetInteger(IL_IMAGE_HEIGHT);


    //
    // Delete image
    //

    ilDeleteImage(imgHandle);


    //
    // Check
    //

    if (width == 0 || height == 0)
    {
        throw SLabException("Could not load image \"" + filepath.string() + "\": image is empty");
    }

    return ImageSize(width, height);
}

RgbaImageData ImageFileTools::LoadImageRgba(std::filesystem::path const & filepath)
{
    return InternalLoadImage<rgbaColor>(
        filepath,
        IL_RGBA,
        IL_ORIGIN_LOWER_LEFT);
}

RgbImageData ImageFileTools::LoadImageRgb(std::filesystem::path const & filepath)
{
    return InternalLoadImage<rgbColor>(
        filepath,
        IL_RGB,
        IL_ORIGIN_LOWER_LEFT);
}

void ImageFileTools::SaveImage(
    std::filesystem::path filepath,
    RgbaImageData const & image)
{
    InternalSaveImage(
        image.Size,
        image.Data.get(),
        4,
        IL_RGBA,
        filepath);
}

void ImageFileTools::SaveImage(
    std::filesystem::path filepath,
    RgbImageData const & image)
{
    InternalSaveImage(
        image.Size,
        image.Data.get(),
        3,
        IL_RGB,
        filepath);
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

void ImageFileTools::CheckInitialized()
{
    if (!mIsInitialized)
    {
        // Initialize DevIL
        ilInit();
        iluInit();

        mIsInitialized = true;
    }
}

unsigned int ImageFileTools::InternalLoadImage(std::filesystem::path const & filepath)
{
    CheckInitialized();

    ILuint imghandle;
    ilGenImages(1, &imghandle);
    ilBindImage(imghandle);

    //
    // Load image
    //

    std::string const filepathStr = filepath.string();
    ILconst_string ilFilename(filepathStr.c_str());
    if (!ilLoadImage(ilFilename))
    {
        ILint const devilError = ilGetError();

        // First check if the file is missing altogether
        if (!std::filesystem::exists(filepath))
        {
            throw SLabException("Could not load image \"" + filepathStr + "\": the file does not exist");
        }

        // Provide DevIL's error message now
        std::string const devilErrorMessage(iluErrorString(devilError));
        throw SLabException("Could not load image \"" + filepathStr + "\": " + devilErrorMessage);
    }

    return static_cast<unsigned int>(imghandle);
}

template <typename TColor>
ImageData<TColor> ImageFileTools::InternalLoadImage(
    std::filesystem::path const & filepath,
    int targetFormat,
    int targetOrigin)
{
    //
    // Load image
    //

    ILuint imgHandle = InternalLoadImage(filepath);

    //
    // Check if we need to convert it
    //

    int imageFormat = ilGetInteger(IL_IMAGE_FORMAT);
    int imageType = ilGetInteger(IL_IMAGE_TYPE);
    if (targetFormat != imageFormat || IL_UNSIGNED_BYTE != imageType)
    {
        if (!ilConvertImage(targetFormat, IL_UNSIGNED_BYTE))
        {
            ILint devilError = ilGetError();
            std::string devilErrorMessage(iluErrorString(devilError));
            throw SLabException("Could not convert image \"" + filepath.string() + "\": " + devilErrorMessage);
        }
    }

    int imageOrigin = ilGetInteger(IL_IMAGE_ORIGIN);
    if (targetOrigin != imageOrigin)
    {
        iluFlipImage();
    }


    //
    // Get metadata
    //

    ImageSize imageSize(
        ilGetInteger(IL_IMAGE_WIDTH),
        ilGetInteger(IL_IMAGE_HEIGHT));
    int const bpp = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);

    assert(bpp == sizeof(TColor));


    //
    // Create data
    //

    ILubyte const * imageData = ilGetData();
    auto data = std::make_unique<TColor[]>(imageSize.Width * imageSize.Height);
    std::memcpy(static_cast<void*>(data.get()), imageData, imageSize.Width * imageSize.Height * bpp);

    //
    // Delete image
    //

    ilDeleteImage(imgHandle);


    return ImageData<TColor>(
        imageSize,
        std::move(data));
}

void ImageFileTools::InternalSaveImage(
    ImageSize imageSize,
    void const * imageData,
    int bpp,
    int format,
    std::filesystem::path filepath)
{
    CheckInitialized();

    ILuint imghandle;
    ilGenImages(1, &imghandle);
    ilBindImage(imghandle);

    ilTexImage(
        imageSize.Width,
        imageSize.Height,
        1,
        static_cast<ILubyte>(bpp),
        format,
        IL_UNSIGNED_BYTE,
        const_cast<void *>(imageData));

    ilEnable(IL_FILE_OVERWRITE);
    ilSave(IL_PNG, filepath.string().c_str());

    ilDeleteImage(imghandle);
}