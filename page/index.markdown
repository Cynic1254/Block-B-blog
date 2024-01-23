---
# Feel free to add content and custom Front Matter to this file.
# To modify the layout, see https://jekyllrb.com/docs/themes/#overriding-theme-defaults

layout: default
---

# Using Clang for static reflection

## What
Static reflection is a key component to writing dynamic code which is easily integrated into systems. However despite this there are few solutions for drag and drop static reflection out there. Unreal Engine's reflection system has a nice way of handling reflection where the user doesn't need to worry about adding their classes to specific functions or files so that the code is able to run it, Instead the user can use macro's to describe what they want to happen with their code:

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

In this snippet the class is marked with the UCLASS() property which is detected by unreal and the class is added to it's reflection system. Variables are marked with the UPROPERTY() macro and additional settings are passed as parameters, these settings tell unreal that the variables cannot be edited by blueprints and when inspected they will be under the Movement category.

My aim was to make a small console based application that the user can invoke just before building their c/c++ application that will generate files based on properties specified by the user similar to how unreal engine's properties work. The difference in my software will be that the user is able to control what gets generated based on what properties.

## Why