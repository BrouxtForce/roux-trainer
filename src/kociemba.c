#include "kociemba.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "common.h"
#include "visual_cube.h"

#define G0_EDGE_PACKING_ORIENTATION_MASK 0b01010101
#define G0_EDGE_PACKING_MIDDLE_MASK      0b10101010

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

static move_e _get_base_move(move_e move) {
    return move - move % 3;
}

static void _decompose_move(move_e move, move_e* base_move, int* count) {
    *count     = move % 3;
    *base_move = move - *count;
    (*count)++;
}

static bool _should_prune_move(move_e prev_base_move, move_e base_move) {
    assert(prev_base_move % 3 == 0 && base_move % 3 == 0);
    return prev_base_move == base_move ||
          (prev_base_move == MOVE_D && base_move == MOVE_U) ||
          (prev_base_move == MOVE_B && base_move == MOVE_F) ||
          (prev_base_move == MOVE_L && base_move == MOVE_R);
}

void g0_execute_move(g0_state_t* g0_state, move_e move) {
    move_e base_move;
    int    count;
    _decompose_move(move, &base_move, &count);

    for (int i = 0; i < count; i++) {
        switch (base_move) {
            case MOVE_U: g0_move_u(g0_state); break;
            case MOVE_D: g0_move_d(g0_state); break;
            case MOVE_F: g0_move_f(g0_state); break;
            case MOVE_B: g0_move_b(g0_state); break;
            case MOVE_R: g0_move_r(g0_state); break;
            case MOVE_L: g0_move_l(g0_state); break;
            default:     assert(false);
        }
    }
}

void g1_execute_move(g1_state_t* g1_state, move_e move) {
    move_e base_move;
    int    count;
    _decompose_move(move, &base_move, &count);

    for (int i = 0; i < count; i++) {
        switch (base_move) {
            case MOVE_U: g1_move_u(g1_state); break;
            case MOVE_D: g1_move_d(g1_state); break;
            case MOVE_F: g1_move_f(g1_state); break;
            case MOVE_B: g1_move_b(g1_state); break;
            case MOVE_R: g1_move_r(g1_state); break;
            case MOVE_L: g1_move_l(g1_state); break;
            default:     assert(false);
        }
    }
}

// TODO: This function could be faster if all of the edges were stored in one uint32_t
static int g0_get_eo_index(g0_state_t state) {
    int edges =
        ((int)(state.ub_ur_uf_ul_edges & G0_EDGE_PACKING_ORIENTATION_MASK) << 0) |
        ((int)(state.df_dr_db_dl_edges & G0_EDGE_PACKING_ORIENTATION_MASK) << 8) |
        ((int)(state.bl_fr_br_fl_edges & G0_EDGE_PACKING_ORIENTATION_MASK) << 16);

    int index = (edges | (edges >> 11)) & 0b11111111111;

    assert(index >= 0 && index < 2048);
    return index;
}

// This function takes a 16-bit unsigned integer which holds 4 4-bit ternary (base-3) digits, and
// converts it to a binary value.
static int _internal_from_ternary_u16(uint16_t senary) {
    int a =  senary >> 12;
    int b = (senary >> 8) & 0b1111;
    int c = (senary >> 4) & 0b1111;
    int d = (senary     ) & 0b1111;

    return 3*(3*(3*a + b) + c) + d;
}

static int g0_get_co_index(g0_state_t state) {
    int u_corners = _internal_from_ternary_u16(state.ufl_ufr_ubr_ubl_corners);
    int d_corners = _internal_from_ternary_u16(state.dfr_dfl_dbl_dbr_corners & 0b0000111111111111);

    int index = 81*d_corners + u_corners;

    assert(index >= 0 && index < 2187);
    return index;
}

static int calc_eslice_index(uint32_t num) {
    assert(popcount_u32(num) == 4);

    int index_0 = 31 - clz_u32(num);
    num &= ~((uint32_t)1 << index_0);

    int index_1 = 31 - clz_u32(num);
    num &= ~((uint32_t)1 << index_1);

    int index_2 = 31 - clz_u32(num);
    num &= ~((uint32_t)1 << index_2);

    int index_3 = 31 - clz_u32(num);

    int result_0 = index_0 * (index_0 - 1) * (index_0 - 2) * (index_0 - 3) / 24;
    int result_1 = index_1 * (index_1 - 1) * (index_1 - 2) / 6;
    int result_2 = index_2 * (index_2 - 1) / 2;
    int result_3 = index_3;

    return result_0 + result_1 + result_2 + result_3;
}

