/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-15
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <algorithm>
#include <cstdint>
#include <ostream>
#include <sstream>
#include <string>

#pragma pack(push)
struct ImageSize
{
public:

    int Width;
    int Height;

    constexpr ImageSize(
        int width,
        int height)
        : Width(width)
        , Height(height)
    {
    }

    ImageSize(ImageSize const & other)
        : Width(other.Width)
        , Height(other.Height)
    {
    }

    ImageSize(ImageSize && other) noexcept
        : Width(other.Width)
        , Height(other.Height)
    {
    }

    ImageSize & operator=(ImageSize const & other)
    {
        this->Width = other.Width;
        this->Height = other.Height;

        return *this;
    }

    inline static ImageSize Zero()
    {
        return ImageSize(0, 0);
    }

    inline bool operator==(ImageSize const & other) const
    {
        return this->Width == other.Width
            && this->Height == other.Height;
    }

    inline bool operator!=(ImageSize const & other) const
    {
        return !(*this == other);
    }

    inline ImageSize operator*(int magnificationFactor) const
    {
        return ImageSize(
            this->Width * magnificationFactor,
            this->Height * magnificationFactor);
    }

    inline int GetPixelCount() const
    {
        return this->Width * this->Height;
    }

    inline ImageSize Union(ImageSize const & other) const
    {
        return ImageSize(
            std::max(this->Width, other.Width),
            std::max(this->Height, other.Height));
    }

    inline ImageSize Intersection(ImageSize const & other) const
    {
        return ImageSize(
            std::min(this->Width, other.Width),
            std::min(this->Height, other.Height));
    }

    std::string ToString() const
    {
        std::stringstream ss;
        ss << Width << "x" << Height;
        return ss.str();
    }
};
#pragma pack(pop)

inline std::basic_ostream<char> & operator<<(std::basic_ostream<char> & os, ImageSize const & is)
{
    os << is.ToString();
    return os;
}
