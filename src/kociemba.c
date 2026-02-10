#include "kociemba.h"

#include <assert.h>
#include <string.h>
#include "common.h"
#include "visual_cube.h"

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
