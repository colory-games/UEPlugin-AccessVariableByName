/*!
 * AccessVariableByName
 *
 * Copyright (c) 2022 Colory Games
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "AccessVariableByNameUtils.h"

#include "Internationalization/Regex.h"
#include "UObject/UnrealType.h"

const FName ExecThenPinName(TEXT("ExecThen"));
const FName VarNamePinName(TEXT("VarName"));
const FName ExtendIfNotPresentPinName(TEXT("ExtendIfNotPresent"));
const FName SuccessPinName(TEXT("Success"));
const FName ResultPinNamePrefix(TEXT("Result_"));
const FName NewValuePinNamePrefix(TEXT("NewValue_"));

const FString ExecThenPinFriendlyName(TEXT(" "));
const FString TargetPinFriendlyName(TEXT("Target"));
const FString VarNamePinFriendlyName(TEXT("Var Name"));
const FString ExtendIfNotPresentPinFriendlyName(TEXT("Extend If not Present"));
const FString SuccessPinFriendlyName(TEXT("Success"));

TerminalProperty GetTerminalProperty(const TArray<FVarDescription>& VarDescs, int32 VarDepth, UScriptStruct* OuterClass);

FEdGraphPinType CreateDefaultPinType()
{
	FEdGraphPinType PinType;
	PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;

	return PinType;
}

UClass* GetClassFromNode(const UEdGraphNode* Node)
{
	UClass* Class = nullptr;

	UEdGraph* Graph = Node->GetGraph();
	UObject* GraphOwner = Graph->GetOutermostObject();
	UBlueprint* Blueprint = Cast<UBlueprint>(GraphOwner);
	if (Blueprint != nullptr)
	{
		Class = Blueprint->GeneratedClass;
	}

	return Class;
}

TerminalProperty GetTerminalPropertyInternal(const TArray<FVarDescription>& VarDescs, int32 VarDepth, FProperty* Property)
{
	if (VarDescs.Num() <= VarDepth)
	{
		return TerminalProperty();
	}

	const FVarDescription& Desc = VarDescs[VarDepth];

	if (Desc.ArrayAccessType == EArrayAccessType::ArrayAccessType_None)
	{
		if (VarDescs.Num() == VarDepth + 1)
		{
			TerminalProperty TP;
			TP.Property = Property;
			return TP;
		}

		if (Property->IsA<FStructProperty>())
		{
			FStructProperty* StructProperty = CastChecked<FStructProperty>(Property);
			UScriptStruct* ScriptStruct = StructProperty->Struct;

			return GetTerminalProperty(VarDescs, VarDepth + 1, ScriptStruct);
		}
		else if (Property->IsA<FObjectProperty>())
		{
			FObjectProperty* ObjectProperty = CastChecked<FObjectProperty>(Property);
			UClass* Class = ObjectProperty->PropertyClass;

			return GetTerminalProperty(VarDescs, VarDepth + 1, Class);
		}

		return TerminalProperty();
	}
	else if (Desc.ArrayAccessType == EArrayAccessType::ArrayAccessType_Integer)
	{
		if (Property->IsA<FArrayProperty>())
		{
			FArrayProperty* ArrayProperty = CastChecked<FArrayProperty>(Property);

			if (VarDescs.Num() == VarDepth + 1)
			{
				TerminalProperty TP;
				TP.ContainerType = EPinContainerType::Array;
				TP.Property = ArrayProperty->Inner;
				return TP;
			}

			FProperty* InnerProperty = ArrayProperty->Inner;
			if (InnerProperty->IsA<FStructProperty>())
			{
				FStructProperty* StructProperty = CastChecked<FStructProperty>(InnerProperty);
				UScriptStruct* ScriptStruct = StructProperty->Struct;

				return GetTerminalProperty(VarDescs, VarDepth + 1, ScriptStruct);
			}
			else if (InnerProperty->IsA<FObjectProperty>())
			{
				FObjectProperty* ObjectProperty = CastChecked<FObjectProperty>(InnerProperty);
				UClass* Class = ObjectProperty->PropertyClass;

				return GetTerminalProperty(VarDescs, VarDepth + 1, Class);
			}

			return TerminalProperty();
		}
		else if (Property->IsA<FMapProperty>())
		{
			FMapProperty* MapProperty = CastChecked<FMapProperty>(Property);

			if (VarDescs.Num() == VarDepth + 1)
			{
				TerminalProperty TP;
				TP.ContainerType = EPinContainerType::Map;
				TP.Property = MapProperty->ValueProp;
				return TP;
			}

			FProperty* ValueProperty = MapProperty->ValueProp;
			if (ValueProperty->IsA<FStructProperty>())
			{
				FStructProperty* StructProperty = CastChecked<FStructProperty>(ValueProperty);
				UScriptStruct* ScriptStruct = StructProperty->Struct;

				return GetTerminalProperty(VarDescs, VarDepth + 1, ScriptStruct);
			}
			else if (ValueProperty->IsA<FObjectProperty>())
			{
				FObjectProperty* ObjectProperty = CastChecked<FObjectProperty>(ValueProperty);
				UClass* Class = ObjectProperty->PropertyClass;

				return GetTerminalProperty(VarDescs, VarDepth + 1, Class);
			}
		}

		return TerminalProperty();
	}
	else if (Desc.ArrayAccessType == EArrayAccessType::ArrayAccessType_String)
	{
		if (!Property->IsA<FMapProperty>())
		{
			return TerminalProperty();
		}
		FMapProperty* MapProperty = CastChecked<FMapProperty>(Property);

		if (VarDescs.Num() == VarDepth + 1)
		{
			TerminalProperty TP;
			TP.ContainerType = EPinContainerType::Map;
			TP.Property = MapProperty->ValueProp;
			return TP;
		}

		FProperty* ValueProperty = MapProperty->ValueProp;
		if (ValueProperty->IsA<FStructProperty>())
		{
			FStructProperty* StructProperty = CastChecked<FStructProperty>(ValueProperty);
			UScriptStruct* ScriptStruct = StructProperty->Struct;

			return GetTerminalProperty(VarDescs, VarDepth + 1, ScriptStruct);
		}
		else if (ValueProperty->IsA<FObjectProperty>())
		{
			FObjectProperty* ObjectProperty = CastChecked<FObjectProperty>(ValueProperty);
			UClass* Class = ObjectProperty->PropertyClass;

			return GetTerminalProperty(VarDescs, VarDepth + 1, Class);
		}

		return TerminalProperty();
	}

	return TerminalProperty();
}

TerminalProperty GetTerminalProperty(const TArray<FVarDescription>& VarDescs, int32 VarDepth, UScriptStruct* OuterClass)
{
	if (VarDescs.Num() <= VarDepth)
	{
		return TerminalProperty();
	}

	const FVarDescription& Desc = VarDescs[VarDepth];

	if (!Desc.bIsValid)
	{
		return TerminalProperty();
	}

	FProperty* Property = FVariableAccessFunctionLibraryUtils::GetScriptStructProperty(OuterClass, Desc.VarName);
	if (Property == nullptr)
	{
		return TerminalProperty();
	}

	return GetTerminalPropertyInternal(VarDescs, VarDepth, Property);
}

TerminalProperty GetTerminalProperty(const TArray<FVarDescription>& VarDescs, int32 VarDepth, UClass* OuterClass)
{
	if (VarDescs.Num() <= VarDepth)
	{
		return TerminalProperty();
	}

	const FVarDescription& Desc = VarDescs[VarDepth];

	if (!Desc.bIsValid)
	{
		return TerminalProperty();
	}

	FProperty* Property = FindFProperty<FProperty>(OuterClass, *Desc.VarName);
	if (Property == nullptr)
	{
		return TerminalProperty();
	}

	return GetTerminalPropertyInternal(VarDescs, VarDepth, Property);
}
