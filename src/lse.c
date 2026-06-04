#include "lse.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include "common.h"

#define LSE_STATE_CORNER_STATE_MASK      0b00000011
#define LSE_STATE_CORNER_STATE_INCREMENT 0b00000001

#define LSE_STATE_CENTER_STATE_MASK      0b00110000
#define LSE_STATE_CENTER_STATE_INCREMENT 0b00010000

#define LSE_STATE_CORNER_CENTER_STATE_MASK (LSE_STATE_CORNER_STATE_MASK | LSE_STATE_CENTER_STATE_MASK)

#define LSE_STATE_EDGE_ORIENTATION_MASK 0b1000
#define LSE_STATE_EDGE_INDEX_MASK       0b0111
#define LSE_STATE_DOUBLE_EDGE_ORIENTATION_MASK 0b10001000

lse_state_t lse_move_u(lse_state_t lse_state) {
    uint8_t swap = lse_state.ul_ur_state;
    lse_state.ul_ur_state = lse_state.uf_ub_state;
    lse_state.uf_ub_state = byte_rotate_left(swap, 4);

    lse_state.center_corner_state += LSE_STATE_CORNER_STATE_INCREMENT;
    lse_state.center_corner_state &= LSE_STATE_CORNER_CENTER_STATE_MASK;

    return lse_state;
}

lse_state_t lse_move_u_prime(lse_state_t lse_state) {
    uint8_t swap = lse_state.uf_ub_state;
    lse_state.uf_ub_state = lse_state.ul_ur_state;
    lse_state.ul_ur_state = byte_rotate_left(swap, 4);

    lse_state.center_corner_state += 3 * LSE_STATE_CORNER_STATE_INCREMENT;
    lse_state.center_corner_state &= LSE_STATE_CORNER_CENTER_STATE_MASK;

    return lse_state;
}

lse_state_t lse_move_u2(lse_state_t lse_state) {
    lse_state.uf_ub_state = byte_rotate_left(lse_state.uf_ub_state, 4);
    lse_state.ul_ur_state = byte_rotate_left(lse_state.ul_ur_state, 4);

    lse_state.center_corner_state += 2 * LSE_STATE_CORNER_STATE_INCREMENT;
    lse_state.center_corner_state &= LSE_STATE_CORNER_CENTER_STATE_MASK;

    return lse_state;
}

lse_state_t lse_move_m(lse_state_t lse_state) {
    uint8_t prev_uf_ub_state = lse_state.uf_ub_state;
    uint8_t prev_df_db_state = lse_state.df_db_state;
    lse_state.uf_ub_state = (uint8_t)(((prev_uf_ub_state << 4) | (prev_df_db_state & 0b00001111)) ^ LSE_STATE_DOUBLE_EDGE_ORIENTATION_MASK);
    lse_state.df_db_state = (uint8_t)(((prev_df_db_state >> 4) | (prev_uf_ub_state & 0b11110000)) ^ LSE_STATE_DOUBLE_EDGE_ORIENTATION_MASK);

    lse_state.center_corner_state += LSE_STATE_CENTER_STATE_INCREMENT;
    lse_state.center_corner_state &= LSE_STATE_CORNER_CENTER_STATE_MASK;

    return lse_state;
}

lse_state_t lse_move_m_prime(lse_state_t lse_state) {
    uint8_t prev_uf_ub_state = lse_state.uf_ub_state;
    uint8_t prev_df_db_state = lse_state.df_db_state;
    lse_state.uf_ub_state = (uint8_t)(((prev_uf_ub_state >> 4) | (prev_df_db_state & 0b11110000)) ^ LSE_STATE_DOUBLE_EDGE_ORIENTATION_MASK);
    lse_state.df_db_state = (uint8_t)(((prev_df_db_state << 4) | (prev_uf_ub_state & 0b00001111)) ^ LSE_STATE_DOUBLE_EDGE_ORIENTATION_MASK);

    lse_state.center_corner_state += 3 * LSE_STATE_CENTER_STATE_INCREMENT;
    lse_state.center_corner_state &= LSE_STATE_CORNER_CENTER_STATE_MASK;

    return lse_state;
}

