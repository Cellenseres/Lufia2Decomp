cmake_minimum_required(VERSION 3.20)
project(Lufia2Decomp VERSION 0.1.0 LANGUAGES C)

# Development options. Warnings default on only for a standalone build, so a
# consumer that uses add_subdirectory keeps its own flags unless it opts in.
# All options apply to lufia2_decomp alone and never change global flags.
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(_lufia2_top_level ON)
else()
    set(_lufia2_top_level OFF)
endif()
option(LUFIA2_ENABLE_WARNINGS "Build lufia2_decomp with compiler warnings"
    ${_lufia2_top_level})
option(LUFIA2_WARNINGS_AS_ERRORS "Treat lufia2_decomp warnings as errors" OFF)
option(LUFIA2_ENABLE_SANITIZERS
    "Build lufia2_decomp with AddressSanitizer and UBSan (GCC/Clang)" OFF)
option(LUFIA2_BUILD_TOOLS "Build the command-line tools" ${_lufia2_top_level})

add_library(lufia2_resource_format STATIC src/system/resource_format.c)
add_library(Lufia2::ResourceFormat ALIAS lufia2_resource_format)
target_include_directories(lufia2_resource_format PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_features(lufia2_resource_format PUBLIC c_std_11)

add_library(lufia2_decomp STATIC
    src/actor/actor_action.c
    src/actor/actor_frontend.c
    src/actor/actor_movement.c
    src/actor/actor_primary.c
    src/actor/actor_secondary.c
    src/actor/actor_slots.c
    src/actor/actor_sprites.c
    src/actor/object_vm.c
    src/actor/player_controller.c
    src/actor/player_update.c
    src/battle/battle_character.c
    src/battle/battle_frame.c
    src/battle/battle_frame_rows.c
    src/battle/battle_ip.c
    src/battle/battle_popup.c
    src/battle/battle_util.c
    src/battle/battle_buffers.c
    src/battle/battle_portrait.c
    src/battle/battle_nmi.c
    src/battle/battle_script.c
    src/battle/battle_transition.c
    src/battle/battle_entry.c
    src/battle/battle_lifecycle_internal.c
    src/battle/battle_setup.c
    src/battle/battle_setup_internal.c
    src/battle/battle_background.c
    src/battle/battle_background_internal.c
    src/battle/battle_exit.c
    src/battle/battle_display_setup.c
    src/battle/battle_display_internal.c
    src/battle/battle_loop.c
    src/battle/battle_loop_children.c
    src/battle/battle_turn_order.c
    src/battle/battle_outcome.c
    src/battle/battle_actions.c
    src/battle/battle_status_tick.c
    src/battle/battle_command.c
    src/battle/battle_commands.c
    src/battle/battle_party_commands.c
    src/battle/battle_submenus.c
    src/battle/battle_targets.c
    src/battle/battle_target_helpers.c
    src/battle/battle_command_lists.c
    src/battle/battle_command_display.c
    src/battle/battle_command_display_helpers.c
    src/battle/battle_result_window.c
    src/battle/battle_results.c
    src/battle/battle_loop_internal.c
    src/cave/ancient_cave.c
    src/cave/cave_grid.c
    src/cave/cave_objects.c
    src/cave/cave_random.c
    src/field/field_effects.c
    src/field/encounter.c
    src/field/field_restore.c
    src/field/field_attributes.c
    src/field/field_event_actors.c
    src/field/field_event_conditions.c
    src/field/field_event_objects.c
    src/field/field_event_script.c
    src/field/field_nmi.c
    src/field/field_scroll.c
    src/field/field_sections.c
    src/field/field_sprites.c
    src/field/field_triggers.c
    src/field/field_update.c
    src/item/inventory.c
    src/item/inventory_receive.c
    src/item/item_records.c
    src/menu/menu.c
    src/menu/menu_screen.c
    src/menu/menu_sprites.c
    src/menu/menu_string.c
    src/menu/menu_window.c
    src/party/capsule.c
    src/party/experience.c
    src/party/level_up.c
    src/party/stat_growth.c
    src/party/party_records.c
    src/party/stats.c
    src/system/decompress.c
    src/system/math.c
    src/system/random.c
    src/system/screen.c
    src/system/sound.c
    src/text/text_engine.c
    src/text/text_conditions.c
    src/text/text_layout.c
    src/text/text_window.c
    src/title/title.c
    src/world/world_map.c)
add_library(Lufia2::Decomp ALIAS lufia2_decomp)
target_link_libraries(lufia2_decomp PUBLIC Lufia2::ResourceFormat)

target_include_directories(lufia2_decomp
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(lufia2_decomp PUBLIC c_std_11)

set(_lufia2_warnings)
if(LUFIA2_ENABLE_WARNINGS)
    if(MSVC)
        list(APPEND _lufia2_warnings /W4)
    elseif(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        list(APPEND _lufia2_warnings -Wall -Wextra -Wpedantic)
    endif()
    if(LUFIA2_WARNINGS_AS_ERRORS)
        if(MSVC)
            list(APPEND _lufia2_warnings /WX)
        else()
            list(APPEND _lufia2_warnings -Werror)
        endif()
    endif()
endif()
target_compile_options(lufia2_decomp PRIVATE ${_lufia2_warnings})
target_compile_options(lufia2_resource_format PRIVATE ${_lufia2_warnings})

if(LUFIA2_ENABLE_SANITIZERS)
    if(NOT CMAKE_C_COMPILER_ID MATCHES "GNU|Clang" OR MSVC)
        message(FATAL_ERROR
            "LUFIA2_ENABLE_SANITIZERS supports GCC and Clang only")
    endif()
    set(_lufia2_sanitizers -fsanitize=address,undefined)
    target_compile_options(lufia2_decomp PRIVATE
        ${_lufia2_sanitizers} -fno-omit-frame-pointer -fno-sanitize-recover=all)
    # The static library needs the sanitizer runtimes in whatever links it.
    target_link_options(lufia2_decomp INTERFACE ${_lufia2_sanitizers})
    target_compile_options(lufia2_resource_format PRIVATE
        ${_lufia2_sanitizers} -fno-omit-frame-pointer -fno-sanitize-recover=all)
    target_link_options(lufia2_resource_format INTERFACE ${_lufia2_sanitizers})
endif()

if(LUFIA2_BUILD_TOOLS)
    add_executable(lufia2-resource tools/lufia2_resource.c)
    target_link_libraries(lufia2-resource PRIVATE Lufia2::ResourceFormat)
    target_compile_options(lufia2-resource PRIVATE ${_lufia2_warnings})
endif()
