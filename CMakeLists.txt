cmake_minimum_required(VERSION 3.20)
project(CubeVulkan C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Every dependency is fetched as source at a pinned version, so no Vulkan SDK
# is required: volk loads the system vulkan-1.dll and glslang is built here to
# compile the shaders.
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(vulkan_headers GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
                     GIT_TAG v1.4.304 GIT_SHALLOW TRUE SOURCE_SUBDIR none)
FetchContent_Declare(volk GIT_REPOSITORY https://github.com/zeux/volk.git
                     GIT_TAG 1.4.304 GIT_SHALLOW TRUE SOURCE_SUBDIR none)
FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git
                     GIT_TAG 1.0.1 GIT_SHALLOW TRUE SOURCE_SUBDIR none)
FetchContent_Declare(miniaudio GIT_REPOSITORY https://github.com/mackron/miniaudio.git
                     GIT_TAG 0.11.21 GIT_SHALLOW TRUE SOURCE_SUBDIR none)
FetchContent_Declare(stb GIT_REPOSITORY https://github.com/nothings/stb.git
                     GIT_TAG a7d4c2175630e1ea5b6f8dd00ab96fed0b97b96d SOURCE_SUBDIR none)
FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git
                     GIT_TAG 3.4 GIT_SHALLOW TRUE)
FetchContent_Declare(glslang GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
                     GIT_TAG 15.0.0 GIT_SHALLOW TRUE)
# FreeType (FreeType License): the rasteriser and stroker the game's interface text is drawn
# with (docs/specs/plasma-shapes-and-text.md, part 5).
FetchContent_Declare(freetype GIT_REPOSITORY https://github.com/freetype/freetype.git
                     GIT_TAG VER-2-13-3 GIT_SHALLOW TRUE)
# SQLite (public domain), read-only: the game's own save (Save/world.db) is a SQLite file.
FetchContent_Declare(sqlite URL https://www.sqlite.org/2024/sqlite-amalgamation-3460100.zip
                     URL_HASH SHA256=77823cb110929c2bcb0f5d48e4833b5c59a8a6e40cdea3936b99e199dbbe5784
                     DOWNLOAD_EXTRACT_TIMESTAMP TRUE SOURCE_SUBDIR none)
# zlib (zlib License): the game's character saves are zlib streams (docs/specs/native-saves.md).
FetchContent_Declare(zlib URL https://github.com/madler/zlib/releases/download/v1.3.1/zlib-1.3.1.tar.gz
                     URL_HASH SHA256=9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23
                     DOWNLOAD_EXTRACT_TIMESTAMP TRUE SOURCE_SUBDIR none)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(ENABLE_OPT OFF CACHE BOOL "" FORCE)
set(ENABLE_HLSL OFF CACHE BOOL "" FORCE)
set(ENABLE_SPVREMAPPER OFF CACHE BOOL "" FORCE)
set(ENABLE_CTEST OFF CACHE BOOL "" FORCE)
set(GLSLANG_TESTS OFF CACHE BOOL "" FORCE)
set(GLSLANG_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
set(BUILD_EXTERNAL OFF CACHE BOOL "" FORCE)
foreach(feature ZLIB BZIP2 PNG HARFBUZZ BROTLI)
    set(FT_DISABLE_${feature} ON CACHE BOOL "" FORCE)
endforeach()
set(SKIP_INSTALL_ALL ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(vulkan_headers volk glm miniaudio stb glfw glslang sqlite zlib)
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE) # (FreeType asks for an old CMake version)
FetchContent_MakeAvailable(freetype)
unset(CMAKE_WARN_DEPRECATED CACHE)
if(MSVC)
    target_compile_options(freetype PRIVATE /w) # (third-party code, built as it comes)
endif()

add_executable(spirv_compile tools/spirv_compile.cpp)
target_link_libraries(spirv_compile PRIVATE glslang glslang-default-resource-limits)

# Shaders: GLSL -> SPIR-V headers (uint32_t arrays) in the build tree.
set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_OUT ${CMAKE_CURRENT_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SHADER_OUT})
set(SHADER_HEADERS)
foreach(shader scene.vert scene.frag shadow.vert water.frag omega_water.frag sky.vert sky.frag overlay.vert overlay.frag
        post.vert copy.frag taa.frag smaa_edge.vert smaa_edge.frag smaa_weight.vert smaa_weight.frag
        smaa_blend.vert smaa_blend.frag alpha_scene.vert alpha_scene.frag alpha_water.frag alpha_sky.frag
        alpha_sprite.vert alpha_sprite.frag alpha_cloud.frag steam_cloud.frag steam_star.frag map.vert map.frag
        light_beam.vert light_beam.frag)
    string(REPLACE "." "_" symbol ${shader})
    set(out ${SHADER_OUT}/${symbol}.h)
    add_custom_command(OUTPUT ${out}
        COMMAND spirv_compile ${SHADER_DIR}/${shader} ${out} spv_${symbol} ${SHADER_DIR}
        DEPENDS ${SHADER_DIR}/${shader} ${SHADER_DIR}/common.glsl ${SHADER_DIR}/lighting.glsl ${SHADER_DIR}/alpha.glsl ${SHADER_DIR}/steam_sky.glsl ${SHADER_DIR}/steam_vertex.glsl ${SHADER_DIR}/steam_fragment.glsl
                ${SHADER_DIR}/frame.glsl ${SHADER_DIR}/steam_colour.glsl ${SHADER_DIR}/steam_pixel.glsl ${SHADER_DIR}/steam_transform.glsl
                ${SHADER_DIR}/smaa_common.glsl ${CMAKE_CURRENT_SOURCE_DIR}/third_party/smaa/SMAA.hlsl spirv_compile
        COMMENT "Compiling ${shader}")
    list(APPEND SHADER_HEADERS ${out})
