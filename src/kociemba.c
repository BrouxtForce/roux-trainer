#include <stdint.h>
#include <assert.h>

uint8_t byte_rotate_left(uint8_t value, uint8_t amount) {
    return __builtin_rotateleft8(value, amount);
}

uint8_t byte_rotate_right(uint8_t value, uint8_t amount) {
    return __builtin_rotateright8(value, amount);
}

uint16_t rotate_left_u16(uint16_t value, uint16_t amount) {
    return __builtin_rotateleft16(value, amount);
}

uint16_t rotate_right_u16(uint16_t value, uint16_t amount) {
    return __builtin_rotateright16(value, amount);
}

typedef struct {
    uint8_t ub_ur_uf_ul_edges;
    uint8_t df_dr_db_dl_edges;
    uint8_t bl_fr_br_fl_edges;
    uint16_t ufl_ufr_ubr_ubl_corners;
    uint16_t dfr_dfl_dbl_dbr_corners;
} g0_state_t;

const g0_state_t G0_STATE_SOLVED = {
    .ub_ur_uf_ul_edges = 0,
    .df_dr_db_dl_edges = 0,
    .bl_fr_br_fl_edges = 0b10101010,
    .ufl_ufr_ubr_ubl_corners = 0,
    .dfr_dfl_dbl_dbr_corners = 0
};

#define G0_EDGE_PACKING_ORIENTATION_MASK 0b01010101

#define G0_CORNER_PACKING_MIDDLE_TWO_MASK 0b0000111111110000
#define G0_CORNER_PACKING_OUTER_TWO_MASK 0b1111000000001111
#define G0_CORNER_PACKING_UPPER_TWO_MASK 0b1111111100000000
#define G0_CORNER_PACKING_LOWER_TWO_MASK 0b0000000011111111
#define G0_CORNER_PACKING_ROTATE_FACE_CW_CCW 0b0010000100100001
#define G0_CORNER_PACKING_ROTATE_FACE_CCW_CW 0b0001001000010010
#define G0_CORNER_PACKING_LOW_ORIENTATION_BITS 0b0001000100010001

void g0_move_u(g0_state_t* state) {
    state->ub_ur_uf_ul_edges = byte_rotate_right(state->ub_ur_uf_ul_edges, 2);
    state->ufl_ufr_ubr_ubl_corners = rotate_left_u16(state->ufl_ufr_ubr_ubl_corners, 4);
}

void g0_move_d(g0_state_t* state) {
    state->df_dr_db_dl_edges = byte_rotate_right(state->df_dr_db_dl_edges, 2);
    state->dfr_dfl_dbl_dbr_corners = rotate_left_u16(state->dfr_dfl_dbl_dbr_corners, 4);
}

uint16_t _g0_face_rotate_corners(uint16_t corners, uint16_t rotation) {
    // TODO: More efficient implementation
    corners += rotation;

    uint16_t a = ((corners >> 12) & 0b1111) % 3;
    uint16_t b = ((corners >> 8 ) & 0b1111) % 3;
    uint16_t c = ((corners >> 4 ) & 0b1111) % 3;
    uint16_t d = ((corners >> 0 ) & 0b1111) % 3;

    return (uint16_t)((a << 12) | (b << 8) | (c << 4) | d);
}

