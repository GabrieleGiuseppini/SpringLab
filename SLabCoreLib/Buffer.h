/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-20
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "SLabMath.h"
#include "SysSpecifics.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <functional>
#include <memory>
#include <stdexcept>

/*
 * A fixed-size buffer which cannot grow more than the size that it is initially
 * constructed with.
 */
template <typename TElement>
class Buffer final
{
public:

    static constexpr size_t CalculateByteSize(size_t element_count)
    {
        return sizeof(TElement) * element_count;
    }

public:

    explicit Buffer(size_t size)
        : mBuffer(make_unique_buffer_aligned_to_vectorization_word<TElement>(size))
        , mSize(size)
        , mCurrentPopulatedSize(0)
    {
    }

    Buffer(
        TElement const * data,
        size_t size)
        : mBuffer(make_unique_buffer_aligned_to_vectorization_word<TElement>(size))
        , mSize(size)
        , mCurrentPopulatedSize(0)
    {
        std::memcpy(mBuffer.get(), data, size);
        mCurrentPopulatedSize = size;
    }

    Buffer(
        size_t size,
        size_t fillStart,
        TElement fillValue)
        : Buffer(size)
    {
        assert(fillStart <= mSize);

        // Fill-in values
        std::fill(
            mBuffer.get() + fillStart,
            mBuffer.get() + mSize,
            fillValue);
    }

    Buffer(
        size_t size,
        size_t fillStart,
        std::function<TElement(size_t)> fillFunction)
        : Buffer(size)
    {
        assert(fillStart <= mSize);

        for (size_t i = fillStart; i < mSize; ++i)
            mBuffer[i] = fillFunction(i);
    }

    Buffer(Buffer && other) noexcept
        : mBuffer(std::move(other.mBuffer))
        , mSize(other.mSize)
        , mCurrentPopulatedSize(other.mCurrentPopulatedSize)
    {
    }

    size_t GetSize() const
    {
        return mSize;
    }

    /*
     * Gets the current number of elements populated in the buffer via emplace_back();
     * less than or equal the declared buffer size.
     */
    size_t GetCurrentPopulatedSize() const
    {
        return mCurrentPopulatedSize;
    }

    /*
     * Adds an element to the buffer. Assumed to be invoked only at initialization time.
     *
     * Cannot add more elements than the size specified at constructor time.
     */
    template <typename... Args>
    TElement & emplace_back(Args&&... args)
    {
        if (mCurrentPopulatedSize < mSize)
        {
            return *new(&(mBuffer[mCurrentPopulatedSize++])) TElement(std::forward<Args>(args)...);
        }
        else
        {
            throw std::runtime_error("The buffer is already full");
        }
    }

    /*
     * Fills the buffer with a value.
     */
    void fill(TElement value)
    {
        TElement * restrict const ptr = mBuffer.get();
        for (size_t i = 0; i < mSize; ++i)
            ptr[i] = value;

        mCurrentPopulatedSize = mSize;
    }

    /*
     * Clears the buffer, by reducing its currently-populated
     * element count to zero, so that it is ready for being re-populated.
     */
    void clear()
    {
        mCurrentPopulatedSize = 0;
    }

    /*
     * Copies a buffer into this buffer.
     *
     * The sizes of the buffers must match.
     */
    void copy_from(Buffer<TElement> const & other)
    {
        assert(mSize == other.mSize);
        std::memcpy(mBuffer.get(), other.mBuffer.get(), mSize * sizeof(TElement));

        mCurrentPopulatedSize = other.mCurrentPopulatedSize;
    }

    inline void swap(Buffer & other) noexcept
    {
        std::swap(mBuffer, other.mBuffer);
        std::swap(mSize, other.mSize);
        std::swap(mCurrentPopulatedSize, other.mCurrentPopulatedSize);
    }

    /*
     * Gets an element.
     */

    inline TElement const & operator[](size_t index) const noexcept
    {
#ifndef NDEBUG
        // Ugly trick to allow setting breakpoints on assert failures
        if (index >= mSize)
        {
            assert(index < mSize);
        }
#endif

        return mBuffer[index];
    }

    inline TElement & operator[](size_t index) noexcept
    {
        assert(index < mSize);

        return mBuffer[index];
    }

    /*
     * Gets the buffer.
     */

    inline TElement const * restrict data() const
    {
        return mBuffer.get();
    }

    inline TElement * data()
    {
        return mBuffer.get();
    }

    unique_aligned_buffer<TElement> mBuffer;
    size_t mSize;
    size_t mCurrentPopulatedSize;
};
