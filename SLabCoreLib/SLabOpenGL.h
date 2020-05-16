/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-15
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "SLabOpenGL_Ext.h"

#include "Log.h"
#include "SLabException.h"

#include <cassert>
#include <cstdio>
#include <string>

/////////////////////////////////////////////////////////////////////////////////////////
// Types
/////////////////////////////////////////////////////////////////////////////////////////

template<typename T, typename TDeleter>
class SLabOpenGLObject
{
public:

    SLabOpenGLObject()
        : mValue(T())
    {}

    SLabOpenGLObject(T value)
        : mValue(value)
    {}

    ~SLabOpenGLObject()
    {
        TDeleter::Delete(mValue);
    }

    SLabOpenGLObject(SLabOpenGLObject const & other) = delete;

    SLabOpenGLObject(SLabOpenGLObject && other)
    {
        mValue = other.mValue;
        other.mValue = T();
    }

    SLabOpenGLObject & operator=(SLabOpenGLObject const & other) = delete;

    SLabOpenGLObject & operator=(SLabOpenGLObject && other)
    {
        if (!!(mValue))
        {
            TDeleter::Delete(mValue);
        }

        mValue = other.mValue;
        other.mValue = T();

        return *this;
    }

    bool operator !() const
    {
        return mValue == T();
    }

    T operator*() const
    {
        return mValue;
    }

    void reset() noexcept
    {
        if (!!(mValue))
        {
            TDeleter::Delete(mValue);
        }

        mValue = T();
    }

    [[nodiscard]] T release() noexcept
    {
        auto value = mValue;
        mValue = T();
        return value;
    }

private:
    T mValue;
};

struct SLabOpenGLProgramDeleter
{
    static void Delete(GLuint p)
    {
        static_assert(GLuint() == 0, "Default value is not zero, i.e. the OpenGL NULL");

        if (p != 0)
        {
            glDeleteProgram(p);
        }
    }
};

struct SLabOpenGLVBODeleter
{
    static void Delete(GLuint p)
    {
        static_assert(GLuint() == 0, "Default value is not zero, i.e. the OpenGL NULL");

        if (p != 0)
        {
            glDeleteBuffers(1, &p);
        }
    }
};

struct SLabOpenGLVAODeleter
{
    static void Delete(GLuint p)
    {
        static_assert(GLuint() == 0, "Default value is not zero, i.e. the OpenGL NULL");

        if (p != 0)
        {
            glDeleteVertexArrays(1, &p);
        }
    }
};

struct SLabOpenGLTextureDeleter
{
    static void Delete(GLuint p)
    {
        static_assert(GLuint() == 0, "Default value is not zero, i.e. the OpenGL NULL");

        if (p != 0)
        {
            glDeleteTextures(1, &p);
        }
    }
};

struct SLabOpenGLFramebufferDeleter
{
    static void Delete(GLuint p)
    {
        static_assert(GLuint() == 0, "Default value is not zero, i.e. the OpenGL NULL");

        if (p != 0)
        {
            glDeleteFramebuffers(1, &p);
        }
    }
};

struct SLabOpenGLRenderbufferDeleter
{
    static void Delete(GLuint p)
    {
        static_assert(GLuint() == 0, "Default value is not zero, i.e. the OpenGL NULL");

        if (p != 0)
        {
            glDeleteRenderbuffers(1, &p);
        }
    }
};

using SLabOpenGLShaderProgram = SLabOpenGLObject<GLuint, SLabOpenGLProgramDeleter>;
using SLabOpenGLVBO = SLabOpenGLObject<GLuint, SLabOpenGLVBODeleter>;
using SLabOpenGLVAO = SLabOpenGLObject<GLuint, SLabOpenGLVAODeleter>;
using SLabOpenGLTexture = SLabOpenGLObject<GLuint, SLabOpenGLTextureDeleter>;
using SLabOpenGLFramebuffer = SLabOpenGLObject<GLuint, SLabOpenGLFramebufferDeleter>;
using SLabOpenGLRenderbuffer = SLabOpenGLObject<GLuint, SLabOpenGLRenderbufferDeleter>;

/////////////////////////////////////////////////////////////////////////////////////////
// SLabOpenGL
/////////////////////////////////////////////////////////////////////////////////////////

class SLabOpenGL
{
public:

    static constexpr int MinOpenGLVersionMaj = 2;
    static constexpr int MinOpenGLVersionMin = 0;

public:

    static int MaxVertexAttributes;
    static int MaxViewportWidth;
    static int MaxViewportHeight;
    static int MaxTextureSize;
    static int MaxRenderbufferSize;

public:

    static void InitOpenGL();

    static void CompileShader(
        std::string const & shaderSource,
        GLenum shaderType,
        SLabOpenGLShaderProgram const & shaderProgram,
        std::string const & programName);

    static void LinkShaderProgram(
        SLabOpenGLShaderProgram const & shaderProgram,
        std::string const & programName);

    static GLint GetParameterLocation(
        SLabOpenGLShaderProgram const & shaderProgram,
        std::string const & parameterName);

    static void BindAttributeLocation(
        SLabOpenGLShaderProgram const & shaderProgram,
        GLuint attributeIndex,
        std::string const & attributeName);

    static void Flush();
};

inline void _CheckOpenGLError(char const * file, int line)
{
    GLenum errorCode = glGetError();
    if (errorCode != GL_NO_ERROR)
    {
        std::string errorCodeString;
        switch (errorCode)
        {
            case GL_INVALID_ENUM:
            {
                errorCodeString = "INVALID_ENUM";
                break;
            }

            case GL_INVALID_VALUE:
            {
                errorCodeString = "INVALID_VALUE";
                break;
            }

            case GL_INVALID_OPERATION:
            {
                errorCodeString = "INVALID_OPERATION";
                break;
            }

            case GL_OUT_OF_MEMORY:
            {
                errorCodeString = "OUT_OF_MEMORY";
                break;
            }

            default:
            {
                errorCodeString = "Other (" + std::to_string(errorCode) + ")";
                break;
            }
        }

        throw SLabException("OpenGL Error \"" + errorCodeString + "\" at file " + std::string(file) + ", line " + std::to_string(line));
    }
}

#define CheckOpenGLError() _CheckOpenGLError(__FILE__, __LINE__)
