---
# Feel free to add content and custom Front Matter to this file.
# To modify the layout, see https://jekyllrb.com/docs/themes/#overriding-theme-defaults

layout: default
---

# Using Clang for static reflection

## What
Static reflection is a key component to writing dynamic code which is easily integrated into systems. However despite this there are few solutions for drag and drop static reflection out there. My aim for this project was to fill this hole with an easy to integrate console based application which will parse c++ files and generate code based on the symbols in them.

### Inspiration
For this project I took a lot of inspiration from Unreal Engine's reflection system. Unreal Engine's reflection system has a nice way of handling reflection where the user doesn't need to worry about adding their classes to specific functions or files so that the code is able to run it, Instead the user can use macro's to describe what they want to happen with their code:

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

My aim was to make a small console based application that the user can invoke just before building their c/c++ application that will generate files based on properties specified by the user similar to how unreal engine's properties work. The difference in my software will be that the user is able to control what gets generated based on what properties.

as an example:
```cpp
CGCLASS(LuaClass)
class Linker
{
    CGMEMBER(LuaInspect)
    std::unordered_map<std::string, std::shared_ptr<Parser>> objects;
public:
   CGMETHOD(LuaInspect)
    void Link(const std::shared_ptr<Parser>& parser);
};
```
generates:
```cpp
#include "C:\Users\games\source\github\Code-Generation\Code_Generation\include\Linker.hpp"

void CreateBindings(sol::state& lua_state)
{
sol::usertype<Linker> Linker_table = lua_state.new_usertype<Linker>("Linker", sol::constructors<Linker()>{});
Linker_table["Link"] = &Linker::Link();
Linker_table["objects"] = &Linker::objects;
}
```
In this example you can see a small input class which is marked with Macro's, these macro's tell the program that this class and these members should be remembered. Additionally parameters have been provided to mark the class as being accessible in Lua. The program sees these macro's and parameters and automatically generates the output file based on them. The output file contains a function to create bindings for Lua.

## Why
Static reflection or reflection in general is a key component to software development and especially important for (game) engines where users should get as much feedback as possible and be able to do as much as possible with as little effort as possible.

Imagine you have a small Entity Component System (ECS) based engine you've made and you want to allow users to create components themselves. This of course isn't that difficult, you simply expose part of your API that provides helper functions for components and voila we have a simple component.
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

Now imagine the user creates an entity and adds the Physics component to it. The user runs the program and... the entity flies in the wrong direction. To fix this the user has to edit their code and recompile the program which takes time. Instead it would be cool if the user is able to inspect the entity and edit the values of the Physics component during runtime.

Of course you already thought of this since you're such an amazing programmer and already implemented an inspector class. One problem though for the inspector to know what it should display it has to know about the class. Not a big problem right? you can just create a template that the user can specialize to describe how the inspector should display the class. Of course the component also needs to be registered so lets add a function which the user has to call during startup to tell the engine about the component.
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
Suddenly adding a new components leaves a lot of work up to the user and a lot of files that the user has to edit. Meanwhile in game engines whenever someone makes a new component all of this code that is required to make it work is generated by the engine itself. This is the power of static reflection allowing the developer of the engine pretend that the component already exists even if it doesn't, then whenever the component (or any other component) does get created the engine can generate code for itself that allows it to integrate the component. This way all the user has to do is create the component and mark it with the correct parameters.

