#ifndef LUFIA2_SYSTEM_WRAM_H
#define LUFIA2_SYSTEM_WRAM_H

/* WRAM shared by several subsystems; meanings from DECOMP_STATUS. */

/* Direct page, DP = 0 in the field loop. */
#define DP_FRAME_COUNTER 0x42u
#define DP_BUTTONS_HELD 0x46u
#define DP_BUTTONS_PRESSED 0x4au
#define DP_NMI_UPLOAD_FLAGS 0x73u
#define DP_PROBE_X 0x8fu
#define DP_PROBE_Y 0x91u
#define DP_ACTOR_SLOT 0xa7u

/* Bank $00/$7E low WRAM. */
#define WRAM_CGRAM_BUFFER 0x0320u
#define WRAM_FADE_CONTROL 0x0581u
#define WRAM_FADE_LEVEL 0x0582u
#define WRAM_BRIGHTNESS 0x0583u
#define WRAM_FIELD_FLAGS 0x05b5u
#define WRAM_FIELD_REQUESTS 0x05b7u
#define WRAM_ACTOR_STATE 0x0622u
#define WRAM_EVENT_FLAGS 0x077eu
#define WRAM_TEXT_STATE 0x099bu
#define WRAM_GOLD 0x0a8au
#define WRAM_SCREEN_EFFECTS 0x1261u
#define WRAM_PALETTE_FADE 0x1262u
#define WRAM_ANIMATION_MASK 0x17aau
#define WRAM_SOUND_COMMAND 0x17acu

/* $0583 values. */
#define BRIGHTNESS_FULL 0x0fu
#define BRIGHTNESS_FORCED_BLANK 0x80u

#endif
