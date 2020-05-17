/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-15
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShaderManager.h"

#include "Utils.h"

#include <regex>
#include <unordered_map>
#include <unordered_set>

static const std::string StaticParametersFilenameStem = "static_parameters";

ShaderManager::ShaderManager(std::filesystem::path const & shadersRoot)
{
    if (!std::filesystem::exists(shadersRoot))
        throw SLabException("Shaders root path \"" + shadersRoot.string() + "\" does not exist");

    //
    // Make static parameters
    //

    std::map<std::string, std::string> staticParameters;

    // 1) From file
    std::filesystem::path localStaticParametersFilepath = shadersRoot / (StaticParametersFilenameStem + ".glslinc");
    if (std::filesystem::exists(localStaticParametersFilepath))
    {
        std::string localStaticParametersSource = Utils::LoadTextFile(localStaticParametersFilepath);
        ParseLocalStaticParameters(localStaticParametersSource, staticParameters);
    }

    //
    // Load all shader files
    //

    // Filename -> (isShader, source)
    std::unordered_map<std::string, std::pair<bool, std::string>> shaderSources;

    for (auto const & entryIt : std::filesystem::directory_iterator(shadersRoot))
    {
        if (std::filesystem::is_regular_file(entryIt.path())
            && (entryIt.path().extension() == ".glsl" || entryIt.path().extension() == ".glslinc")
            && entryIt.path().stem() != StaticParametersFilenameStem)
        {
            std::string shaderFilename = entryIt.path().filename().string();

            assert(shaderSources.count(shaderFilename) == 0); // Guaranteed by file system

            shaderSources[shaderFilename] = std::make_pair<bool, std::string>(
                entryIt.path().extension() == ".glsl",
                Utils::LoadTextFile(entryIt.path()));
        }
    }


    //
    // Compile all shader files
    //

    for (auto const & entryIt : shaderSources)
    {
        if (entryIt.second.first)
        {
            CompileShader(
                entryIt.first,
                entryIt.second.second,
                shaderSources,
                staticParameters);
        }
    }


    //
    // Verify all expected programs have been loaded
    //

    for (uint32_t i = 0; i <= static_cast<uint32_t>(ProgramType::_Last); ++i)
    {
        if (i >= mPrograms.size() || !(mPrograms[i].OpenGLHandle))
        {
            throw SLabException("Cannot find GLSL source file for program \"" + ProgramTypeToStr(static_cast<ProgramType>(i)) + "\"");
        }
    }
}