endforeach()
add_custom_target(shaders DEPENDS ${SHADER_HEADERS})

set(ZLIB_SOURCES)
foreach(file adler32 compress crc32 deflate infback inffast inflate inftrees trees uncompr zutil)
    list(APPEND ZLIB_SOURCES ${zlib_SOURCE_DIR}/${file}.c)
endforeach()
if(MSVC)
    set_source_files_properties(${ZLIB_SOURCES} PROPERTIES COMPILE_OPTIONS /w) # (third-party code, built as it comes)
endif()
add_library(deps STATIC
    ${volk_SOURCE_DIR}/volk.c
    ${sqlite_SOURCE_DIR}/sqlite3.c
    ${ZLIB_SOURCES}
    src/third_party.c)
target_include_directories(deps PUBLIC
    ${vulkan_headers_SOURCE_DIR}/include ${volk_SOURCE_DIR} ${glm_SOURCE_DIR}
    ${miniaudio_SOURCE_DIR} ${stb_SOURCE_DIR} ${sqlite_SOURCE_DIR} ${zlib_SOURCE_DIR})
target_compile_definitions(deps PUBLIC VK_NO_PROTOTYPES GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_FORCE_RADIANS NOMINMAX WIN32_LEAN_AND_MEAN)

# Shared precision-sensitive Alpha terrain core, separate from Steam generation.
set(ALPHA_TERRAIN_SOURCES
    src/alphagen/Random.cpp
    src/alphagen/Regions.cpp
    src/alphagen/Locations.cpp
    src/alphagen/Zone.cpp
    src/alphagen/Terrain.cpp
    src/alphagen/SurfaceStore.cpp
    src/alphagen/SteamBridge.cpp
    src/alphagen/Relief.cpp
    src/alphagen/ReliefStore.cpp
    src/alphagen/Trees.cpp
    src/alphagen/TreeModels.cpp
    src/alphagen/Landscape.cpp
    src/alphagen/LandscapeStore.cpp
    src/alphagen/Vegetation.cpp
    src/alphagen/PlantModels.cpp
    src/worldgen/AlphaTerrain.cpp)

