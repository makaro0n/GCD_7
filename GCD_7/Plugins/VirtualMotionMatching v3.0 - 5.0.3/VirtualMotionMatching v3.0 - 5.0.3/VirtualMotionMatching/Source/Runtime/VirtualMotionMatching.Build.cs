// Copyright 2021-2022 ZhangJiaBin. All Rights Reserved.

using UnrealBuildTool;

public class VirtualMotionMatching : ModuleRules
{
	public VirtualMotionMatching(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
          new string[] {
                System.IO.Path.Combine(ModuleDirectory,"Public")
          }
          );


        PrivateIncludePaths.AddRange(
            new string[] {
                System.IO.Path.Combine(ModuleDirectory,"Private")
			}
            );


        PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"NetCore",
				"Engine",
				"DataRegistry",
				"GameplayTags",
				"GameplayTasks",
				"GameplayAbilities",
				"EnhancedInput"
			}
            );
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Slate",
				"SlateCore",
				"InputCore",
				"AnimationCore",
                "AnimGraphRuntime",
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);

        if (Target.bBuildEditor == true)
        {
            PrivateIncludePaths.AddRange(
           new string[] {
            System.IO.Path.GetFullPath(Target.RelativeEnginePath) + "Source/Editor/Blutility/Private",
           }
           );

            PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                    "Blutility",
            }
            );

            PublicDependencyModuleNames.AddRange(new string[]
            {
                    "Slate",
                    "UnrealEd",
                    "Persona",
                    //"AnimationModifiers", // UE4
					"AnimationBlueprintLibrary", // UE5
					"EditorScriptingUtilities",
            });
        }
    }
}
