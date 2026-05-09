#pragma once

#include <stdint.h>
#include "visual_cube.h"

typedef struct {
    uint8_t ub_ur_uf_ul_edges;
    uint8_t df_dr_db_dl_edges;
    uint8_t bl_fr_br_fl_edges;
    uint8_t _padding;
    uint16_t ufl_ufr_ubr_ubl_corners;
    uint16_t dfr_dfl_dbl_dbr_corners;
} g0_state_t;

static_assert(sizeof(g0_state_t) == sizeof(uint64_t));

static const g0_state_t G0_STATE_SOLVED = {
    .ub_ur_uf_ul_edges = 0,
    .df_dr_db_dl_edges = 0,
    .bl_fr_br_fl_edges = 0b10101010,
    .ufl_ufr_ubr_ubl_corners = 0,
    .dfr_dfl_dbl_dbr_corners = 0
};

static const g0_state_t G0_STATE_NULL = {};

void g0_move_u(g0_state_t* state);
void g0_move_d(g0_state_t* state);
void g0_move_r(g0_state_t* state);
void g0_move_f(g0_state_t* state);
void g0_move_l(g0_state_t* state);
void g0_move_b(g0_state_t* state);

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

static_assert(sizeof(g1_state_t) == 10);

static const g1_state_t G1_STATE_SOLVED = {
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

static const g1_state_t G1_STATE_NULL = {};

void g1_move_u(g1_state_t* state);
void g1_move_d(g1_state_t* state);
void g1_move_r(g1_state_t* state);
void g1_move_l(g1_state_t* state);
void g1_move_f(g1_state_t* state);
void g1_move_b(g1_state_t* state);

void g0_g1_state_write_visual_cube_state(visual_cube_state_t* visual_cube_state, g0_state_t g0_state, g1_state_t g1_state);

void g0_execute_move(g0_state_t* g0_state, move_e move);
void g1_execute_move(g1_state_t* g1_state, move_e move);

#define G0_TABLE_SIZE  95039
#define G0_TABLE_DEPTH 5

typedef struct {
    g0_state_t state;
    int next_index;
    int distance_from_solved;
} g0_table_node_t;

typedef struct {
    g0_table_node_t entries[G0_TABLE_SIZE];
    uint32_t count;
    uint32_t next_free;
    uint64_t magic;
} g0_table_t;

void g0_init_table(g0_table_t* g0_table);

bool g0_is_solved(g0_state_t g0_state);

solution_list_t solve_g0(const g0_table_t* g0_table, g0_state_t g0_state, allocator_e);

#define G1_TABLE_SIZE 883485
#define G1_TABLE_DEPTH 7

typedef struct {
    g1_state_t state;
    int next_index;
    int distance_from_solved;
} g1_table_node_t;

typedef struct {
    g1_table_node_t entries[G1_TABLE_SIZE];
    uint32_t count;
    uint32_t next_free;
    uint64_t magic;
} g1_table_t;

void g1_init_table(g1_table_t* g1_table);

bool g1_is_solved(g1_state_t g1_state);

solution_list_t solve_g1(g1_table_t* g1_table, g1_state_t g1_state, allocator_e allocator);

move_list_t solve_g0_g1(g0_table_t* g0_table, g1_table_t* g1_table, g0_state_t g0_state, g1_state_t g1_state, allocator_e allocator);