# The engine core is a library so the tests link the same code as the game.
add_library(engine STATIC
    src/Assets.cpp
    src/DataCheck.cpp
    src/OwnerSetup.cpp
    src/SteamSession.cpp
    src/CubModel.cpp
    src/Terrain.cpp
    ${ALPHA_TERRAIN_SOURCES}
    src/Extracted.cpp
    src/VegetationWind.cpp
    src/Character.cpp
    src/Creatures.cpp
    src/Actors.cpp
    src/EnemyRespawn.cpp
    src/Hud.cpp
    src/Frontend.cpp
    src/FrontendRun.cpp
    src/Gear.cpp
    src/Saves.cpp
    src/NativeSaves.cpp
    src/NativeProgress.cpp
    src/GameOptions.cpp
    src/GameInput.cpp
    src/ZoneStates.cpp
    src/GameWorld.cpp
    src/TalkScripts.cpp
    src/TalkGen.cpp
    src/worldgen/Noise.cpp
    src/worldgen/Regions.cpp
    src/worldgen/Places.cpp
    src/worldgen/Height.cpp
    src/worldgen/WorldValues.cpp
    src/worldgen/BlockColours.cpp
    src/worldgen/ZoneTerrain.cpp
    src/worldgen/ZoneLight.cpp
    src/worldgen/StructureTables.cpp
    src/worldgen/CellObjects.cpp
    src/worldgen/ZoneBuild.cpp
    src/worldgen/Mason.cpp
    src/worldgen/Nature.cpp
    src/worldgen/BuildingRules.cpp
    src/worldgen/Buildings.cpp
    src/worldgen/Rooms.cpp
    src/worldgen/SetPieces.cpp
    src/worldgen/ZoneContents.cpp
    src/worldgen/Structures.cpp
    src/worldgen/CreatureTables.cpp
    src/worldgen/Looks.cpp
    src/worldgen/Gear.cpp
    src/worldgen/Planner.cpp
    src/worldgen/CreatureBuilder.cpp
    src/worldgen/ZoneCreatures.cpp
    src/worldgen/RoleThings.cpp
    src/worldgen/Residents.cpp
    src/worldgen/Options.cpp
    src/worldgen/Generator.cpp
    src/worldgen/MapModel.cpp
    src/NativePose.cpp
    src/NativePoseState.cpp
    src/NativePoseSpirits.cpp
    src/NativeRules.cpp
    src/Spawned.cpp
    src/Missions.cpp
    src/Lore.cpp
    src/MapMarks.cpp
    src/MapPreview.cpp
    src/MapHistory.cpp
    src/MapControls.cpp
    src/PanelScroll.cpp
    src/HudMap.cpp
    src/UsePrompt.cpp
    src/NativeItems.cpp
    src/ProceduralItemData.cpp
    src/ProceduralItems.cpp
    src/NativeCrt.cpp
    src/NativeUpdate.cpp
    src/NativeAI.cpp
    src/NativeUpkeep.cpp
    src/NativeAbility.cpp
    src/NativeHit.cpp
    src/NativePlayerHit.cpp
    src/NativePet.cpp
    src/NativeEffects.cpp
    src/NativeDrop.cpp
    src/NativeInteract.cpp
    src/NativePlayer.cpp
    src/NativeControl.cpp
    src/Plx.cpp
    src/Inflate.cpp
    src/PlasmaShape.cpp
    src/PlasmaText.cpp
    src/GameNames.cpp
    src/PlasmaScene.cpp
    src/PlasmaInput.cpp
    src/PlasmaNodeAnimation.cpp
    src/ItemSelector.cpp
    src/HudPanels.cpp
    src/HudIdentification.cpp
    src/HudItems.cpp
    src/HudFrontend.cpp
    src/GameShop.cpp
    src/GameCraft.cpp
    src/GroundItems.cpp
    src/IdentifyWindow.cpp
    src/UpgradeWidget.cpp
    src/VoxelEdit.cpp
    src/VoxelCustomization.cpp
    src/ItemPreview.cpp
    src/ItemInformation.cpp
    src/HudCustomization.cpp
    src/HairStyle.cpp
    src/ChatLine.cpp
    src/ChatCommands.cpp
    src/FeatText.cpp
    src/HudChat.cpp
    src/WorldMap.cpp
    src/MapLook.cpp
    src/HudCoin.cpp
    src/HudCompass.cpp
    src/NativeHudContext.cpp
    src/GuiItem.cpp
    src/PyroCreatures.cpp
    src/OverworldEvents.cpp
    src/events/StateStore.cpp
    src/ItemNames.cpp
    src/Speech.cpp
    src/DeathEffects.cpp
    src/net/Wire.cpp
    src/net/CreatureDelta.cpp
    src/net/Packets.cpp
    src/net/Loopback.cpp
    src/net/SteamTransport.cpp
    src/net/UdpTransport.cpp
    src/net/Session.cpp
    src/net/HostGame.cpp
    src/net/ClientGame.cpp
    src/NetWorlds.cpp
    src/Multiplayer.cpp
    src/net/Recording.cpp
    src/ItemCatalog.cpp
    src/World.cpp
    src/AlphaLook.cpp
    src/AlphaMoon.cpp
    src/Clouds.cpp
    src/SteamSky.cpp
    src/AlphaLight.cpp
    src/AlphaScenery.cpp
    src/InventorySlots.cpp
    src/Renderer.cpp
    src/RendererPost.cpp
    src/RendererGui.cpp
    src/RendererLayers.cpp
    src/RendererCachedLayers.cpp
    src/RendererWater.cpp
    src/WaterLook.cpp
    src/RendererMap.cpp
    src/Audio.cpp
    src/SoundRules.cpp
    src/TerrainRay.cpp
    src/CameraPosition.cpp
    src/NativeDeadCreatures.cpp
    src/NativeParticles.cpp
    src/ParticleScene.cpp
    src/NativeWorldObjects.cpp
    src/ResidentEffects.cpp
    src/ResidentPass.cpp
    src/DrawnState.cpp
    src/LinkedPlacement.cpp
    src/Mods.cpp
    src/Config.cpp)
