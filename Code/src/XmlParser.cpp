#include <iostream>
#include "XmlParser.hpp"

XmlParser::XmlParser(const std::filesystem::path &input_file_path) : input_file_path(input_file_path){
    const auto result = doc.load_file(input_file_path.c_str());
    
    if(!result) {
        //log error
        std::cerr << "XML [" << input_file_path << "] parsed with errors, attr value: [" << doc.child("node").attribute("attr").value() << "]" << std::endl;
        std::cerr << "Error description: " << result.description() << std::endl;
        std::cerr << "Error offset: " << result.offset << " (error at [..." << (input_file_path.c_str() + result.offset) << "]" << std::endl;
        
        throw std::runtime_error("XML parsing error");
    }
}

std::vector<std::filesystem::path> XmlParser::GetAllHeaders() const {
    std::vector<std::filesystem::path> headers;
    
    std::string output_stream = "Included headers:\n";
    
    for(const auto& item_group : doc.child("Project").children("ItemGroup")) {
        for(const auto& cl_include : item_group.children("ClInclude")) {
            //log file
            output_stream += (input_file_path.parent_path() / cl_include.attribute("Include").value()).string() + "\n";
            
            headers.emplace_back(input_file_path.parent_path() / cl_include.attribute("Include").value());
        }
    }
    
    std::cout << output_stream << std::endl;
    
    return headers;
}

std::vector<std::filesystem::path> XmlParser::GetAllSources() const {
    std::vector<std::filesystem::path> sources;
    
    for(const auto& item_group : doc.child("Project").children("ItemGroup")) {
        for(const auto& cl_compile : item_group.children("ClCompile")) {
            //log file
            std::cout << "Compile: " << input_file_path.parent_path() / cl_compile.attribute("Include").value() << std::endl;
            
            sources.emplace_back(input_file_path.parent_path() / cl_compile.attribute("Include").value());
        }
    }
    
    return sources;
}

std::filesystem::path XmlParser::GetDirectoryRoot() const {
    return input_file_path.parent_path();
}
