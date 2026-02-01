#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdlib.h>

#include "kociemba.c"

#define array_append(array, element) \
    do { \
        if ((array).size >= (array).capacity) { \
            if ((array).capacity == 0) { \
                (array).capacity = 8; \
            } else { \
                (array).capacity *= 2; \
            } \
            (array).data = realloc((array).data, (array).capacity * sizeof *(array).data); \
        } \
        (array).data[(array).size++] = (element); \
    } while (false)

#define array_copy(src_array, copy_array) \
    do { \
        (copy_array).data = malloc((src_array).size); \
        (copy_array).size = (src_array).size; \
        (copy_array).capacity = (src_array).capacity; \
        memcpy((copy_array).data, (src_array).data, (src_array).size * sizeof *(copy_array).data); \
    } while (false)

#define array_free(array) \
    do { \
        free((array).data); \
        (array).data = NULL; \
        (array).size = 0; \
        (array).capacity = 0; \
    } while (false)

int byte_popcount(uint8_t value) {
    return __builtin_popcountg(value);
}

int byte_ctz(uint8_t value) {
    return __builtin_ctzg(value);
}

bool byte_has_single_bit(uint8_t value) {
    return byte_popcount(value) == 1;
}

typedef struct {
    // Both corner state and center state are stored in 2 bits, as there are only four possibilities
    // for either in LSE.
    uint8_t center_corner_state;

    // Edges are stored as 4-bit sequences, with the 3 least significant bits representing the
    // specific edge piece by index, and the most significant bit representing EO (zero representing
    // an oriented edge, and one representing a misoriented edge). Since there are six edges, each of
    // the six 4-bit sequences can be packed into three 8-bit integers.
    uint8_t uf_ub_state;
    uint8_t ul_ur_state;
    uint8_t df_db_state;
} lse_state_t;

#define LSE_STATE_CORNER_STATE_MASK      0b00000011
#define LSE_STATE_CORNER_STATE_INCREMENT 0b00000001

#define LSE_STATE_CENTER_STATE_MASK      0b00110000
#define LSE_STATE_CENTER_STATE_INCREMENT 0b00010000

#define LSE_STATE_CORNER_CENTER_STATE_MASK (LSE_STATE_CORNER_STATE_MASK | LSE_STATE_CENTER_STATE_MASK)

#define LSE_STATE_EDGE_ORIENTATION_MASK 0b1000
#define LSE_STATE_EDGE_INDEX_MASK       0b0111
#define LSE_STATE_DOUBLE_EDGE_ORIENTATION_MASK 0b10001000

typedef enum : uint8_t {
    LSE_STATE_EDGE_INDEX_UF = 0b0000,
    LSE_STATE_EDGE_INDEX_UB = 0b0001,
    LSE_STATE_EDGE_INDEX_UL = 0b0010,
    LSE_STATE_EDGE_INDEX_UR = 0b0011,
    LSE_STATE_EDGE_INDEX_DF = 0b0100,
    LSE_STATE_EDGE_INDEX_DB = 0b0101
} lse_state_edge_index_e;

const lse_state_t SOLVED_LSE_STATE = {
    .center_corner_state = 0,
    .uf_ub_state = (LSE_STATE_EDGE_INDEX_UF << 4) | LSE_STATE_EDGE_INDEX_UB,
    .ul_ur_state = (LSE_STATE_EDGE_INDEX_UL << 4) | LSE_STATE_EDGE_INDEX_UR,
    .df_db_state = (LSE_STATE_EDGE_INDEX_DF << 4) | LSE_STATE_EDGE_INDEX_DB
};

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
    lse_state.center_corner_state = rand() & LSE_STATE_CORNER_CENTER_STATE_MASK;

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
            swap_index = i + rand() % (6 - i);
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

    int edge_orientation = rand();
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

typedef enum : uint8_t {
    LSE_MOVE_U       = 0b001,
    LSE_MOVE_U2      = 0b010,
    LSE_MOVE_U_PRIME = 0b011,
    LSE_MOVE_M       = 0b101,
    LSE_MOVE_M2      = 0b110,
    LSE_MOVE_M_PRIME = 0b111
} lse_move_e;

