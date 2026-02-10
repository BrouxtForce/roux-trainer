#pragma once

#include "common.h"

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

void visual_cube_state_reset(visual_cube_state_t* visual_cube_state);

int visual_cube_get_edge_sticker_index(face_index_e primary_face_index, face_index_e secondary_face_index);
int visual_cube_get_corner_sticker_index(visual_cube_corner_e corner, face_index_e primary_face_index);

void visual_cube_state_write_center(visual_cube_state_t* visual_cube_state, face_index_e center, face_index_e state);

void visual_cube_state_write_corner(visual_cube_state_t* visual_cube_state, visual_cube_corner_e corner,
                                    face_index_e primary_sticker, face_index_e secondary_sticker, face_index_e tertiary_sticker);

void draw_visual_cube_state(const visual_cube_state_t* visual_cube_state);
