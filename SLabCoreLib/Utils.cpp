/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2020-05-15
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Utils.h"

#include "SLabException.h"

#include <cassert>
#include <memory>
#include <regex>

namespace /* anonymous */ {

    std::string RemoveJSONComments(std::string const & source)
    {
        std::stringstream sSource(source);
        std::stringstream sSubstitutedSource;
        std::string line;
        while (std::getline(sSource, line))
        {
            auto commentStart = line.find("//");
            if (commentStart != std::string::npos)
            {
                sSubstitutedSource << line.substr(0, commentStart);
            }
            else
            {
                sSubstitutedSource << line;
            }
        }

        return sSubstitutedSource.str();
    }
}

picojson::value Utils::ParseJSONFile(std::filesystem::path const & filepath)
{
	std::string fileContents = RemoveJSONComments(Utils::LoadTextFile(filepath));

    return ParseJSONString(fileContents);
}

picojson::value Utils::ParseJSONStream(std::istream const & stream)
{
    std::string fileContents = RemoveJSONComments(Utils::LoadTextStream(stream));

    return ParseJSONString(fileContents);
}

picojson::value Utils::ParseJSONString(std::string const & jsonString)
{
    picojson::value jsonContent;
    std::string parseError = picojson::parse(jsonContent, jsonString);
    if (!parseError.empty())
    {
        throw SLabException("Error parsing JSON string: " + parseError);
    }

    return jsonContent;
}

void Utils::SaveJSONFile(
    picojson::value const & value,
    std::filesystem::path const & filepath)
{
    std::string serializedJson = value.serialize(true);

    Utils::SaveTextFile(
        serializedJson,
        filepath);
}