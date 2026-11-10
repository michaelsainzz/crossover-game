#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/StaticMesh.h"
#include "GameMapsSettings.h"
#include "Prototype/CrossoverPracticeMode.h"
#include "Prototype/CrossoverPracticePlayer.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrossoverSmoke, "Crossover.Smoke",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrossoverSmoke::RunTest(const FString& Parameters)
{
    const ACrossoverPracticeMode* Mode = GetDefault<ACrossoverPracticeMode>();
    TestTrue(TEXT("Project selects the practice game mode"),
        UGameMapsSettings::GetGlobalDefaultGameMode() == ACrossoverPracticeMode::StaticClass()->GetPathName());
    TestTrue(TEXT("Practice mode selects the player pawn"), Mode->DefaultPawnClass == ACrossoverPracticePlayer::StaticClass());
    TestTrue(TEXT("Practice mode selects the HUD"), Mode->HUDClass == ACrossoverPracticeHUD::StaticClass());
    TestNotNull(TEXT("Court primitive loads"),
        LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
    TestNotNull(TEXT("Player primitive loads"),
        LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
    TestNotNull(TEXT("Ball primitive loads"),
        LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
    return true;
}

#endif
