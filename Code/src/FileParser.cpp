#include "FileParser.hpp"
#include "FileGenerator.hpp"

#include <iostream>
#include <regex>

std::vector<ASTFileParser> ASTFileParser::parsers{};

bool ASTFileParser::TraverseCXXRecordDecl(clang::CXXRecordDecl *decl) {

    //test if decl is from current file, if not skip recursive traversal and continue to next decl
    if (DeclIsIncluded(*decl)) {
        //std::cout << "Skipping: " << decl->getQualifiedNameAsString() << std::endl;
        return true;
    }

    auto properties = GetProperties(*decl, "CGCLASS");
    //if properties is empty skip recursive traversal and continue to next decl
    if (properties.empty()) {
        return true;
    }

    //handle setup here:
    {
        auto &Class = classes.emplace_back();
        classStack.push(classes.size() - 1);

        Class.fullNamespace = decl->getQualifiedNameAsString();
        Class.name = decl->getNameAsString();
//        auto& sourceManager = decl->getASTContext().getSourceManager();
        Class.path = path/*sourceManager.getFileEntryRefForID(
                sourceManager.getFileID(decl->getLocation())
                )->getName().str()*/;
        Class.properties = properties;
    }

    bool result = clang::RecursiveASTVisitor<ASTFileParser>::TraverseCXXRecordDecl(decl);

    //handle teardown here:
    {
        classStack.pop();
    }

    return result;
}

bool ASTFileParser::TraverseFieldDecl(clang::FieldDecl *decl) {
    //test if decl is from current file, if not skip recursive traversal and continue to next decl
    if (DeclIsIncluded(*decl)) {
        //std::cout << "Skipping: " << decl->getQualifiedNameAsString() << std::endl;
        return true;
    }

    auto properties = GetProperties(*decl, "CGMEMBER");
    //if properties is empty skip recursive traversal and continue to next decl
    if (properties.empty()) {
        return true;
    }

    auto &Class = classes[classStack.top()];
    auto &var = Class.variables.emplace_back();

    var.fullNamespace = decl->getQualifiedNameAsString();
    var.name = decl->getNameAsString();
//    auto& sourceManager = decl->getASTContext().getSourceManager();
    var.path = path/*sourceManager.getFileEntryRefForID(
            sourceManager.getFileID(decl->getLocation())
    )->getName().str()*/;

    var.type = GetTypeAsString(*decl);
    var.properties = properties;

    return clang::RecursiveASTVisitor<ASTFileParser>::TraverseFieldDecl(decl);
}

bool ASTFileParser::TraverseCXXMethodDecl(clang::CXXMethodDecl *decl) {
    //test if decl is from current file, if not skip recursive traversal and continue to next decl
    if (DeclIsIncluded(*decl)) {
        //std::cout << "Skipping: " << decl->getQualifiedNameAsString() << std::endl;
        return true;
    }

    auto properties = GetProperties(*decl, "CGMETHOD");
    //if properties is empty skip recursive traversal and continue to next decl
    if (properties.empty()) {
        return true;
    }

    //handle setup here:
    {
        auto &Function = functionStack.emplace();
        Function.fullNamespace = decl->getQualifiedNameAsString();
        Function.name = decl->getNameAsString();
//        auto& sourceManager = decl->getASTContext().getSourceManager();
        Function.path = path/*sourceManager.getFileEntryRefForID(
                sourceManager.getFileID(decl->getLocation())
        )->getName().str()*/;

        Function.returnType = GetTypeAsString(*decl);
        Function.properties = properties;
    }

    bool result = clang::RecursiveASTVisitor<ASTFileParser>::TraverseCXXMethodDecl(decl);

    //handle teardown here:
    {
        classes[classStack.top()].functions.emplace_back(std::move(functionStack.top()));
        functionStack.pop();
    }

    return result;
}

bool ASTFileParser::TraverseParmVarDecl(clang::ParmVarDecl *decl) {
    //test if decl is from current file, if not skip recursive traversal and continue to next decl
    if (DeclIsIncluded(*decl)) {
        //std::cout << "Skipping: " << decl->getQualifiedNameAsString() << std::endl;
        return true;
    }

    if (!functionStack.empty()) {
        auto &Function = functionStack.top();
        auto &var = Function.parameters.emplace_back();

        var.fullNamespace = decl->getQualifiedNameAsString();
        var.name = decl->getNameAsString();
//        auto& sourceManager = decl->getASTContext().getSourceManager();
        var.path = path/*sourceManager.getFileEntryRefForID(
                sourceManager.getFileID(decl->getLocation())
        )->getName().str()*/;

        var.type = GetTypeAsString(*decl);
    } else {
        skipped_parameters++;
        //print warning
//        std::cout << "Warning: Parameter outside of function\n"
//                  << "Line: " << decl->getASTContext().getSourceManager().getSpellingLineNumber(decl->getLocation()) << '\n'
//                  << "This is likely due to a function pointer" << std::endl;
    }
    //print line number

    return clang::RecursiveASTVisitor<ASTFileParser>::TraverseParmVarDecl(decl);
}