void g0_move_r(g0_state_t* state) {
    uint8_t r_face_edges = (uint8_t)(
         (state->ub_ur_uf_ul_edges & 0b00110000) |
        ((state->bl_fr_br_fl_edges & 0b00110000) >> 2) |
        ((state->df_dr_db_dl_edges & 0b00110000) >> 4) |
        ((state->bl_fr_br_fl_edges & 0b00001100) << 4)
    );
    r_face_edges = byte_rotate_left(r_face_edges, 2);

    state->ub_ur_uf_ul_edges &= ~0b00110000;
    state->ub_ur_uf_ul_edges |= r_face_edges & 0b00110000;

    state->bl_fr_br_fl_edges &= ~0b00111100;
    state->bl_fr_br_fl_edges |= (r_face_edges << 2) & 0b00110000;
    state->bl_fr_br_fl_edges |= (r_face_edges >> 4) & 0b00001100;

    state->df_dr_db_dl_edges &= ~0b00110000;
    state->df_dr_db_dl_edges |= (r_face_edges << 4) & 0b00110000;

    uint16_t r_corners = (state->ufl_ufr_ubr_ubl_corners & G0_CORNER_PACKING_MIDDLE_TWO_MASK) | (state->dfr_dfl_dbl_dbr_corners & G0_CORNER_PACKING_OUTER_TWO_MASK);
    r_corners = _g0_face_rotate_corners(rotate_right_u16(r_corners, 4), G0_CORNER_PACKING_ROTATE_FACE_CW_CCW);

    state->ufl_ufr_ubr_ubl_corners &= ~G0_CORNER_PACKING_MIDDLE_TWO_MASK;
    state->ufl_ufr_ubr_ubl_corners |= r_corners & G0_CORNER_PACKING_MIDDLE_TWO_MASK;

    state->dfr_dfl_dbl_dbr_corners &= ~G0_CORNER_PACKING_OUTER_TWO_MASK;
    state->dfr_dfl_dbl_dbr_corners |= r_corners & G0_CORNER_PACKING_OUTER_TWO_MASK;
}

void g0_move_f(g0_state_t* state) {
    uint8_t f_face_edges = (state->ub_ur_uf_ul_edges & 0b00001100) |
                           (state->df_dr_db_dl_edges & 0b11000000) |
                           (state->bl_fr_br_fl_edges & 0b00110011);
    f_face_edges = byte_rotate_left(f_face_edges, 2) ^ G0_EDGE_PACKING_ORIENTATION_MASK;

    state->ub_ur_uf_ul_edges &= ~0b00001100;
    state->ub_ur_uf_ul_edges |= f_face_edges & 0b00001100;

    state->df_dr_db_dl_edges &= ~0b11000000;
    state->df_dr_db_dl_edges |= f_face_edges & 0b11000000;

    state->bl_fr_br_fl_edges &= ~0b00110011;
    state->bl_fr_br_fl_edges |= f_face_edges & 0b00110011;

    uint16_t f_face_corners = (state->ufl_ufr_ubr_ubl_corners & G0_CORNER_PACKING_UPPER_TWO_MASK) |
                             ((state->dfr_dfl_dbl_dbr_corners & G0_CORNER_PACKING_UPPER_TWO_MASK) >> 8);
    f_face_corners = _g0_face_rotate_corners(rotate_right_u16(f_face_corners, 4), G0_CORNER_PACKING_ROTATE_FACE_CCW_CW);

    state->ufl_ufr_ubr_ubl_corners &= ~G0_CORNER_PACKING_UPPER_TWO_MASK;
    state->ufl_ufr_ubr_ubl_corners |= f_face_corners & G0_CORNER_PACKING_UPPER_TWO_MASK;

    state->dfr_dfl_dbl_dbr_corners &= ~G0_CORNER_PACKING_UPPER_TWO_MASK;
    state->dfr_dfl_dbl_dbr_corners |= (f_face_corners << 8) & G0_CORNER_PACKING_UPPER_TWO_MASK;
}

