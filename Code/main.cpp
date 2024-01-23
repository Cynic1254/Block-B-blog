#include <clang/Tooling/Tooling.h>
#include <clang/Frontend/CompilerInstance.h>

#include <iostream>
#include <filesystem>

#include "XmlParser.hpp"
#include "FileParser.hpp"
#include "SimpleOptionParser.hpp"
#include "FileGenerator.hpp"

void HandleClass(FileGenerator &fileGenerator, const Class &class_);

void HandleMember(FileGenerator &fileGenerator, const Variable &variable);

void HandleMethod(FileGenerator &fileGenerator, const Function &function);

int main(int argc, char *argv[]) {
    //argc = 3;

    //run the program 1000 times to get a good average
    //for (int i = 0; i < 1000; ++i) 
    {

        if (argc < 3) {
            std::cout << "Usage: " << argv[0] << " <solution file> <output directory>" << std::endl;
            return 1;
        }

        std::cout << "Solution file: " << argv[1] << std::endl;
        const std::filesystem::path solution_file{argv[1]};
        if (!exists(solution_file) || solution_file.extension() != ".vcxproj") {
            std::cerr << "Error: " << solution_file << " is not a valid solution file" << std::endl;
            return -1;
        }

        std::cout << "Output directory: " << argv[2] << std::endl;
        const std::filesystem::path output_directory{argv[2]};
        if (!exists(output_directory) || !is_directory(output_directory)) {
            std::cerr << "Error: " << output_directory << " is not a valid directory" << std::endl;
            return -1;
        }

        XmlParser xmlParser{solution_file};
        const auto headers = xmlParser.GetAllHeaders();

        //construct command line arguments for clang
        //TODO: make this dynamic, for now they will be hard coded
        std::vector<std::string> args{};
        //clang should work on all files in the headers vector
        //clang should only create the AST, not compile the files
        //clang should only care about the provided headers
        //clang should only care about definitions, not function bodies
        //clang should use the C++17 standard
        args.emplace_back("clang");
        args.emplace_back("-fsyntax-only");
        args.emplace_back("-std=c++17");
        args.emplace_back("-IC:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.38.33130/include");

        //create a compile database
        SimpleOptionParser optionParser{args, headers};
        optionParser.SetOutputDirectory(output_directory);
        optionParser.SetInputDirectory(xmlParser.GetDirectoryRoot());

        clang::tooling::ClangTool tool{optionParser, optionParser.getAllFiles()};

        //reserve space for the FileParser
        ASTFileParser::Reserve(headers.size());

        //run the tool
        tool.run(clang::tooling::newFrontendActionFactory<ASTFrontendAction>().get());

        FileGenerator File{};
        FileGenerator::output_directory = output_directory;

        File.ParseClass = HandleClass;
        File.ParseMember = HandleMember;
        File.ParseMethod = HandleMethod;

        for (const auto &parser: ASTFileParser::GetParsers()) {
            File.Parse(parser);
        }

        FileGenerator::WriteFiles();

    }
    return 0;
}

const Class *currentClass = nullptr;

void HandleClass(FileGenerator &fileGenerator, const Class &class_) {
    currentClass = &class_;

    auto &properties = class_.properties;
    if (FileGenerator::GetProperty(properties, "LuaClass") != nullptr) {
        FileGenerator::files["LuaBindings.cpp.gen"].includes.insert(class_.path);
        auto &function = FileGenerator::files["LuaBindings.cpp.gen"].functions["CreateBindings"];
        function.header.AddVariable({"sol::state&", "lua_state"});

        auto &body = function.body;
        std::string userTypeCreation =
                "sol::usertype<" + class_.fullNamespace + "> " + class_.name + "_table = lua_state.new_usertype<" +
                class_.fullNamespace + ">(\"" + class_.name + "\", sol::constructors<" + class_.fullNamespace +
                "()>{});";

        body.emplace_back(userTypeCreation);
    }
}

void HandleMember(FileGenerator &fileGenerator, const Variable &variable) {
    auto &properties = variable.properties;
    if (FileGenerator::GetProperty(properties, "LuaInspect") != nullptr) {
        FileGenerator::files["LuaBindings.cpp.gen"].includes.insert(variable.path);
        auto &function = FileGenerator::files["LuaBindings.cpp.gen"].functions["CreateBindings"];
        function.header.AddVariable({"sol::state&", "lua_state"});

        auto &body = function.body;
        std::string propertyCreation =
                currentClass->name + "_table[\"" + variable.name + "\"] = &" + variable.fullNamespace + ";";
        body.emplace_back(propertyCreation);
    }
}

void HandleMethod(FileGenerator &fileGenerator, const Function &function) {
    auto &properties = function.properties;
    if (FileGenerator::GetProperty(properties, "LuaInspect") != nullptr) {
        FileGenerator::files["LuaBindings.cpp.gen"].includes.insert(function.path);
        auto &fullFunction = FileGenerator::files["LuaBindings.cpp.gen"].functions["CreateBindings"];
        fullFunction.header.AddVariable({"sol::state&", "lua_state"});

        auto &body = fullFunction.body;
        std::string propertyCreation =
                currentClass->name + "_table[\"" + function.name + "\"] = &" + function.fullNamespace + "();";
        body.emplace_back(propertyCreation);
    }
}