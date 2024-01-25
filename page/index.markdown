---
# Feel free to add content and custom Front Matter to this file.
# To modify the layout, see https://jekyllrb.com/docs/themes/#overriding-theme-defaults

layout: default
---

# Using Clang for static reflection

## What
Static reflection is a key component to writing dynamic code which is easily integrated into systems. However, despite this there are few solutions for drag and drop static reflection out there. My aim for this project was to fill this hole with an easy to integrate console-based application which will parse c++ files and generate code based on the symbols in them.

### Inspiration
For this project I took a lot of inspiration from Unreal Engine's reflection system. Unreal Engine's reflection system has a nice way of handling reflection where the user doesn't need to worry about adding their classes to specific functions or files so that the code is able to run it, Instead the user can use macros to describe what they want to happen with their code:

```cpp
UCLASS()
class LEARN_UE5_API UEchoAnimInstance : public UAnimInstance
{
  GENERATED_BODY()
public:
  virtual void NativeInitializeAnimation() override;
  virtual void NativeUpdateAnimation(float DeltaSeconds) override;

  UPROPERTY(BlueprintReadOnly)
    AEcho* Character;

  UPROPERTY(BlueprintReadOnly, Category = "Movement")
  UCharacterMovementComponent* EchoMovement;

  UPROPERTY(BlueprintReadOnly, Category = "Movement")
  float GroundSpeed;

  UPROPERTY(BlueprintReadOnly, Category = "Movement")
  bool IsFalling;

  UPROPERTY(BlueprintReadOnly, Category = "Movement | CharacterState")
  ECharacterState CharacterState;
};
```

In this snippet the class is marked with the `UCLASS()` property which is detected by unreal and the class is added to it's reflection system. Variables are marked with the `UPROPERTY()` macro and additional settings are passed as parameters, these settings tell unreal that the variables cannot be edited by blueprints and when inspected they will be under the Movement category.

My aim was to make a small console-based application that the user can invoke just before building their c/c++ application that will generate files based on properties specified by the user like how unreal engine's properties work. The difference in my software will be that the user is able to control what gets generated based on what properties.

as an example:
```cpp
CGCLASS(LuaClass)
class Foo
{
    CGMEMBER(LuaInspect)
    std::unordered_map<std::string, std::shared_ptr<Bar>> Var;
public:
   CGMETHOD(LuaInspect)
    void Func(const std::shared_ptr<Bar>& bar);
};
```
generates:
```cpp
#include "C:\Users\games\source\github\Code-Generation\Code_Generation\include\Linker.hpp"

void CreateBindings(sol::state& lua_state)
{
sol::usertype<Foo> Foo_table = lua_state.new_usertype<Foo>("Foo", sol::constructors<Foo()>{});
Foo_table["Func"] = &Foo::Func();
Foo_table["Var"] = &Foo::Var;
}
```
In this example you can see a small input class which is marked with Macro's, these macros tell the program that this class and these members should be remembered. Additionally, parameters have been provided to mark the class as being accessible in Lua. The program sees these macros and parameters and automatically generates the output file based on them. The output file contains a function to create bindings for Lua.

## Why
Static reflection or reflection in general is a key component to software development and especially important for (game) engines where users should get as much feedback as possible and be able to do as much as possible with as little effort as possible.

Imagine you have a small Entity Component System (ECS) based engine you've made, and you want to allow users to create components themselves. This of course isn't that difficult; you simply expose part of your API that provides helper functions for components and voila we have a simple component.
```cpp
struct Physics
{
    glm::vec3 velocity{0.0f};
    glm::vec3 gravity{0.0f};
    glm::vec3 max_velocity{0.0f};
    bool has_gravity = true;
};
```
The above code showcases a simple Physics component that could be made using a Data Oriented Design (DOD) approach. simple enough right?

