#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <stddef.h>

#include "visual_cube.h"
#include "kociemba.h"
#include "revenge.h"

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

double timer_duration_ms(timer_t timer) {
    return 1000.0 * timer_duration(timer);
}

move_t consume_next_move(const char** alg_string) {
    while (true) {
        char next_char = (*alg_string)[0];
        if (next_char == '\0') {
            return (move_t){ .move = MOVE_NULL, .width = 1 };
        }
        if (next_char != ' ') {
            break;
        }

        (*alg_string)++;
    }

    char    move_char        = (*alg_string)[0];
    char    move_modifier    = (*alg_string)[1];
    uint8_t move_enum_offset = 0;
    uint8_t width            = 1;

    if (move_modifier == 'w') {
        width = 2;
        move_modifier = (*alg_string)[2];
        (*alg_string)++;
    }
    if (move_modifier == '2')  move_enum_offset = 1;
    if (move_modifier == '\'') move_enum_offset = 2;

    (*alg_string)++;
    if (move_enum_offset != 0) {
        (*alg_string)++;
    }

    switch (move_char) {
        case 'U': return (move_t){ .move = MOVE_U + move_enum_offset, .width = width };
        case 'D': return (move_t){ .move = MOVE_D + move_enum_offset, .width = width };
        case 'F': return (move_t){ .move = MOVE_F + move_enum_offset, .width = width };
        case 'B': return (move_t){ .move = MOVE_B + move_enum_offset, .width = width };
        case 'R': return (move_t){ .move = MOVE_R + move_enum_offset, .width = width };
        case 'L': return (move_t){ .move = MOVE_L + move_enum_offset, .width = width };
    }

    unreachable();
}

void g0_g1_execute_alg_string(g0_state_t* g0_state, g1_state_t* g1_state, const char* alg_string) {
    while (true) {
        move_e move = consume_next_move(&alg_string).move;
        if (move == MOVE_NULL) {
            break;
        }

        g0_execute_move(g0_state, move);
        g1_execute_move(g1_state, move);
    }
}

