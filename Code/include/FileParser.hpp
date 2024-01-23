#ifndef CODEGENERATOR_FILEPARSER_HPP
#define CODEGENERATOR_FILEPARSER_HPP

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/FrontendActions.h>

#include <utility>
#include <vector>
#include <stack>
#include <fstream>

#include "Objects.hpp"

/// \brief parses a file and stores the data in the objects
/// \note this class serves both as a parser and a container for the data
class ASTFileParser : public clang::RecursiveASTVisitor<ASTFileParser> {
    static std::vector<ASTFileParser> parsers;
    int skipped_parameters = 0;

    friend class ASTConsumer;
    friend class FileGenerator;

    std::vector<Class> classes;
    std::vector<Function> functions;
    std::vector<Variable> variables;

    std::stack<size_t> classStack;
    std::stack<Function> functionStack;

    std::ifstream file;
    
    std::filesystem::path path;

    /// \brief parses the properties of the decl
    /// \param decl decl to parse properties for
    /// \return vector of properties
    /// \note Properties should be in the form of a comma separated list, A property is defined as follows:
    /// \note [name]=[value] (value is optional and will be empty if not provided)
    /// \note A property list should be directly above the decl and may not span multiple lines
    std::vector<Property> GetProperties(const clang::Decl &decl, std::string keyword = "CGCLASS");

    /// \brief returns the line above the decl
    /// \param decl decl to get the line above for
    /// \return line above the decl or empty string if decl starts at the beginning of the file
    std::string GetLineAbove(const clang::Decl &decl);

    /// \brief returns the type of the decl as a string exactly as it appears in the source code
    /// \param type decl of the type to get
    /// \return type as a string
    static std::string GetTypeAsString(const clang::NamedDecl &type);

    bool DeclIsIncluded(const clang::Decl &decl);
public:
    explicit ASTFileParser(std::filesystem::path file) : path(std::move(file)) {};

    static void Reserve(size_t size) {
        parsers.reserve(size);
    };

    //get a read only reference to the parsers
    static const std::vector<ASTFileParser> &GetParsers() {
        return parsers;
    };
    
    //Traversal methods, each method handles a specific type of decl and parses it
    //CXXRecordDecl is a class, struct, or union
    //CXXConstructorDecl is a constructor, however internally this will be handled as a function
    //FieldDecl is a field, this is a variable inside a class
    //CXXMethodDecl is a method, this is a function inside a class
    //ParmVarDecl is a parameter, this is a variable inside a function
    //VarDecl is a variable, this is a variable inside a function or global variable
    //FunctionDecl is a function, this is a function outside a class

    bool TraverseCXXRecordDecl(clang::CXXRecordDecl *decl);

    bool TraverseCXXConstructorDecl(clang::CXXConstructorDecl *decl);

    bool TraverseFieldDecl(clang::FieldDecl *decl);

    bool TraverseCXXMethodDecl(clang::CXXMethodDecl *decl);

    bool TraverseParmVarDecl(clang::ParmVarDecl *decl);

    bool TraverseVarDecl(clang::VarDecl *decl);

    bool TraverseFunctionDecl(clang::FunctionDecl *decl);
};

//implement consumer
class ASTConsumer : public clang::ASTConsumer {
public:
    void HandleTranslationUnit(clang::ASTContext &context) override;
};

//implement frontend action
class ASTFrontendAction : public clang::SyntaxOnlyAction {
public:
    std::unique_ptr<clang::ASTConsumer>
    CreateASTConsumer(clang::CompilerInstance &compilerInstance, llvm::StringRef inFile) override;
};

#endif //CODEGENERATOR_FILEPARSER_HPP
