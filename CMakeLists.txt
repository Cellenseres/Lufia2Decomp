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
    src/actor/actor_state_mark.c
    src/actor/actor_slots.c
    src/actor/sprite_reset.c
    src/actor/actor_sprites.c
    src/actor/actor_sprite_resources.c
    src/actor/actor_sprite_allocation.c
    src/actor/object_vm.c
    src/actor/player_controller.c
    src/actor/player_update.c
    src/battle/battle_character.c
    src/battle/battle_frame.c
    src/battle/battle_frame_setup.c
    src/battle/battle_effect_video.c
    src/battle/battle_effect_graphics.c
    src/battle/battle_effect_slots.c
    src/battle/battle_effect_selection.c
    src/battle/battle_sprite_lookups.c
    src/battle/battle_target_resolution.c
    src/battle/battle_action_effects.c
    src/battle/battle_action_presentation.c
    src/battle/battle_action_handlers.c
    src/battle/battle_action_followups.c
    src/battle/battle_action_records.c
    src/battle/battle_special_actions.c
    src/battle/battle_collective_actions.c
    src/battle/battle_action_dispatch.c
    src/battle/battle_effect_flow.c
    src/battle/battle_effect_engine.c
    src/battle/battle_drift.c
    src/battle/battle_vector.c
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
    src/battle/battle_background_wave.c
    src/battle/battle_effect_ops.c
    src/battle/battle_tile_blit.c
    src/menu/menu_image_load.c
    src/menu/menu_graphics_setup.c
    src/menu/menu_actor_callers.c
    src/field/field_menu.c
    src/menu/menu_main_screen.c
    src/menu/menu_main_dispatch.c
    src/menu/menu_selection_state.c
    src/menu/menu_selection_save.c
    src/menu/menu_selection_reset.c
    src/menu/menu_field_dispatch.c
    src/menu/menu_display_callers.c
    src/menu/menu_cursor_callers.c
    src/menu/menu_presentation_callers.c
    src/menu/menu_list_controls.c
    src/menu/menu_list_scroll.c
    src/menu/menu_list_motion.c
    src/field/menu_actor_updates.c
    src/field/field_object_flags.c
    src/field/field_cell_edges.c
    src/field/field_scene_actors.c
    src/field/field_session_setup.c
    src/system/block_move.c
    src/battle/battle_circle_window.c
    src/battle/battle_exit.c
    src/battle/battle_display_setup.c
    src/battle/battle_display_internal.c
    src/battle/battle_loop.c
    src/battle/battle_loop_children.c
    src/battle/battle_turn_order.c
    src/battle/battle_turn_queue.c
    src/battle/battle_turn_priority.c
    src/battle/battle_mirror_sprites.c
    src/battle/battle_message_copy.c
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
    src/battle/battle_action_message.c
    src/battle/battle_message_parser.c
    src/battle/battle_result_window.c
    src/battle/battle_result_hdma.c
    src/battle/battle_results.c
    src/battle/battle_turn_display.c
    src/battle/battle_party_status.c
    src/battle/battle_party_status_detail.c
    src/battle/battle_status_support.c
    src/battle/battle_status_recovery.c
    src/battle/battle_indexed_message.c
    src/battle/battle_sprite_grid.c
    src/battle/battle_message_display.c
    src/battle/battle_message_length.c
    src/battle/battle_gauges.c
    src/battle/battle_message_render.c
    src/battle/battle_message_glyph.c
    src/battle/battle_message_effect.c
    src/battle/battle_status_icons.c
    src/battle/battle_frame_input.c
    src/battle/battle_sprite_build.c
    src/battle/battle_monster_oam.c
    src/battle/battle_palette_fade.c
    src/battle/battle_action_work.c
    src/battle/battle_action_script.c
    src/battle/battle_loop_internal.c
    src/cave/ancient_cave.c
    src/cave/cave_exit.c
    src/cave/cave_reset.c
    src/cave/cave_header.c
    src/cave/cave_grid.c
    src/cave/cave_objects.c
    src/cave/cave_random.c
    src/field/field_effects.c
    src/field/field_palette_scale.c
    src/field/encounter.c
    src/field/field_restore.c
    src/field/field_attributes.c
    src/field/field_event_actors.c
    src/field/field_event_conditions.c
    src/field/field_event_objects.c
    src/field/field_event_script.c
    src/field/field_nmi.c
    src/field/field_area_transition.c
    src/field/field_battle_transition.c
    src/field/field_map_load.c
    src/field/field_map_install.c
    src/field/field_object_graphics.c
    src/field/field_object_transitions.c
    src/field/field_object_lookup.c
    src/field/field_object_conditions.c
    src/field/field_object_initialization.c
    src/field/field_event_reset.c
    src/field/field_redraw_flags.c
    src/field/field_pending_initialization.c
    src/field/field_coordinate_math.c
    src/field/field_object_motion.c
    src/field/field_probe_records.c
    src/field/field_probe_collision.c
    src/field/field_probe_direction.c
    src/field/field_event_map_state.c
    src/field/field_event_triggers.c
    src/field/field_event_object_region.c
    src/field/field_actor_event_helpers.c
    src/field/field_object_release.c
    src/field/field_object_motion_resources.c
    src/field/field_actor_probe.c
    src/field/field_object_transition.c
    src/field/field_scene_record_select.c
    src/field/field_scene_records.c
    src/field/field_scene_reset.c
    src/field/field_display_callers.c
    src/field/field_scene_actor_records.c
    src/actor/actor_scene_resources.c
    src/actor/party_scene.c
    src/actor/actor_scene_rebuild.c
    src/actor/actor_bootstrap.c
    src/field/field_bootstrap.c
    src/field/scene_bootstrap.c
    src/field/field_event_control.c
    src/field/field_event_control_script.c
    src/field/field_event_actor_refresh.c
    src/field/field_record_search.c
    src/field/field_object_bits.c
    src/field/field_layer_scale.c
    src/field/field_layer_scroll.c
    src/field/field_object_tiles.c
    src/field/field_object_copy.c
    src/field/field_object_attributes.c
    src/field/field_object_render.c
    src/field/field_object_update.c
    src/field/scene_script.c
    src/field/field_scroll.c
    src/battle/battle_sprite_descriptors.c
    src/battle/battle_status_cache.c
    src/battle/battle_record_reset.c
    src/battle/battle_encounter_loader.c
    src/battle/battle_encounter_generation.c
    src/battle/battle_encounter_state.c
    src/battle/battle_transition_setup.c
    src/battle/battle_enemy_graphics.c
    src/battle/battle_enemy_sprite_setup.c
    src/battle/battle_capsule_graphics.c
    src/battle/battle_portrait_state.c
    src/battle/battle_window_fades.c
    src/battle/battle_effect_actor_tiles.c
    src/battle/battle_effect_frame_setup.c
    src/battle/battle_effect_target_popups.c
    src/battle/battle_effect_popup_actor.c
    src/battle/battle_effect_conditional_sound.c
    src/battle/battle_effect_vector_actor.c
    src/battle/battle_effect_centered_parameter.c
    src/battle/battle_effect_sound_command.c
    src/battle/battle_effect_sprite_rebuild.c
    src/battle/battle_effect_turn_display.c
    src/battle/battle_effect_portrait_status.c
    src/battle/battle_effect_portrait_restore.c
    src/battle/battle_effect_portrait_pose.c
    src/battle/battle_effect_tilemap_callers.c
    src/battle/battle_effect_graphics_resource.c
    src/battle/battle_effect_target_toggle_callers.c
    src/battle/battle_effect_target_toggle.c
    src/battle/battle_effect_palette_brightness.c
    src/battle/battle_effect_conditional_script.c
    src/battle/battle_effect_tile_rectangle.c
    src/battle/battle_effect_record_controls.c
    src/battle/battle_effect_script_commands.c
    src/battle/battle_effect_actor_commands.c
    src/battle/battle_layer_color_dma.c
    src/battle/battle_effect_circle_window.c
    src/battle/battle_effect_palette_load.c
    src/battle/battle_effect_target_state.c
    src/battle/battle_effect_stream_fields.c
    src/battle/battle_effect_delay_presets.c
    src/battle/battle_effect_vector_placement.c
    src/battle/battle_effect_palette_copy.c
    src/battle/battle_effect_byte_acceleration.c
    src/battle/battle_effect_parameter_words.c
    src/battle/battle_effect_draw_state.c
    src/battle/battle_effect_angle_motion.c
    src/party/capsule_menu_chain.c
    src/menu/menu_tile_clear.c
    src/party/capsule_status_address.c
    src/party/capsule_item_choice.c
    src/party/capsule_selection_addresses.c
    src/battle/battle_effect_enemy_scripts.c
    src/system/hardware_division.c
    src/battle/battle_ripple_setup.c
    src/battle/battle_effect_current_target.c
    src/system/vector_angle.c
    src/system/vector_division.c
    src/battle/battle_effect_saved_actor.c
    src/battle/battle_effect_display_transition.c
    src/battle/battle_effect_target_motion.c
    src/battle/battle_name_tiles.c
    src/battle/battle_glyph_caller.c
    src/battle/battle_text_tiles.c
    src/world/world_map_load.c
    src/world/world_palette_step.c
    src/world/world_flagged_blocks.c
    src/system/system_flag_read.c
    src/world/world_depth.c
    src/world/world_math_resources.c
    src/world/world_perspective.c
    src/field/field_row_buffer.c
    src/world/world_stream.c
    src/battle/battle_effect_scroll.c
    src/world/world_scene.c
    src/party/capsule_stat_growth.c
    src/field/field_contact_edges.c
    src/field/field_actor_contact.c
    src/field/field_talk.c
    src/party/capsule_stat_callers.c
    src/world/world_resource_pointers.c
    src/field/field_event_pointer.c
    src/world/world_cell_center.c
    src/party/capsule_experience.c
    src/field/field_sprite_selector.c
    src/field/field_actor_select.c
    src/battle/battle_party_clear.c
    src/battle/battle_relative_script.c
    src/field/field_section_entries.c
    src/field/field_event_coordinate.c
    src/field/field_event_value.c
    src/field/field_event_reader.c
    src/field/field_metatile_stream.c
    src/field/field_cell_address.c
    src/field/field_event_byte.c
    src/field/field_sections.c
    src/field/field_sprites.c
    src/field/field_triggers.c
    src/field/field_metatile_graphics.c
    src/field/field_scene_graphics.c
    src/field/field_scene_tracks.c
    src/field/field_scene_display.c
    src/field/field_reload.c
    src/field/field_camera.c
    src/field/field_update.c
    src/field/field_diagnostics.c
    src/field/field_radial_objects.c
    src/field/field_presentation_reset.c
    src/field/field_object_parameters.c
    src/field/field_prefixed_name.c
    src/field/field_recovery.c
    src/field/field_animation_regions.c
    src/field/field_animation_slots.c
    src/field/field_animation_cells.c
    src/item/inventory.c
    src/item/inventory_selection.c
    src/item/inventory_receive.c
    src/item/item_records.c
    src/menu/menu.c
    src/menu/menu_status.c
    src/menu/menu_screen.c
    src/menu/menu_spell_shop.c
    src/menu/menu_sprites.c
    src/menu/menu_string.c
    src/menu/menu_character.c
    src/menu/menu_window.c
    src/menu/menu_tilemap.c
    src/menu/menu_cursor.c
    src/menu/menu_cursor_slide.c
    src/menu/menu_input.c
    src/menu/menu_scene_setup.c
    src/party/capsule.c
    src/party/capsule_records.c
    src/party/capsule_abilities.c
    src/party/experience.c
    src/party/level_up.c
    src/party/stat_growth.c
    src/party/party_records.c
    src/party/stats.c
    src/party/party_equipment.c
    src/party/party_refresh.c
    src/system/decompress.c
    src/system/math.c
    src/system/multiply_16_by_8.c
    src/system/random.c
    src/system/save.c
    src/system/default_records.c
    src/menu/menu_selection_palettes.c
    src/menu/save_menu_build.c
    src/menu/save_menu_parent.c
    src/menu/save_menu_actions.c
    src/menu/save_menu_dispatch.c
    src/menu/name_entry.c
    src/menu/menu_record_services.c
    src/menu/menu_confirmation.c
    src/menu/menu_price_followup.c
    src/menu/menu_context_services.c
    src/menu/menu_save_erase.c
    src/menu/menu_dispatch_callers.c
    src/menu/menu_inventory_capsule_callers.c
    src/menu/menu_selection_callers.c
    src/system/save_file_check.c
    src/system/save_file_flags.c
    src/title/title_selection.c
    src/system/save_party_stats.c
    src/system/save_party_pack.c
    src/system/save_state_pack.c
    src/system/save_state_restore.c
    src/system/save_capsule_record.c
    src/system/save_party_check.c
    src/system/save_party_unpack.c
    src/system/field_save_state.c
    src/system/save_record_checksum.c
    src/system/music.c
    src/system/sound_commands.c
    src/system/sound_driver.c
    src/system/sound_tables.c
    src/system/sound_transfer.c
    src/system/sound_queue.c
    src/system/sound_payload.c
    src/system/sound_reply.c
    src/system/sound_upload.c
    src/system/nmi.c
    src/system/nmi_uploads.c
    src/system/scene_uploads.c
    src/system/screen.c
    src/system/sound.c
    src/text/text_engine.c
    src/text/text_conditions.c
    src/text/text_layout.c
    src/text/text_window.c
    src/text/scene_text_resources.c
    src/text/scene_label_callers.c
    src/title/title.c
    src/world/world_map.c
    src/world/world_map_plane.c
    src/world/world_plane_rows.c
    src/world/world_scroll_step.c
    src/world/world_map_objects.c)
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
