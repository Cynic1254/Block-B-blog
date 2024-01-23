#ifndef CODEGENERATOR_SIMPLEOPTIONPARSER_HPP
#define CODEGENERATOR_SIMPLEOPTIONPARSER_HPP

#include <clang/Tooling/CompilationDatabase.h>

#include <string>
#include <vector>
#include <filesystem>

class SimpleOptionParser : public clang::tooling::CompilationDatabase {
    std::vector<std::string> args;
    std::vector<std::filesystem::path> headers;
    std::filesystem::path output_directory;
    std::filesystem::path input_directory;
public:
    SimpleOptionParser() = default;
    explicit SimpleOptionParser(const std::vector<std::string>& args, const std::vector<std::filesystem::path>& headers);
    ~SimpleOptionParser() override = default;
    void SetOutputDirectory(const std::filesystem::path& output_directory);
    void SetInputDirectory(const std::filesystem::path& input_directory);
    
    [[nodiscard]] std::vector<clang::tooling::CompileCommand> getAllCompileCommands() const override;
    [[nodiscard]] std::vector<std::string> getAllFiles() const override;
    [[nodiscard]] std::vector<clang::tooling::CompileCommand> getCompileCommands(llvm::StringRef FilePath) const override;
};


#endif //CODEGENERATOR_SIMPLEOPTIONPARSER_HPP
