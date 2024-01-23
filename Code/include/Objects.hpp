#ifndef CODEGENERATOR_OBJECTS_HPP
#define CODEGENERATOR_OBJECTS_HPP

#include <string>
#include <utility>
#include <variant>
#include <vector>
#include <filesystem>
#include <unordered_map>

//enum class AccessLevel {
//    None,
//    Public,
//    Protected,
//    Private
//};

/// \brief represents a property, a property is a name and a value
/// \note value can be either a single string or a comma seperated list of properties
struct Property {
    std::string name;
    //value can be either a single string or a comma seperated list of properties
    //EG. "name"="value" or "name"={ "name"="value", "name"="value" }
    std::variant<std::string, std::vector<Property>> value;
};

/// \brief represents an object, an object is a name, a namespace, a path, and a list of properties
/// \brief all other objects inherit from this
struct Object {
    std::string name;
    std::string fullNamespace;
    std::filesystem::path path;
    std::vector<Property> properties{};
};

/// \brief represents a variable, a variable is an object with a type
struct Variable : public Object {
    std::string type = "int";
    //std::string value;
    
    //default constructor
    Variable() = default;
    
    Variable(std::string type, std::string name) :
        Object{std::move(name), "", "", {}},
        type{std::move(type)}
    {}
};

/// \brief represents a function, a function is an object with a return type and a list of parameters
struct Function : public Object {
    std::string returnType = "void";
    std::vector<Variable> parameters{};
    bool isConstruptor = false;
    
    //default constructor
    Function() = default;
    
    Function(std::string returnType, std::string name, std::vector<Variable> parameters) :
        Object{std::move(name), "", "", {}},
        returnType{std::move(returnType)},
        parameters{std::move(parameters)}
    {}
    
    void AddVariable(Variable variable) {
        //test if variable is already in parameters based on name
        for (auto& parameter : parameters) {
            if (parameter.name == variable.name) {
                return;
            }
        }
        
        parameters.emplace_back(std::move(variable));
    }
};

/// \brief represents a class, a class is an object with a list of variables and a list of functions
struct Class : public Object {
    std::vector<Variable> variables{};
    std::vector<Function> functions{};
};

#endif //CODEGENERATOR_OBJECTS_HPP