// TODO: This function could be faster if all of the edges were stored in one uint32_t
static int g0_get_eslice_index(g0_state_t state) {
    uint32_t edges =
        ((uint32_t)(state.ub_ur_uf_ul_edges & G0_EDGE_PACKING_MIDDLE_MASK) << 0) |
        ((uint32_t)(state.df_dr_db_dl_edges & G0_EDGE_PACKING_MIDDLE_MASK) << 8) |
        ((uint32_t)(state.bl_fr_br_fl_edges & G0_EDGE_PACKING_MIDDLE_MASK) << 16);
    edges = (edges | (edges >> 13)) & 0b111111111111;

    return calc_eslice_index(edges);
}

static int g0_get_eo_and_eslice_index(g0_state_t state, int eslice_index) {
    int eo_index     = g0_get_eo_index(state);
    int index        = G0_NUM_ESLICE_COMBINATIONS * eo_index + eslice_index;

    assert(index >= 0 && index < G0_EO_AND_ESLICE_TABLE_SIZE);
    return index;
}

static int g0_get_co_and_eslice_index(g0_state_t state, int eslice_index) {
    int co_index     = g0_get_co_index(state);
    int index        = G0_NUM_ESLICE_COMBINATIONS * co_index + eslice_index;

    assert(index >= 0 && index < G0_CO_AND_ESLICE_TABLE_SIZE);
    return index;
}

void _recursive_g0_fill_table(g0_table_t* g0_table, g0_state_t g0_state, move_e prev_base_move, int depth, int distance_from_solved) {
    int eslice_index = g0_get_eslice_index(g0_state);
    int eo_index = g0_get_eo_and_eslice_index(g0_state, eslice_index);
    int co_index = g0_get_co_and_eslice_index(g0_state, eslice_index);

    bool eo_insert = false, co_insert = false;
    if (distance_from_solved < g0_table->eo_and_eslice_table[eo_index]) {
        g0_table->eo_and_eslice_table[eo_index] = distance_from_solved;
        eo_insert = true;
    }
    if (distance_from_solved < g0_table->co_and_eslice_table[co_index]) {
        g0_table->co_and_eslice_table[co_index] = distance_from_solved;
        co_insert = true;
    }

    if (!eo_insert && !co_insert) {
        return;
    }

    if (depth <= 0) {
        return;
    }

    g0_state_t original_state = g0_state;
    for (int i = 0; i < 6; i++) {
        move_e base_move = 3 * i;

        if (_should_prune_move(prev_base_move, base_move)) {
            continue;
        }

        g0_state = original_state;
        for (int j = 0; j < 3; j++) {
            switch (base_move) {
                case MOVE_U: g0_move_u(&g0_state); break;
                case MOVE_D: g0_move_d(&g0_state); break;
                case MOVE_F: g0_move_f(&g0_state); break;
                case MOVE_B: g0_move_b(&g0_state); break;
                case MOVE_R: g0_move_r(&g0_state); break;
                case MOVE_L: g0_move_l(&g0_state); break;
                default:     assert(false);
            }
            _recursive_g0_fill_table(g0_table, g0_state, base_move, depth - 1, distance_from_solved + 1);
        }
    }
}

g0_table_t* g0_init_table(allocator_e allocator) {
    // In my tests, I've found for the specific tables that I'm generating below that the maximum
    // distance from solved for any of these states is 9. This means that we can just set the default
    // value for each result in the table to be 9, and only search up to depth 8 to fill the table.
    static const int MAX_DISTANCE_FROM_SOLVED = 9;

    g0_table_t* g0_table = alloc(sizeof(g0_table_t), allocator, SOURCE_LOCATION);
    memset(g0_table, MAX_DISTANCE_FROM_SOLVED, sizeof(*g0_table));

    _recursive_g0_fill_table(g0_table, G0_STATE_SOLVED, MOVE_NULL, MAX_DISTANCE_FROM_SOLVED - 1, 0);

    return g0_table;
}

bool g0_is_solved(g0_state_t g0_state) {
    return memcmp(&g0_state, &G0_STATE_SOLVED, sizeof(g0_state_t)) == 0;
}

