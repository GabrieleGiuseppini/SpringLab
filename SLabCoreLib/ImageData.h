/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-15
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Colors.h"
#include "ImageSize.h"
#include "Vectors.h"

#include <memory>

template <typename TColor>
struct ImageData
{
public:

    using color_type = TColor;

public:

    ImageSize const Size;
    std::unique_ptr<color_type[]> Data;

    ImageData(
        int width,
        int height,
        std::unique_ptr<color_type[]> data)
        : Size(width, height)
        , Data(std::move(data))
    {
    }

    ImageData(
        ImageSize size,
        std::unique_ptr<color_type[]> data)
        : Size(size)
        , Data(std::move(data))
    {
    }

    ImageData(ImageData && other) noexcept
        : Size(other.Size)
        , Data(std::move(other.Data))
    {
    }
};

using RgbImageData = ImageData<rgbColor>;
using RgbaImageData = ImageData<rgbaColor>;
using Vec3fImageData = ImageData<vec3f>;