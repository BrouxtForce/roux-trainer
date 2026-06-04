#include "visual_cube.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>

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

void get_visual_cube_state_string(const visual_cube_state_t* visual_cube_state, char buffer[55])
{
    int buffer_index = 0;

    face_index_e face_order[6] = {
        FACE_INDEX_U, FACE_INDEX_L, FACE_INDEX_F, FACE_INDEX_R, FACE_INDEX_B, FACE_INDEX_D
    };
    for (int i = 0; i < 6; i++) {
        face_index_e face = face_order[i];

        for (int sticker = 0; sticker < 9; sticker++) {
            switch (visual_cube_state->stickers[face][sticker]) {
                case FACE_INDEX_U: buffer[buffer_index++] = 'U'; break;
                case FACE_INDEX_D: buffer[buffer_index++] = 'D'; break;
                case FACE_INDEX_F: buffer[buffer_index++] = 'F'; break;
                case FACE_INDEX_B: buffer[buffer_index++] = 'B'; break;
                case FACE_INDEX_R: buffer[buffer_index++] = 'R'; break;
                case FACE_INDEX_L: buffer[buffer_index++] = 'L'; break;
                default: assert(false);
            }
        }
    }

    assert(buffer_index == 54);
    buffer[buffer_index] = '\0';
}

void big_visual_cube_state_init(big_visual_cube_state_t* state, int n, allocator_e allocator) {
    int stickers_per_face = n * n;
    face_index_e* all_stickers = alloc(6 * (size_t)stickers_per_face * sizeof(face_index_e), allocator, SOURCE_LOCATION);
    for (face_index_e face = 0; face < 6; face++) {
        face_index_e* stickers = all_stickers + (face * stickers_per_face);
        for (int j = 0; j < stickers_per_face; j++) {
            stickers[j] = face;
        }
        state->stickers[face] = stickers;
    }
    state->n = n;
    state->allocator = allocator;
}

void big_visual_cube_state_free(big_visual_cube_state_t* state) {
    free_alloc(state->stickers[0], state->allocator);
}

void big_visual_cube_state_reset(big_visual_cube_state_t* state) {
    int stickers_per_face = state->n * state->n;
    for (int face = 0; face < 6; face++) {
        for (int i = 0; i < stickers_per_face; i++) {
            state->stickers[face][i] = (face_index_e)face;
        }
    }
}

void big_visual_cube_state_write_center(big_visual_cube_state_t* state, face_index_e face, int center_index, face_index_e sticker_state) {
    int center_row = center_index / (state->n - 2);
    int center_col = center_index % (state->n - 2);

    int sticker_index = state->n * (center_row + 1) + center_col + 1;

    state->stickers[face][sticker_index] = sticker_state;
}

// TODO: This function only works for 4x4
static int big_visual_cube_get_wing_sticker_index(face_index_e primary_face_index, face_index_e secondary_face_index, bool is_primary) {
    static int index_lookup_table[2][6][6] = {
        {
            [FACE_INDEX_U] = { [FACE_INDEX_B] = 2, [FACE_INDEX_R] = 11, [FACE_INDEX_F] = 13, [FACE_INDEX_L] = 4 },
            [FACE_INDEX_D] = { [FACE_INDEX_F] = 2, [FACE_INDEX_R] = 11, [FACE_INDEX_B] = 13, [FACE_INDEX_L] = 4 },
            [FACE_INDEX_F] = { [FACE_INDEX_U] = 2, [FACE_INDEX_R] = 11, [FACE_INDEX_D] = 13, [FACE_INDEX_L] = 4 },
            [FACE_INDEX_B] = { [FACE_INDEX_U] = 2, [FACE_INDEX_L] = 11, [FACE_INDEX_D] = 13, [FACE_INDEX_R] = 4 },
            [FACE_INDEX_R] = { [FACE_INDEX_U] = 2, [FACE_INDEX_B] = 11, [FACE_INDEX_D] = 13, [FACE_INDEX_F] = 4 },
            [FACE_INDEX_L] = { [FACE_INDEX_U] = 2, [FACE_INDEX_F] = 11, [FACE_INDEX_D] = 13, [FACE_INDEX_B] = 4 }
        }, {
            [FACE_INDEX_U] = { [FACE_INDEX_B] = 1, [FACE_INDEX_R] = 7, [FACE_INDEX_F] = 14, [FACE_INDEX_L] = 8 },
            [FACE_INDEX_D] = { [FACE_INDEX_F] = 1, [FACE_INDEX_R] = 7, [FACE_INDEX_B] = 14, [FACE_INDEX_L] = 8 },
            [FACE_INDEX_F] = { [FACE_INDEX_U] = 1, [FACE_INDEX_R] = 7, [FACE_INDEX_D] = 14, [FACE_INDEX_L] = 8 },
            [FACE_INDEX_B] = { [FACE_INDEX_U] = 1, [FACE_INDEX_L] = 7, [FACE_INDEX_D] = 14, [FACE_INDEX_R] = 8 },
            [FACE_INDEX_R] = { [FACE_INDEX_U] = 1, [FACE_INDEX_B] = 7, [FACE_INDEX_D] = 14, [FACE_INDEX_F] = 8 },
            [FACE_INDEX_L] = { [FACE_INDEX_U] = 1, [FACE_INDEX_F] = 7, [FACE_INDEX_D] = 14, [FACE_INDEX_B] = 8 }
        }
    };
    return index_lookup_table[is_primary][primary_face_index][secondary_face_index];
}