static void _search_g0_helper(const g0_table_t* g0_table, g0_state_t g0_state, move_list_t* move_list, solution_list_t* solution_list, int depth) {
    int eslice_index = g0_get_eslice_index(g0_state);
    int distance_from_solved = max_i32(
        g0_table->eo_and_eslice_table[g0_get_eo_and_eslice_index(g0_state, eslice_index)],
        g0_table->co_and_eslice_table[g0_get_co_and_eslice_index(g0_state, eslice_index)]
    );

    if (depth < distance_from_solved) return;

    if (distance_from_solved == 0) {
        move_list_t copy_move_list = { .allocator = solution_list->allocator };
        array_copy(*move_list, copy_move_list);
        array_append(*solution_list, copy_move_list);
        return;
    }

    g0_state_t original_state = g0_state;

    for (int i = 0; i < 6; i++) {
        move_e base_move = 3 * i;

        if (move_list->size > 0 && _should_prune_move(_get_base_move(move_list->data[move_list->size - 1]), base_move)) {
            continue;
        }

        g0_state = original_state;
        for (int j = 0; j < 3; j++) {
            array_append(*move_list, base_move + j);
            switch (base_move) {
                case MOVE_U: g0_move_u(&g0_state); break;
                case MOVE_D: g0_move_d(&g0_state); break;
                case MOVE_F: g0_move_f(&g0_state); break;
                case MOVE_B: g0_move_b(&g0_state); break;
                case MOVE_R: g0_move_r(&g0_state); break;
                case MOVE_L: g0_move_l(&g0_state); break;
                default:     assert(false);
            }
            _search_g0_helper(g0_table, g0_state, move_list, solution_list, depth - 1);
            array_pop(*move_list);
        }
    }
}

static solution_list_t _solve_g0_at_depth(const g0_table_t* g0_table, g0_state_t g0_state, int depth, allocator_e allocator) {
    solution_list_t solution_list = { .allocator = allocator };
    move_list_t move_list = { .allocator = TEMP_ALLOCATOR };

    _search_g0_helper(g0_table, g0_state, &move_list, &solution_list, depth);

    return solution_list;
}

solution_list_t solve_g0(const g0_table_t* g0_table, g0_state_t g0_state, allocator_e allocator) {
    if (g0_is_solved(g0_state)) {
        solution_list_t out = { .allocator = allocator };
        array_append(out, (move_list_t){});
        return out;
    }

    for (int depth = 1; depth <= 12; depth++) {
        solution_list_t solutions = _solve_g0_at_depth(g0_table, g0_state, depth, allocator);
        if (solutions.size > 0) {
            return solutions;
        }
    }

    assert(false);
    return (solution_list_t){};
}

static bool g1_state_equals(g1_state_t left, g1_state_t right) {
    return memcmp(&left, &right, sizeof(g1_state_t)) == 0;
}

#define HASH_TABLE_NAME g1_table
#define STATE_TYPE g1_state_t
#define STATE_EQUALS(a, b) g1_state_equals(a, b)
#define STATE_TO_U64(state) (*(uint64_t*)&state)
#define STATE_NULL G1_STATE_NULL
#define HASH_TABLE_SIZE G1_TABLE_SIZE
#include "hash_table.h"

void _recursive_g1_fill_table(g1_table_t* g1_table, g1_state_t g1_state, move_e prev_base_move, int depth, int distance_from_solved) {
    if (!g1_table_insert(g1_table, g1_state, distance_from_solved)) {
        return;
    }

    if (depth <= 0) {
        return;
    }

    g1_state_t original_state = g1_state;
    for (int i = 0; i < 6; i++) {
        move_e base_move = 3 * i;

        if (_should_prune_move(prev_base_move, base_move)) {
            continue;
        }

        g1_state = original_state;

        if (base_move == MOVE_F || base_move == MOVE_B || base_move == MOVE_R || base_move == MOVE_L) {
            for (int i = 0; i < 2; i++) {
                switch (base_move) {
                    case MOVE_F: g1_move_f(&g1_state); break;
                    case MOVE_B: g1_move_b(&g1_state); break;
                    case MOVE_R: g1_move_r(&g1_state); break;
                    case MOVE_L: g1_move_l(&g1_state); break;
                    default:     assert(false);
                }
            }
            _recursive_g1_fill_table(g1_table, g1_state, base_move, depth - 1, distance_from_solved + 1);
            continue;
        }

        for (int j = 0; j < 3; j++) {
            switch (base_move) {
                case MOVE_U: g1_move_u(&g1_state); break;
                case MOVE_D: g1_move_d(&g1_state); break;
                default:     assert(false);
            }
            _recursive_g1_fill_table(g1_table, g1_state, base_move, depth - 1, distance_from_solved + 1);
        }
    }
}

g1_table_t* g1_init_table(allocator_e allocator) {
    g1_table_t* g1_table = alloc(sizeof(g1_table_t), allocator, SOURCE_LOCATION);
    memset(g1_table, 0, sizeof(*g1_table));

    g1_table->magic = random_u64();
    _recursive_g1_fill_table(g1_table, G1_STATE_SOLVED, MOVE_NULL, G1_TABLE_DEPTH, 0);

    assert(g1_table->count == G1_TABLE_SIZE);

    return g1_table;
}

bool g1_is_solved(g1_state_t g1_state) {
    return memcmp(&g1_state, &G1_STATE_SOLVED, sizeof(g1_state_t)) == 0;
}

