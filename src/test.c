#include <stdio.h>
#include <string.h>
#include <assert.h>

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

bool g0_g1_test_alg(const char* alg_string, const char* expected_state) {
    g0_state_t g0_state = G0_STATE_SOLVED;
    g1_state_t g1_state = G1_STATE_SOLVED;

    while (true) {
        move_e move = consume_next_move(&alg_string);
        if (move == MOVE_NULL) {
            break;
        }

        g0_execute_move(&g0_state, move);
        g1_execute_move(&g1_state, move);
    }

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