void revenge_execute_alg_string(revenge_g0_state_t* g0_state, revenge_g1_state_t* g1_state, const char* alg_string) {
    while (true) {
        move_t move = consume_next_move(&alg_string);
        if (move.move == MOVE_NULL) {
            break;
        }

        move_composition_t move_composition = decompose_move(move.move);
        if (move.width == 1) {
            for (int i = 0; i < move_composition.count_cw; i++) {
                revenge_g0_move_face_cw(g0_state, move_composition.face_index);
                switch (move_composition.face_index) {
                    case FACE_INDEX_U: revenge_g1_move_u(g1_state); break;
                    case FACE_INDEX_D: revenge_g1_move_d(g1_state); break;
                    case FACE_INDEX_F: revenge_g1_move_f(g1_state); break;
                    case FACE_INDEX_B: revenge_g1_move_b(g1_state); break;
                    case FACE_INDEX_R: revenge_g1_move_r(g1_state); break;
                    case FACE_INDEX_L: revenge_g1_move_l(g1_state); break;
                    default: assert(false);
                }
            }
            continue;
        }
        if (move.width == 2) {
            for (int i = 0; i < move_composition.count_cw; i++) {
                switch (move_composition.face_index) {
                    case FACE_INDEX_U: revenge_g0_move_uw(g0_state); revenge_g1_move_uw(g1_state); break;
                    case FACE_INDEX_F: revenge_g0_move_fw(g0_state); revenge_g1_move_fw(g1_state); break;
                    case FACE_INDEX_R: revenge_g0_move_rw(g0_state); revenge_g1_move_rw(g1_state); break;
                    default: assert(false);
                }
            }
            continue;
        }
        assert(false);
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

bool revenge_test_alg(const char* alg_string, const char* expected_state) {
    g0_state_t g0_state = G0_STATE_SOLVED;
    g1_state_t g1_state = G1_STATE_SOLVED;
    g0_g1_execute_alg_string(&g0_state, &g1_state, alg_string);

    revenge_g0_state_t revenge_g0_state = REVENGE_G0_STATE_SOLVED;
    revenge_g1_state_t revenge_g1_state = REVENGE_G1_STATE_SOLVED;
    revenge_execute_alg_string(&revenge_g0_state, &revenge_g1_state, alg_string);

    big_visual_cube_state_t visual_cube_state = {};
    big_visual_cube_state_init(&visual_cube_state, 4, TEMP_ALLOCATOR);
    revenge_write_visual_cube_state(&visual_cube_state, revenge_g0_state, revenge_g1_state, g0_state, g1_state);

    char* state_string = get_big_visual_cube_state_string(&visual_cube_state, TEMP_ALLOCATOR);
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

bool test_4x4() {
    const char* scrambles[][2] = {
        { "B L D L2 F U' L D R U2 L2 F R2 U2 F2 B' R2 U2 L2 D2 R' Uw2 Rw2 F Fw2 L2 F Uw2 R' Uw2 B Rw2 B2 L Uw' L2 F' Uw2 R Fw' R D' Uw2 R2 Uw R2",
          "BBFRUULUFURDFBDFULRLULBLFBUFRLUDDRBDDLRFDRLRBDUURBRDLBURDFRLRDBUBLRLFDDRUFFUFLLBLFFFBFDRBBDUUBDL" },
        { "B2 R U' F2 B2 U F L' D B2 U' R2 L2 U' L2 B2 R2 F2 U' B' Fw2 D2 L Rw2 Fw2 L U' F2 U' L2 Uw2 B Rw2 Fw L D Fw2 R2 Fw Uw' L' Rw2 Fw Rw'",
          "UDLDBDLRDDFRDDLBFURBUFLLUBDRBFBURFUDFFRLFRRLBBFFLBBFBUFFDULRRFBRLBFLUURRUUBLFDRLRDLDDBLURDBUUDLU" },
        { "R' D L2 U' L2 F2 R2 D F2 U' F2 D' F L B U' B2 R B L' B Rw2 Uw2 Fw2 D F2 R' D' R2 Uw2 L' U2 Fw' Rw2 Fw' U2 F' U' R' F Rw D L2 U Rw",
          "DDDBDRURBFRLDRLFRRURFRRDBULDBUBLFUUULFDLFBBRBBLFLUBDBLUFDDUBDRRFLBFBLFBRDLBLUFUUUUDLRDDFFFLURFLR" }
    };
    const int num_scrambles = sizeof(scrambles) / sizeof(*scrambles);

    for (int i = 0; i < num_scrambles; i++) {
        if (!revenge_test_alg(scrambles[i][0], scrambles[i][1])) {
            return false;
        }
    }

    return true;
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

    if (!test_4x4()) {
        all_tests_succeeded = false;
    }

    if (all_tests_succeeded) {
        printf("All tests succeeded!\n");
    }
}

void kociemba_test() {
    g0_table_t* g0_table = g0_init_table(MAIN_ALLOCATOR);
    g1_table_t* g1_table = g1_init_table(MAIN_ALLOCATOR);

    int num_scrambles = 1000;

    distribution_t solve_time_distribution = { .allocator = MAIN_ALLOCATOR };
    distribution_t movecount_distribution = { .allocator = MAIN_ALLOCATOR };

    uint32_t total_moves = 0;
    double   total_time  = 0.0;
    for (int i = 0; i < num_scrambles; i++) {
        g0_state_t g0_state = G0_STATE_SOLVED;
        g1_state_t g1_state = G1_STATE_SOLVED;

        move_list_t scramble = generate_random_move_scramble(30, TEMP_ALLOCATOR);
        for (size_t i = 0; i < scramble.size; i++) {
            g0_execute_move(&g0_state, scramble.data[i]);
            g1_execute_move(&g1_state, scramble.data[i]);
        }

        timer_t solve_timer;
        timer_start(&solve_timer);
        move_list_t move_list = solve_g0_g1(g0_table, g1_table, g0_state, g1_state, TEMP_ALLOCATOR);
        timer_stop(&solve_timer);

        distribution_add(&solve_time_distribution, (int)timer_duration_ms(solve_timer));
        total_time += timer_duration_ms(solve_timer);

        printf("Solved %i scrambles\r", i + 1);
        fflush(stdout);

        total_moves += move_list.size;
        distribution_add(&movecount_distribution, (int)move_list.size);

        for (size_t i = 0; i < move_list.size; i++) {
            g0_execute_move(&g0_state, move_list.data[i]);
            g1_execute_move(&g1_state, move_list.data[i]);
        }

        assert(g0_is_solved(g0_state) && g1_is_solved(g1_state));

        temp_allocator_free_all();
    }

    double average_time      = total_time / num_scrambles;
    double average_movecount = (double)total_moves / num_scrambles;

    printf("Average time per solve:  %fms\n", average_time);
    printf("Average moves per solve: %f HTM\n", average_movecount);

    printf("\nMovecount Distribution (HTM):\n");
    distribution_print(&movecount_distribution, 50);

    printf("\nSolve time distribution (ms):\n");
    distribution_print(&solve_time_distribution, 50);

    array_free(solve_time_distribution);
    array_free(movecount_distribution);

    free_alloc(g0_table, MAIN_ALLOCATOR);
    free_alloc(g1_table, MAIN_ALLOCATOR);
}

void run_perf_tests() {
    const char* superflip = "U R2 F B R B2 R U2 L B2 R U' D' R2 F R' L B2 U2 F2";

    g0_state_t g0_state = G0_STATE_SOLVED;
    g1_state_t g1_state = G1_STATE_SOLVED;
    g0_g1_execute_alg_string(&g0_state, &g1_state, superflip);

    timer_t timer;

    g0_table_t* g0_table = NULL;
    double g0_table_init_total_time = 0.0;
    int g0_table_init_iterations = 10;
    for (int i = 0; i < g0_table_init_iterations; i++) {
        timer_start(&timer);
        free_alloc(g0_table, MAIN_ALLOCATOR);
        g0_table = g0_init_table(MAIN_ALLOCATOR);
        timer_stop(&timer);
        printf("G0 table initialization: %fs\n", timer_duration(timer));
        g0_table_init_total_time += timer_duration(timer);
    }

    double g0_solve_total_time = 0.0;
    int g0_solve_iterations = 10;
    for (int i = 0; i < g0_solve_iterations; i++) {
        timer_start(&timer);
        solution_list_t solutions = solve_g0(g0_table, g0_state, TEMP_ALLOCATOR);
        timer_stop(&timer);
        printf("G0 solve: %fs\n", timer_duration(timer));
        g0_solve_total_time += timer_duration(timer);

        // Apply G0 solution to G1 state (only once)
        if (i == 0) {
            assert(solutions.size > 0);
            move_list_t solution = solutions.data[0];
            for (size_t j = 0; j < solution.size; j++) {
                g1_execute_move(&g1_state, solution.data[j]);
            }
        }
    }

    g1_table_t* g1_table = NULL;
    double g1_table_init_total_time = 0.0;
    int g1_table_init_iterations = 10;
    for (int i = 0; i < g1_table_init_iterations; i++) {
        timer_start(&timer);
        free_alloc(g1_table, MAIN_ALLOCATOR);
        g1_table = g1_init_table(MAIN_ALLOCATOR);
        timer_stop(&timer);
        printf("G1 table initialization: %fs\n", timer_duration(timer));
        g1_table_init_total_time += timer_duration(timer);
    }

    double g1_solve_total_time = 0.0;
    int g1_solve_iterations = 10;
    for (int i = 0; i < g1_solve_iterations; i++) {
        timer_start(&timer);
        solve_g1(g1_table, g1_state, TEMP_ALLOCATOR);
        timer_stop(&timer);
        printf("G1 solve: %fs\n", timer_duration(timer));
        g1_solve_total_time += timer_duration(timer);
    }

    printf("--- PERFORMANCE ---\n");
    printf("G0 Table Init: %fs (average)\n", g0_table_init_total_time / g0_table_init_iterations);
    printf("G0 Solve:      %fs (average)\n", g0_solve_total_time / g0_solve_iterations);
    printf("G1 Table Init: %fs (average)\n", g1_table_init_total_time / g1_table_init_iterations);
    printf("G1 Solve:      %fs (average)\n", g1_solve_total_time / g1_solve_iterations);
}
