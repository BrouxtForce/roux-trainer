#include "revenge.h"
#include <string.h>

void revenge_g0_move_face_cw(revenge_g0_state_t* state, face_index_e face) {
    state->centers[face] = byte_rotate_right(state->centers[face], 2);
}

// TODO: If revenge_g0_state_t is rewritten to be a 64-bit integer, this function can be simplified
// to use fewer operations very easily
void revenge_g0_move_uw(revenge_g0_state_t* state) {
    revenge_g0_move_face_cw(state, FACE_INDEX_U);

    uint8_t swap = state->centers[FACE_INDEX_F] & 0b11110000;

    state->centers[FACE_INDEX_F] &= 0b00001111;
    state->centers[FACE_INDEX_F] |= state->centers[FACE_INDEX_R] & 0b11110000;

    state->centers[FACE_INDEX_R] &= 0b00001111;
    state->centers[FACE_INDEX_R] |= state->centers[FACE_INDEX_B] & 0b11110000;

    state->centers[FACE_INDEX_B] &= 0b00001111;
    state->centers[FACE_INDEX_B] |= state->centers[FACE_INDEX_L] & 0b11110000;

    state->centers[FACE_INDEX_L] &= 0b00001111;
    state->centers[FACE_INDEX_L] |= swap;

    state->parity ^= 1;
}

void revenge_g0_move_fw(revenge_g0_state_t* state) {
    revenge_g0_move_face_cw(state, FACE_INDEX_F);

    uint8_t swap = state->centers[FACE_INDEX_U] & 0b00001111;

    state->centers[FACE_INDEX_U] &= 0b11110000;
    state->centers[FACE_INDEX_U] |= (state->centers[FACE_INDEX_L] & 0b00111100) >> 2;

    state->centers[FACE_INDEX_L] &= 0b11000011;
    state->centers[FACE_INDEX_L] |= (state->centers[FACE_INDEX_D] & 0b11110000) >> 2;

    state->centers[FACE_INDEX_D] &= 0b00001111;
    state->centers[FACE_INDEX_D] |= byte_rotate_right(state->centers[FACE_INDEX_R] & 0b11000011, 2);

    state->centers[FACE_INDEX_R] &= 0b00111100;
    state->centers[FACE_INDEX_R] |= byte_rotate_right(swap, 2);

    state->parity ^= 1;
}

void revenge_g0_move_rw(revenge_g0_state_t* state) {
    revenge_g0_move_face_cw(state, FACE_INDEX_R);

    uint8_t swap = state->centers[FACE_INDEX_U] & 0b00111100;

    state->centers[FACE_INDEX_U] &= 0b11000011;
    state->centers[FACE_INDEX_U] |= state->centers[FACE_INDEX_F] & 0b00111100;

    state->centers[FACE_INDEX_F] &= 0b11000011;
    state->centers[FACE_INDEX_F] |= state->centers[FACE_INDEX_D] & 0b00111100;

    state->centers[FACE_INDEX_D] &= 0b11000011;
    state->centers[FACE_INDEX_D] |= byte_rotate_right(state->centers[FACE_INDEX_B] & 0b11000011, 4);

    state->centers[FACE_INDEX_B] &= 0b00111100;
    state->centers[FACE_INDEX_B] |= byte_rotate_right(swap, 4);

    state->parity ^= 1;
}

void revenge_g0_execute_move(revenge_g0_state_t* state, move_t move) {
    move_composition_t move_composition = decompose_move(move.move);

    if (move.width == 2) {
        for (int i = 0; i < move_composition.count_cw; i++) {
            switch (move_composition.face_index) {
                case FACE_INDEX_U: revenge_g0_move_uw(state); break;
                case FACE_INDEX_F: revenge_g0_move_fw(state); break;
                case FACE_INDEX_R: revenge_g0_move_rw(state); break;
                default: assert(false);
            }
        }
        return;
    }

    assert(move.width == 1);
    for (int i = 0; i < move_composition.count_cw; i++) {
        revenge_g0_move_face_cw(state, move_composition.face_index);
    }
}

static void revenge_g1_move_centers(revenge_g1_state_t* state, face_index_e face) {
    state->centers[face] = byte_rotate_right(state->centers[face], 2);
}