#define LSE_MOVE_TYPE_MASK  0b100
#define LSE_MOVE_COUNT_MASK 0b011

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

typedef struct {
    lse_move_e* data;
    size_t size;
    size_t capacity;
} lse_move_list_t;

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

typedef struct {
    lse_move_list_t* data;
    size_t size;
    size_t capacity;
} lse_solution_list_t;

void free_lse_solution_list(lse_solution_list_t* solution_list) {
    for (int i = 0; i < solution_list->size; i++) {
        array_free(solution_list->data[i]);
    }
    array_free(*solution_list);
}

void _solve_eolr_recursive_m(lse_state_t lse_state, lse_move_list_t* moves, lse_solution_list_t* solutions, int depth);

void _solve_eolr_recursive_u(lse_state_t lse_state, lse_move_list_t* moves, lse_solution_list_t* solutions, int depth) {
    if (was_eolr_just_solved(lse_state)) {
        lse_move_list_t solution = {};
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
        lse_move_list_t solution = {};
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

lse_solution_list_t solve_eolr(lse_state_t lse_state) {
    lse_move_list_t moves = {};

    lse_solution_list_t solutions = {};
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
        lse_move_list_t solution = {};
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
        lse_move_list_t solution = {};
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

lse_solution_list_t solve_lse(lse_state_t lse_state) {
    lse_move_list_t moves = {};

    lse_solution_list_t solutions = {};
    // TODO: Maximum number of moves to solve LSE
    for (int depth = 0; depth <= 18 && solutions.size == 0; depth++) {
        printf("Searching depth %i\n", depth);
        _solve_lse_recursive_u(lse_state, &moves, &solutions, depth);
        _solve_lse_recursive_m(lse_state, &moves, &solutions, depth);
    }
    array_free(moves);

    return solutions;
}

typedef enum : uint8_t {
    FACE_INDEX_U,
    FACE_INDEX_D,
    FACE_INDEX_F,
    FACE_INDEX_B,
    FACE_INDEX_R,
    FACE_INDEX_L
} face_index_e;

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
    assert(false);
}

face_index_e lse_state_get_center_face(int center_state, int target_center) {
    switch ((center_state + target_center) & 0b11) {
        case 0: return FACE_INDEX_U;
        case 1: return FACE_INDEX_B;
        case 2: return FACE_INDEX_D;
        case 3: return FACE_INDEX_F;
    }
    assert(false);
}

face_index_e lse_state_get_corner_front_face(int corner_state, int target_corner) {
    switch ((corner_state + target_corner) & 0b11) {
        case 0: return FACE_INDEX_F;
        case 1: return FACE_INDEX_R;
        case 2: return FACE_INDEX_B;
        case 3: return FACE_INDEX_L;
    }
    assert(false);
}

typedef struct {
    face_index_e stickers[6][9];
} visual_cube_state_t;

typedef enum : uint8_t {
    VISUAL_CUBE_EDGE_UB = (1 << FACE_INDEX_U) | (1 << FACE_INDEX_B),
    VISUAL_CUBE_EDGE_UR = (1 << FACE_INDEX_U) | (1 << FACE_INDEX_R),
    VISUAL_CUBE_EDGE_UF = (1 << FACE_INDEX_U) | (1 << FACE_INDEX_F),
    VISUAL_CUBE_EDGE_UL = (1 << FACE_INDEX_U) | (1 << FACE_INDEX_L),
    VISUAL_CUBE_EDGE_BL = (1 << FACE_INDEX_B) | (1 << FACE_INDEX_L),
    VISUAL_CUBE_EDGE_FL = (1 << FACE_INDEX_F) | (1 << FACE_INDEX_L),
    VISUAL_CUBE_EDGE_FR = (1 << FACE_INDEX_F) | (1 << FACE_INDEX_R),
    VISUAL_CUBE_EDGE_BR = (1 << FACE_INDEX_B) | (1 << FACE_INDEX_R),
    VISUAL_CUBE_EDGE_DF = (1 << FACE_INDEX_D) | (1 << FACE_INDEX_F),
    VISUAL_CUBE_EDGE_DR = (1 << FACE_INDEX_D) | (1 << FACE_INDEX_R),
    VISUAL_CUBE_EDGE_DB = (1 << FACE_INDEX_D) | (1 << FACE_INDEX_B),
    VISUAL_CUBE_EDGE_DL = (1 << FACE_INDEX_D) | (1 << FACE_INDEX_L)
} visual_cube_edge_e;

typedef enum : uint8_t {
    VISUAL_CUBE_CORNER_UBL = (1 << FACE_INDEX_U) | (1 << FACE_INDEX_B) | (1 << FACE_INDEX_L),
    VISUAL_CUBE_CORNER_UBR = (1 << FACE_INDEX_U) | (1 << FACE_INDEX_B) | (1 << FACE_INDEX_R),
    VISUAL_CUBE_CORNER_UFR = (1 << FACE_INDEX_U) | (1 << FACE_INDEX_F) | (1 << FACE_INDEX_R),
    VISUAL_CUBE_CORNER_UFL = (1 << FACE_INDEX_U) | (1 << FACE_INDEX_F) | (1 << FACE_INDEX_L),
    VISUAL_CUBE_CORNER_DFL = (1 << FACE_INDEX_D) | (1 << FACE_INDEX_F) | (1 << FACE_INDEX_L),
    VISUAL_CUBE_CORNER_DFR = (1 << FACE_INDEX_D) | (1 << FACE_INDEX_F) | (1 << FACE_INDEX_R),
    VISUAL_CUBE_CORNER_DBR = (1 << FACE_INDEX_D) | (1 << FACE_INDEX_B) | (1 << FACE_INDEX_R),
    VISUAL_CUBE_CORNER_DBL = (1 << FACE_INDEX_D) | (1 << FACE_INDEX_B) | (1 << FACE_INDEX_L)
} visual_cube_corner_e;

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

void visual_cube_state_write_lse_state_edge(visual_cube_state_t* visual_cube_state, visual_cube_edge_e edge, int state) {
    face_index_e primary_face_index   = (face_index_e)byte_ctz(edge);
    face_index_e secondary_face_index = (face_index_e)byte_ctz((uint8_t)(edge ^ (1 << primary_face_index)));

    int primary_sticker_index   = visual_cube_get_edge_sticker_index(primary_face_index, secondary_face_index);
    int secondary_sticker_index = visual_cube_get_edge_sticker_index(secondary_face_index, primary_face_index);

    visual_cube_state->stickers[ primary_face_index ][ primary_sticker_index ] = lse_state_get_edge_face(state, false);
    visual_cube_state->stickers[secondary_face_index][secondary_sticker_index] = lse_state_get_edge_face(state, true);
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

void visual_cube_state_write_edge_g(visual_cube_state_t* visual_cube_state, visual_cube_edge_e edge, g1_edge_index_e piece, bool orientation) {
    face_index_e primary_face_index   = (face_index_e)byte_ctz(edge);
    face_index_e secondary_face_index = (face_index_e)byte_ctz((uint8_t)(edge ^ (1 << primary_face_index)));

    int primary_sticker_index   = visual_cube_get_edge_sticker_index(primary_face_index, secondary_face_index);
    int secondary_sticker_index = visual_cube_get_edge_sticker_index(secondary_face_index, primary_face_index);

    static face_index_e g1_edge_faces_lookup[][2] = {
        [G1_EDGE_INDEX_UF] = { FACE_INDEX_U, FACE_INDEX_F },
        [G1_EDGE_INDEX_UB] = { FACE_INDEX_U, FACE_INDEX_B },
        [G1_EDGE_INDEX_UL] = { FACE_INDEX_U, FACE_INDEX_L },
        [G1_EDGE_INDEX_UR] = { FACE_INDEX_U, FACE_INDEX_R },
        [G1_EDGE_INDEX_DF] = { FACE_INDEX_D, FACE_INDEX_F },
        [G1_EDGE_INDEX_DB] = { FACE_INDEX_D, FACE_INDEX_B },
        [G1_EDGE_INDEX_DL] = { FACE_INDEX_D, FACE_INDEX_L },
        [G1_EDGE_INDEX_DR] = { FACE_INDEX_D, FACE_INDEX_R },
        [G1_EDGE_INDEX_FL] = { FACE_INDEX_F, FACE_INDEX_L },
        [G1_EDGE_INDEX_FR] = { FACE_INDEX_F, FACE_INDEX_R },
        [G1_EDGE_INDEX_BL] = { FACE_INDEX_B, FACE_INDEX_L },
        [G1_EDGE_INDEX_BR] = { FACE_INDEX_B, FACE_INDEX_R }
    };

    visual_cube_state->stickers[ primary_face_index ][ primary_sticker_index ] = g1_edge_faces_lookup[piece][orientation];
    visual_cube_state->stickers[secondary_face_index][secondary_sticker_index] = g1_edge_faces_lookup[piece][orientation ^ 1];
}

void visual_cube_state_write_corner_g(visual_cube_state_t* visual_cube_state, visual_cube_corner_e corner, g1_corner_index_e piece, int orientation) {
    assert(orientation >= 0 && orientation <= 2);

    // The faces of each corner are ordered in a clockwise direction, and each array is extended by two
    // to avoid having to take the modulus
    static const face_index_e g1_corner_faces_lookup[][5] = {
        [G1_CORNER_INDEX_UFR] = { FACE_INDEX_U, FACE_INDEX_R, FACE_INDEX_F, FACE_INDEX_U, FACE_INDEX_R },
        [G1_CORNER_INDEX_UFL] = { FACE_INDEX_U, FACE_INDEX_F, FACE_INDEX_L, FACE_INDEX_U, FACE_INDEX_F },
        [G1_CORNER_INDEX_UBR] = { FACE_INDEX_U, FACE_INDEX_B, FACE_INDEX_R, FACE_INDEX_U, FACE_INDEX_B },
        [G1_CORNER_INDEX_UBL] = { FACE_INDEX_U, FACE_INDEX_L, FACE_INDEX_B, FACE_INDEX_U, FACE_INDEX_L },
        [G1_CORNER_INDEX_DFR] = { FACE_INDEX_D, FACE_INDEX_F, FACE_INDEX_R, FACE_INDEX_D, FACE_INDEX_F },
        [G1_CORNER_INDEX_DFL] = { FACE_INDEX_D, FACE_INDEX_L, FACE_INDEX_F, FACE_INDEX_D, FACE_INDEX_L },
        [G1_CORNER_INDEX_DBR] = { FACE_INDEX_D, FACE_INDEX_R, FACE_INDEX_B, FACE_INDEX_D, FACE_INDEX_R },
        [G1_CORNER_INDEX_DBL] = { FACE_INDEX_D, FACE_INDEX_B, FACE_INDEX_L, FACE_INDEX_D, FACE_INDEX_B }
    };

    face_index_e faces[3] = {};
    memcpy(&faces, g1_corner_faces_lookup[piece] + orientation, sizeof faces);

    // Depending on which corner we are attempting to set, we may need to swap two of the stickers due to the
    // primary, secondary, and tertiary stickers not being ordered consistently in a clockwise or counterclockwise
    // direction.
    if ((byte_popcount(corner ^ VISUAL_CUBE_CORNER_UFL) & 2) == 2) {
        face_index_e swap = faces[1];
        faces[1] = faces[2];
        faces[2] = swap;
    }
    visual_cube_state_write_corner(visual_cube_state, corner, faces[0], faces[1], faces[2]);
}

void g0_g1_state_write_visual_cube_state(visual_cube_state_t* visual_cube_state, g0_state_t g0_state, g1_state_t g1_state) {
    visual_cube_state_write_edge_g(visual_cube_state, VISUAL_CUBE_EDGE_UF, g1_state.uf_ub_edges >> 4,     (g0_state.ub_ur_uf_ul_edges >> 2) & 1);
    visual_cube_state_write_edge_g(visual_cube_state, VISUAL_CUBE_EDGE_UB, g1_state.uf_ub_edges & 0b1111, (g0_state.ub_ur_uf_ul_edges >> 6) & 1);
    visual_cube_state_write_edge_g(visual_cube_state, VISUAL_CUBE_EDGE_UL, g1_state.ul_ur_edges >> 4,     (g0_state.ub_ur_uf_ul_edges >> 0) & 1);
    visual_cube_state_write_edge_g(visual_cube_state, VISUAL_CUBE_EDGE_UR, g1_state.ul_ur_edges & 0b1111, (g0_state.ub_ur_uf_ul_edges >> 4) & 1);
    visual_cube_state_write_edge_g(visual_cube_state, VISUAL_CUBE_EDGE_DF, g1_state.df_db_edges >> 4,     (g0_state.df_dr_db_dl_edges >> 6) & 1);
    visual_cube_state_write_edge_g(visual_cube_state, VISUAL_CUBE_EDGE_DB, g1_state.df_db_edges & 0b1111, (g0_state.df_dr_db_dl_edges >> 2) & 1);
    visual_cube_state_write_edge_g(visual_cube_state, VISUAL_CUBE_EDGE_DL, g1_state.dl_dr_edges >> 4,     (g0_state.df_dr_db_dl_edges >> 0) & 1);
    visual_cube_state_write_edge_g(visual_cube_state, VISUAL_CUBE_EDGE_DR, g1_state.dl_dr_edges & 0b1111, (g0_state.df_dr_db_dl_edges >> 4) & 1);
    visual_cube_state_write_edge_g(visual_cube_state, VISUAL_CUBE_EDGE_FL, g1_state.fl_fr_edges >> 4,     (g0_state.bl_fr_br_fl_edges >> 0) & 1);
    visual_cube_state_write_edge_g(visual_cube_state, VISUAL_CUBE_EDGE_FR, g1_state.fl_fr_edges & 0b1111, (g0_state.bl_fr_br_fl_edges >> 4) & 1);
    visual_cube_state_write_edge_g(visual_cube_state, VISUAL_CUBE_EDGE_BL, g1_state.bl_br_edges >> 4,     (g0_state.bl_fr_br_fl_edges >> 6) & 1);
    visual_cube_state_write_edge_g(visual_cube_state, VISUAL_CUBE_EDGE_BR, g1_state.bl_br_edges & 0b1111, (g0_state.bl_fr_br_fl_edges >> 2) & 1);

    visual_cube_state_write_corner_g(visual_cube_state, VISUAL_CUBE_CORNER_UFR, g1_state.ufr_ubl_corners >> 4,     (g0_state.ufl_ufr_ubr_ubl_corners >> 8 ) & 0b11);
    visual_cube_state_write_corner_g(visual_cube_state, VISUAL_CUBE_CORNER_UBL, g1_state.ufr_ubl_corners & 0b1111, (g0_state.ufl_ufr_ubr_ubl_corners >> 0 ) & 0b11);
    visual_cube_state_write_corner_g(visual_cube_state, VISUAL_CUBE_CORNER_UFL, g1_state.ufl_ubr_corners >> 4,     (g0_state.ufl_ufr_ubr_ubl_corners >> 12) & 0b11);
    visual_cube_state_write_corner_g(visual_cube_state, VISUAL_CUBE_CORNER_UBR, g1_state.ufl_ubr_corners & 0b1111, (g0_state.ufl_ufr_ubr_ubl_corners >> 4 ) & 0b11);
    visual_cube_state_write_corner_g(visual_cube_state, VISUAL_CUBE_CORNER_DFR, g1_state.dfr_dbl_corners >> 4,     (g0_state.dfr_dfl_dbl_dbr_corners >> 12) & 0b11);
    visual_cube_state_write_corner_g(visual_cube_state, VISUAL_CUBE_CORNER_DBL, g1_state.dfr_dbl_corners & 0b1111, (g0_state.dfr_dfl_dbl_dbr_corners >> 4 ) & 0b11);
    visual_cube_state_write_corner_g(visual_cube_state, VISUAL_CUBE_CORNER_DFL, g1_state.dfl_dbr_corners >> 4,     (g0_state.dfr_dfl_dbl_dbr_corners >> 8 ) & 0b11);
    visual_cube_state_write_corner_g(visual_cube_state, VISUAL_CUBE_CORNER_DBR, g1_state.dfl_dbr_corners & 0b1111, (g0_state.dfr_dfl_dbl_dbr_corners >> 0 ) & 0b11);
}

#define ESC_ERASE_ENTIRE_SCREEN "\x1b[H\x1b[0J"
#define ESC_SAVE_SCREEN "\x1b[?47h"
#define ESC_RESTORE_SCREEN "\x1b[?47l"

#define ESC_COLOR_WHITE  "\x1b[47m"
#define ESC_COLOR_ORANGE "\x1b[48;5;208m"
#define ESC_COLOR_GREEN  "\x1b[42m"
#define ESC_COLOR_RED    "\x1b[41m"
#define ESC_COLOR_BLUE   "\x1b[48;5;27m"
#define ESC_COLOR_YELLOW "\x1b[48;5;190m"

#define ESC_COLOR_RESET "\x1b[0m"

static struct termios initial_termios_state;
static bool is_initial_termios_state_initialized = false;
void enter_raw_mode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    if (!is_initial_termios_state_initialized) {
        initial_termios_state = raw;
        is_initial_termios_state_initialized = true;
    }

    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void restore_initial_mode() {
    if (is_initial_termios_state_initialized) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &initial_termios_state);
    }
}

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

void draw_lse_state(lse_state_t lse_state) {
    visual_cube_state_t visual_cube_state;
    visual_cube_state_reset(&visual_cube_state);

    lse_state_write_visual_cube_state(lse_state, &visual_cube_state);

    draw_visual_cube_state(&visual_cube_state);
}

void draw_g0_g1_cube_state(g0_state_t g0_state, g1_state_t g1_state) {
    visual_cube_state_t visual_cube_state;
    visual_cube_state_reset(&visual_cube_state);

    g0_g1_state_write_visual_cube_state(&visual_cube_state, g0_state, g1_state);

    draw_visual_cube_state(&visual_cube_state);
}

void clear_trailing_whitespace(char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (isspace(str[i])) {
            str[i] = '\0';
            return;
        }
    }
}