lse_state_t lse_move_m2(lse_state_t lse_state) {
    uint8_t swap = lse_state.uf_ub_state;
    lse_state.uf_ub_state = byte_rotate_left(lse_state.df_db_state, 4);
    lse_state.df_db_state = byte_rotate_left(swap, 4);

    lse_state.center_corner_state += 2 * LSE_STATE_CENTER_STATE_INCREMENT;
    lse_state.center_corner_state &= LSE_STATE_CORNER_CENTER_STATE_MASK;

    return lse_state;
}

lse_state_t generate_random_lse_state() {
    lse_state_t lse_state;
    lse_state.center_corner_state = random_u32() & LSE_STATE_CORNER_CENTER_STATE_MASK;

    bool odd_center_state = (bool)(lse_state.center_corner_state & LSE_STATE_CENTER_STATE_INCREMENT);
    bool odd_corner_state = (bool)(lse_state.center_corner_state & LSE_STATE_CORNER_STATE_INCREMENT);
    bool is_even_parity = odd_center_state == odd_corner_state;

    lse_state_edge_index_e edge_indices[6] = {
        LSE_STATE_EDGE_INDEX_UF, LSE_STATE_EDGE_INDEX_UB, LSE_STATE_EDGE_INDEX_UL,
        LSE_STATE_EDGE_INDEX_UR, LSE_STATE_EDGE_INDEX_DF, LSE_STATE_EDGE_INDEX_DB
    };
    for (int i = 0; i < 5; i++) {
        int swap_index;
        if (i < 4) {
            swap_index = i + (int)random_uniform(6 - (uint32_t)i);
        } else {
            swap_index = is_even_parity ? 4 : 5;
        }

        lse_state_edge_index_e swap = edge_indices[i];
        edge_indices[i] = edge_indices[swap_index];
        edge_indices[swap_index] = swap;
        is_even_parity ^= (swap_index != i);
    }
    assert(is_even_parity);

    lse_state.uf_ub_state = (uint8_t)((edge_indices[LSE_STATE_EDGE_INDEX_UF] << 4) | edge_indices[LSE_STATE_EDGE_INDEX_UB]);
    lse_state.ul_ur_state = (uint8_t)((edge_indices[LSE_STATE_EDGE_INDEX_UL] << 4) | edge_indices[LSE_STATE_EDGE_INDEX_UR]);
    lse_state.df_db_state = (uint8_t)((edge_indices[LSE_STATE_EDGE_INDEX_DF] << 4) | edge_indices[LSE_STATE_EDGE_INDEX_DB]);

    uint32_t edge_orientation = random_u32();
    lse_state.uf_ub_state ^= (edge_orientation << 0) & LSE_STATE_DOUBLE_EDGE_ORIENTATION_MASK;
    lse_state.ul_ur_state ^= (edge_orientation << 1) & LSE_STATE_DOUBLE_EDGE_ORIENTATION_MASK;
    lse_state.df_db_state ^= (edge_orientation << 2) & LSE_STATE_DOUBLE_EDGE_ORIENTATION_MASK;

    bool has_illegal_eo = byte_has_single_bit(
        (lse_state.uf_ub_state ^ lse_state.ul_ur_state ^ lse_state.df_db_state) & LSE_STATE_DOUBLE_EDGE_ORIENTATION_MASK
    );
    if (has_illegal_eo) {
        lse_state.df_db_state ^= LSE_STATE_EDGE_ORIENTATION_MASK;
    }

    return lse_state;
}

bool is_lse_solved(lse_state_t lse_state) {
    return memcmp(&lse_state, &SOLVED_LSE_STATE, sizeof(lse_state)) == 0;
}

// Checks if EOLR is solved if the UL/UR edges are in the UF/UB slots
bool was_eolr_just_solved(lse_state_t lse_state) {
    bool is_ul_ur_solved = lse_state.uf_ub_state == ((LSE_STATE_EDGE_INDEX_UL << 4) | LSE_STATE_EDGE_INDEX_UR) ||
                          lse_state.uf_ub_state == ((LSE_STATE_EDGE_INDEX_UR << 4) | LSE_STATE_EDGE_INDEX_UL);

    uint8_t df_db_eo = lse_state.df_db_state & LSE_STATE_DOUBLE_EDGE_ORIENTATION_MASK;
    uint8_t ul_ur_eo = lse_state.ul_ur_state & LSE_STATE_DOUBLE_EDGE_ORIENTATION_MASK;
    bool matching_eo = df_db_eo == ul_ur_eo && !byte_has_single_bit(df_db_eo);

    bool eo_matches_centers = (bool)(lse_state.center_corner_state & LSE_STATE_CENTER_STATE_INCREMENT) ==
                              (bool)(lse_state.df_db_state & LSE_STATE_EDGE_ORIENTATION_MASK);

    uint8_t corner_state = lse_state.center_corner_state & LSE_STATE_CORNER_STATE_MASK;
    bool is_ul_in_ub = (lse_state.uf_ub_state & LSE_STATE_EDGE_INDEX_MASK) == LSE_STATE_EDGE_INDEX_UL;

    const uint8_t CORNER_MOD = LSE_STATE_CORNER_STATE_INCREMENT;
    bool corners_align_with_ul_ur = (bool)(corner_state & CORNER_MOD) && (corner_state == CORNER_MOD) == is_ul_in_ub;

    return is_ul_ur_solved && matching_eo && eo_matches_centers && corners_align_with_ul_ur;
}

