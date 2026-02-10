#pragma once

#include <stdint.h>
#include <stdlib.h>

#include "visual_cube.h"

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

typedef enum : uint8_t {
    LSE_STATE_EDGE_INDEX_UF = 0b0000,
    LSE_STATE_EDGE_INDEX_UB = 0b0001,
    LSE_STATE_EDGE_INDEX_UL = 0b0010,
    LSE_STATE_EDGE_INDEX_UR = 0b0011,
    LSE_STATE_EDGE_INDEX_DF = 0b0100,
    LSE_STATE_EDGE_INDEX_DB = 0b0101
} lse_state_edge_index_e;

static const lse_state_t SOLVED_LSE_STATE = {
    .center_corner_state = 0,
    .uf_ub_state = (LSE_STATE_EDGE_INDEX_UF << 4) | LSE_STATE_EDGE_INDEX_UB,
    .ul_ur_state = (LSE_STATE_EDGE_INDEX_UL << 4) | LSE_STATE_EDGE_INDEX_UR,
    .df_db_state = (LSE_STATE_EDGE_INDEX_DF << 4) | LSE_STATE_EDGE_INDEX_DB
};

lse_state_t lse_move_u(lse_state_t lse_state);
lse_state_t lse_move_u_prime(lse_state_t lse_state);
lse_state_t lse_move_u2(lse_state_t lse_state);

lse_state_t lse_move_m(lse_state_t lse_state);
lse_state_t lse_move_m_prime(lse_state_t lse_state);
lse_state_t lse_move_m2(lse_state_t lse_state);

lse_state_t generate_random_lse_state();

typedef enum : uint8_t {
    LSE_MOVE_U       = 0b001,
    LSE_MOVE_U2      = 0b010,
    LSE_MOVE_U_PRIME = 0b011,
    LSE_MOVE_M       = 0b101,
    LSE_MOVE_M2      = 0b110,
    LSE_MOVE_M_PRIME = 0b111
} lse_move_e;

const char* lse_move_to_string(lse_move_e move);

typedef struct {
    lse_move_e* data;
    size_t size;
    size_t capacity;
} lse_move_list_t;

void lse_move_list_simplify_append(lse_move_list_t* list, lse_move_e move);

typedef struct {
    lse_move_list_t* data;
    size_t size;
    size_t capacity;
} lse_solution_list_t;

void free_lse_solution_list(lse_solution_list_t* solution_list);

lse_solution_list_t solve_eolr(lse_state_t lse_state);
lse_solution_list_t solve_lse(lse_state_t lse_state);

void lse_state_write_visual_cube_state(lse_state_t lse_state, visual_cube_state_t* visual_cube_state);
