#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdlib.h>

#include "common.h"
#include "visual_cube.h"
#include "lse.h"
#include "kociemba.h"
#include "revenge.h"

#define ESC_ERASE_ENTIRE_SCREEN "\x1b[H\x1b[0J"
#define ESC_SAVE_SCREEN "\x1b[?47h"
#define ESC_RESTORE_SCREEN "\x1b[?47l"

static struct termios initial_termios_state;
static bool is_initial_termios_state_initialized = false;
void enter_raw_mode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    if (!is_initial_termios_state_initialized) {
        initial_termios_state = raw;
        is_initial_termios_state_initialized = true;
    }

    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void restore_initial_mode() {
    if (is_initial_termios_state_initialized) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &initial_termios_state);
    }
}

// NOTE: This function will replace the first whitespace in the string with a null terminator
void clear_trailing_whitespace(char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (isspace(str[i])) {
            str[i] = '\0';
            return;
        }
    }
}

typedef struct {
    lse_state_t* data;
    size_t size;
    size_t capacity;
    allocator_e allocator;
} lse_state_list_t;

static lse_state_list_t prev_lse_states = { .allocator = MAIN_ALLOCATOR };

void execute_command_scramble(lse_state_t* lse_state, lse_move_list_t* lse_move_list, lse_solution_list_t* lse_solution_list) {
    *lse_state = generate_random_lse_state();
    array_append(prev_lse_states, *lse_state);

    lse_move_list->size = 0;
    free_lse_solution_list(lse_solution_list);
}

void execute_command_curr_scramble(lse_state_t* lse_state, lse_move_list_t* lse_move_list)
{
    if (prev_lse_states.size >= 1) {
        *lse_state = prev_lse_states.data[prev_lse_states.size - 1];
    } else {
        *lse_state = SOLVED_LSE_STATE;
    }

    lse_move_list->size = 0;
}

void execute_command_prev_scramble(lse_state_t* lse_state, lse_move_list_t* lse_move_list, lse_solution_list_t* lse_solution_list) {
    if (prev_lse_states.size >= 2) {
        *lse_state = prev_lse_states.data[prev_lse_states.size - 2];
        prev_lse_states.size--;
    } else {
        prev_lse_states.size = 0;
        *lse_state = SOLVED_LSE_STATE;
    }

    lse_move_list->size = 0;
    free_lse_solution_list(lse_solution_list);
}

void read_and_execute_command(lse_state_t* lse_state, lse_move_list_t* lse_move_list, lse_solution_list_t* lse_solution_list) {
    restore_initial_mode();

    char command[16] = {};
    if (fgets(command, sizeof command, stdin) != NULL) {
        clear_trailing_whitespace(command);

        if (strcmp(command, "scramble") == 0) {
            execute_command_scramble(lse_state, lse_move_list, lse_solution_list);
        }
        if (strcmp(command, "prev") == 0) {
            execute_command_prev_scramble(lse_state, lse_move_list, lse_solution_list);
        }
        if (strcmp(command, "curr") == 0) {
            execute_command_curr_scramble(lse_state, lse_move_list);
        }
        if (strcmp(command, "eolr") == 0) {
            free_lse_solution_list(lse_solution_list);
            *lse_solution_list = solve_eolr(*lse_state, MAIN_ALLOCATOR);
        }
        if (strcmp(command, "lse") == 0) {
            free_lse_solution_list(lse_solution_list);
            *lse_solution_list = solve_lse(*lse_state, MAIN_ALLOCATOR);
        }
    }

    enter_raw_mode();
}

void draw_lse_state(lse_state_t lse_state) {
    visual_cube_state_t visual_cube_state;
    visual_cube_state_reset(&visual_cube_state);

    lse_state_write_visual_cube_state(lse_state, &visual_cube_state);

    draw_visual_cube_state(&visual_cube_state);
}

void draw_g0_g1_cube_state(g0_state_t g0_state, g1_state_t g1_state) {
    visual_cube_state_t visual_cube_state;
    visual_cube_state_reset(&visual_cube_state);

    g0_g1_state_write_visual_cube_state(&visual_cube_state, g0_state, g1_state);

    draw_visual_cube_state(&visual_cube_state);
}

// TODO: Put this in a header file
void run_tests();
void kociemba_test();
void run_perf_tests();

