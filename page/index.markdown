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

# How