std::string ASTFileParser::GetTypeAsString(const clang::NamedDecl &type) {
    //get file location
    //get start and ending character
    clang::SourceRange range = type.getSourceRange();
    clang::SourceManager &sm = type.getASTContext().getSourceManager();

    llvm::StringRef source = clang::Lexer::getSourceText(clang::CharSourceRange::getCharRange(range), sm,
                                                         clang::LangOptions());

    std::string name = type.getNameAsString();
    //find string before the first space before name
    std::string typeString = source.str().substr(0, source.str().rfind(' ', source.str().find(name)));
    //remove "static" from the string
    std::regex staticRegex(R"(static\s+)");
    typeString = std::regex_replace(typeString, staticRegex, "");

    return typeString;
}

bool ASTFileParser::TraverseCXXConstructorDecl(clang::CXXConstructorDecl *decl) {
    //test if decl is from current file, if not skip recursive traversal and continue to next decl
    if (DeclIsIncluded(*decl)) {
        //std::cout << "Skipping: " << decl->getQualifiedNameAsString() << std::endl;
        return true;
    }

    auto properties = GetProperties(*decl, "CGCONSTRUCTOR");
    //if properties is empty skip recursive traversal and continue to next decl
    if (properties.empty()) {
        return true;
    }

    //handle setup here:
    {
        auto &Function = functionStack.emplace();
        Function.fullNamespace = decl->getQualifiedNameAsString();
        Function.name = decl->getNameAsString();
//        auto& sourceManager = decl->getASTContext().getSourceManager();
        Function.path = path/*sourceManager.getFileEntryRefForID(
                sourceManager.getFileID(decl->getLocation())
        )->getName().str()*/;

        Function.returnType = "void";
        Function.isConstruptor = true;
        Function.properties = properties;
    }

    bool result = clang::RecursiveASTVisitor<ASTFileParser>::TraverseCXXConstructorDecl(decl);

    //handle teardown here:
    {
        classes[classStack.top()].functions.emplace_back(std::move(functionStack.top()));
        functionStack.pop();
    }

    return result;
}

bool ASTFileParser::TraverseVarDecl(clang::VarDecl *decl) {
    //test if decl is from current file, if not skip recursive traversal and continue to next decl
    if (DeclIsIncluded(*decl)) {
        //std::cout << "Skipping: " << decl->getQualifiedNameAsString() << std::endl;
        return true;
    }

    auto properties = GetProperties(*decl, "CGVARIABLE");
    //if properties is empty skip recursive traversal and continue to next decl
    if (properties.empty()) {
        return true;
    }

    auto &var = variables.emplace_back();

    var.fullNamespace = decl->getQualifiedNameAsString();
    var.name = decl->getNameAsString();
//    auto& sourceManager = decl->getASTContext().getSourceManager();
    var.path = path/*sourceManager.getFileEntryRefForID(
            sourceManager.getFileID(decl->getLocation())
    )->getName().str()*/;

    var.type = GetTypeAsString(*decl);
    var.properties = properties;

    return clang::RecursiveASTVisitor<ASTFileParser>::TraverseVarDecl(decl);
}

bool ASTFileParser::TraverseFunctionDecl(clang::FunctionDecl *decl) {
    //test if decl is from current file, if not skip recursive traversal and continue to next decl
    if (DeclIsIncluded(*decl)) {
        //std::cout << "Skipping: " << decl->getQualifiedNameAsString() << std::endl;
        return true;
    }

    auto properties = GetProperties(*decl, "CGFUNCTION");
    //if properties is empty skip recursive traversal and continue to next decl
    if (properties.empty()) {
        return true;
    }

    //handle setup here:
    {
        auto &Function = functionStack.emplace();
        Function.fullNamespace = decl->getQualifiedNameAsString();
        Function.name = decl->getNameAsString();
//        auto& sourceManager = decl->getASTContext().getSourceManager();
        Function.path = path/*sourceManager.getFileEntryRefForID(
                sourceManager.getFileID(decl->getLocation())
        )->getName().str()*/;

        Function.returnType = GetTypeAsString(*decl);
        Function.properties = properties;
    }

    bool result = clang::RecursiveASTVisitor<ASTFileParser>::TraverseFunctionDecl(decl);

    //handle teardown here:
    {
        functions.emplace_back(std::move(functionStack.top()));
        functionStack.pop();
    }

    return result;
}