const char* lse_move_to_string(lse_move_e move) {
    switch (move) {
        case LSE_MOVE_U:       return "U";
        case LSE_MOVE_U2:      return "U2";
        case LSE_MOVE_U_PRIME: return "U'";
        case LSE_MOVE_M:       return "M";
        case LSE_MOVE_M2:      return "M2";
        case LSE_MOVE_M_PRIME: return "M'";
    }
    assert(false);
}

#define LSE_MOVE_TYPE_MASK  0b100
#define LSE_MOVE_COUNT_MASK 0b011
void lse_move_list_simplify_append(lse_move_list_t* list, lse_move_e move) {
    if (list->size > 0) {
        lse_move_e* prev_move = &list->data[list->size - 1];
        if ((*prev_move & LSE_MOVE_TYPE_MASK) == (move & LSE_MOVE_TYPE_MASK)) {
            int move_count = (*prev_move + move) & LSE_MOVE_COUNT_MASK;
            if (move_count == 0) {
                list->size--;
            } else {
                *prev_move = (lse_move_e)((move & LSE_MOVE_TYPE_MASK) | move_count);
            }
            return;
        }
    }
    array_append(*list, move);
}

void free_lse_solution_list(lse_solution_list_t* solution_list) {
    for (size_t i = 0; i < solution_list->size; i++) {
        array_free(solution_list->data[i]);
    }
    array_free(*solution_list);
}

void _solve_eolr_recursive_m(lse_state_t lse_state, lse_move_list_t* moves, lse_solution_list_t* solutions, int depth);

void _solve_eolr_recursive_u(lse_state_t lse_state, lse_move_list_t* moves, lse_solution_list_t* solutions, int depth) {
    if (was_eolr_just_solved(lse_state)) {
        lse_move_list_t solution = { .allocator = solutions->allocator };
        array_copy(*moves, solution);
        array_append(*solutions, solution);
        return;
    }
    if (depth <= 0) {
        return;
    }

    array_append(*moves, LSE_MOVE_U);
    _solve_eolr_recursive_m(lse_move_u(lse_state), moves, solutions, depth - 1);

    moves->data[moves->size - 1] = LSE_MOVE_U_PRIME;
    _solve_eolr_recursive_m(lse_move_u_prime(lse_state), moves, solutions, depth - 1);

    moves->data[moves->size - 1] = LSE_MOVE_U2;
    _solve_eolr_recursive_m(lse_move_u2(lse_state), moves, solutions, depth - 1);

    moves->size--;
}

void _solve_eolr_recursive_m(lse_state_t lse_state, lse_move_list_t* moves, lse_solution_list_t* solutions, int depth) {
    if (was_eolr_just_solved(lse_state)) {
        lse_move_list_t solution = { .allocator = solutions->allocator };
        array_copy(*moves, solution);
        array_append(*solutions, solution);
        return;
    }
    if (depth <= 0) {
        return;
    }

    array_append(*moves, LSE_MOVE_M);
    _solve_eolr_recursive_u(lse_move_m(lse_state), moves, solutions, depth - 1);

    moves->data[moves->size - 1] = LSE_MOVE_M_PRIME;
    _solve_eolr_recursive_u(lse_move_m_prime(lse_state), moves, solutions, depth - 1);

    moves->data[moves->size - 1] = LSE_MOVE_M2;
    _solve_eolr_recursive_u(lse_move_m2(lse_state), moves, solutions, depth - 1);

    moves->size--;
}