Now imagine the user creates an entity and adds the Physics component to it. The user runs the program and... the entity flies in the wrong direction. To fix this the user must edit their code and recompile the program which takes time. Instead, it would be cool if the user is able to inspect the entity and edit the values of the Physics component during runtime.

Of course, you already thought of this since you're such an amazing programmer and already implemented an inspector class. One problem though, for the inspector to know what it should display it has to know about the class. Not a big problem, right? you can just create a template that the user can specialize to describe how the inspector should display the class. Of course, the component also needs to be registered so let’s add a function which the user must call during startup to tell the engine about the component.
```cpp
struct Physics
{
    glm::vec3 velocity{0.0f};
    glm::vec3 gravity{0.0f};
    glm::vec3 max_velocity{0.0f};
    bool has_gravity = true;
};

template<>
void ComponentEditorWidget<components::Physics>(entt::registry& registry, entt::entity entity)
{
    auto& physics = registry.get<components::Physics>(entity);

    //velocity
    ImGui::DragFloat3("Velocity", glm::value_ptr(physics.velocity), 0.1f);
    //gravity
    ImGui::DragFloat3("Gravity", glm::value_ptr(physics.gravity), 0.1f);
    //max velocity
    ImGui::DragFloat3("Max Velocity", glm::value_ptr(physics.max_velocity), 0.1f);
    //has gravity
    ImGui::Checkbox("Has Gravity", &physics.has_gravity);
};

void EngineClass::Initialize()
{
  ecs_->RegisterComponent<components::Physics>("Physics");
}
```
Suddenly adding a new component leaves a lot of work up to the user and a lot of files that the user must edit. Meanwhile in game engines whenever someone makes a new component all this code that is required to make it work is generated by the engine itself. This is the power of reflection allowing the developer of the engine to pretend that the component already exists even if it doesn't, then whenever the component (or any other component) does get created the engine can generate code for itself that allows it to integrate the component. This way all the users must do is create the component and mark it with the correct parameters, this allows users to focus on what is important: Making their program.