// TODO: For now, we use the g0 functions for executing g1. We should probably write custom functions for g1.
static void revenge_g1_move_centers_wide(revenge_g1_state_t* state, face_index_e face) {
    revenge_g0_state_t g0_state = {};
    memcpy(g0_state.centers, &state->centers, sizeof(state->centers));
    switch (face) {
        case FACE_INDEX_U: revenge_g0_move_uw(&g0_state); break;
        case FACE_INDEX_F: revenge_g0_move_fw(&g0_state); break;
        case FACE_INDEX_R: revenge_g0_move_rw(&g0_state); break;
        default: assert(false);
    }
    memcpy(state->centers, g0_state.centers, sizeof(state->centers));
}

static void revenge_g1_cycle_wings_cw(revenge_g1_state_t* state, wing_index_e wing_a, wing_index_e wing_b, wing_index_e wing_c, wing_index_e wing_d) {
    wing_index_e swap = state->wings[wing_a];
    state->wings[wing_a] = state->wings[wing_d];
    state->wings[wing_d] = state->wings[wing_c];
    state->wings[wing_c] = state->wings[wing_b];
    state->wings[wing_b] = swap;
}

void revenge_g1_move_u(revenge_g1_state_t* state) {
    revenge_g1_move_centers(state, FACE_INDEX_U);
    revenge_g1_cycle_wings_cw(state, WING_INDEX_UB, WING_INDEX_UR, WING_INDEX_UF, WING_INDEX_UL);
    revenge_g1_cycle_wings_cw(state, WING_INDEX_BU, WING_INDEX_RU, WING_INDEX_FU, WING_INDEX_LU);
}

void revenge_g1_move_d(revenge_g1_state_t* state) {
    revenge_g1_move_centers(state, FACE_INDEX_D);
    revenge_g1_cycle_wings_cw(state, WING_INDEX_DF, WING_INDEX_DR, WING_INDEX_DB, WING_INDEX_DL);
    revenge_g1_cycle_wings_cw(state, WING_INDEX_FD, WING_INDEX_RD, WING_INDEX_BD, WING_INDEX_LD);
}

void revenge_g1_move_f(revenge_g1_state_t* state) {
    revenge_g1_move_centers(state, FACE_INDEX_F);
    revenge_g1_cycle_wings_cw(state, WING_INDEX_FU, WING_INDEX_FR, WING_INDEX_FD, WING_INDEX_FL);
    revenge_g1_cycle_wings_cw(state, WING_INDEX_UF, WING_INDEX_RF, WING_INDEX_DF, WING_INDEX_LF);
}

void revenge_g1_move_b(revenge_g1_state_t* state) {
    revenge_g1_move_centers(state, FACE_INDEX_B);
    revenge_g1_cycle_wings_cw(state, WING_INDEX_BU, WING_INDEX_BL, WING_INDEX_BD, WING_INDEX_BR);
    revenge_g1_cycle_wings_cw(state, WING_INDEX_UB, WING_INDEX_LB, WING_INDEX_DB, WING_INDEX_RB);
}

void revenge_g1_move_r(revenge_g1_state_t* state) {
    revenge_g1_move_centers(state, FACE_INDEX_R);
    revenge_g1_cycle_wings_cw(state, WING_INDEX_RU, WING_INDEX_RB, WING_INDEX_RD, WING_INDEX_RF);
    revenge_g1_cycle_wings_cw(state, WING_INDEX_UR, WING_INDEX_BR, WING_INDEX_DR, WING_INDEX_FR);
}

void revenge_g1_move_l(revenge_g1_state_t* state) {
    revenge_g1_move_centers(state, FACE_INDEX_L);
    revenge_g1_cycle_wings_cw(state, WING_INDEX_LU, WING_INDEX_LF, WING_INDEX_LD, WING_INDEX_LB);
    revenge_g1_cycle_wings_cw(state, WING_INDEX_UL, WING_INDEX_FL, WING_INDEX_DL, WING_INDEX_BL);
}

void revenge_g1_move_uw(revenge_g1_state_t* state) {
    revenge_g1_move_centers_wide(state, FACE_INDEX_U);
    revenge_g1_cycle_wings_cw(state, WING_INDEX_UB, WING_INDEX_UR, WING_INDEX_UF, WING_INDEX_UL);
    revenge_g1_cycle_wings_cw(state, WING_INDEX_BU, WING_INDEX_RU, WING_INDEX_FU, WING_INDEX_LU);
    revenge_g1_cycle_wings_cw(state, WING_INDEX_FR, WING_INDEX_LF, WING_INDEX_BL, WING_INDEX_RB);
}

