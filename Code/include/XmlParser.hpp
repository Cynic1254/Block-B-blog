#ifndef CODEGENERATOR_XMLPARSER_HPP
#define CODEGENERATOR_XMLPARSER_HPP

#include "pugixml.hpp"

#include <filesystem>
#include <vector>

class XmlParser {
    pugi::xml_document doc;
    std::filesystem::path input_file_path;
public:
    explicit XmlParser(const std::filesystem::path& input_file_path);
    
    [[nodiscard]] std::vector<std::filesystem::path> GetAllHeaders() const;
    [[nodiscard]] std::vector<std::filesystem::path> GetAllSources() const;
    [[nodiscard]] std::filesystem::path GetDirectoryRoot() const;
};

#endif //CODEGENERATOR_XMLPARSER_HPP