void g0_move_l(g0_state_t* state) {
    uint8_t l_face_edges = (uint8_t)(
        ((state->ub_ur_uf_ul_edges & 0b00000011) << 6) |
        ((state->bl_fr_br_fl_edges & 0b11000000) >> 2) |
        ((state->df_dr_db_dl_edges & 0b00000011) << 2) |
         (state->bl_fr_br_fl_edges & 0b00000011)
    );
    l_face_edges = byte_rotate_left(l_face_edges, 2);

    state->ub_ur_uf_ul_edges &= ~0b00000011;
    state->ub_ur_uf_ul_edges |= (l_face_edges >> 6) & 0b00000011;

    state->bl_fr_br_fl_edges &= ~0b11000011;
    state->bl_fr_br_fl_edges |= (l_face_edges << 2) & 0b11000000;
    state->bl_fr_br_fl_edges |= l_face_edges & 0b00000011;

    state->df_dr_db_dl_edges &= ~0b00000011;
    state->df_dr_db_dl_edges |= (l_face_edges >> 2) & 0b00000011;

    uint16_t l_corners = (state->ufl_ufr_ubr_ubl_corners & G0_CORNER_PACKING_OUTER_TWO_MASK) | (state->dfr_dfl_dbl_dbr_corners & G0_CORNER_PACKING_MIDDLE_TWO_MASK);
    l_corners = _g0_face_rotate_corners(rotate_right_u16(l_corners, 4), G0_CORNER_PACKING_ROTATE_FACE_CW_CCW);

    state->ufl_ufr_ubr_ubl_corners &= ~G0_CORNER_PACKING_OUTER_TWO_MASK;
    state->ufl_ufr_ubr_ubl_corners |= l_corners & G0_CORNER_PACKING_OUTER_TWO_MASK;

    state->dfr_dfl_dbl_dbr_corners &= ~G0_CORNER_PACKING_MIDDLE_TWO_MASK;
    state->dfr_dfl_dbl_dbr_corners |= l_corners & G0_CORNER_PACKING_MIDDLE_TWO_MASK;
}

void g0_move_b(g0_state_t* state) {
    uint8_t b_face_edges = (state->ub_ur_uf_ul_edges & 0b11000000) |
                           (state->df_dr_db_dl_edges & 0b00001100) |
                          ((state->bl_fr_br_fl_edges & 0b11001100) >> 2);
    b_face_edges = byte_rotate_right(b_face_edges, 2) ^ G0_EDGE_PACKING_ORIENTATION_MASK;

    state->ub_ur_uf_ul_edges &= ~0b11000000;
    state->ub_ur_uf_ul_edges |= b_face_edges & 0b11000000;

    state->df_dr_db_dl_edges &= ~0b00001100;
    state->df_dr_db_dl_edges |= b_face_edges & 0b00001100;

    state->bl_fr_br_fl_edges &= ~0b11001100;
    state->bl_fr_br_fl_edges |= (b_face_edges << 2) & 0b11001100;

    uint16_t b_face_corners = (uint16_t)(
        (state->ufl_ufr_ubr_ubl_corners & G0_CORNER_PACKING_LOWER_TWO_MASK) |
        ((state->dfr_dfl_dbl_dbr_corners & G0_CORNER_PACKING_LOWER_TWO_MASK) << 8)
    );
    b_face_corners = _g0_face_rotate_corners(rotate_right_u16(b_face_corners, 4), G0_CORNER_PACKING_ROTATE_FACE_CCW_CW);

    state->ufl_ufr_ubr_ubl_corners &= ~G0_CORNER_PACKING_LOWER_TWO_MASK;
    state->ufl_ufr_ubr_ubl_corners |= b_face_corners & G0_CORNER_PACKING_LOWER_TWO_MASK;

    state->dfr_dfl_dbl_dbr_corners &= ~G0_CORNER_PACKING_LOWER_TWO_MASK;
    state->dfr_dfl_dbl_dbr_corners |= (b_face_corners >> 8) & G0_CORNER_PACKING_LOWER_TWO_MASK;
}

typedef enum : uint8_t {
    G1_EDGE_INDEX_UF,
    G1_EDGE_INDEX_UB,
    G1_EDGE_INDEX_UL,
    G1_EDGE_INDEX_UR,
    G1_EDGE_INDEX_DF,
    G1_EDGE_INDEX_DB,
    G1_EDGE_INDEX_DL,
    G1_EDGE_INDEX_DR,
    G1_EDGE_INDEX_FL,
    G1_EDGE_INDEX_FR,
    G1_EDGE_INDEX_BL,
    G1_EDGE_INDEX_BR
} g1_edge_index_e;