std::string ASTFileParser::GetLineAbove(const clang::Decl &decl) {
    //get the line number of the declaration
    size_t lineNumber = decl.getASTContext().getSourceManager().getSpellingLineNumber(decl.getLocation());

    //test if line number is 1, if so return empty string
    if (lineNumber == 1) {
        return {};
    }

    //get the file
    std::string fileName = decl.getASTContext().getSourceManager().getFilename(decl.getLocation()).str();

    //test if the file is open, if not open it
    if (!file.is_open() || file.rdstate() != std::ifstream::goodbit) {
        file.open(fileName);
    }

    //set cursor to the beginning of the file
    file.seekg(std::ios::beg);
    //set cursor to the line before the declaration
    for (size_t i = 0; i < lineNumber - 2; ++i) {
        file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    //get the line
    std::string line;
    std::getline(file, line);

    //close the file
    file.close();

    //return the line
    return line;
}

//TODO: Function does not handle list of properties as value
std::vector<Property> ASTFileParser::GetProperties(const clang::Decl &decl, std::string keyword) {
    auto line = GetLineAbove(decl);

    //test if the line starts with the keyword (ignoring whitespace)
    //if not return empty vector
    //remove leading whitespace
    line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](int ch) { return !std::isspace(ch); }));
    if (line.find(keyword) != 0) {
//        auto *namedDecl = const_cast<clang::NamedDecl *>(clang::dyn_cast<clang::NamedDecl>(&decl));
//        std::cerr << "No properties found for:" <<  namedDecl->getQualifiedNameAsString() << std::endl;
        return {};
    }

    //find the first opening brace
    auto openBracePos = line.find('(');
    //find matching closing brace
    auto closeBracePos = line.find(')', openBracePos);

    //get the substring between the braces
    auto propertiesString = line.substr(openBracePos + 1, closeBracePos - openBracePos - 1);

    //split the string by commas
    std::vector<std::string> properties{};
    std::string::size_type pos = 0;
    std::string::size_type prev = 0;
    while ((pos = propertiesString.find(',', pos)) != std::string::npos) {
        properties.emplace_back(propertiesString.substr(prev, pos - prev));
        prev = ++pos;
    }

    //add the last property
    properties.emplace_back(propertiesString.substr(prev, pos - prev));

    //parse the properties.
    //properties should be in the format "name"="value".
    //value is optional, and if it is not present the value should be an empty string.
    //TODO: Value can also be a comma seperated list of properties
    std::vector<Property> result{{keyword}};
    for (const auto &property: properties) {
        //find the first equals sign
        auto equalsPos = property.find('=');

        //get the name
        auto name = property.substr(0, equalsPos);

        //get the value
        auto value = property.substr(equalsPos + 1);

        //remove whitespace from the name
        name.erase(std::remove_if(name.begin(), name.end(), [](char c) { return std::isspace(c); }), name.end());
        //remove whitespace from the value
        value.erase(std::remove_if(value.begin(), value.end(), [](char c) { return std::isspace(c); }), value.end());

        //add the property to the result
        result.emplace_back(Property{name, value});
    }

    return result;
}

bool ASTFileParser::DeclIsIncluded(const clang::Decl &decl) {
    const auto &SourceManager = decl.getASTContext().getSourceManager();
    const auto FileEntryRef = SourceManager.getFileEntryRefForID(
            SourceManager.getFileID(decl.getLocation())
    );
    return FileEntryRef.has_value() && FileEntryRef->getName().str() != path;
}

std::unique_ptr<clang::ASTConsumer>
ASTFrontendAction::CreateASTConsumer(clang::CompilerInstance &compilerInstance, llvm::StringRef inFile) {
    std::cout << "Parsing file: " << inFile.str() << std::endl;
    return std::make_unique<ASTConsumer>();
}

void ASTConsumer::HandleTranslationUnit(clang::ASTContext &context) {
    auto path = context.getSourceManager().getFileEntryRefForID(context.getSourceManager().getMainFileID())->getName();

    ASTFileParser parser{path.str()};

    parser.TraverseDecl(context.getTranslationUnitDecl());
    
    std::cout << "Skipped " << parser.skipped_parameters << " parameters, This is likely due to a function pointer" << std::endl;

    //test if parser has any classes, variables, or functions
    if (parser.classes.empty() && parser.variables.empty() && parser.functions.empty()) {
        return;
    }

    ASTFileParser::parsers.emplace_back(std::move(parser));
}
