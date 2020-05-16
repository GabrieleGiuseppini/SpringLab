/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-15
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "SlabOpenGL.h"

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

template <typename Traits>
class ShaderManager
{
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

    static std::unique_ptr<ShaderManager> CreateInstance(std::filesystem::path const & shadersRoot)
    {
        return std::unique_ptr<ShaderManager>(
            new ShaderManager(shadersRoot));
    }

    template <typename Traits::ProgramType Program>
    inline void SetTextureParameters()
    {
        size_t programIndex = static_cast<size_t>(Program);

        // Find all texture parameters
        for (size_t parameterIndex = 0; parameterIndex < mPrograms[programIndex].UniformLocations.size(); ++parameterIndex)
        {
            if (mPrograms[programIndex].UniformLocations[parameterIndex] != NoParameterLocation)
            {
                typename Traits::ProgramParameterType parameter = static_cast<typename Traits::ProgramParameterType>(parameterIndex);

                // See if it's a texture/sampler parameter
                if (parameter >= Traits::ProgramParameterType::_FirstTexture
                    && parameter <= Traits::ProgramParameterType::_LastTexture)
                {
                    //
                    // Set it
                    //

                    auto const textureUnitIndex = static_cast<uint8_t>(parameter) - static_cast<uint8_t>(Traits::ProgramParameterType::_FirstTexture);

                    assert(mPrograms[programIndex].UniformLocations[parameterIndex] != NoParameterLocation);

                    glUniform1i(
                        mPrograms[programIndex].UniformLocations[parameterIndex],
                        textureUnitIndex);

                    CheckUniformError(Program, parameter);
                }
            }
        }
    }

    template <typename Traits::ProgramType Program, typename Traits::ProgramParameterType Parameter>
    inline void SetProgramParameter(float value)
    {
        SetProgramParameter<Parameter>(Program, value);
    }

    template <typename Traits::ProgramParameterType Parameter>
    inline void SetProgramParameter(
        typename Traits::ProgramType program,
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

    template <typename Traits::ProgramType Program, typename Traits::ProgramParameterType Parameter>
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

    template <typename Traits::ProgramType Program, typename Traits::ProgramParameterType Parameter>
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

    template <typename Traits::ProgramType Program, typename Traits::ProgramParameterType Parameter>
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

    template <typename Traits::ProgramType Program, typename Traits::ProgramParameterType Parameter>
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
    template <typename Traits::ProgramType Program>
    inline void ActivateProgram()
    {
        ActivateProgram(Program);
    }

    // At any given moment, only one program may be active
    inline void ActivateProgram(typename Traits::ProgramType program)
    {
        uint32_t const programIndex = static_cast<uint32_t>(program);

        glUseProgram(*(mPrograms[programIndex].OpenGLHandle));

        CheckOpenGLError();
    }


    // At any given moment, only one texture (unit) may be active
    template <typename Traits::ProgramParameterType Parameter>
    inline void ActivateTexture()
    {
        GLenum const textureUnit = static_cast<GLenum>(Parameter) - static_cast<GLenum>(Traits::ProgramParameterType::_FirstTexture);
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        CheckOpenGLError();
    }

private:

    template <typename Traits::ProgramType Program, typename Traits::ProgramParameterType Parameter>
    static inline void CheckUniformError()
    {
        GLenum glError = glGetError();
        if (GL_NO_ERROR != glError)
        {
            throw GameException("Error setting uniform for parameter \"" + Traits::ProgramParameterTypeToStr(Parameter) + "\" on program \"" + Traits::ProgramTypeToStr(Program) + "\"");
        }
    }

    static inline void CheckUniformError(
        typename Traits::ProgramType program,
        typename Traits::ProgramParameterType parameter)
    {
        GLenum glError = glGetError();
        if (GL_NO_ERROR != glError)
        {
            throw GameException("Error setting uniform for parameter \"" + Traits::ProgramParameterTypeToStr(parameter) + "\" on program \"" + Traits::ProgramTypeToStr(program) + "\"");
        }
    }

private:

    ShaderManager(
        std::filesystem::path const & shadersRoot);

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

    static std::set<typename Traits::ProgramParameterType> ExtractShaderParameters(std::string const & source);

    static std::set<std::string> ExtractVertexAttributeNames(std::string const & source);

private:

    struct ProgramInfo
    {
        // The OpenGL handle to the program
        GameOpenGLShaderProgram OpenGLHandle;

        // The uniform locations, indexed by shader parameter type;
        // set to NoLocation when not specified in the shader
        std::vector<GLint> UniformLocations;
    };

    // All programs, indexed by program type
    std::vector<ProgramInfo> mPrograms;

private:

    friend class ShaderManagerTests_ProcessesIncludes_OneLevel_Test;
    friend class ShaderManagerTests_ProcessesIncludes_MultipleLevels_Test;
    friend class ShaderManagerTests_ProcessesIncludes_DetectsLoops_Test;
    friend class ShaderManagerTests_ProcessesIncludes_ComplainsWhenIncludeNotFound_Test;

    friend class ShaderManagerTests_SplitsShaders_Test;
    friend class ShaderManagerTests_SplitsShaders_ErrorsOnMissingVertexSection_Test;
    friend class ShaderManagerTests_SplitsShaders_ErrorsOnMissingVertexSection_EmptyFile_Test;
    friend class ShaderManagerTests_SplitsShaders_ErrorsOnMissingFragmentSection_Test;

    friend class ShaderManagerTests_ParsesStaticParameters_Single_Test;
    friend class ShaderManagerTests_ParsesStaticParameters_Multiple_Test;
    friend class ShaderManagerTests_ParsesStaticParameters_Multiple_Test;
    friend class ShaderManagerTests_ParsesStaticParameters_ErrorsOnRepeatedParameter_Test;
    friend class ShaderManagerTests_ParsesStaticParameters_ErrorsOnMalformedDefinition_Test;

    friend class ShaderManagerTests_SubstitutesStaticParameters_Single_Test;
    friend class ShaderManagerTests_SubstitutesStaticParameters_Multiple_Different_Test;
    friend class ShaderManagerTests_SubstitutesStaticParameters_Multiple_Repeated_Test;
    friend class ShaderManagerTests_SubstitutesStaticParameters_ErrorsOnUnrecognizedParameter_Test;

    friend class ShaderManagerTests_ExtractsShaderParameters_Single_Test;
    friend class ShaderManagerTests_ExtractsShaderParameters_Multiple_Test;
    friend class ShaderManagerTests_ExtractsShaderParameters_IgnoresTrailingComment_Test;
    friend class ShaderManagerTests_ExtractsShaderParameters_IgnoresCommentedOutParameters_Test;
    friend class ShaderManagerTests_ExtractsShaderParameters_ErrorsOnUnrecognizedParameter_Test;
    friend class ShaderManagerTests_ExtractsShaderParameters_ErrorsOnRedefinedParameter_Test;

    friend class ShaderManagerTests_ExtractsVertexAttributeNames_Single_Test;
    friend class ShaderManagerTests_ExtractsVertexAttributeNames_Multiple_Test;
    friend class ShaderManagerTests_ExtractsVertexAttributeNames_ErrorsOnUnrecognizedAttribute_Test;
    friend class ShaderManagerTests_ExtractsVertexAttributeNames_ErrorsOnRedeclaredAttribute_Test;
};

#include "ShaderManager.cpp.inl"
