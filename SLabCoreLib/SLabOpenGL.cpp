/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-15
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SLabOpenGL.h"

#include "SysSpecifics.h"

#include <algorithm>
#include <memory>
#include <numeric>

int SLabOpenGL::MaxVertexAttributes = 0;
int SLabOpenGL::MaxViewportWidth = 0;
int SLabOpenGL::MaxViewportHeight = 0;
int SLabOpenGL::MaxTextureSize = 0;
int SLabOpenGL::MaxRenderbufferSize = 0;

void SLabOpenGL::InitOpenGL()
{
    int status = gladLoadGL();
    if (!status)
    {
        throw SLabException("We are sorry, but this game requires OpenGL and it seems your graphics driver does not support it; the error is: failed to initialize GLAD");
    }

    //
    // Check OpenGL version
    //

    LogMessage("OpenGL version: ", GLVersion.major, ".", GLVersion.minor);

    if (GLVersion.major < MinOpenGLVersionMaj
        || (GLVersion.major == MinOpenGLVersionMaj && GLVersion.minor < MinOpenGLVersionMin))
    {
        throw SLabException(
            std::string("We are sorry, but this game requires at least OpenGL ")
            + std::to_string(MinOpenGLVersionMaj) + "." + std::to_string(MinOpenGLVersionMin)
            + ", while the version currently supported by your graphics driver is "
            + std::to_string(GLVersion.major) + "." + std::to_string(GLVersion.minor));
    }


    //
    // Init our extensions
    //

    InitOpenGLExt();


    //
    // Get some constants
    //

    GLint tmpConstant;

    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &tmpConstant);
    MaxVertexAttributes = tmpConstant;
    LogMessage("GL_MAX_VERTEX_ATTRIBS=", MaxVertexAttributes);

    GLint maxViewportDims[2];
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, &(maxViewportDims[0]));
    MaxViewportWidth = maxViewportDims[0];
    MaxViewportHeight = maxViewportDims[1];
    LogMessage("GL_MAX_VIEWPORT_DIMS=", MaxViewportWidth, "x", MaxViewportHeight);

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &tmpConstant);
    MaxTextureSize = tmpConstant;
    LogMessage("GL_MAX_TEXTURE_SIZE=", MaxTextureSize);

    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &tmpConstant);
    MaxRenderbufferSize = tmpConstant;
    LogMessage("GL_MAX_RENDERBUFFER_SIZE=", MaxRenderbufferSize);
}

void SLabOpenGL::CompileShader(
    std::string const & shaderSource,
    GLenum shaderType,
    SLabOpenGLShaderProgram const & shaderProgram,
    std::string const & programName)
{
    char const * shaderSourceCString = shaderSource.c_str();
    std::string const shaderTypeName = (shaderType == GL_VERTEX_SHADER) ? "vertex" : "fragment";

    // Set source
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &shaderSourceCString, NULL);
    GLenum glError = glGetError();
    if (GL_NO_ERROR != glError)
    {
        throw SLabException("Error setting " + shaderTypeName + " shader source for program \"" + programName + "\"");
    }

    // Compile
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (GL_FALSE == success)
    {
        char infoLog[1024];
        glGetShaderInfoLog(shader, sizeof(infoLog) - 1, NULL, infoLog);
        throw SLabException("Error compiling " + shaderTypeName + " shader: " + std::string(infoLog));
    }

    // Attach to program
    glAttachShader(*shaderProgram, shader);
    glError = glGetError();
    if (GL_NO_ERROR != glError)
    {
        throw SLabException("Error attaching compiled " + shaderTypeName + " shader to program \"" + programName + "\"");
    }

    // Delete shader
    glDeleteShader(shader);
}

void SLabOpenGL::LinkShaderProgram(
    SLabOpenGLShaderProgram const & shaderProgram,
    std::string const & programName)
{
    glLinkProgram(*shaderProgram);

    // Check
    int success;
    glGetProgramiv(*shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[1024];
        glGetShaderInfoLog(*shaderProgram, sizeof(infoLog), NULL, infoLog);
        throw SLabException("Error linking " + programName + " shader program: " + std::string(infoLog));
    }
}

GLint SLabOpenGL::GetParameterLocation(
    SLabOpenGLShaderProgram const & shaderProgram,
    std::string const & parameterName)
{
    GLint parameterLocation = glGetUniformLocation(*shaderProgram, parameterName.c_str());

    GLenum glError = glGetError();
    if (parameterLocation == -1
        || GL_NO_ERROR != glError)
    {
        throw SLabException("Cannot retrieve location of parameter \"" + parameterName + "\"");
    }

    return parameterLocation;
}

void SLabOpenGL::BindAttributeLocation(
    SLabOpenGLShaderProgram const & shaderProgram,
    GLuint attributeIndex,
    std::string const & attributeName)
{
    glBindAttribLocation(
        *shaderProgram,
        attributeIndex,
        attributeName.c_str());

    GLenum glError = glGetError();
    if (GL_NO_ERROR != glError)
    {
        throw SLabException("Error binding attribute location for attribute \"" + attributeName + "\"");
    }
}

void SLabOpenGL::Flush()
{
    // We do it here to have this call in the stack, helping
    // performance profiling
    glFlush();
}