void big_visual_cube_state_write_wing(big_visual_cube_state_t* state, face_index_e primary_face, face_index_e secondary_face,
                                      face_index_e primary_sticker, face_index_e secondary_sticker) {
    int primary_wing_index   = big_visual_cube_get_wing_sticker_index(primary_face, secondary_face, true);
    int secondary_wing_index = big_visual_cube_get_wing_sticker_index(secondary_face, primary_face, false);

    state->stickers[primary_face][primary_wing_index]     = primary_sticker;
    state->stickers[secondary_face][secondary_wing_index] = secondary_sticker;
}

void big_visual_cube_state_copy_corners(big_visual_cube_state_t* big_visual_cube_state, visual_cube_state_t* visual_cube_state) {
    int n = big_visual_cube_state->n;
    for (int face = 0; face < 6; face++) {
        big_visual_cube_state->stickers[face][0]       = visual_cube_state->stickers[face][0];
        big_visual_cube_state->stickers[face][n - 1]   = visual_cube_state->stickers[face][2];
        big_visual_cube_state->stickers[face][n*n - n] = visual_cube_state->stickers[face][6];
        big_visual_cube_state->stickers[face][n*n - 1] = visual_cube_state->stickers[face][8];
    }
}

void draw_big_visual_cube_state(const big_visual_cube_state_t* big_visual_cube_state) {
    int n = big_visual_cube_state->n;

    int num_spaces = 2*n + 3;
    assert(num_spaces > 0);

    char spaces[num_spaces];
    memset(&spaces, ' ', (size_t)num_spaces - 1);
    spaces[num_spaces - 1] = '\0';

    for (int y = 0; y < n; y++) {
        printf("%s", spaces);
        for (int x = 0; x < n; x++) {
            draw_face_color(big_visual_cube_state->stickers[FACE_INDEX_U][y * n + x]);
        }
        printf("\n");
    }
    printf("\n");
    for (int y = 0; y < n; y++) {
        for (int x = 0; x < n; x++) {
            draw_face_color(big_visual_cube_state->stickers[FACE_INDEX_L][y * n + x]);
        }
        printf("  ");
        for (int x = 0; x < n; x++) {
            draw_face_color(big_visual_cube_state->stickers[FACE_INDEX_F][y * n + x]);
        }
        printf("  ");
        for (int x = 0; x < n; x++) {
            draw_face_color(big_visual_cube_state->stickers[FACE_INDEX_R][y * n + x]);
        }
        printf("  ");
        for (int x = 0; x < n; x++) {
            draw_face_color(big_visual_cube_state->stickers[FACE_INDEX_B][y * n + x]);
        }
        printf("\n");
    }
    printf("\n");
    for (int y = 0; y < n; y++) {
        printf("%s", spaces);
        for (int x = 0; x < n; x++) {
            draw_face_color(big_visual_cube_state->stickers[FACE_INDEX_D][y * n + x]);
        }
        printf("\n");
    }
}

char* get_big_visual_cube_state_string(const big_visual_cube_state_t* state, allocator_e allocator) {
    const int buffer_size = 6 * state->n * state->n + 1;
    char* buffer = alloc((size_t)buffer_size, allocator, SOURCE_LOCATION);
    int   buffer_index = 0;

    face_index_e face_order[6] = {
        FACE_INDEX_U, FACE_INDEX_L, FACE_INDEX_F, FACE_INDEX_R, FACE_INDEX_B, FACE_INDEX_D
    };

    int stickers_per_face = state->n * state->n;
    for (int i = 0; i < 6; i++) {
        face_index_e face = face_order[i];
        for (int j = 0; j < stickers_per_face; j++) {
            switch (state->stickers[face][j]) {
                case FACE_INDEX_U: buffer[buffer_index++] = 'U'; break;
                case FACE_INDEX_D: buffer[buffer_index++] = 'D'; break;
                case FACE_INDEX_F: buffer[buffer_index++] = 'F'; break;
                case FACE_INDEX_B: buffer[buffer_index++] = 'B'; break;
                case FACE_INDEX_R: buffer[buffer_index++] = 'R'; break;
                case FACE_INDEX_L: buffer[buffer_index++] = 'L'; break;
                default: assert(false);
            }
        }
    }

    buffer[buffer_index++] = '\0';

    assert(buffer_index == buffer_size);
    return buffer;
}