typedef struct {
    lse_state_t* data;
    size_t size;
    size_t capacity;
} lse_state_list_t;

static lse_state_list_t prev_lse_states = {};

void execute_command_scramble(lse_state_t* lse_state, lse_move_list_t* lse_move_list, lse_solution_list_t* lse_solution_list) {
    *lse_state = generate_random_lse_state();
    array_append(prev_lse_states, *lse_state);

    lse_move_list->size = 0;
    free_lse_solution_list(lse_solution_list);
}

void execute_command_curr_scramble(lse_state_t* lse_state, lse_move_list_t* lse_move_list)
{
    if (prev_lse_states.size >= 1) {
        *lse_state = prev_lse_states.data[prev_lse_states.size - 1];
    } else {
        *lse_state = SOLVED_LSE_STATE;
    }

    lse_move_list->size = 0;
}

void execute_command_prev_scramble(lse_state_t* lse_state, lse_move_list_t* lse_move_list, lse_solution_list_t* lse_solution_list) {
    if (prev_lse_states.size >= 2) {
        *lse_state = prev_lse_states.data[prev_lse_states.size - 2];
        prev_lse_states.size--;
    } else {
        prev_lse_states.size = 0;
        *lse_state = SOLVED_LSE_STATE;
    }

    lse_move_list->size = 0;
    free_lse_solution_list(lse_solution_list);
}