static void _search_g1_helper(g1_table_t* g1_table, g1_state_t g1_state, move_list_t* move_list, solution_list_t* solution_list, int depth) {
    if (g1_is_solved(g1_state)) {
        move_list_t copy_move_list = { .allocator = solution_list->allocator };
        array_copy(*move_list, copy_move_list);
        array_append(*solution_list, copy_move_list);
        return;
    }

    if (depth <= 0) return;

    int distance_from_solved = g1_table_lookup(g1_table, g1_state);
    if (distance_from_solved == -1) {
        // Best case scenario
        distance_from_solved = G1_TABLE_DEPTH + 1;
    }

    if (depth < distance_from_solved) return;

    g1_state_t original_state = g1_state;

    for (int i = 0; i < 6; i++) {
        move_e base_move = 3 * i;

        if (move_list->size > 0 && _should_prune_move(_get_base_move(move_list->data[move_list->size - 1]), base_move)) {
            continue;
        }

        g1_state = original_state;

        if (base_move == MOVE_F || base_move == MOVE_B || base_move == MOVE_R || base_move == MOVE_L) {
            for (int i = 0; i < 2; i++) {
                switch (base_move) {
                    case MOVE_F: g1_move_f(&g1_state); break;
                    case MOVE_B: g1_move_b(&g1_state); break;
                    case MOVE_R: g1_move_r(&g1_state); break;
                    case MOVE_L: g1_move_l(&g1_state); break;
                    default:     assert(false);
                }
            }
            array_append(*move_list, base_move + 1);
            _search_g1_helper(g1_table, g1_state, move_list, solution_list, depth - 1);
            array_pop(*move_list);
            continue;
        }

        for (int j = 0; j < 3; j++) {
            array_append(*move_list, base_move + j);
            switch (base_move) {
                case MOVE_U: g1_move_u(&g1_state); break;
                case MOVE_D: g1_move_d(&g1_state); break;
                default:     assert(false);
            }
            _search_g1_helper(g1_table, g1_state, move_list, solution_list, depth - 1);
            array_pop(*move_list);
        }
    }
}

static solution_list_t _solve_g1_at_depth(g1_table_t* g1_table, g1_state_t g1_state, int depth, allocator_e allocator) {
    solution_list_t solution_list = { .allocator = allocator };
    move_list_t move_list = { .allocator = TEMP_ALLOCATOR };

    _search_g1_helper(g1_table, g1_state, &move_list, &solution_list, depth);

    return solution_list;
}

solution_list_t solve_g1(g1_table_t* g1_table, g1_state_t g1_state, allocator_e allocator) {
    if (g1_is_solved(g1_state)) {
        solution_list_t out = { .allocator = allocator };
        array_append(out, (move_list_t){});
        return out;
    }

    for (int depth = 1; depth <= 18; depth++) {
        printf("[G1] Searching depth %i\n", depth);
        solution_list_t solutions = _solve_g1_at_depth(g1_table, g1_state, depth, allocator);
        if (solutions.size > 0) {
            return solutions;
        }
    }

    assert(false);
    return (solution_list_t){};
}

move_list_t solve_g0_g1(g0_table_t* g0_table, g1_table_t* g1_table, g0_state_t g0_state, g1_state_t g1_state, allocator_e allocator) {
    // TODO: Search suboptimal G0 solutions
    solution_list_t g0_solutions = solve_g0(g0_table, g0_state, TEMP_ALLOCATOR);
    assert(g0_solutions.size > 0);

    move_list_t best_g0_solution = { .allocator = TEMP_ALLOCATOR };
    move_list_t best_g1_solution = { .allocator = TEMP_ALLOCATOR };

    bool found_solution = false;
    for (int depth = 1; depth <= 18; depth++) {
        for (size_t i = 0; i < g0_solutions.size; i++) {
            move_list_t g0_solution = g0_solutions.data[i];

            g1_state_t new_state = g1_state;
            for (size_t j = 0; j < g0_solution.size; j++) {
                g1_execute_move(&new_state, g0_solution.data[j]);
            }

            solution_list_t g1_solutions = _solve_g1_at_depth(g1_table, new_state, depth, TEMP_ALLOCATOR);
            if (g1_solutions.size > 0) {
                best_g0_solution = g0_solution;
                best_g1_solution = g1_solutions.data[0];
                found_solution = true;
                break;
            }
        }
        if (found_solution) break;
    }

    move_list_t solution = { .allocator = allocator };
    array_reserve(solution, best_g0_solution.size + best_g1_solution.size);

    array_append_array(solution, best_g0_solution);
    array_append_array(solution, best_g1_solution);

    return solution;
}
