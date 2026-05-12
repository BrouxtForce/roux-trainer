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

void get_visual_cube_state_string(const visual_cube_state_t* visual_cube_state, char buffer[55]);

// TODO: Should this be merged with the original visual_cube_state_t?
typedef struct {
    face_index_e* stickers[6];
    int n;
    allocator_e allocator;
} big_visual_cube_state_t;

void big_visual_cube_state_init(big_visual_cube_state_t* state, int n, allocator_e allocator);
void big_visual_cube_state_free(big_visual_cube_state_t* state);
void big_visual_cube_state_reset(big_visual_cube_state_t* state);

void big_visual_cube_state_write_center(big_visual_cube_state_t* state, face_index_e face, int center_index, face_index_e sticker_state);

// TODO: This function only works for 4x4
void big_visual_cube_state_write_wing(big_visual_cube_state_t* state, face_index_e primary_face, face_index_e secondary_face,
                                      face_index_e primary_sticker, face_index_e secondary_sticker);

void big_visual_cube_state_copy_corners(big_visual_cube_state_t* big_visual_cube_state, visual_cube_state_t* visual_cube_state);

void draw_big_visual_cube_state(const big_visual_cube_state_t* big_visual_cube_state);

char* get_big_visual_cube_state_string(const big_visual_cube_state_t* state, allocator_e allocator);