lse_solution_list_t solve_eolr(lse_state_t lse_state, allocator_e allocator) {
    lse_move_list_t moves = { .allocator = TEMP_ALLOCATOR };

    lse_solution_list_t solutions = { .allocator = allocator };
    // TODO: Maximum number of moves to solve EOLR
    for (int depth = 0; depth <= 18 && solutions.size == 0; depth++) {
        printf("Searching depth %i\n", depth);
        _solve_eolr_recursive_u(lse_state, &moves, &solutions, depth);
        _solve_eolr_recursive_m(lse_state, &moves, &solutions, depth);
    }
    array_free(moves);

    return solutions;
}

void _solve_lse_recursive_m(lse_state_t lse_state, lse_move_list_t* moves, lse_solution_list_t* solutions, int depth);

void _solve_lse_recursive_u(lse_state_t lse_state, lse_move_list_t* moves, lse_solution_list_t* solutions, int depth) {
    if (is_lse_solved(lse_state)) {
        lse_move_list_t solution = { .allocator = solutions->allocator };
        array_copy(*moves, solution);
        array_append(*solutions, solution);
        return;
    }
    if (depth <= 0) {
        return;
    }

    array_append(*moves, LSE_MOVE_U);
    _solve_lse_recursive_m(lse_move_u(lse_state), moves, solutions, depth - 1);

    moves->data[moves->size - 1] = LSE_MOVE_U_PRIME;
    _solve_lse_recursive_m(lse_move_u_prime(lse_state), moves, solutions, depth - 1);

    moves->data[moves->size - 1] = LSE_MOVE_U2;
    _solve_lse_recursive_m(lse_move_u2(lse_state), moves, solutions, depth - 1);

    moves->size--;
}

void _solve_lse_recursive_m(lse_state_t lse_state, lse_move_list_t* moves, lse_solution_list_t* solutions, int depth) {
    if (is_lse_solved(lse_state)) {
        lse_move_list_t solution = { .allocator = solutions->allocator };
        array_copy(*moves, solution);
        array_append(*solutions, solution);
        return;
    }
    if (depth <= 0) {
        return;
    }

    array_append(*moves, LSE_MOVE_M);
    _solve_lse_recursive_u(lse_move_m(lse_state), moves, solutions, depth - 1);

    moves->data[moves->size - 1] = LSE_MOVE_M_PRIME;
    _solve_lse_recursive_u(lse_move_m_prime(lse_state), moves, solutions, depth - 1);

    moves->data[moves->size - 1] = LSE_MOVE_M2;
    _solve_lse_recursive_u(lse_move_m2(lse_state), moves, solutions, depth - 1);

    moves->size--;
}

lse_solution_list_t solve_lse(lse_state_t lse_state, allocator_e allocator) {
    lse_move_list_t moves = { .allocator = TEMP_ALLOCATOR };

    lse_solution_list_t solutions = { .allocator = allocator };
    // TODO: Maximum number of moves to solve LSE
    for (int depth = 0; depth <= 18 && solutions.size == 0; depth++) {
        printf("Searching depth %i\n", depth);
        _solve_lse_recursive_u(lse_state, &moves, &solutions, depth);
        _solve_lse_recursive_m(lse_state, &moves, &solutions, depth);
    }
    array_free(moves);

    return solutions;
}

face_index_e lse_state_get_edge_face(int edge_state, bool is_misoriented) {
    bool should_return_oriented = (bool)(edge_state & LSE_STATE_EDGE_ORIENTATION_MASK) == is_misoriented;
    switch (edge_state & LSE_STATE_EDGE_INDEX_MASK) {
        case LSE_STATE_EDGE_INDEX_UF:
            return should_return_oriented ? FACE_INDEX_U : FACE_INDEX_F;
        case LSE_STATE_EDGE_INDEX_UB:
            return should_return_oriented ? FACE_INDEX_U : FACE_INDEX_B;
        case LSE_STATE_EDGE_INDEX_UL:
            return should_return_oriented ? FACE_INDEX_U : FACE_INDEX_L;
        case LSE_STATE_EDGE_INDEX_UR:
            return should_return_oriented ? FACE_INDEX_U : FACE_INDEX_R;
        case LSE_STATE_EDGE_INDEX_DF:
            return should_return_oriented ? FACE_INDEX_D : FACE_INDEX_F;
        case LSE_STATE_EDGE_INDEX_DB:
            return should_return_oriented ? FACE_INDEX_D : FACE_INDEX_B;
    }
    unreachable();
}