## How
The first step to making the program is finding a way to parse c++ files and extract all useful symbols from them. Depending on how well you want your parser to work you can parse it yourself or have a library do it for you. For this project we will be using clang and their c++ API ClangTooling, Clang has the advantage of being a well tested compiler so most c++ code you throw at it will be correctly parsed into an AST (We'll discuss what that is later). since I mainly develop on windows getting the clang source code was more of a challenge than I would like to admit but the short of it is that LLVM (the company maintaining clang) doesn't provide any builds for windows other than libclang which is the c version of the api. Instead we will have to build clang ourselves and that is exactly what I did.

A small note: if you download the project's source code from the repo you will manually have to add clang to the External folder as it is a bit too big to include otherwise.

Although Clang is a widely used compiler it's libraries are not as widely used, due to this there is not much information to find about ClangTooling outside of the [doxygen](https://clang.llvm.org/doxygen/namespaceclang_1_1tooling.html)

Once setup the parser can be invoked by creating a `clang::tooling::ClangTool` and calling `.run` on it. When creating the tool you will need to provide a `CompilationDatabase` which contains the compile commands used to compile the files. Additionally the tool also requires a vector containing the paths of all the files to compile.

### The CompilationDatabase
The CompilationDatabase must be inherited from `clang::tooling::CompilationDatabase` and implement the `getCompileCommands` function which takes in a filepath and returns a vector containing all compile commands that apply to that file. clang provides a couple classes which inherit from `CompilationDatabase` that are intended to be used, these classes are: `ArgumentsAdjustingCompilations`, `FixedCompilationDatabase` and `JSONCompilationDatabase`. All of these classes provide their own ways of handling compile commands however, none of them are particularly useful so we create our own simple class which just adds the arguments to whatever file is passed in.

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
This class takes in a list of strings for it's constructor and simply outputs them whenever getCompileCommands is called.

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

The `ASTConsumer` is responsible for handling parsed c++ files. this is done through yet another class called a `RecursiveASTVisitor` The `ASTConsumer` class provides the virtual `HandleTranslationUnit` function which takes in an `ASTContext`

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
That's a lot of classes isn't it so let's go through what clang is doing step by step:

- First of when the `run` function is called we passed it a `FrontendActionFactory` which is simply responsible for creating an instance of our custom `ASTFrontendAction` class for every file it will parse.
- After that for every file that gets parsed a new `ASTFrontendAction` will be created and the `CreateASTConsumer` function is called on it with the current file.
- The `CreateASTConsumer` function (which is implemented by us) will return a pointer to our custom `ASTConsumer` class.
- clang parses the file and turns it into an Abstract Syntax Tree (AST). an AST is a hierarchical tree-like data structure which is commonly used for representing source code, the AST stores the hierarchy of all the objects in the code and allows for easy traversal.
- The root node of the AST is passed to the `HandleTranslationUnit` function on our `ASTConsumer` class, this function is responsible to handle the AST node which in our case simply means we pass it to our last class which is the `RecursiveASTVisitor` that we need to implement.
- The default behavior of the `RecursiveASTVisitor` is to simply visit all the nodes in the AST.

as you might realize the behavior of `RecursiveASTVisitor` is just what we need to parse all our files.

### Extracting the data
now that we have finally reached the point where we can extract the parsed data let's talk about how we want to save that data.

first let's take a look at how clang structures the AST, This can be done by invoking clang.exe with the `-Xclang -ast-dump -fsyntax-only` flags running it on this small test snippet:
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

For now the part that we are interested in is this:
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
this output should give us a good idea of how the `RecursiveASTVisitor` will work so lets get back to the code shall we.

The `RecursiveASTVisitor` works a little differently than the other classes we have inherited from. To inherit from this class you must provide the class as a template argument like so:
```cpp
class ASTFileParser : public clang::RecursiveASTVisitor<ASTFileParser> {
}
```
As mentioned above this class is created and managed by the `ASTConsumer` class. The `ASTConsumer` creates an instance of the visitor and calls the `TraverseDecl` function passing the ASTContext as it's argument. The `TraverseDecl` function is a general version of the TraverseFunctions available on the visitor class. The way how `TraverseDecl` works is as follows:
- First we test if the decl is valid
- After verifying it's validity we check if the user wants to visit implicit declarations which is false by default.
- Then we check what kind of decl we have and call TraverseXXDecl where XX is the type of the decl.
- The TraverseXXDecl will recurse

