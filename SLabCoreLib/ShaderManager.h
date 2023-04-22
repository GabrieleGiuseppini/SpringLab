/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-15
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ResourceLocator.h"
#include "SLabException.h"
#include "SLabOpenGL.h"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <unordered_map>
#include <vector>

class ShaderManager
{
public:

    enum class ProgramType : size_t
    {
        Points = 0,
        Springs = 1,
        Grid = 2,

        _Last = Grid
    };

    enum class ProgramParameterType : size_t
    {
        OrthoMatrix = 0,
        PixelWorldWidth = 1,
        WorldStep = 2
    };

    enum class VertexAttributeType : size_t
    {
        PointAttributeGroup1 = 0,
        PointAttributeGroup2 = 1,
        PointAttributeGroup3 = 2,

        SpringAttributeGroup1 = 0,
        SpringAttributeGroup2 = 1,
        SpringAttributeGroup3 = 2,

        GridAttributeGroup1 = 0
    };

private:

    static constexpr GLint NoParameterLocation = std::numeric_limits<GLint>::min();

    template <typename T>
    static std::string ToString(T const & v)
    {
        return std::to_string(v);
    }

    std::string ToString(float const & v)
    {
        std::stringstream stream;
        stream << std::fixed << v;
        return stream.str();
    }

public:

    static std::unique_ptr<ShaderManager> CreateInstance()
    {
        return std::unique_ptr<ShaderManager>(
            new ShaderManager(ResourceLocator::GetShadersRootFolderPath()));
    }

    template <ProgramType Program, ProgramParameterType Parameter>
    inline void SetProgramParameter(float value)
    {
        SetProgramParameter<Parameter>(Program, value);
    }

    template <ProgramParameterType Parameter>
    inline void SetProgramParameter(
        ProgramType program,
        float value)
    {
        const uint32_t programIndex = static_cast<uint32_t>(program);
        constexpr uint32_t parameterIndex = static_cast<uint32_t>(Parameter);

        assert(mPrograms[programIndex].UniformLocations[parameterIndex] != NoParameterLocation);

        glUniform1f(
            mPrograms[programIndex].UniformLocations[parameterIndex],
            value);

        CheckUniformError(program, Parameter);
    }

    template <ProgramType Program, ProgramParameterType Parameter>
    inline void SetProgramParameter(float val1, float val2)
    {
        constexpr uint32_t programIndex = static_cast<uint32_t>(Program);
        constexpr uint32_t parameterIndex = static_cast<uint32_t>(Parameter);

        assert(mPrograms[programIndex].UniformLocations[parameterIndex] != NoParameterLocation);

        glUniform2f(
            mPrograms[programIndex].UniformLocations[parameterIndex],
            val1,
            val2);

        CheckUniformError<Program, Parameter>();
    }

    template <ProgramType Program, ProgramParameterType Parameter>
    inline void SetProgramParameter(float val1, float val2, float val3)
    {
        constexpr uint32_t programIndex = static_cast<uint32_t>(Program);
        constexpr uint32_t parameterIndex = static_cast<uint32_t>(Parameter);

        assert(mPrograms[programIndex].UniformLocations[parameterIndex] != NoParameterLocation);

        glUniform3f(
            mPrograms[programIndex].UniformLocations[parameterIndex],
            val1,
            val2,
            val3);

        CheckUniformError<Program, Parameter>();
    }

    template <ProgramType Program, ProgramParameterType Parameter>
    inline void SetProgramParameter(float val1, float val2, float val3, float val4)
    {
        constexpr uint32_t programIndex = static_cast<uint32_t>(Program);
        constexpr uint32_t parameterIndex = static_cast<uint32_t>(Parameter);

        assert(mPrograms[programIndex].UniformLocations[parameterIndex] != NoParameterLocation);

        glUniform4f(
            mPrograms[programIndex].UniformLocations[parameterIndex],
            val1,
            val2,
            val3,
            val4);

        CheckUniformError<Program, Parameter>();
    }

    template <ProgramType Program, ProgramParameterType Parameter>
    inline void SetProgramParameter(float const(&value)[4][4])
    {
        constexpr uint32_t programIndex = static_cast<uint32_t>(Program);
        constexpr uint32_t parameterIndex = static_cast<uint32_t>(Parameter);

        assert(mPrograms[programIndex].UniformLocations[parameterIndex] != NoParameterLocation);

        glUniformMatrix4fv(
            mPrograms[programIndex].UniformLocations[parameterIndex],
            1,
            GL_FALSE,
            &(value[0][0]));

        CheckUniformError<Program, Parameter>();
    }

    // At any given moment, only one program may be active
    template <ProgramType Program>
    inline void ActivateProgram()
    {
        ActivateProgram(Program);
    }

    // At any given moment, only one program may be active
    inline void ActivateProgram(ProgramType program)
    {
        uint32_t const programIndex = static_cast<uint32_t>(program);

        glUseProgram(*(mPrograms[programIndex].OpenGLHandle));

        CheckOpenGLError();
    }

private:

    template <ProgramType Program, ProgramParameterType Parameter>
    static inline void CheckUniformError()
    {
        GLenum glError = glGetError();
        if (GL_NO_ERROR != glError)
        {
            throw SLabException("Error setting uniform for parameter \"" + ProgramParameterTypeToStr(Parameter) + "\" on program \"" + ProgramTypeToStr(Program) + "\"");
        }
    }

    static inline void CheckUniformError(
        ProgramType program,
        ProgramParameterType parameter)
    {
        GLenum glError = glGetError();
        if (GL_NO_ERROR != glError)
        {
            throw SLabException("Error setting uniform for parameter \"" + ProgramParameterTypeToStr(parameter) + "\" on program \"" + ProgramTypeToStr(program) + "\"");
        }
    }

private:

    ShaderManager(std::filesystem::path const & shadersRoot);

    void CompileShader(
        std::string const & shaderFilename,
        std::string const & shaderSource,
        std::unordered_map<std::string, std::pair<bool, std::string>> const & shaderSources,
        std::map<std::string, std::string> const & staticParameters);

    static std::string ResolveIncludes(
        std::string const & shaderSource,
        std::unordered_map<std::string, std::pair<bool, std::string>> const & shaderSources);

    static std::tuple<std::string, std::string> SplitSource(std::string const & source);

    static void ParseLocalStaticParameters(
        std::string const & localStaticParametersSource,
        std::map<std::string, std::string> & staticParameters);

    static std::string SubstituteStaticParameters(
        std::string const & source,
        std::map<std::string, std::string> const & staticParameters);

    static std::set<ProgramParameterType> ExtractShaderParameters(std::string const & source);

    static std::set<std::string> ExtractVertexAttributeNames(std::string const & source);

    static ProgramType ShaderFilenameToProgramType(std::string const & str);
    static std::string ProgramTypeToStr(ProgramType program);

    static ProgramParameterType StrToProgramParameterType(std::string const & str);
    static std::string ProgramParameterTypeToStr(ProgramParameterType programParameter);

    static VertexAttributeType StrToVertexAttributeType(std::string const & str);

private:

    struct ProgramInfo
    {
        // The OpenGL handle to the program
        SLabOpenGLShaderProgram OpenGLHandle;

        // The uniform locations, indexed by shader parameter type;
        // set to NoLocation when not specified in the shader
        std::vector<GLint> UniformLocations;
    };

    // All programs, indexed by program type
    std::vector<ProgramInfo> mPrograms;
};