void ShaderManager::CompileShader(
    std::string const & shaderFilename,
    std::string const & shaderSource,
    std::unordered_map<std::string, std::pair<bool, std::string>> const & shaderSources,
    std::map<std::string, std::string> const & staticParameters)
{
    try
    {
        // Get the program type
        std::filesystem::path shaderFilenamePath(shaderFilename);
        ProgramType const program = ShaderFilenameToProgramType(shaderFilenamePath.stem().string());
        std::string const programName = ProgramTypeToStr(program);

        // Resolve includes
        std::string preprocessedShaderSource = ResolveIncludes(
            shaderSource,
            shaderSources);

        // Make sure we have room for it
        size_t programIndex = static_cast<size_t>(program);
        if (programIndex + 1 > mPrograms.size())
        {
            mPrograms.resize(programIndex + 1);
        }

        // First time we see it (guaranteed by file system)
        assert(!(mPrograms[programIndex].OpenGLHandle));

        // Split the source file
        auto [vertexShaderSource, fragmentShaderSource] = SplitSource(preprocessedShaderSource);

        // Create program
        mPrograms[programIndex].OpenGLHandle = glCreateProgram();
        CheckOpenGLError();


        //
        // Compile vertex shader
        //

        vertexShaderSource = SubstituteStaticParameters(vertexShaderSource, staticParameters);

        SLabOpenGL::CompileShader(
            vertexShaderSource,
            GL_VERTEX_SHADER,
            mPrograms[programIndex].OpenGLHandle,
            programName);


        //
        // Compile fragment shader
        //

        fragmentShaderSource = SubstituteStaticParameters(fragmentShaderSource, staticParameters);

        SLabOpenGL::CompileShader(
            fragmentShaderSource,
            GL_FRAGMENT_SHADER,
            mPrograms[programIndex].OpenGLHandle,
            programName);


        //
        // Extract attribute names from vertex shader and bind them
        //

        std::set<std::string> vertexAttributeNames = ExtractVertexAttributeNames(vertexShaderSource);

        for (auto const & vertexAttributeName : vertexAttributeNames)
        {
            auto vertexAttribute = StrToVertexAttributeType(vertexAttributeName);

            SLabOpenGL::BindAttributeLocation(
                mPrograms[programIndex].OpenGLHandle,
                static_cast<GLuint>(vertexAttribute),
                "in" + vertexAttributeName);
        }


        //
        // Link
        //

        SLabOpenGL::LinkShaderProgram(mPrograms[programIndex].OpenGLHandle, programName);


        //
        // Extract uniform locations
        //

        std::vector<GLint> uniformLocations;

        auto allProgramParameters = ExtractShaderParameters(vertexShaderSource);
        auto fragmentShaderParameters = ExtractShaderParameters(fragmentShaderSource);
        allProgramParameters.merge(fragmentShaderParameters);

        for (ProgramParameterType programParameter : allProgramParameters)
        {
            // Make sure there is room
            size_t programParameterIndex = static_cast<size_t>(programParameter);
            while (mPrograms[programIndex].UniformLocations.size() <= programParameterIndex)
            {
                mPrograms[programIndex].UniformLocations.push_back(NoParameterLocation);
            }

            // Get and store
            mPrograms[programIndex].UniformLocations[programParameterIndex] = SLabOpenGL::GetParameterLocation(
                mPrograms[programIndex].OpenGLHandle,
                "param" + ProgramParameterTypeToStr(programParameter));
        }
    }
    catch (SLabException const & ex)
    {
        throw SLabException("Error compiling shader file \"" + shaderFilename + "\": " + ex.what());
    }
}

std::string ShaderManager::ResolveIncludes(
    std::string const & shaderSource,
    std::unordered_map<std::string, std::pair<bool, std::string>> const & shaderSources)
{
    static std::regex IncludeRegex(R"!(^\s*#include\s+\"\s*([_a-zA-Z0-9\.]+)\s*\"\s*$)!");

    std::unordered_set<std::string> resolvedIncludes;

    std::string resolvedSource = shaderSource;

    for (bool hasResolved = true; hasResolved; )
    {
        std::stringstream sSource(resolvedSource);
        std::stringstream sSubstitutedSource;

        hasResolved = false;

        std::string line;
        while (std::getline(sSource, line))
        {
            std::smatch match;
            if (std::regex_search(line, match, IncludeRegex))
            {
                //
                // Found an include
                //

                assert(2 == match.size());

                auto includeFilename = match[1].str();
                auto includeIt = shaderSources.find(includeFilename);
                if (includeIt == shaderSources.end())
                {
                    throw SLabException("Cannot find include file \"" + includeFilename + "\"");
                }

                if (resolvedIncludes.count(includeFilename) > 0)
                {
                    throw SLabException("Detected include file loop at include file \"" + includeFilename + "\"");
                }

                // Insert include
                sSubstitutedSource << includeIt->second.second << sSource.widen('\n');

                // Remember the files we've included in this path
                resolvedIncludes.insert(includeFilename);

                hasResolved = true;
            }
            else
            {
                sSubstitutedSource << line << sSource.widen('\n');
            }
        }

        resolvedSource = sSubstitutedSource.str();
    }

    return resolvedSource;
}

