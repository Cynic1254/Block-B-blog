#include <fstream>
#include <iostream>
#include "FileGenerator.hpp"

std::filesystem::path FileGenerator::output_directory;
std::unordered_map<std::string, File> FileGenerator::files;

void FileGenerator::WriteFiles() {
    for( const auto& file : files)
    {
        const auto output_file = output_directory / file.first;
        
        //create the directory if it doesn't exist
        std::filesystem::create_directories(output_file.parent_path());
        
        //open the file or create it if it doesn't exist
        std::ofstream output{output_file};
        //test if the file is open
        if (!output.is_open())
        {
            std::cerr << "Error: could not open " << output_file << std::endl;
            continue;
        }
        
        const auto& file_data = file.second;
        
        //write the includes
        for (const auto& include : file_data.includes)
        {
            output << GetFileInclude(include) << std::endl;
        }
        
        std::cout << std::endl;
        
        //write the header
        for (const auto& line : file_data.header)
        {
            output << line << std::endl;
        }
        
        output << std::endl;
        
        //write the functions
        for (const auto& function : file_data.functions)
        {
            //write the prefix
            if (!function.second.prefix.empty())
            {
                output << function.second.prefix;
            }
            
            //write the header
            output << function.second.header.returnType << " " << function.first << "(";
            for (size_t i = 0; i < function.second.header.parameters.size(); ++i)
            {
                output << function.second.header.parameters[i].type << " " << function.second.header.parameters[i].name;
                if (i != function.second.header.parameters.size() - 1)
                {
                    output << ", ";
                }
            }
            output << ")" << std::endl << "{" << std::endl;
            
            //write the body
            for (const auto& line : function.second.body)
            {
                output << line << std::endl;
            }
            
            output << "}" << std::endl;
            
            output << std::endl;
        }
    }
}

void FileGenerator::Parse(const ASTFileParser &parser) {
    if (ParseFile)
    {
        ParseFile(*this, parser);
    }
    
    for (const auto& function : parser.functions)
    {
        if (ParseFunction)
        {
            ParseFunction(*this, function);
        }
    }
    
    for (const auto& variable : parser.variables)
    {
        if (ParseVariable)
        {
            ParseVariable(*this, variable);
        }
    }
    
    for (const auto& class_ : parser.classes)
    {
        if (ParseClass)
        {
            ParseClass(*this, class_);
        }
        
        for (const auto& function : class_.functions)
        {
            if (ParseMethod)
            {
                ParseMethod(*this, function);
            }
        }
        
        for (const auto& variable : class_.variables)
        {
            if (ParseMember)
            {
                ParseMember(*this, variable);
            }
        }
    }
}

std::string FileGenerator::GetFileInclude(const std::filesystem::path &path) {
    //for now use absolute paths, later on this should be changed to relative paths
    return "#include \"" + path.string() + "\"";
}

std::string FileGenerator::GetFileInclude(const ASTFileParser &parser) {
    return GetFileInclude(parser.path);
}

const Property *FileGenerator::GetProperty(const std::vector<Property> &properties, const std::string &name) {
    const auto it = std::find_if(properties.begin(), properties.end(), [&name](const Property& in) { return in.name == name; });
    if (it != properties.end())
    {
        return &*it;
    }
    return nullptr;
}
