#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "visual_cube.h"
#include "kociemba.h"

move_e consume_next_move(const char** alg_string) {
    while (true) {
        char next_char = (*alg_string)[0];
        if (next_char == '\0') {
            return MOVE_NULL;
        }
        if (next_char != ' ') {
            break;
        }

        (*alg_string)++;
    }

    char move_char        = (*alg_string)[0];
    char move_modifier    = (*alg_string)[1];
    int  move_enum_offset = 0;

    if (move_modifier == '2')  move_enum_offset = 1;
    if (move_modifier == '\'') move_enum_offset = 2;

    (*alg_string)++;
    if (move_enum_offset != 0) {
        (*alg_string)++;
    }

    switch (move_char) {
        case 'U': return MOVE_U + move_enum_offset;
        case 'D': return MOVE_D + move_enum_offset;
        case 'F': return MOVE_F + move_enum_offset;
        case 'B': return MOVE_B + move_enum_offset;
        case 'R': return MOVE_R + move_enum_offset;
        case 'L': return MOVE_L + move_enum_offset;
    }

    assert(false);
}

void g0_g1_execute_alg_string(g0_state_t* g0_state, g1_state_t* g1_state, const char* alg_string) {
    while (true) {
        move_e move = consume_next_move(&alg_string);
        if (move == MOVE_NULL) {
            break;
        }

        g0_execute_move(g0_state, move);
        g1_execute_move(g1_state, move);
    }
}

bool g0_g1_test_alg(const char* alg_string, const char* expected_state) {
    g0_state_t g0_state = G0_STATE_SOLVED;
    g1_state_t g1_state = G1_STATE_SOLVED;
    g0_g1_execute_alg_string(&g0_state, &g1_state, alg_string);

    visual_cube_state_t visual_cube_state;
    visual_cube_state_reset(&visual_cube_state);
    g0_g1_state_write_visual_cube_state(&visual_cube_state, g0_state, g1_state);

    char state_string[55];
    get_visual_cube_state_string(&visual_cube_state, state_string);

    if (strcmp(state_string, expected_state) == 0) {
        return true;
    }

    printf("Failed test:\n");
    printf("Alg:            %s\n", alg_string);
    printf("Actual state:   %s\n", state_string);
    printf("Expected state: %s\n", expected_state);
    printf("\n");

    return false;
}

void run_tests() {
    const char* g0_g1_state_tests[][2] = {
        { "",                                                       "UUUUUUUUULLLLLLLLLFFFFFFFFFRRRRRRRRRBBBBBBBBBDDDDDDDDD" },
        { "D B2 U2 F2 R2 D F2 D' R2 U2 R2 B' R' U' R' F2 L' B' R2", "BBBUUDLDRDFBLLLUUUDRFDFDRLLDFLBRLURLURRBBFFFRFUFBDUBRD" },
        { "U' R' U2 F R2 U B' R D2 B2 L2 U2 F2 L D2 F2 U2 L2 B' U", "FDRFULRFLURFFLFDBDDDFLFDBRLUUBRRLBBFULRBBUDBBLUURDURDL" },
        { "D2 L2 R2 U' R2 D F2 R2 B2 D R B' R F2 D B F R' F'",      "FRUUUUURDUBBLLLLDDLDFFFFFLBLFLRRLRDDFBRUBBRRBRDUBDFDUB" }
    };
    const int G0_G1_NUM_TESTS = sizeof(g0_g1_state_tests) / sizeof(*g0_g1_state_tests);

    bool all_tests_succeeded = true;
    for (int i = 0; i < G0_G1_NUM_TESTS; i++) {
        const char* alg_string     = g0_g1_state_tests[i][0];
        const char* expected_state = g0_g1_state_tests[i][1];

        if (!g0_g1_test_alg(alg_string, expected_state)) {
            all_tests_succeeded = false;
            break;
        }
    }

    if (all_tests_succeeded) {
        printf("All tests succeeded!\n");
    }
}

typedef struct {
    clock_t start;
    clock_t end;
} timer_t;

void timer_start(timer_t* timer) {
    timer->start = clock();
}

void timer_stop(timer_t* timer) {
    timer->end = clock();
}

double timer_duration(timer_t timer) {
    return (double)(timer.end - timer.start) / CLOCKS_PER_SEC;
}

void run_perf_tests() {
    srand(42);

    const char* superflip = "U R2 F B R B2 R U2 L B2 R U' D' R2 F R' L B2 U2 F2";

    g0_state_t g0_state = G0_STATE_SOLVED;
    g1_state_t g1_state = G1_STATE_SOLVED;
    g0_g1_execute_alg_string(&g0_state, &g1_state, superflip);

    timer_t timer;

    g0_table_t g0_table;
    double g0_table_init_total_time = 0.0;
    int g0_table_init_iterations = 10;
    for (int i = 0; i < g0_table_init_iterations; i++) {
        timer_start(&timer);
        g0_init_table(&g0_table);
        timer_stop(&timer);
        printf("G0 table initialization: %fs\n", timer_duration(timer));
        g0_table_init_total_time += timer_duration(timer);
    }

    double g0_solve_total_time = 0.0;
    int g0_solve_iterations = 10;
    for (int i = 0; i < g0_solve_iterations; i++) {
        timer_start(&timer);
        solve_g0(&g0_table, g0_state);
        timer_stop(&timer);
        printf("G0 solve: %fs\n", timer_duration(timer));
        g0_solve_total_time += timer_duration(timer);
    }

    printf("--- PERFORMANCE ---\n");
    printf("G0 Table Init: %fs (average)\n", g0_table_init_total_time / g0_table_init_iterations);
    printf("G0 Solve:      %fs (average)\n", g0_solve_total_time / g0_solve_iterations);
}
