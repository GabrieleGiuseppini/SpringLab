/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-15
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "SLabOpenGL.h"

#include <cassert>
#include <cstdlib>

/*
 * This class is an OpenGL mapped buffer hidden behind a vector-like facade.
 */
template<typename TElement, GLenum TTarget>
class SLabOpenGLMappedBuffer
{
public:

    SLabOpenGLMappedBuffer()
        : mMappedBuffer(nullptr)
        , mSize(0u)
        , mAllocatedSize(0u)
    {
    }

    ~SLabOpenGLMappedBuffer()
    {
        if (nullptr != mMappedBuffer)
        {
            unmap();
        }
    }

    inline void map(size_t size)
    {
        assert(nullptr == mMappedBuffer);

        if (size != 0)
        {
            mMappedBuffer = glMapBuffer(TTarget, GL_WRITE_ONLY);
            CheckOpenGLError();

            if (nullptr == mMappedBuffer)
            {
                throw SLabException("glMapBuffer returned null pointer");
            }
        }

        mSize = 0u;
        mAllocatedSize = size;
    }

    inline void map_and_fill(size_t size)
    {
        map(size);
        mSize = size; // "Fill" up the buffer
    }

    inline void unmap()
    {
        // Might not be mapped in case the size was zero
        if (nullptr != mMappedBuffer)
        {
            glUnmapBuffer(TTarget);
            mMappedBuffer = nullptr;
        }

        // Leave size and allocated size as they are, as this
        // buffer may still be asked for its size (regardless
        // of whether or not its data has been uploaded)
    }

    template<typename... TArgs>
    inline TElement & emplace_back(TArgs&&... args)
    {
        assert(nullptr != mMappedBuffer);
        assert(mSize < mAllocatedSize);
        return *new(&(((TElement *)mMappedBuffer)[mSize++])) TElement(std::forward<TArgs>(args)...);
    }

    template<typename... TArgs>
    inline TElement & emplace_at(
        size_t index,
        TArgs&&... args)
    {
        assert(nullptr != mMappedBuffer);
        assert(index < mSize);
        return *new(&(((TElement *)mMappedBuffer)[index])) TElement(std::forward<TArgs>(args)...);
    }

    inline void reset()
    {
        assert(nullptr == mMappedBuffer);

        mSize = 0u;
        mAllocatedSize = 0u;
    }

    inline size_t size() const noexcept
    {
        return mSize;
    }

	inline size_t max_size() const noexcept
	{
		return mAllocatedSize;
	}

private:

    void * mMappedBuffer;
    size_t mSize;
    size_t mAllocatedSize;
};
