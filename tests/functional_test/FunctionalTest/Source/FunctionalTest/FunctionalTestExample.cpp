#include "Engine/Blueprint.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTestSample, "TestSample.TestSample Test", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FTestSample::RunTest(const FString& Parameters)
{
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, TEXT("/Game/TestSample.TestSample"));
	TestNotNull(TEXT("Blueprint should not be null"), Blueprint);

	UClass* GeneratedClass = Blueprint->GeneratedClass;
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	AActor* Actor = World->SpawnActor<AActor>(GeneratedClass);
	TestNotNull(TEXT("Actor should not be null"), Actor);

	for (TFieldIterator<UFunction> It(GeneratedClass); It; ++It)
	{
		UFunction* Function = *It;
		if (Function->GetFName().ToString().StartsWith("Test_"))
		{
			struct FParameters
			{
				int32 ReturnValue = 0;
			};
			int32 Expect = 1;
			FParameters Params;
			Actor->ProcessEvent(Function, &Params);
			AddInfo(FString::Format(TEXT("Run '{0}'"), {Function->GetFName().ToString()}));
			TestEqual(TEXT("The return value is incorrect"), Params.ReturnValue, Expect);
		}
	}

	return true;
}