## How
The first step to making the program is finding a way to parse c++ files and extract all useful symbols from them. Depending on how well you want your parser to work you can parse it yourself or have a library do it for you. For this project we will be using clang and their c++ API ClangTooling, Clang has the advantage of being a well tested compiler so most c++ code you throw at it will be correctly parsed into an AST (We'll discuss what that is later). since I mainly develop on windows getting the clang source code was more of a challenge than I would like to admit but the short of it is that LLVM (the organization maintaining clang) doesn't provide any builds for windows other than libclang which is the c version of the api. Instead, we will have to build clang ourselves and that is exactly what I did.

A small note: if you download the project's source code from the repo you will manually have to add clang to the External folder as it is a bit too big to include otherwise.

Although Clang is a widely used compiler its libraries are not as widely used, due to this there is not much information to find about ClangTooling outside of the [doxygen](https://clang.llvm.org/doxygen/namespaceclang_1_1tooling.html)

Once setup the parser can be invoked by creating a `clang::tooling::ClangTool` and calling `.run` on it. When creating the tool, you will need to provide a `CompilationDatabase` which contains the compile commands used to compile the files. Additionally, the tool also requires a vector containing the paths of all the files to compile.

### The CompilationDatabase
The CompilationDatabase must be inherited from `clang::tooling::CompilationDatabase` and implement the `getCompileCommands` function which takes in a file path and returns a vector containing all compile commands that apply to that file. clang provides a couple classes which inherit from `CompilationDatabase` that could be used, these classes are: `ArgumentsAdjustingCompilations`, `FixedCompilationDatabase` and `JSONCompilationDatabase`. All these classes provide their own ways of handling compile commands however, none of them are particularly useful so we create our own simple class which just adds the arguments to whatever file is passed in.

```cpp
//header
class SimpleOptionParser : public clang::tooling::CompilationDatabase {
    std::vector<std::string> args;
    std::filesystem::path output_directory;
public:
    explicit SimpleOptionParser(const std::vector<std::string>& args);
    void SetOutputDirectory(const std::filesystem::path& output_directory);
    
    [[nodiscard]] std::vector<clang::tooling::CompileCommand> getCompileCommands(llvm::StringRef FilePath) const override;
};

```
```cpp
SimpleOptionParser::SimpleOptionParser(const std::vector<std::string> &args) : args(args) {}

std::vector<clang::tooling::CompileCommand> SimpleOptionParser::getCompileCommands(llvm::StringRef FilePath) const {
    std::filesystem::path file{FilePath.str()};
    
    std::vector<std::string> newArgs{this->args};
    newArgs.emplace_back(FilePath.str());
    
    std::vector<clang::tooling::CompileCommand> commands{
            clang::tooling::CompileCommand(file.parent_path().string(), file.filename().string(), newArgs, output_directory.string() + "/" + FilePath.str())
    };
    return commands;
}
```
This class takes in a list of strings for its constructor and simply outputs them whenever getCompileCommands is called.

### Running Clang
in our main function we simply pass an instance of the optionsParser to the tool:
```cpp
SimpleOptionParser optionParser{args, headers};
clang::tooling::ClangTool tool{optionParser, headers};
```
now clang is setup and ready to be used.

To get clang to run our custom code we must provide the `.run` function with our custom RunAction, this can be done by creating a class which inherits from `clang::ASTFrontendAction` or any of it's children, in our case we will be inheriting from `SyntaxOnlyAction` as we will not need anything else. Then to pass our custom class to the run function we do:
```cpp
//ASTFrontenAction is our custom class
tool.run(clang::tooling::newFrontendActionFactory<ASTFrontendAction>().get());
```

### Running our custom code
The `ASTFrontendAction` class has a singular purpose (at least as far as we are concerned) which is to create an `ASTConsumer` for each file. This is done by implementing the `CreateASTConsumer` function which will take a `compilerInstance` and a file and return a `std::unique_ptr` to a `clang::ASTConsumer`

The `ASTConsumer` is responsible for handling parsed c++ files. this is done through, yet another class called a `RecursiveASTVisitor` The `ASTConsumer` class provides the virtual `HandleTranslationUnit` function which takes in an `ASTContext`

```cpp
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
```
```cpp
std::unique_ptr<clang::ASTConsumer> ASTFrontendAction::CreateASTConsumer(clang::CompilerInstance &compilerInstance, llvm::StringRef inFile) {
    return std::make_unique<ASTConsumer>();
}

void ASTConsumer::HandleTranslationUnit(clang::ASTContext &context) {
    ASTFileParser parser{};

    parser.TraverseDecl(context.getTranslationUnitDecl());
}
```
That's a lot of classes, isn't it? so let's go through what clang is doing step by step:

- First of when the `run` function is called we passed it a `FrontendActionFactory` which is simply responsible for creating an instance of our custom `ASTFrontendAction` class for every file it will parse.
- After that for every file that gets parsed a new `ASTFrontendAction` will be created and the `CreateASTConsumer` function is called on it with the current file.
- The `CreateASTConsumer` function (which is implemented by us) will return a pointer to our custom `ASTConsumer` class.
- clang parses the file and turns it into an Abstract Syntax Tree (AST). an AST is a hierarchical tree-like data structure which is commonly used for representing source code, the AST stores the hierarchy of all the objects in the code and allows for easy traversal.
- The root node of the AST is passed to the `HandleTranslationUnit` function on our `ASTConsumer` class, this function is responsible to handle the AST node which in our case simply means we pass it to our last class which is the `RecursiveASTVisitor` that we need to implement.
- The default behavior of the `RecursiveASTVisitor` is to simply visit all the nodes in the AST.

as you might realize the behavior of `RecursiveASTVisitor` is just what we need to parse all our files.

### Extracting the data
now that we have finally reached the point where we can extract the parsed data let's talk about how we want to save that data.

first let's take a look at how clang structures the AST, this can be done by invoking clang.exe with the `-Xclang -ast-dump -fsyntax-only` flags running it on this small test snippet:
```cpp
#pragma once

#include <unordered_map>

class Linker
{
    std::unordered_map<std::string, std::shared_ptr<int>> objects;
public:
    void Link(const std::shared_ptr<int>& parser);
};
```
gives us this:
```
TranslationUnitDecl 0x17e93276f58 <<invalid sloc>> <invalid sloc>
|-TypedefDecl 0x17e932777c8 <<invalid sloc>> <invalid sloc> implicit __int128_t '__int128'
| `-BuiltinType 0x17e93277520 '__int128'
|-TypedefDecl 0x17e93277838 <<invalid sloc>> <invalid sloc> implicit __uint128_t 'unsigned __int128'
| `-BuiltinType 0x17e93277540 'unsigned __int128'
|-TypedefDecl 0x17e93277bb0 <<invalid sloc>> <invalid sloc> implicit __NSConstantString '__NSConstantString_tag'
| `-RecordType 0x17e93277920 '__NSConstantString_tag'
|   `-CXXRecord 0x17e93277890 '__NSConstantString_tag'
|-TypedefDecl 0x17e93277c58 <<invalid sloc>> <invalid sloc> implicit __builtin_ms_va_list 'char *'
| `-PointerType 0x17e93277c10 'char *'
|   `-BuiltinType 0x17e93277000 'char'
|-TypedefDecl 0x17e93277cc8 <<invalid sloc>> <invalid sloc> implicit __builtin_va_list 'char *'
| `-PointerType 0x17e93277c10 'char *'
|   `-BuiltinType 0x17e93277000 'char'
`-CXXRecordDecl 0x17e93277d20 <C:\Users\games\source\github\Code-Generation\Code_Generation\include\Linker.hpp:5:1, line:10:1> line:5:7 class Linker definition
  |-DefinitionData pass_in_registers empty aggregate standard_layout trivially_copyable pod trivial literal has_constexpr_non_copy_move_ctor can_const_default_init
  | |-DefaultConstructor exists trivial constexpr needs_implicit defaulted_is_constexpr
  | |-CopyConstructor simple trivial has_const_param needs_implicit implicit_has_const_param
  | |-MoveConstructor exists simple trivial needs_implicit
  | |-CopyAssignment simple trivial has_const_param needs_implicit implicit_has_const_param
  | |-MoveAssignment exists simple trivial needs_implicit
  | `-Destructor simple irrelevant trivial needs_implicit
  |-CXXRecordDecl 0x17e93277e38 <col:1, col:7> col:7 implicit class Linker
  |-AccessSpecDecl 0x17e93277f10 <line:8:1, col:7> col:1 public
  `-CXXMethodDecl 0x17e932dacc0 <line:9:5, col:49> col:10 invalid Link 'void (const int &)'
    `-ParmVarDecl 0x17e932dabc8 <col:15, col:43> col:43 invalid parser 'const int &'
```
quite confusing right? I recommend testing it yourself as with the colors that clang adds it will be a lot easier to see what's going on.

For now, the part that we are interested in is this:
```
CXXRecordDecl 0x1f0435a4810 <C:\Users\games\source\github\Code-Generation\Code_Generation\include\Linker.hpp:9:1, line:16:1> line:9:7 class Linker definition
  |-DefinitionData pass_in_registers empty aggregate standard_layout trivially_copyable pod trivial literal has_constexpr_non_copy_move_ctor can_const_default_init
  | |-DefaultConstructor exists trivial constexpr needs_implicit defaulted_is_constexpr
  | |-CopyConstructor simple trivial has_const_param needs_implicit implicit_has_const_param
  | |-MoveConstructor exists simple trivial needs_implicit
  | |-CopyAssignment simple trivial has_const_param needs_implicit implicit_has_const_param
  | |-MoveAssignment exists simple trivial needs_implicit
  | `-Destructor simple irrelevant trivial needs_implicit
  |-CXXRecordDecl 0x1f0435a4928 <col:1, col:7> col:7 implicit class Linker
  |-AccessSpecDecl 0x1f0435a49b8 <line:13:1, col:7> col:1 public
  `-CXXMethodDecl 0x1f0435a4b30 <line:15:5, col:52> col:10 invalid Link 'void (const int &)'
    `-ParmVarDecl 0x1f0435a4a38 <col:15, col:46> col:46 invalid parser 'const int &'
```
This part which is right at the bottom is the actual parsed class. Looking through this snippet you might see some patterns. 

first off you might notice that there is this section with `DefinitionData` as the root, this section contains all the implicitly generated data and can also be ignored giving us this:
```
CXXRecordDecl 0x1f0435a4810 <C:\Users\games\source\github\Code-Generation\Code_Generation\include\Linker.hpp:9:1, line:16:1> line:9:7 class Linker definition
  |-CXXRecordDecl 0x1f0435a4928 <col:1, col:7> col:7 implicit class Linker
  |-AccessSpecDecl 0x1f0435a49b8 <line:13:1, col:7> col:1 public
  `-CXXMethodDecl 0x1f0435a4b30 <line:15:5, col:52> col:10 invalid Link 'void (const int &)'
    `-ParmVarDecl 0x1f0435a4a38 <col:15, col:46> col:46 invalid parser 'const int &'
```
this output should give us a good idea of how the `RecursiveASTVisitor` will work so let’s get back to the code shall we.

The `RecursiveASTVisitor` works a little differently than the other classes we have inherited from. To inherit from this class, you must provide the class as a template argument like so:
```cpp
class ASTFileParser : public clang::RecursiveASTVisitor<ASTFileParser> {
}
```
As mentioned above this class is created and managed by the `ASTConsumer` class. The `ASTConsumer` creates an instance of the visitor and calls the `TraverseDecl` function passing the ASTContext as it's argument. The `TraverseDecl` function is a general version of the TraverseFunctions available on the visitor class. The way how `TraverseDecl` works is as follows:
- First, we test if the Decl is valid
- After verifying its validity we check if the user wants to visit implicit declarations which is false by default.
- Then we check what kind of Decl we have and call TraverseXXDecl where XX is the type of the Decl.
- The TraverseXXDecl will call TraverseDecl on all it's child nodes

The `RecursiveASTVisitor` class provides 3 different function groups which all contain every Decl type, these groups are:
- `TraverseDecl` which is the function responsible for recursively visiting all nodes, TraverseDecl will call the other functions in the order that they are listed here
- `WalkUpFromDecl` which by default does nothing
- `VisitDecl` which also does nothing by default

Your first instinct looking at this list might be to overwrite the WalkUpFrom and Visit Decl groups... and it was mine as well, however as it turns out WalkUpFrom is called before actually visiting the Decl. This makes things a lot harder since for some Decl's it is important to know when we exit the Decl since classes can be nested and there would be no easy way to know if the member is part of the child class or the parent class. Sadly, the only way to remedy this is by implementing the Traverse group which means that we must also handle the recursive visiting.

So... now that we know what group of functions to overwrite let's talk about the data we want to extract. Since we want our program to be user friendly, we don't want to have our users worry about recursion and keeping track of where exactly in the AST we are. To solve this let's store the data in a more linear way by simply storing them inside vectors. We know that (most) objects have a Name, Namespace, File Path and list of properties (for the reflection) so let's put those in a base class:
```cpp
struct Object {
    std::string name;
    std::string fullNamespace;
    std::filesystem::path path;
    std::vector<Property> properties{};
};
```
Now the objects we want to store are Classes, Functions (and methods) and variables (and members) so let's make some classes for those which inherit from Object:
```cpp
struct Variable : public Object {
    std::string type = "int";
};

struct Function : public Object {
    std::string returnType = "void";
    std::vector<Variable> parameters{};
    bool isConstruptor = false;
};

struct Class : public Object {
    std::vector<Variable> variables{};
    std::vector<Function> functions{};
};
```
Let's also create a struct for Properties:
```cpp
struct Property {
    std::string name;
    std::string value;
};
```
With this we know what data we need to extract so let's do it.

The first step is to figure out which Decl's we need to visit, remember the output we got from running clang? The names displayed there are the exact names of the types we need to overwrite, writing some more test code and running clang on it we find out that the Decl's we are interested in are: `CXXRecord`, `CXXConstructor`, `Field`, `CXXMethod`, `ParmVar`, `Var`, `Function`. Implementing these functions gives us this class declaration:
```cpp
class ASTFileParser : public clang::RecursiveASTVisitor<ASTFileParser> {
    friend class ASTConsumer;
    friend class FileGenerator;

    std::vector<Class> classes;
    std::vector<Function> functions;
    std::vector<Variable> variables;

    std::filesystem::path path;
public:
    explicit ASTFileParser(std::filesystem::path file) : path(std::move(file)) {};

    bool TraverseCXXRecordDecl(clang::CXXRecordDecl *decl);

    bool TraverseCXXConstructorDecl(clang::CXXConstructorDecl *decl);

    bool TraverseFieldDecl(clang::FieldDecl *decl);

    bool TraverseCXXMethodDecl(clang::CXXMethodDecl *decl);

    bool TraverseParmVarDecl(clang::ParmVarDecl *decl);

    bool TraverseVarDecl(clang::VarDecl *decl);

    bool TraverseFunctionDecl(clang::FunctionDecl *decl);
};
```
We use our class to represent a singular object file, so we store all our objects in the class.

Implementing these functions will follow the following template:
```cpp
bool ASTFileParser::TraverseXXDecl(clang::XXDecl *decl) {
    {
        //We handle our custom visit here.
    }

    bool result = clang::RecursiveASTVisitor<ASTFileParser>::TraverseXXDecl(decl);

    {
        //we handle our walk up from here.
    }

    return result;
}
```
A simple template right, instead of worrying about calling the right functions for recursion we simply call the base classes Traverse function which will handle the recursion for us.

Next up is the problem of nested classes, sometimes a class can be declared inside another class, to solve this we will use a `std::stack` which will keep track of our current top level class. Technically the same problem can happen with functions where a function gets declared inside the body of another function so we might as well use a stack for those as well.

our next problem is that sometimes when a type fails to parse clang will use `int` as a fallback type, as such it is more reliable to get the type from the file directly instead of relying on clang to parse the type. for this we will create a helper function like so:
```cpp
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
```
This code should be self explanatory, we retrieve the location of the type stored in the Decl and simply copy the text that is at that location in the file. Clang includes the static keyword by default which doesn't affect the type at all, so we ignore it.

Finally, we will also write some functions which will parse the properties of the current object for reflection.
```cpp
std::vector<Property> ASTFileParser::GetProperties(const clang::Decl &decl, std::string keyword) {
    auto line = GetLineAbove(decl);

    //test if the line starts with the keyword (ignoring whitespace)
    //if not return empty vector
    //remove leading whitespace
    line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](int ch) { return !std::isspace(ch); }));
    if (line.find(keyword) != 0) {
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
```
once again this is self explanatory but it's quite long so let's step through it.

The very first thing we do is retrieve the line right above the decl which will be the properties. we use a small helper function for this that retrieves the line for us. After getting the line we check if the line starts with the provided keyword (The keywords we use are CGCLASS, CGFUNCTION, CGVARIABLE, CGMETHOD and CGMEMBER) if it does, we know we found a valid property list and we can parse them. Next up is to find the start and end of the list of properties, we get the substring contained between the brackets and split it using the `,` as a separator. finally, we split the parameter name and value and return the list of properties.

Most Traverse functions are roughly the same so let's talk about the `TraverseCXXRecordDecl`
```cpp
bool ASTFileParser::TraverseCXXRecordDecl(clang::CXXRecordDecl *decl) {

    //test if decl is from current file, if not skip recursive traversal and continue to next decl
    if (DeclIsIncluded(*decl)) {
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
        Class.path = path;
        Class.properties = properties;
    }

    bool result = clang::RecursiveASTVisitor<ASTFileParser>::TraverseCXXRecordDecl(decl);

    //handle teardown here:
    {
        classStack.pop();
    }

    return result;
}
```
To start of we test if the Decl is even from the file we want to parse, clang automatically includes files based on the includes of the current file. The `DeclIsIncluded` function simply compares the path of the decl and the current file and sees if they are the same. If they are not, we won't even bother calling the recursive function as we can assume that all children of this decl are also in the other file (The exception would be leaked namespaces and incorrectly managed scope but since that is generally considered bad practice, we will ignore that). After making sure we are in the right file we will parse the properties of the class, if the keyword is not found the GetProperties function will return an empty vector while otherwise the first value will always be the keyword. if the list of properties is empty, we know that we don't have to bother recursing as we require a class to have properties before, we will parse it. After getting the properties we fill in the other data which clang provides functions for that we can just call. Then we recurse and afterwards we pop the stack again.

Most other Decl's follow a similar pattern and can be found in the provided [source code](https://github.com/Cynic1254/Block-B-blog/blob/main/Code/src/FileParser.cpp)

One difference I will note if for `TraverseParmVarDecl` as sometimes the AST will have a ParmVarDecl without being inside a function, this happens when a variable type is a callback at which point the AST will generate ParmVar nodes for the callback’s parameters. Due to this we first check if the function stack is empty and if it is we don't parse the parameter.

### Using the data
Now that we parsed our files, we are left with a nice list of parsed data containing all the symbols we care about. The next step is to use the parsed data and create a class that users can use to define how to generate their files. For this we need a simple way to represent our files so let's create a class for it:
```cpp
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
```
by using a map to store our functions users can easily access any function they need regardless of if it exists or not, if it doesn't exist the map will simply create a new function. Our `FullFunction` object simply stores an instance of the `Function` object we created before as it is more than enough for our needs, and we don't want to reinvent the wheel.

Let's look at our `FileGenerator` class:
```cpp
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
```

since most of the functions are commented I won't discuss them right now, but I would like to point out how we store our files, once again we use a map to store our data for the same reason as we did with our functions, It allows the user to easily access the data and not have to worry if the file exists or not. The class has 6 callback functions the user can bind which will get called whenever `Parse` is called. There are also a couple helper functions that the user can use.

For now, lets take a look at the `WriteFiles` and `Parse` functions:
```cpp
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
```

The `WriteFiles` function simply loops over all the data in the map and writes it to files.
The `Parse` function calls the callback functions (if they are bound) on the data.

Using this class, we can write the following code to generate the LuaBindings shown earlier in the post:
```cpp
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
```
As seen the user must worry about little things as much care has been taken to make the experience as streamlined as possible, the only lines in the callbacks are those that output strings to the function body.

## Final words
Although not perfect I believe that having a simple way to do reflection is vital for many projects and I hope that this blogpost and program encourage some people to make a better program (or improve the current one). I am very aware that this program has many shortcomings and this being my first experience with clang it also meant that I had to learn as I went. After writing this blog I will be working in a team to make a game engine and I hope I can convince people to use my program, so it gets some more practical examples.

All code showed is available to download and see on the [github page](https://github.com/Cynic1254/Block-B-blog/tree/main/Code).