face_index_e lse_state_get_center_face(int center_state, int target_center) {
    switch ((center_state + target_center) & 0b11) {
        case 0: return FACE_INDEX_U;
        case 1: return FACE_INDEX_B;
        case 2: return FACE_INDEX_D;
        case 3: return FACE_INDEX_F;
    }
    unreachable();
}

face_index_e lse_state_get_corner_front_face(int corner_state, int target_corner) {
    switch ((corner_state + target_corner) & 0b11) {
        case 0: return FACE_INDEX_F;
        case 1: return FACE_INDEX_R;
        case 2: return FACE_INDEX_B;
        case 3: return FACE_INDEX_L;
    }
    unreachable();
}

void visual_cube_state_write_lse_state_edge(visual_cube_state_t* visual_cube_state, visual_cube_edge_e edge, int state) {
    face_index_e primary_face_index   = (face_index_e)byte_ctz(edge);
    face_index_e secondary_face_index = (face_index_e)byte_ctz((uint8_t)(edge ^ (1 << primary_face_index)));

    int primary_sticker_index   = visual_cube_get_edge_sticker_index(primary_face_index, secondary_face_index);
    int secondary_sticker_index = visual_cube_get_edge_sticker_index(secondary_face_index, primary_face_index);

    visual_cube_state->stickers[ primary_face_index ][ primary_sticker_index ] = lse_state_get_edge_face(state, false);
    visual_cube_state->stickers[secondary_face_index][secondary_sticker_index] = lse_state_get_edge_face(state, true);
}

void lse_state_write_visual_cube_state(lse_state_t lse_state, visual_cube_state_t* visual_cube_state) {
    visual_cube_state_reset(visual_cube_state);

    visual_cube_state_write_lse_state_edge(visual_cube_state, VISUAL_CUBE_EDGE_UF, lse_state.uf_ub_state >> 4);
    visual_cube_state_write_lse_state_edge(visual_cube_state, VISUAL_CUBE_EDGE_UB, lse_state.uf_ub_state & 0b1111);
    visual_cube_state_write_lse_state_edge(visual_cube_state, VISUAL_CUBE_EDGE_UL, lse_state.ul_ur_state >> 4);
    visual_cube_state_write_lse_state_edge(visual_cube_state, VISUAL_CUBE_EDGE_UR, lse_state.ul_ur_state & 0b1111);
    visual_cube_state_write_lse_state_edge(visual_cube_state, VISUAL_CUBE_EDGE_DF, lse_state.df_db_state >> 4);
    visual_cube_state_write_lse_state_edge(visual_cube_state, VISUAL_CUBE_EDGE_DB, lse_state.df_db_state & 0b1111);

    int center_state = (lse_state.center_corner_state & LSE_STATE_CENTER_STATE_MASK) >> 4;
    visual_cube_state_write_center(visual_cube_state, FACE_INDEX_U, lse_state_get_center_face(center_state, 0));
    visual_cube_state_write_center(visual_cube_state, FACE_INDEX_B, lse_state_get_center_face(center_state, 1));
    visual_cube_state_write_center(visual_cube_state, FACE_INDEX_D, lse_state_get_center_face(center_state, 2));
    visual_cube_state_write_center(visual_cube_state, FACE_INDEX_F, lse_state_get_center_face(center_state, 3));

    int corner_state = lse_state.center_corner_state & LSE_STATE_CORNER_STATE_MASK;
    face_index_e face_index_f = lse_state_get_corner_front_face(corner_state, 0);
    face_index_e face_index_r = lse_state_get_corner_front_face(corner_state, 1);
    face_index_e face_index_b = lse_state_get_corner_front_face(corner_state, 2);
    face_index_e face_index_l = lse_state_get_corner_front_face(corner_state, 3);
    visual_cube_state_write_corner(visual_cube_state, VISUAL_CUBE_CORNER_UBL, FACE_INDEX_U, face_index_b, face_index_l);
    visual_cube_state_write_corner(visual_cube_state, VISUAL_CUBE_CORNER_UBR, FACE_INDEX_U, face_index_b, face_index_r);
    visual_cube_state_write_corner(visual_cube_state, VISUAL_CUBE_CORNER_UFR, FACE_INDEX_U, face_index_f, face_index_r);
    visual_cube_state_write_corner(visual_cube_state, VISUAL_CUBE_CORNER_UFL, FACE_INDEX_U, face_index_f, face_index_l);
}
