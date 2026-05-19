#pragma once

#include "common.h"
#include "visual_cube.h"
#include "kociemba.h"

// The first phase of is simply solving a 3-color version of the 4x4 center pieces. That is, we treat
// opposite colors as if they are the same color, and solve the centers from there. Also, it is in this phase
// that we solve OLL parity (by doing either an even or odd number of quarter wide turns depending on the scramble)
//
// Each center is represented by an 8-bit integer. Each center piece is 2 bits, with 0b00 representing
// U/D, 0b01 representing F/B, and 0b10 representing R/L. Center pieces are ordered just as they are
// in the speffz lettering scheme, with the higher bits starting with the top left center piece and the
// lower bits ending with the bottom left center piece (and the other center pieces in between in a clockwise
// ordering).
//
// Then, to track parity, an 8-bit integer that flips between 0 and 1 is used.
typedef struct {
    alignas(uint64_t) uint8_t centers[6];
    uint8_t parity;
    uint8_t _padding;
} revenge_g0_state_t ;

static_assert(sizeof (revenge_g0_state_t) == sizeof (uint64_t));
static_assert(alignof(revenge_g0_state_t) == alignof(uint64_t));

// NOTE: Because the 4x4 does not have any fixed centers, there are multiple possible solved states
static const revenge_g0_state_t REVENGE_G0_STATE_SOLVED = {
    .centers = {
        [FACE_INDEX_U] = 0b00000000,
        [FACE_INDEX_D] = 0b00000000,
        [FACE_INDEX_F] = 0b01010101,
        [FACE_INDEX_B] = 0b01010101,
        [FACE_INDEX_R] = 0b10101010,
        [FACE_INDEX_L] = 0b10101010,
    },
    .parity = 0
};

static const revenge_g0_state_t REVENGE_G0_STATE_NULL = {};

void revenge_g0_move_face_cw(revenge_g0_state_t* state, face_index_e face);
void revenge_g0_move_uw(revenge_g0_state_t* state);
void revenge_g0_move_fw(revenge_g0_state_t* state);
void revenge_g0_move_rw(revenge_g0_state_t* state);

void revenge_g0_execute_move(revenge_g0_state_t* state, move_t move);

// The second phase is taking the result of the first phase and bringing it to 3x3 stage. The only
// thing to be careful about here is that we cannot have PLL parity going into 3x3 stage.
//
// The centers are represented in a similar way to how they are in phase 1. We still use 2 bits
// for each center piece, but this time instead of having 3 possible states for each center piece,
// we only have 2 possible states for each center piece. This does mean we have dead bits, and also
// that we have room there for reducing the size of this structure. U/F/R will be represented with 0,
// and D/B/L will be represented with 1.
//
// The wings are represented as one 8-bit integer for each wing position (of which there are 24 of).
// Because of how wings work, there is not any edge orientation to worry about, so we only need to
// store permutation.
typedef struct {
    uint8_t centers[6];
    uint8_t wings[24];
} revenge_g1_state_t;

// NOTE: Just like revenge_g0_state_t, there are multiple possible solved states
static const revenge_g1_state_t REVENGE_G1_STATE_SOLVED = {
    .centers = {
        [FACE_INDEX_U] = 0b00000000,
        [FACE_INDEX_D] = 0b01010101,
        [FACE_INDEX_F] = 0b00000000,
        [FACE_INDEX_B] = 0b01010101,
        [FACE_INDEX_R] = 0b00000000,
        [FACE_INDEX_L] = 0b01010101,
    },

    .wings = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23 }
};

typedef enum : uint8_t {
    WING_INDEX_UB, WING_INDEX_UR, WING_INDEX_UF, WING_INDEX_UL,
    WING_INDEX_LU, WING_INDEX_LF, WING_INDEX_LD, WING_INDEX_LB,
    WING_INDEX_FU, WING_INDEX_FR, WING_INDEX_FD, WING_INDEX_FL,
    WING_INDEX_RU, WING_INDEX_RB, WING_INDEX_RD, WING_INDEX_RF,
    WING_INDEX_BU, WING_INDEX_BL, WING_INDEX_BD, WING_INDEX_BR,
    WING_INDEX_DF, WING_INDEX_DR, WING_INDEX_DB, WING_INDEX_DL
} wing_index_e;

void revenge_g1_move_u(revenge_g1_state_t* state);
void revenge_g1_move_d(revenge_g1_state_t* state);
void revenge_g1_move_f(revenge_g1_state_t* state);
void revenge_g1_move_b(revenge_g1_state_t* state);
void revenge_g1_move_r(revenge_g1_state_t* state);
void revenge_g1_move_l(revenge_g1_state_t* state);
void revenge_g1_move_uw(revenge_g1_state_t* state);
void revenge_g1_move_fw(revenge_g1_state_t* state);
void revenge_g1_move_rw(revenge_g1_state_t* state);

void revenge_write_visual_cube_state(big_visual_cube_state_t* state, revenge_g0_state_t revenge_g0_state, revenge_g1_state_t revenge_g1_state, g0_state_t g0_state, g1_state_t g1_state);

#define REVENGE_G0_TABLE_SIZE 1753769
#define REVENGE_G0_TABLE_DEPTH 6

typedef struct revenge_g0_table_t revenge_g0_table_t;

revenge_g0_table_t* revenge_g0_init_table(allocator_e allocator);

bool revenge_g0_is_solved(revenge_g0_state_t state);

g_solution_list_t solve_revenge_g0(revenge_g0_table_t* table, revenge_g0_state_t state, allocator_e allocator);