typedef enum : uint8_t {
    G1_CORNER_INDEX_UFR,
    G1_CORNER_INDEX_UFL,
    G1_CORNER_INDEX_UBR,
    G1_CORNER_INDEX_UBL,
    G1_CORNER_INDEX_DFR,
    G1_CORNER_INDEX_DFL,
    G1_CORNER_INDEX_DBR,
    G1_CORNER_INDEX_DBL
} g1_corner_index_e;

typedef struct {
    uint8_t uf_ub_edges;
    uint8_t ul_ur_edges;
    uint8_t df_db_edges;
    uint8_t dl_dr_edges;
    uint8_t fl_fr_edges;
    uint8_t bl_br_edges;

    uint8_t ufr_ubl_corners;
    uint8_t ufl_ubr_corners;
    uint8_t dfr_dbl_corners;
    uint8_t dfl_dbr_corners;
} g1_state_t;

const g1_state_t G1_STATE_SOLVED = {
    .uf_ub_edges = (G1_EDGE_INDEX_UF << 4) | G1_EDGE_INDEX_UB,
    .ul_ur_edges = (G1_EDGE_INDEX_UL << 4) | G1_EDGE_INDEX_UR,
    .df_db_edges = (G1_EDGE_INDEX_DF << 4) | G1_EDGE_INDEX_DB,
    .dl_dr_edges = (G1_EDGE_INDEX_DL << 4) | G1_EDGE_INDEX_DR,
    .fl_fr_edges = (G1_EDGE_INDEX_FL << 4) | G1_EDGE_INDEX_FR,
    .bl_br_edges = (G1_EDGE_INDEX_BL << 4) | G1_EDGE_INDEX_BR,

    .ufr_ubl_corners = (G1_CORNER_INDEX_UFR << 4) | G1_CORNER_INDEX_UBL,
    .ufl_ubr_corners = (G1_CORNER_INDEX_UFL << 4) | G1_CORNER_INDEX_UBR,
    .dfr_dbl_corners = (G1_CORNER_INDEX_DFR << 4) | G1_CORNER_INDEX_DBL,
    .dfl_dbr_corners = (G1_CORNER_INDEX_DFL << 4) | G1_CORNER_INDEX_DBR
};

void g1_move_u(g1_state_t* state) {
    uint8_t swap = state->ul_ur_edges;
    state->ul_ur_edges = state->uf_ub_edges;
    state->uf_ub_edges = byte_rotate_left(swap, 4);

    swap = state->ufl_ubr_corners;
    state->ufl_ubr_corners = state->ufr_ubl_corners;
    state->ufr_ubl_corners = byte_rotate_left(swap, 4);
}

void g1_move_d(g1_state_t* state) {
    uint8_t swap = state->df_db_edges;
    state->df_db_edges = state->dl_dr_edges;
    state->dl_dr_edges = byte_rotate_left(swap, 4);

    swap = state->dfr_dbl_corners;
    state->dfr_dbl_corners = state->dfl_dbr_corners;
    state->dfl_dbr_corners = byte_rotate_left(swap, 4);
}

void g1_move_r(g1_state_t* state) {
    uint8_t swap = state->ul_ur_edges;

    state->ul_ur_edges &= 0b11110000;
    state->ul_ur_edges |= state->fl_fr_edges & 0b1111;

    state->fl_fr_edges &= 0b11110000;
    state->fl_fr_edges |= state->dl_dr_edges & 0b1111;

    state->dl_dr_edges &= 0b11110000;
    state->dl_dr_edges |= state->bl_br_edges & 0b1111;

    state->bl_br_edges &= 0b11110000;
    state->bl_br_edges |= swap & 0b1111;

    swap = state->ufr_ubl_corners;

    state->ufr_ubl_corners &= 0b1111;
    state->ufr_ubl_corners |= state->dfr_dbl_corners & 0b11110000;

    state->dfr_dbl_corners &= 0b1111;
    state->dfr_dbl_corners |= state->dfl_dbr_corners << 4;

    state->dfl_dbr_corners &= 0b11110000;
    state->dfl_dbr_corners |= state->ufl_ubr_corners & 0b1111;

    state->ufl_ubr_corners &= 0b11110000;
    state->ufl_ubr_corners |= swap >> 4;
}