void revenge_g1_move_fw(revenge_g1_state_t* state) {
    revenge_g1_move_centers_wide(state, FACE_INDEX_F);
    revenge_g1_cycle_wings_cw(state, WING_INDEX_FU, WING_INDEX_FR, WING_INDEX_FD, WING_INDEX_FL);
    revenge_g1_cycle_wings_cw(state, WING_INDEX_UF, WING_INDEX_RF, WING_INDEX_DF, WING_INDEX_LF);
    revenge_g1_cycle_wings_cw(state, WING_INDEX_RU, WING_INDEX_DR, WING_INDEX_LD, WING_INDEX_UL);
}

void revenge_g1_move_rw(revenge_g1_state_t* state) {
    revenge_g1_move_centers_wide(state, FACE_INDEX_R);
    revenge_g1_cycle_wings_cw(state, WING_INDEX_RU, WING_INDEX_RB, WING_INDEX_RD, WING_INDEX_RF);
    revenge_g1_cycle_wings_cw(state, WING_INDEX_UR, WING_INDEX_BR, WING_INDEX_DR, WING_INDEX_FR);
    revenge_g1_cycle_wings_cw(state, WING_INDEX_UF, WING_INDEX_BU, WING_INDEX_DB, WING_INDEX_FD);
}

static void revenge_g1_wing_get_primary_and_secondary_stickers(wing_index_e wing_index, face_index_e* primary_sticker, face_index_e* secondary_sticker) {
    // TODO: We probably don't need a lookup table for the primary sticker
    static face_index_e primary_lookup[24] = {
        [WING_INDEX_UB] = FACE_INDEX_U, [WING_INDEX_UR] = FACE_INDEX_U, [WING_INDEX_UF] = FACE_INDEX_U, [WING_INDEX_UL] = FACE_INDEX_U,
        [WING_INDEX_LU] = FACE_INDEX_L, [WING_INDEX_LF] = FACE_INDEX_L, [WING_INDEX_LD] = FACE_INDEX_L, [WING_INDEX_LB] = FACE_INDEX_L,
        [WING_INDEX_FU] = FACE_INDEX_F, [WING_INDEX_FR] = FACE_INDEX_F, [WING_INDEX_FD] = FACE_INDEX_F, [WING_INDEX_FL] = FACE_INDEX_F,
        [WING_INDEX_RU] = FACE_INDEX_R, [WING_INDEX_RB] = FACE_INDEX_R, [WING_INDEX_RD] = FACE_INDEX_R, [WING_INDEX_RF] = FACE_INDEX_R,
        [WING_INDEX_BU] = FACE_INDEX_B, [WING_INDEX_BL] = FACE_INDEX_B, [WING_INDEX_BD] = FACE_INDEX_B, [WING_INDEX_BR] = FACE_INDEX_B,
        [WING_INDEX_DF] = FACE_INDEX_D, [WING_INDEX_DR] = FACE_INDEX_D, [WING_INDEX_DB] = FACE_INDEX_D, [WING_INDEX_DL] = FACE_INDEX_D
    };
    static face_index_e secondary_lookup[24] = {
        [WING_INDEX_UB] = FACE_INDEX_B, [WING_INDEX_UR] = FACE_INDEX_R, [WING_INDEX_UF] = FACE_INDEX_F, [WING_INDEX_UL] = FACE_INDEX_L,
        [WING_INDEX_LU] = FACE_INDEX_U, [WING_INDEX_LF] = FACE_INDEX_F, [WING_INDEX_LD] = FACE_INDEX_D, [WING_INDEX_LB] = FACE_INDEX_B,
        [WING_INDEX_FU] = FACE_INDEX_U, [WING_INDEX_FR] = FACE_INDEX_R, [WING_INDEX_FD] = FACE_INDEX_D, [WING_INDEX_FL] = FACE_INDEX_L,
        [WING_INDEX_RU] = FACE_INDEX_U, [WING_INDEX_RB] = FACE_INDEX_B, [WING_INDEX_RD] = FACE_INDEX_D, [WING_INDEX_RF] = FACE_INDEX_F,
        [WING_INDEX_BU] = FACE_INDEX_U, [WING_INDEX_BL] = FACE_INDEX_L, [WING_INDEX_BD] = FACE_INDEX_D, [WING_INDEX_BR] = FACE_INDEX_R,
        [WING_INDEX_DF] = FACE_INDEX_F, [WING_INDEX_DR] = FACE_INDEX_R, [WING_INDEX_DB] = FACE_INDEX_B, [WING_INDEX_DL] = FACE_INDEX_L
    };

    *primary_sticker   = primary_lookup[wing_index];
    *secondary_sticker = secondary_lookup[wing_index];
}

