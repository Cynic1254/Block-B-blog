#include <iostream>
#include "SimpleOptionParser.hpp"

SimpleOptionParser::SimpleOptionParser(const std::vector<std::string> &args,
                                       const std::vector<std::filesystem::path> &headers) : args(args),
                                                                                            headers(headers) {}

std::vector<clang::tooling::CompileCommand> SimpleOptionParser::getAllCompileCommands() const {
    //create a CompileCommand for each header, each file will have the same arguments
    std::vector<clang::tooling::CompileCommand> commands{};
    commands.reserve(headers.size());
    
    //std::cout << "Compile Commands called for all files" << std::endl;

    for (const auto &header: headers) {
        //append the header to the newArgs
        std::vector<std::string> newArgs{this->args};
        newArgs.emplace_back(header.string());
        commands.emplace_back(header.parent_path().string(), header.filename().string(), newArgs, "");
    }

    return commands;
}

std::vector<std::string> SimpleOptionParser::getAllFiles() const {
    std::vector<std::string> files{};
    files.reserve(headers.size());

    for (const auto &header: headers) {
        files.emplace_back(header.string());
    }

    return files;
}

std::vector<clang::tooling::CompileCommand> SimpleOptionParser::getCompileCommands(llvm::StringRef FilePath) const {
    //create a compile command for the header and return it inside a vector
    
    //std::cout << "Compile Commands called for: " << FilePath.str() << std::endl;
    
    //find file parent path
    std::filesystem::path file{FilePath.str()};
    
    std::vector<std::string> newArgs{this->args};
    newArgs.emplace_back(FilePath.str());
    
    std::vector<clang::tooling::CompileCommand> commands{
            clang::tooling::CompileCommand(file.parent_path().string(), file.filename().string(), newArgs, output_directory.string() + "/" + FilePath.str())
    };
    return commands;
}

void SimpleOptionParser::SetOutputDirectory(const std::filesystem::path &output_directory) {
    this->output_directory = output_directory;
}

void SimpleOptionParser::SetInputDirectory(const std::filesystem::path &input_directory) {
    this->input_directory = input_directory;
}