void read_and_execute_command(lse_state_t* lse_state, lse_move_list_t* lse_move_list, lse_solution_list_t* lse_solution_list) {
    restore_initial_mode();

    char command[16] = {};
    if (fgets(command, sizeof command, stdin) != NULL) {
        clear_trailing_whitespace(command);

        if (strcmp(command, "scramble") == 0) {
            execute_command_scramble(lse_state, lse_move_list, lse_solution_list);
        }
        if (strcmp(command, "prev") == 0) {
            execute_command_prev_scramble(lse_state, lse_move_list, lse_solution_list);
        }
        if (strcmp(command, "curr") == 0) {
            execute_command_curr_scramble(lse_state, lse_move_list);
        }
        if (strcmp(command, "eolr") == 0) {
            free_lse_solution_list(lse_solution_list);
            *lse_solution_list = solve_eolr(*lse_state);
        }
        if (strcmp(command, "lse") == 0) {
            free_lse_solution_list(lse_solution_list);
            *lse_solution_list = solve_lse(*lse_state);
        }
    }

    enter_raw_mode();
}

int main() {
    srand((unsigned)time(NULL));

    lse_state_t lse_state = SOLVED_LSE_STATE;
    lse_move_list_t lse_move_list = {};
    lse_solution_list_t lse_solution_list = {};

    printf(ESC_SAVE_SCREEN);

    enter_raw_mode();
    while (true) {
        printf(ESC_ERASE_ENTIRE_SCREEN);
        draw_lse_state(lse_state);
        for (int i = 0; i < lse_move_list.size; i++) {
            if (i != 0) {
                printf(" ");
            }
            switch (lse_move_list.data[i]) {
                case LSE_MOVE_U:       printf("U");  break;
                case LSE_MOVE_U_PRIME: printf("U'"); break;
                case LSE_MOVE_U2:      printf("U2"); break;
                case LSE_MOVE_M:       printf("M");  break;
                case LSE_MOVE_M_PRIME: printf("M'"); break;
                case LSE_MOVE_M2:      printf("M2"); break;
            }
        }
        printf("\n(%zu STM)\n", lse_move_list.size);

        printf("Solutions:\n");
        for (int i = 0; i < lse_solution_list.size; i++) {
            lse_move_list_t solution = lse_solution_list.data[i];
            for (int j = 0; j < solution.size; j++) {
                printf("%s ", lse_move_to_string(solution.data[j]));
            }
            printf("(%zu STM)\n", solution.size);
        }

        handle_input:
        switch (getchar()) {
            case 'm':
                lse_state = lse_move_m(lse_state);
                lse_move_list_simplify_append(&lse_move_list, LSE_MOVE_M);
                continue;
            case 'k':
                lse_state = lse_move_m_prime(lse_state);
                lse_move_list_simplify_append(&lse_move_list, LSE_MOVE_M_PRIME);
                continue;
            case 's':
                lse_state = lse_move_u(lse_state);
                lse_move_list_simplify_append(&lse_move_list, LSE_MOVE_U);
                continue;
            case 'd':
                lse_state = lse_move_u_prime(lse_state);
                lse_move_list_simplify_append(&lse_move_list, LSE_MOVE_U_PRIME);
                continue;
            case '/':
                printf("/");
                read_and_execute_command(&lse_state, &lse_move_list, &lse_solution_list);
                continue;
            case '\n':
                execute_command_scramble(&lse_state, &lse_move_list, &lse_solution_list);
                continue;
            case ' ':
                execute_command_curr_scramble(&lse_state, &lse_move_list);
                continue;
            case 'p':
                execute_command_prev_scramble(&lse_state, &lse_move_list, &lse_solution_list);
                continue;
            case 'q': break;
            default: goto handle_input;
        }

        printf(ESC_ERASE_ENTIRE_SCREEN);
        break;
    }
    restore_initial_mode();

    printf(ESC_RESTORE_SCREEN);

    return 0;
}
