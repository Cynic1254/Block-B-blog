#ifndef CODEGENERATOR_FILEGENERATOR_HPP
#define CODEGENERATOR_FILEGENERATOR_HPP

#include "Objects.hpp"
#include "FileParser.hpp"

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <functional>

struct FullFunction {
    //allow the user to specify a string which will be inserted before the function
    //this approach allows for easy creation of template functions as the prefix can be used to specify the template
    //afterward the user can use the function name to specify the template arguments
    std::string prefix;
    
    Function header;
    
    std::vector<std::string> body;
};

struct File  {
    std::unordered_set<std::filesystem::path> includes;
    
    std::vector<std::string> header;
    
    std::unordered_map<std::string, FullFunction> functions;
};

class FileGenerator {
public:
    static std::filesystem::path output_directory;
    static std::unordered_map<std::string, File> files;
    
    /// \brief The callback function that gets called whenever a new ASTFileParser gets parsed
    /// \param fileGenerator the FileGenerator that called this function
    /// \param parser the ASTFileParser that was parsed
    std::function<void(FileGenerator&, const ASTFileParser&)> ParseFile{};
    
    /// \brief The callback function that gets called whenever a new Class gets parsed
    /// \param fileGenerator the FileGenerator that called this function
    /// \param classes the Class that was parsed
    std::function<void(FileGenerator&, const Class& classes)> ParseClass{};
    
    /// \brief The callback function that gets called whenever a new Member variable gets parsed
    /// \param fileGenerator the FileGenerator that called this function
    /// \param variables the Member variable that was parsed
    std::function<void(FileGenerator&, const Variable& variables)> ParseMember{};
    
    /// \brief The callback function that gets called whenever a new Method gets parsed
    /// \param fileGenerator the FileGenerator that called this function
    /// \param functions the Method that was parsed
    std::function<void(FileGenerator&, const Function& functions)> ParseMethod{};
    
    /// \brief The callback function that gets called whenever a new variable gets parsed
    /// \param fileGenerator the FileGenerator that called this function
    /// \param variables the variable that was parsed
    /// \note this function is called for global variables, for member variables see ParseMember
    std::function<void(FileGenerator&, const Variable& variables)> ParseVariable{};
    
    /// \brief The callback function that gets called whenever a new function gets parsed
    /// \param fileGenerator the FileGenerator that called this function
    /// \param functions the function that was parsed
    /// \note this function is called for global functions, for member functions see ParseMethod
    std::function<void(FileGenerator&, const Function& functions)> ParseFunction{};
    
    /// \brief The parser function responsible for calling the callback functions
    /// \param parser the ASTFileParser to parse
    void Parse(const ASTFileParser& parser);
    
    /// \brief parses the provided ASTFileParser and returns the #include macro that will include the file
    /// \param parser the ASTFileParser to get the path from
    /// \return string in the form of #include "path"
    /// \note this function returns absolute paths so using this outside generated files will not work for others
    static std::string GetFileInclude(const ASTFileParser& parser);
    
    /// \brief returns the #include macro that will include the file based on the provided path
    /// \param path path to get the #include macro for
    /// \return string in the form of #include "path"
    static std::string GetFileInclude(const std::filesystem::path& path);
    
    /// \brief Get the property with the provided name from the provided properties
    /// \param properties vector of properties to search in
    /// \param name name of the property to search for
    /// \return pointer to the property if found, nullptr otherwise
    static const Property* GetProperty(const std::vector<Property>& properties, const std::string& name);
    
    /// \brief Write all files to disk
    /// \note function names are based on their keys in the files map, not their name in the variable
    /// \code{.cpp} files["file.cpp"].functions["Name1] = {"void", "Name2", {}} \endcode
    /// \note will have the name: Name1 in file.cpp
    static void WriteFiles();
};

#endif //CODEGENERATOR_FILEGENERATOR_HPP