void revenge_write_visual_cube_state(big_visual_cube_state_t* state, revenge_g0_state_t revenge_g0_state, revenge_g1_state_t revenge_g1_state, g0_state_t g0_state, g1_state_t g1_state) {
    for (int face = 0; face < 6; face++) {
        face_index_e a0 =  revenge_g0_state.centers[face] >> 6;
        face_index_e b0 = (revenge_g0_state.centers[face] >> 4) & 0b11;
        face_index_e c0 = (revenge_g0_state.centers[face] >> 2) & 0b11;
        face_index_e d0 = (revenge_g0_state.centers[face] >> 0) & 0b11;

        face_index_e a1 =  revenge_g1_state.centers[face] >> 6;
        face_index_e b1 = (revenge_g1_state.centers[face] >> 4) & 0b11;
        face_index_e c1 = (revenge_g1_state.centers[face] >> 2) & 0b11;
        face_index_e d1 = (revenge_g1_state.centers[face] >> 0) & 0b11;

        big_visual_cube_state_write_center(state, face, 0, 2*a0 + a1);
        big_visual_cube_state_write_center(state, face, 1, 2*b0 + b1);
        big_visual_cube_state_write_center(state, face, 2, 2*d0 + d1);
        big_visual_cube_state_write_center(state, face, 3, 2*c0 + c1);
    }

    for (int wing = 0; wing < 24; wing++) {
        face_index_e primary_face, secondary_face;
        revenge_g1_wing_get_primary_and_secondary_stickers(wing, &primary_face, &secondary_face);

        face_index_e primary_sticker, secondary_sticker;
        revenge_g1_wing_get_primary_and_secondary_stickers(revenge_g1_state.wings[wing], &primary_sticker, &secondary_sticker);

        big_visual_cube_state_write_wing(state, primary_face, secondary_face, primary_sticker, secondary_sticker);
    }

    visual_cube_state_t visual_cube_state = {};
    g0_g1_state_write_visual_cube_state(&visual_cube_state, g0_state, g1_state);

    big_visual_cube_state_copy_corners(state, &visual_cube_state);
}

static bool revenge_g0_state_equals(revenge_g0_state_t left, revenge_g0_state_t right) {
    assert(left._padding == 0 && right._padding == 0);
    return memcmp(&left, &right, sizeof(revenge_g0_state_t)) == 0;
}

#define HASH_TABLE_NAME revenge_g0_table
#define STATE_TYPE revenge_g0_state_t
#define STATE_EQUALS(a, b) revenge_g0_state_equals(a, b)
#define STATE_TO_U64(state) (*(uint64_t*)&state)
#define STATE_NULL REVENGE_G0_STATE_NULL
#define HASH_TABLE_SIZE REVENGE_G0_TABLE_SIZE
#include "hash_table.h"

void _recursive_revenge_g0_fill_table(revenge_g0_table_t* g0_table, revenge_g0_state_t g0_state, int depth, int distance_from_solved) {
    if (!revenge_g0_table_insert(g0_table, g0_state, distance_from_solved)) {
        return;
    }

    if (depth <= 0) {
        return;
    }

    revenge_g0_state_t original_state = g0_state;
    for (int face = 0; face < 6; face++) {
        move_e base_move = 3 * face;

        // TODO: Move pruning

        g0_state = original_state;
        for (int j = 0; j < 3; j++) {
            revenge_g0_move_face_cw(&g0_state, face);
            _recursive_revenge_g0_fill_table(g0_table, g0_state, depth - 1, distance_from_solved + 1);
        }

        g0_state = original_state;
        for (int j = 0; j < 3; j++) {
            switch (base_move) {
                case MOVE_U: revenge_g0_move_uw(&g0_state); break;
                case MOVE_F: revenge_g0_move_fw(&g0_state); break;
                case MOVE_R: revenge_g0_move_rw(&g0_state); break;
                default: continue;
            }
            _recursive_revenge_g0_fill_table(g0_table, g0_state, depth - 1, distance_from_solved + 1);
        }
    }
}