std::tuple<std::string, std::string> ShaderManager::SplitSource(std::string const & source)
{
    static std::regex VertexHeaderRegex(R"!(\s*###VERTEX\s*)!");
    static std::regex FragmentHeaderRegex(R"!(\s*###FRAGMENT\s*)!");

    std::stringstream sSource(source);

    std::string line;

    //
    // Vertex shader
    //

    // Eat blank lines
    while (true)
    {
        if (!std::getline(sSource, line))
        {
            throw SLabException("Cannot find ***VERTEX declaration");
        }

        if (!line.empty())
        {
            if (!std::regex_match(line, VertexHeaderRegex))
            {
                throw SLabException("Cannot find ***VERTEX declaration");
            }

            break;
        }
    }

    std::stringstream vertexShader;

    while (true)
    {
        if (!std::getline(sSource, line))
            throw SLabException("Cannot find ***FRAGMENT declaration");

        if (std::regex_match(line, FragmentHeaderRegex))
            break;

        vertexShader << line << sSource.widen('\n');
    }


    //
    // Fragment shader
    //

    std::stringstream fragmentShader;

    while (std::getline(sSource, line))
    {
        fragmentShader << line << sSource.widen('\n');
    }


    return std::make_tuple(vertexShader.str(), fragmentShader.str());
}

void ShaderManager::ParseLocalStaticParameters(
    std::string const & localStaticParametersSource,
    std::map<std::string, std::string> & staticParameters)
{
    static std::regex StaticParamDefinitionRegex(R"!(^\s*([_a-zA-Z][_a-zA-Z0-9]*)\s*=\s*(.*?)\s*$)!");

    std::stringstream sSource(localStaticParametersSource);
    std::string line;
    while (std::getline(sSource, line))
    {
        line = Utils::Trim(line);

        if (!line.empty())
        {
            std::smatch match;
            if (!std::regex_search(line, match, StaticParamDefinitionRegex))
            {
                throw SLabException("Error parsing static parameter definition \"" + line + "\"");
            }

            assert(3 == match.size());
            auto staticParameterName = match[1].str();
            auto staticParameterValue = match[2].str();

            // Check whether it's a dupe
            if (staticParameters.count(staticParameterName) > 0)
            {
                throw SLabException("Static parameters \"" + staticParameterName + "\" has already been defined");
            }

            // Store
            staticParameters.insert(
                std::make_pair(
                    staticParameterName,
                    staticParameterValue));
        }
    }
}

std::string ShaderManager::SubstituteStaticParameters(
    std::string const & source,
    std::map<std::string, std::string> const & staticParameters)
{
    static std::regex StaticParamNameRegex("%([_a-zA-Z][_a-zA-Z0-9]*)%");

    std::string remainingSource = source;
    std::stringstream sSubstitutedSource;
    std::smatch match;
    while (std::regex_search(remainingSource, match, StaticParamNameRegex))
    {
        assert(2 == match.size());
        auto staticParameterName = match[1].str();

        // Lookup the parameter
        auto const & paramIt = staticParameters.find(staticParameterName);
        if (paramIt == staticParameters.end())
        {
            throw SLabException("Static parameter \"" + staticParameterName + "\" is not recognized");
        }

        // Substitute the parameter
        sSubstitutedSource << match.prefix();
        sSubstitutedSource << paramIt->second;

        // Advance
        remainingSource = match.suffix();
    }

    sSubstitutedSource << remainingSource;

    return sSubstitutedSource.str();
}