add_dependencies(engine shaders)
target_include_directories(engine PUBLIC src include ${SHADER_OUT})
# glu32: Windows' GLU, whose polygon tessellator fills the interface's outline shapes as Plasma's does.
target_link_libraries(engine PUBLIC deps glfw ws2_32 glu32 freetype winmm)
target_sources(engine PRIVATE src/GamepadUi.cpp)
target_sources(engine PRIVATE src/ObjectVisual.cpp)
if(MSVC)
    target_compile_options(engine PRIVATE /W3 /utf-8)
    # Native voxel, fixed-coordinate and preparation-budget paths depend on numeric staging.
    set_source_files_properties(
        src/NativeVehicle.cpp src/ProceduralItems.cpp src/NativeParticles.cpp src/OwnedGroundCubes.cpp src/Clouds.cpp src/SteamMeshPreparation.cpp src/SteamZoneDemand.cpp ${ALPHA_TERRAIN_SOURCES}
        PROPERTIES COMPILE_OPTIONS /fp:strict)
    # sscanf/getenv are the portable C calls; MSVC's _s variants are not.
    target_compile_definitions(engine PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    set_source_files_properties(
        src/NativeVehicle.cpp src/ProceduralItems.cpp src/NativeParticles.cpp src/OwnedGroundCubes.cpp src/Clouds.cpp src/SteamMeshPreparation.cpp src/SteamZoneDemand.cpp ${ALPHA_TERRAIN_SOURCES}
        PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
endif()

add_executable(CubeVulkan src/main.cpp)
target_link_libraries(CubeVulkan PRIVATE engine dbghelp)
# Mods link against nothing: the API is a table of function pointers.
set_target_properties(CubeVulkan PROPERTIES ENABLE_EXPORTS OFF)

add_library(HelloMod SHARED examples/HelloMod/HelloMod.cpp)
target_include_directories(HelloMod PRIVATE include)
# The example adds streetlights. Build it separately so a default engine run
# does not silently alter the owner's world; copy it into mods/ to opt in.
set_target_properties(HelloMod PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples)

enable_testing()
add_executable(EngineTests tests/EngineTests.cpp tests/AlphaRenderTests.cpp tests/SkyTests.cpp tests/RaceKeyTests.cpp tests/WorldMapTests.cpp tests/SaveTests.cpp tests/FrontendTests.cpp tests/WorldgenTerrainTests.cpp tests/WorldgenStructureTests.cpp tests/WorldgenCreatureTests.cpp tests/NetTests.cpp tests/OwnerSetupTests.cpp tests/PoseHudTests.cpp tests/LootWindowTests.cpp tests/FeatTextTests.cpp tests/WaterLookTests.cpp tests/WindContinueTests.cpp tests/EnemyRespawnTests.cpp tests/PlayerHitTests.cpp)
target_link_libraries(EngineTests PRIVATE engine)
target_sources(EngineTests PRIVATE tests/FontCatalogTests.cpp tests/ChatCacheTests.cpp)
target_sources(EngineTests PRIVATE tests/DialoguePageTests.cpp)
target_sources(EngineTests PRIVATE tests/AlphaSceneryTests.cpp)
target_sources(EngineTests PRIVATE tests/InventorySlotTests.cpp tests/HudContextTests.cpp)
target_sources(EngineTests PRIVATE tests/PetTests.cpp)
target_sources(EngineTests PRIVATE
    tests/GamepadUiTests.cpp
    tests/SteamSceneRangeTests.cpp
    tests/CaptureTickContextTests.cpp
    tests/NativeSpriteCatalogTests.cpp
    tests/NativeTerrainBufferTests.cpp
    tests/NativeLoadedZoneTests.cpp
    tests/CaptureWorldSectionTests.cpp
    tests/CapturePluginTests.cpp
    tests/NativeZoneTerrainTests.cpp
    tests/ItemNameTests.cpp
    tests/ProperNameTests.cpp
    tests/UseCreatureCollectionTests.cpp
    tests/VehicleTests.cpp
    tests/NativeItemRecordTests.cpp
    tests/NativeCreatureViewsTests.cpp
    tests/NativeCreatureScalarsTests.cpp
    tests/NativeCreatureAppearanceTests.cpp
    tests/NativeCreaturePoseTests.cpp
    tests/NativeInventoryTests.cpp
    tests/NativeConversationTests.cpp
    tests/NativeGeneratedTextTests.cpp
    tests/NativeEventQueueTests.cpp
    tests/NativeGameCameraTests.cpp
    tests/GenerationRequestTests.cpp
    tests/InventoryUiTests.cpp
    tests/PickupReceiptTests.cpp
    tests/CraftingCountTests.cpp
    tests/StoredWorldDiagnosticsTests.cpp)
target_sources(EngineTests PRIVATE tests/PlasmaHitTests.cpp)
target_sources(EngineTests PRIVATE tests/ObjectVisualTests.cpp)
add_test(NAME EngineTests COMMAND EngineTests)

# CubeVulkan's own art (assets/: the OverworldEvents quest sprites, MIT) next to the executable.
add_custom_command(TARGET CubeVulkan POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:CubeVulkan>/assets)

# A starting CubeVulkan.ini next to the executable (never overwrites edits).
if(NOT EXISTS ${CMAKE_BINARY_DIR}/CubeVulkan.ini)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/config/CubeVulkan.ini DESTINATION ${CMAKE_BINARY_DIR})
endif()

# Launcher / configuration tool (like OpenMW's launcher).
add_executable(CubeVulkanSetup WIN32 setup/Setup.cpp setup/Wizard.cpp)
target_link_libraries(CubeVulkanSetup PRIVATE engine comctl32 shell32 ole32)
target_compile_definitions(CubeVulkanSetup PRIVATE UNICODE _UNICODE)

add_executable(CubeVulkanDataCheck tools/DataCheck.cpp)
target_link_libraries(CubeVulkanDataCheck PRIVATE engine)

# ZoneExtractor: CubeModLoader mod that saves the game's generated zones.
# Original project adapters implement the native/loader boundaries without CWSDK.
unset(CWSDK_DIR CACHE) # Retire the obsolete external-header cache setting.
if(WIN32)
    add_library(ZoneExtractor SHARED tools/ZoneExtractor/ZoneExtractor.cpp src/ProceduralItemData.cpp)
    target_compile_definitions(ZoneExtractor PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(ZoneExtractor PRIVATE user32 d3d11)
    set_target_properties(ZoneExtractor PROPERTIES CXX_STANDARD 17)
    # Static runtime: the game ships an old MSVCP140 whose std::mutex is
    # incompatible with newer headers.
    set_target_properties(ZoneExtractor PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded"
                          RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tools)
endif()

# HudExtract: cuts the game's HUD art out of frames captured in game (ZoneExtractor F10).
add_executable(HudExtract tools/HudExtract.cpp)
target_include_directories(HudExtract PRIVATE ${stb_SOURCE_DIR})
set_target_properties(HudExtract PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tools)

target_sources(engine PRIVATE src/LightBeam.cpp src/UseCreatureCollection.cpp src/NativeVehicle.cpp)
target_sources(EngineTests PRIVATE tests/ObjectClockTests.cpp tests/WorldObjectTests.cpp tests/DeadCreatureTests.cpp tests/BlockMutationTests.cpp)
target_sources(engine PRIVATE src/SteamScenery.cpp)
target_sources(EngineTests PRIVATE tests/SceneCaptureTests.cpp)

target_sources(engine PRIVATE src/RegionLevels.cpp src/SceneVisibility.cpp src/SteamSceneRange.cpp src/SteamMeshPreparation.cpp src/SteamZoneDemand.cpp)
target_sources(EngineTests PRIVATE tests/RegionLevelsTests.cpp)

target_sources(EngineTests PRIVATE tests/SceneVisibilityTests.cpp)

target_sources(engine PRIVATE src/SteamLighting.cpp)

target_sources(EngineTests PRIVATE tests/SteamLightingTests.cpp)

target_sources(engine PRIVATE src/SteamVoxel.cpp)

target_sources(EngineTests PRIVATE tests/SteamVoxelTests.cpp)

target_sources(EngineTests PRIVATE tests/SteamTerrainTests.cpp)
target_sources(EngineTests PRIVATE tests/SteamShaderTests.cpp tests/GuiItemTests.cpp tests/GuiRenderTests.cpp tests/PlasmaNodeAnimationTests.cpp tests/ItemSelectorTests.cpp)
target_sources(EngineTests PRIVATE tests/VoxelEditTests.cpp tests/VoxelCustomizationTests.cpp tests/ItemPreviewTests.cpp tests/MapPreviewTests.cpp)
target_sources(EngineTests PRIVATE tests/ProceduralItemTests.cpp)
target_sources(EngineTests PRIVATE tests/MapHistoryTests.cpp)
target_sources(EngineTests PRIVATE tests/MapControlsTests.cpp)
target_sources(EngineTests PRIVATE tests/MapRenderTests.cpp)
target_sources(EngineTests PRIVATE tests/PanelScrollTests.cpp)
target_sources(EngineTests PRIVATE tests/PlasmaInputTests.cpp tests/AlphaTerrainTests.cpp)
target_sources(EngineTests PRIVATE tests/PlasmaLayoutTests.cpp)
target_sources(EngineTests PRIVATE tests/ResidentPassTests.cpp tests/ResidentEffectsTests.cpp)
target_sources(EngineTests PRIVATE tests/CameraMotionTests.cpp tests/CameraRenderTests.cpp tests/ControlPhaseTests.cpp tests/TargetStageTests.cpp tests/TerrainRayTests.cpp tests/CameraPositionTests.cpp tests/DrawnStateTests.cpp tests/LinkedPlacementTests.cpp)
target_sources(EngineTests PRIVATE tests/HairStyleTests.cpp)

target_sources(engine PRIVATE src/SteamPointLights.cpp)
target_sources(EngineTests PRIVATE tests/SteamPointLightsTests.cpp)

target_sources(engine PRIVATE src/ExtractedLights.cpp)

target_sources(EngineTests PRIVATE tests/SteamLightOracleTests.cpp)

target_sources(engine PRIVATE src/DialogueWords.cpp src/DialogueChoices.cpp)

target_sources(EngineTests PRIVATE tests/DialogueChoicesTests.cpp)

target_sources(engine PRIVATE src/HudDialogue.cpp)

target_sources(engine PRIVATE src/DialoguePanel.cpp)

target_sources(engine PRIVATE src/SteamTransientLights.cpp)

target_sources(EngineTests PRIVATE tests/SteamTransientLightsTests.cpp)

target_sources(engine PRIVATE src/SteamCreatureLights.cpp)

target_sources(EngineTests PRIVATE tests/SteamCreatureLightsTests.cpp)

target_sources(engine PRIVATE src/NpcSpeech.cpp)
target_sources(EngineTests PRIVATE tests/NpcSpeechTests.cpp)

target_sources(EngineTests PRIVATE tests/ParticleTests.cpp)

target_sources(EngineTests PRIVATE tests/BodyParticleTests.cpp)

target_sources(EngineTests PRIVATE tests/LightPublicationTests.cpp)
target_sources(engine PRIVATE src/CreatureScene.cpp)
target_sources(EngineTests PRIVATE tests/CreatureSceneTests.cpp)
target_sources(engine PRIVATE src/CharacterCard.cpp)
target_sources(EngineTests PRIVATE tests/CharacterCardTests.cpp)
target_sources(EngineTests PRIVATE tests/TerrainBandTests.cpp)
target_sources(engine PRIVATE src/StaticObjectScene.cpp)
target_sources(EngineTests PRIVATE tests/StaticObjectSceneTests.cpp)
target_sources(EngineTests PRIVATE tests/StaticObjectStreamingTests.cpp)
target_sources(engine PRIVATE src/WorldModelControls.cpp)
target_sources(engine PRIVATE src/GroundItemScene.cpp)
target_sources(EngineTests PRIVATE tests/GroundItemSceneTests.cpp)
target_sources(EngineTests PRIVATE tests/WorldModelControlsTests.cpp)
target_sources(engine PRIVATE src/OwnedGroundCubes.cpp)
target_sources(EngineTests PRIVATE tests/OwnedGroundCubesTests.cpp)
target_sources(EngineTests PRIVATE tests/ObjectRewardCaptureTests.cpp)