void run_kociemba() {
    g0_state_t g0_state = G0_STATE_SOLVED;
    g1_state_t g1_state = G1_STATE_SOLVED;

    move_list_t scramble = generate_random_move_scramble(20, TEMP_ALLOCATOR);
    for (size_t i = 0; i < scramble.size; i++) {
        g0_execute_move(&g0_state, scramble.data[i]);
        g1_execute_move(&g1_state, scramble.data[i]);
    }

    g0_table_t* g0_table = g0_init_table(MAIN_ALLOCATOR);
    g1_table_t* g1_table = g1_init_table(MAIN_ALLOCATOR);

    printf("Scramble: ");
    for (size_t i = 0; i < scramble.size; i++) {
        printf("%s ", move_to_string(scramble.data[i]));
    }
    printf("\n");

    draw_g0_g1_cube_state(g0_state, g1_state);

    move_list_t solution = solve_g0_g1(g0_table, g1_table, g0_state, g1_state, TEMP_ALLOCATOR);
    printf("Solution (%zu HTM): ", solution.size);
    for (size_t i = 0; i < solution.size; i++) {
        printf("%s ", move_to_string(solution.data[i]));
    }
    printf("\n");

    free_alloc(g0_table, MAIN_ALLOCATOR);
    free_alloc(g1_table, MAIN_ALLOCATOR);
}

int main(int argc, char** argv) {
    srand((unsigned)time(NULL));

    // TODO: Proper command line argument parsing
    if (argc >= 2 && strcmp(argv[1], "test") == 0) {
        run_tests();
        return 0;
    }
    if (argc >= 2 && strcmp(argv[1], "kociemba_test") == 0) {
        kociemba_test();
        return 0;
    }
    if (argc >= 2 && strcmp(argv[1], "perf") == 0) {
        run_perf_tests();
        return 0;
    }
    if (argc >= 2 && strcmp(argv[1], "kociemba") == 0) {
        run_kociemba();
        return 0;
    }

    lse_state_t lse_state = SOLVED_LSE_STATE;
    lse_move_list_t lse_move_list = { .allocator = MAIN_ALLOCATOR };
    lse_solution_list_t lse_solution_list = { .allocator = MAIN_ALLOCATOR };

    printf(ESC_SAVE_SCREEN);

    enter_raw_mode();
    while (true) {
        printf(ESC_ERASE_ENTIRE_SCREEN);

        draw_lse_state(lse_state);
        for (size_t i = 0; i < lse_move_list.size; i++) {
            if (i != 0) {
                printf(" ");
            }
            switch (lse_move_list.data[i]) {
                case LSE_MOVE_U:       printf("U");  break;
                case LSE_MOVE_U_PRIME: printf("U'"); break;
                case LSE_MOVE_U2:      printf("U2"); break;
                case LSE_MOVE_M:       printf("M");  break;
                case LSE_MOVE_M_PRIME: printf("M'"); break;
                case LSE_MOVE_M2:      printf("M2"); break;
            }
        }
        printf("\n(%zu STM)\n", lse_move_list.size);

        printf("Solutions:\n");
        for (size_t i = 0; i < lse_solution_list.size; i++) {
            lse_move_list_t solution = lse_solution_list.data[i];
            for (size_t j = 0; j < solution.size; j++) {
                printf("%s ", lse_move_to_string(solution.data[j]));
            }
            printf("(%zu STM)\n", solution.size);
        }

        handle_input:
        temp_allocator_free_all();
        switch (getchar()) {
            case 'm':
                lse_state = lse_move_m(lse_state);
                lse_move_list_simplify_append(&lse_move_list, LSE_MOVE_M);
                continue;
            case 'k':
                lse_state = lse_move_m_prime(lse_state);
                lse_move_list_simplify_append(&lse_move_list, LSE_MOVE_M_PRIME);
                continue;
            case 's':
                lse_state = lse_move_u(lse_state);
                lse_move_list_simplify_append(&lse_move_list, LSE_MOVE_U);
                continue;
            case 'd':
                lse_state = lse_move_u_prime(lse_state);
                lse_move_list_simplify_append(&lse_move_list, LSE_MOVE_U_PRIME);
                continue;
            case '/':
                printf("/");
                read_and_execute_command(&lse_state, &lse_move_list, &lse_solution_list);
                continue;
            case '\n':
                execute_command_scramble(&lse_state, &lse_move_list, &lse_solution_list);
                continue;
            case ' ':
                execute_command_curr_scramble(&lse_state, &lse_move_list);
                continue;
            case 'p':
                execute_command_prev_scramble(&lse_state, &lse_move_list, &lse_solution_list);
                continue;
            case 'q': break;
            default: goto handle_input;
        }

        printf(ESC_ERASE_ENTIRE_SCREEN);
        break;
    }
    restore_initial_mode();

    printf(ESC_RESTORE_SCREEN);

    return 0;
}