void g1_move_l(g1_state_t* state) {
    uint8_t swap = state->ul_ur_edges;

    state->ul_ur_edges &= ~0b11110000;
    state->ul_ur_edges |= state->bl_br_edges & 0b11110000;

    state->bl_br_edges &= ~0b11110000;
    state->bl_br_edges |= state->dl_dr_edges & 0b11110000;

    state->dl_dr_edges &= ~0b11110000;
    state->dl_dr_edges |= state->fl_fr_edges & 0b11110000;

    state->fl_fr_edges &= ~0b11110000;
    state->fl_fr_edges |= swap & 0b11110000;

    swap = state->ufr_ubl_corners;

    state->ufr_ubl_corners &= ~0b00001111;
    state->ufr_ubl_corners |= state->dfr_dbl_corners & 0b00001111;

    state->dfr_dbl_corners &= ~0b00001111;
    state->dfr_dbl_corners |= state->dfl_dbr_corners >> 4;

    state->dfl_dbr_corners &= ~0b11110000;
    state->dfl_dbr_corners |= state->ufl_ubr_corners & 0b11110000;

    state->ufl_ubr_corners &= ~0b11110000;
    state->ufl_ubr_corners |= swap << 4;
}

void g1_move_f(g1_state_t* state) {
    uint8_t swap = state->uf_ub_edges;

    state->uf_ub_edges &= ~0b11110000;
    state->uf_ub_edges |= state->fl_fr_edges & 0b11110000;

    state->fl_fr_edges &= ~0b11110000;
    state->fl_fr_edges |= state->df_db_edges & 0b11110000;

    state->df_db_edges &= ~0b11110000;
    state->df_db_edges |= state->fl_fr_edges << 4;

    state->fl_fr_edges &= ~0b00001111;
    state->fl_fr_edges |= swap >> 4;

    swap = state->ufr_ubl_corners;

    state->ufr_ubl_corners &= ~0b11110000;
    state->ufr_ubl_corners |= state->ufl_ubr_corners & 0b11110000;

    state->ufl_ubr_corners &= ~0b11110000;
    state->ufl_ubr_corners |= state->dfl_dbr_corners & 0b11110000;

    state->dfl_dbr_corners &= ~0b11110000;
    state->dfl_dbr_corners |= state->dfr_dbl_corners & 0b11110000;

    state->dfr_dbl_corners &= ~0b11110000;
    state->dfr_dbl_corners |= swap & 0b11110000;
}

void g1_move_b(g1_state_t* state) {
    uint8_t swap = state->uf_ub_edges;

    state->uf_ub_edges &= ~0b00001111;
    state->uf_ub_edges |= state->bl_br_edges & 0b00001111;

    state->bl_br_edges &= ~0b00001111;
    state->bl_br_edges |= state->df_db_edges & 0b00001111;

    state->df_db_edges &= ~0b00001111;
    state->df_db_edges |= state->bl_br_edges >> 4;

    state->bl_br_edges &= ~0b11110000;
    state->bl_br_edges |= swap << 4;

    swap = state->ufr_ubl_corners;

    state->ufr_ubl_corners &= ~0b00001111;
    state->ufr_ubl_corners |= state->ufl_ubr_corners & 0b00001111;

    state->ufl_ubr_corners &= ~0b00001111;
    state->ufl_ubr_corners |= state->dfl_dbr_corners & 0b00001111;

    state->dfl_dbr_corners &= ~0b00001111;
    state->dfl_dbr_corners |= state->dfr_dbl_corners & 0b00001111;

    state->dfr_dbl_corners &= ~0b00001111;
    state->dfr_dbl_corners |= swap & 0b00001111;
}
