#include "visual_cube.h"

#include <stdio.h>
#include <assert.h>

void visual_cube_state_reset(visual_cube_state_t* visual_cube_state) {
    for (int face = 0; face < 6; face++) {
        for (int i = 0; i < 9; i++) {
            visual_cube_state->stickers[face][i] = (face_index_e)face;
        }
    }
}

int visual_cube_get_edge_sticker_index(face_index_e primary_face_index, face_index_e secondary_face_index) {
    static int index_lookup_table[6][6] = {
        [FACE_INDEX_U] = { [FACE_INDEX_B] = 1, [FACE_INDEX_R] = 5, [FACE_INDEX_F] = 7, [FACE_INDEX_L] = 3 },
        [FACE_INDEX_D] = { [FACE_INDEX_F] = 1, [FACE_INDEX_R] = 5, [FACE_INDEX_B] = 7, [FACE_INDEX_L] = 3 },
        [FACE_INDEX_F] = { [FACE_INDEX_U] = 1, [FACE_INDEX_R] = 5, [FACE_INDEX_D] = 7, [FACE_INDEX_L] = 3 },
        [FACE_INDEX_B] = { [FACE_INDEX_U] = 1, [FACE_INDEX_L] = 5, [FACE_INDEX_D] = 7, [FACE_INDEX_R] = 3 },
        [FACE_INDEX_R] = { [FACE_INDEX_U] = 1, [FACE_INDEX_B] = 5, [FACE_INDEX_D] = 7, [FACE_INDEX_F] = 3 },
        [FACE_INDEX_L] = { [FACE_INDEX_U] = 1, [FACE_INDEX_F] = 5, [FACE_INDEX_D] = 7, [FACE_INDEX_B] = 3 }
    };
    return index_lookup_table[primary_face_index][secondary_face_index];
}

int visual_cube_get_corner_sticker_index(visual_cube_corner_e corner, face_index_e primary_face_index) {
    switch (corner) {
        case VISUAL_CUBE_CORNER_UBL: return primary_face_index == FACE_INDEX_B ? 2 : 0;
        case VISUAL_CUBE_CORNER_UBR: return primary_face_index == FACE_INDEX_B ? 0 : 2;
        case VISUAL_CUBE_CORNER_UFR: return primary_face_index == FACE_INDEX_U ? 8 : (primary_face_index == FACE_INDEX_F ? 2 : 0);
        case VISUAL_CUBE_CORNER_UFL: return primary_face_index == FACE_INDEX_U ? 6 : (primary_face_index == FACE_INDEX_F ? 0 : 2);
        case VISUAL_CUBE_CORNER_DFL: return primary_face_index == FACE_INDEX_D ? 0 : (primary_face_index == FACE_INDEX_F ? 6 : 8);
        case VISUAL_CUBE_CORNER_DFR: return primary_face_index == FACE_INDEX_D ? 2 : (primary_face_index == FACE_INDEX_F ? 8 : 6);
        case VISUAL_CUBE_CORNER_DBR: return primary_face_index == FACE_INDEX_B ? 6 : 8;
        case VISUAL_CUBE_CORNER_DBL: return primary_face_index == FACE_INDEX_B ? 8 : 6;
    }
    assert(false);
}

void visual_cube_state_write_center(visual_cube_state_t* visual_cube_state, face_index_e center, face_index_e state) {
    visual_cube_state->stickers[center][4] = state;
}

void visual_cube_state_write_corner(visual_cube_state_t* visual_cube_state, visual_cube_corner_e corner,
                                    face_index_e primary_sticker, face_index_e secondary_sticker, face_index_e tertiary_sticker) {
    face_index_e primary_face_index   = (face_index_e)byte_ctz(corner);
    face_index_e secondary_face_index = (face_index_e)byte_ctz((uint8_t)(corner ^ (1 << primary_face_index)));
    face_index_e tertiary_face_index  = (face_index_e)byte_ctz((uint8_t)(corner ^ (1 << primary_face_index) ^ (1 << secondary_face_index)));

    int primary_sticker_index   = visual_cube_get_corner_sticker_index(corner, primary_face_index);
    int secondary_sticker_index = visual_cube_get_corner_sticker_index(corner, secondary_face_index);
    int tertiary_sticker_index  = visual_cube_get_corner_sticker_index(corner, tertiary_face_index);

    visual_cube_state->stickers[ primary_face_index ][ primary_sticker_index ] = primary_sticker;
    visual_cube_state->stickers[secondary_face_index][secondary_sticker_index] = secondary_sticker;
    visual_cube_state->stickers[tertiary_face_index ][tertiary_sticker_index ] = tertiary_sticker;
}

#define ESC_COLOR_WHITE  "\x1b[47m"
#define ESC_COLOR_ORANGE "\x1b[48;5;208m"
#define ESC_COLOR_GREEN  "\x1b[42m"
#define ESC_COLOR_RED    "\x1b[41m"
#define ESC_COLOR_BLUE   "\x1b[48;5;27m"
#define ESC_COLOR_YELLOW "\x1b[48;5;190m"

#define ESC_COLOR_RESET "\x1b[0m"

void draw_face_color(face_index_e face_index) {
    switch (face_index) {
        case FACE_INDEX_U: printf(ESC_COLOR_WHITE  "  " ESC_COLOR_RESET); return;
        case FACE_INDEX_L: printf(ESC_COLOR_ORANGE "  " ESC_COLOR_RESET); return;
        case FACE_INDEX_F: printf(ESC_COLOR_GREEN  "  " ESC_COLOR_RESET); return;
        case FACE_INDEX_R: printf(ESC_COLOR_RED    "  " ESC_COLOR_RESET); return;
        case FACE_INDEX_B: printf(ESC_COLOR_BLUE   "  " ESC_COLOR_RESET); return;
        case FACE_INDEX_D: printf(ESC_COLOR_YELLOW "  " ESC_COLOR_RESET); return;
    }
    assert(false);
}

void draw_visual_cube_state(const visual_cube_state_t* visual_cube_state) {
    for (int y = 0; y < 3; y++) {
        printf("        ");
        for (int x = 0; x < 3; x++) {
            draw_face_color(visual_cube_state->stickers[FACE_INDEX_U][y * 3 + x]);
        }
        printf("\n");
    }
    printf("\n");
    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            draw_face_color(visual_cube_state->stickers[FACE_INDEX_L][y * 3 + x]);
        }
        printf("  ");
        for (int x = 0; x < 3; x++) {
            draw_face_color(visual_cube_state->stickers[FACE_INDEX_F][y * 3 + x]);
        }
        printf("  ");
        for (int x = 0; x < 3; x++) {
            draw_face_color(visual_cube_state->stickers[FACE_INDEX_R][y * 3 + x]);
        }
        printf("  ");
        for (int x = 0; x < 3; x++) {
            draw_face_color(visual_cube_state->stickers[FACE_INDEX_B][y * 3 + x]);
        }
        printf("\n");
    }
    printf("\n");
    for (int y = 0; y < 3; y++) {
        printf("        ");
        for (int x = 0; x < 3; x++) {
            draw_face_color(visual_cube_state->stickers[FACE_INDEX_D][y * 3 + x]);
        }
        printf("\n");
    }
}