std::set<ShaderManager::ProgramParameterType> ShaderManager::ExtractShaderParameters(std::string const & source)
{
    static std::regex ShaderParamNameRegex(R"!(^\s*(//\s*)?\buniform\s+.*\s+param([_a-zA-Z0-9]+);\s*(?://.*)?$)!");

    std::set<ProgramParameterType> shaderParameters;

    std::stringstream sSource(source);
    std::string line;
    std::smatch match;
    while (std::getline(sSource, line))
    {
        if (std::regex_match(line, match, ShaderParamNameRegex))
        {
            assert(3 == match.size());
            if (!match[1].matched) // Not a comment
            {
                auto const & shaderParameterName = match[2].str();

                // Lookup the parameter
                ProgramParameterType shaderParameter = StrToProgramParameterType(shaderParameterName);

                // Store it, making sure it's not specified more than once
                if (!shaderParameters.insert(shaderParameter).second)
                {
                    throw SLabException("Shader parameter \"" + shaderParameterName + "\" is declared more than once");
                }
            }
        }
    }

    return shaderParameters;
}

std::set<std::string> ShaderManager::ExtractVertexAttributeNames(std::string const & source)
{
    static std::regex AttributeNameRegex(R"!(\bin\s+.*?\s+in([_a-zA-Z][_a-zA-Z0-9]*);)!");

    std::set<std::string> attributeNames;

    std::string remainingSource = source;
    std::smatch match;
    while (std::regex_search(remainingSource, match, AttributeNameRegex))
    {
        assert(2 == match.size());
        auto const & attributeName = match[1].str();

        // Lookup the attribute name - just as a sanity check
        StrToVertexAttributeType(attributeName);

        // Store it, making sure it's not specified more than once
        if (!attributeNames.insert(attributeName).second)
        {
            throw SLabException("Attribute name \"" + attributeName + "\" is declared more than once");
        }

        // Advance
        remainingSource = match.suffix();
    }

    return attributeNames;
}

ShaderManager::ProgramType ShaderManager::ShaderFilenameToProgramType(std::string const & str)
{
    if (Utils::CaseInsensitiveEquals(str, "Points"))
        return ProgramType::Points;
    else if (Utils::CaseInsensitiveEquals(str, "Springs"))
        return ProgramType::Springs;
    else
        throw SLabException("Unrecognized program \"" + str + "\"");
}

std::string ShaderManager::ProgramTypeToStr(ProgramType program)
{
    switch (program)
    {
        case ProgramType::Points:
            return "Points";
        case ProgramType::Springs:
            return "Springs";
        default:
            assert(false);
            throw SLabException("Unsupported ProgramType");
    }
}

ShaderManager::ProgramParameterType ShaderManager::StrToProgramParameterType(std::string const & str)
{
    if (str == "OrthoMatrix")
        return ProgramParameterType::OrthoMatrix;
    else
        throw SLabException("Unrecognized program parameter \"" + str + "\"");
}

std::string ShaderManager::ProgramParameterTypeToStr(ProgramParameterType programParameter)
{
    switch (programParameter)
    {
        case ProgramParameterType::OrthoMatrix:
            return "OrthoMatrix";
        default:
            assert(false);
            throw SLabException("Unsupported ProgramParameterType");
    }
}

ShaderManager::VertexAttributeType ShaderManager::StrToVertexAttributeType(std::string const & str)
{
    if (Utils::CaseInsensitiveEquals(str, "PointAttributeGroup1"))
        return VertexAttributeType::PointAttributeGroup1;
    else if (Utils::CaseInsensitiveEquals(str, "PointAttributeGroup2"))
        return VertexAttributeType::PointAttributeGroup2;
    else if (Utils::CaseInsensitiveEquals(str, "PointAttributeGroup3"))
        return VertexAttributeType::PointAttributeGroup3;
    else if (Utils::CaseInsensitiveEquals(str, "SpringAttributeGroup1"))
        return VertexAttributeType::SpringAttributeGroup1;
    else if (Utils::CaseInsensitiveEquals(str, "SpringAttributeGroup2"))
        return VertexAttributeType::SpringAttributeGroup2;
    else if (Utils::CaseInsensitiveEquals(str, "SpringAttributeGroup3"))
        return VertexAttributeType::SpringAttributeGroup3;
    else
        throw SLabException("Unrecognized vertex attribute \"" + str + "\"");
}