revenge_g0_table_t* revenge_g0_init_table(allocator_e allocator) {
    revenge_g0_table_t* table = alloc(sizeof(revenge_g0_table_t), allocator, SOURCE_LOCATION);
    memset(table, 0, sizeof *table);

    // TODO: Accomodate for the multiple solved states
    table->magic = random_u64();
    _recursive_revenge_g0_fill_table(table, REVENGE_G0_STATE_SOLVED, REVENGE_G0_TABLE_DEPTH, 0);

    assert(table->count == REVENGE_G0_TABLE_SIZE);

    return table;
}

bool revenge_g0_is_solved(revenge_g0_state_t state) {
    if (state.parity) {
        return false;
    }
    for (int i = 0; i < 6; i++) {
        uint8_t center = state.centers[i];
        uint8_t a = (center) & 0b11;
        uint8_t b = (center >> 2) & 0b11;
        uint8_t c = (center >> 4) & 0b11;
        uint8_t d = center >> 6;

        if (a == b && b == c && c == d) {
            continue;
        }
        return false;
    }
    return state.centers[FACE_INDEX_U] == state.centers[FACE_INDEX_D] &&
           state.centers[FACE_INDEX_F] == state.centers[FACE_INDEX_B] &&
           state.centers[FACE_INDEX_R] == state.centers[FACE_INDEX_L];
}

static void _search_revenge_g0_helper(revenge_g0_table_t* table, revenge_g0_state_t state, g_move_list_t* move_list, g_solution_list_t* solution_list, int depth) {
    if (revenge_g0_is_solved(state)) {
        g_move_list_t copy_move_list = { .allocator = solution_list->allocator };
        array_copy(*move_list, copy_move_list);
        array_append(*solution_list, copy_move_list);
        return;
    }

    if (depth <= 0) return;

    int distance_from_solved = revenge_g0_table_lookup(table, state);
    assert(distance_from_solved != 0);
    if (distance_from_solved == -1) {
        // Best case scenario
        distance_from_solved = REVENGE_G0_TABLE_DEPTH + 1;
    }

    if (depth < distance_from_solved) return;

    revenge_g0_state_t original_state = state;

    for (int face = 0; face < 6; face++) {
        move_e base_move = 3 * face;

        // TODO: Move pruning

        state = original_state;
        for (int j = 0; j < 3; j++) {
            revenge_g0_move_face_cw(&state, face);

            move_t move = { .move = base_move + j, .width = 1 };

            array_append(*move_list, move);
            _search_revenge_g0_helper(table, state, move_list, solution_list, depth - 1);
            array_pop(*move_list);
        }

        state = original_state;
        for (int j = 0; j < 3; j++) {
            switch (base_move) {
                case MOVE_U: revenge_g0_move_uw(&state); break;
                case MOVE_F: revenge_g0_move_fw(&state); break;
                case MOVE_R: revenge_g0_move_rw(&state); break;
                default: continue;
            }

            move_t move = { .move = base_move + j, .width = 2 };

            array_append(*move_list, move);
            _search_revenge_g0_helper(table, state, move_list, solution_list, depth - 1);
            array_pop(*move_list);
        }
    }
}

static g_solution_list_t _solve_revenge_g0_at_depth(revenge_g0_table_t* table, revenge_g0_state_t state, int depth, allocator_e allocator) {
    g_solution_list_t solution_list = { .allocator = allocator };
    g_move_list_t move_list = { .allocator = TEMP_ALLOCATOR };

    _search_revenge_g0_helper(table, state, &move_list, &solution_list, depth);

    return solution_list;
}

g_solution_list_t solve_revenge_g0(revenge_g0_table_t* table, revenge_g0_state_t state, allocator_e allocator) {
    // TODO: What is the farthest away from solved that revenge_g0 can be?
    for (int depth = 0; depth <= 18; depth++) {
        g_solution_list_t solutions = _solve_revenge_g0_at_depth(table, state, depth, allocator);
        if (solutions.size > 0) {
            return solutions;
        }
    }

    assert(false);
    return (g_solution_list_t){};
}
