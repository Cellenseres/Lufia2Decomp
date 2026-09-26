cmake_minimum_required(VERSION 3.20)
project(Lufia2Decomp VERSION 0.1.0 LANGUAGES C)

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
    src/battle/battle_frame.c
    src/battle/battle_nmi.c
    src/battle/battle_script.c
    src/field/field_effects.c
    src/field/field_event_actors.c
    src/field/field_event_conditions.c
    src/field/field_event_script.c
    src/field/field_nmi.c
    src/field/field_scroll.c
    src/field/field_sprites.c
    src/field/field_triggers.c
    src/field/field_update.c
    src/menu/menu.c
    src/party/experience.c
    src/system/math.c
    src/system/random.c
    src/system/screen.c
    src/system/sound.c
    src/text/text_engine.c
    src/title/title.c
    src/world/world_map.c)
add_library(Lufia2::Decomp ALIAS lufia2_decomp)

target_include_directories(lufia2_decomp
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(lufia2_decomp PUBLIC c_std_11)
