// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class VirtualMotionMatchingEditor : ModuleRules
{
    public VirtualMotionMatchingEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
           new string[] {
                System.IO.Path.Combine(ModuleDirectory,"Public"),
           }
           );

        PrivateIncludePaths.AddRange(
            new string[] {
                System.IO.Path.Combine(ModuleDirectory,"Private"),
            }
			);

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "Slate",
                "AnimGraphRuntime",
                "BlueprintGraph",
                "VirtualMotionMatching",
                "DataRegistry",
                "GameplayTags"
            }
            );

        PrivateDependencyModuleNames.AddRange(
			new string[] {
			    "InputCore",
				"SlateCore",
				"UnrealEd",
				"GraphEditor",
				"PropertyEditor",
				"EditorStyle",
				"ContentBrowser",
				"KismetWidgets",
				"ToolMenus",
				"KismetCompiler",
				"Kismet",
				"EditorWidgets",
				"AnimGraph",
				"Persona",
			}
        );

        PrivateIncludePathModuleNames.AddRange(
            new string[] {
                "Persona",
                "SkeletonEditor",
                "AdvancedPreviewScene",
                "AnimationBlueprintEditor",
            }
        );

    }
}
