#ifndef LUFIA2_MENU_SPRITE_SLOTS_H
#define LUFIA2_MENU_SPRITE_SLOTS_H

/* Menu sprite slots: 48 byte arrays in bank $7E. */
enum {
    MENU_SPRITE_SLOTS = 0x30u,
    MENU_SPRITE_ACTIVE = 0x11d8u,    /* non-zero: slot is in use */
    MENU_SPRITE_ANIMATION = 0x1208u, /* animation number */
    MENU_SPRITE_BANK = 0x1238u,      /* bank of the animation data */
    MENU_SPRITE_LIST_LOW = 0x1268u,  /* animation list pointer */
    MENU_SPRITE_LIST_HIGH = 0x1298u,
    MENU_SPRITE_SCRIPT_LOW = 0x12c8u, /* current animation's frame list */
    MENU_SPRITE_SCRIPT_HIGH = 0x12f8u,
    MENU_SPRITE_FRAME_LOW = 0x1328u, /* current frame record */
    MENU_SPRITE_FRAME_HIGH = 0x1358u,
    MENU_SPRITE_X_LOW = 0x1388u, /* screen position, 16 bits split */
    MENU_SPRITE_X_HIGH = 0x13b8u,
    MENU_SPRITE_Y_LOW = 0x13e8u,
    MENU_SPRITE_Y_HIGH = 0x1418u,
    MENU_SPRITE_FRAME_INDEX = 0x1448u, /* frame within the animation */
    MENU_SPRITE_TIMER = 0x1478u,       /* frames left on the current frame */
};

#endif
