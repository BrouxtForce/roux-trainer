#include "common.h"

// TODO: Better random number generation
uint64_t random_u64() {
    uint64_t a = rand(), b = rand(), c = rand();
    return (a << 42) | (b << 21) | c;
}

const char* move_to_string(move_e move) {
    switch(move) {
        case MOVE_U:       return "U";
        case MOVE_U2:      return "U2";
        case MOVE_U_PRIME: return "U'";

        case MOVE_D:       return "D";
        case MOVE_D2:      return "D2";
        case MOVE_D_PRIME: return "D'";

        case MOVE_F:       return "F";
        case MOVE_F2:      return "F2";
        case MOVE_F_PRIME: return "F'";

        case MOVE_B:       return "B";
        case MOVE_B2:      return "B2";
        case MOVE_B_PRIME: return "B'";

        case MOVE_R:       return "R";
        case MOVE_R2:      return "R2";
        case MOVE_R_PRIME: return "R'";

        case MOVE_L:       return "L";
        case MOVE_L2:      return "L2";
        case MOVE_L_PRIME: return "L'";

        case MOVE_NULL: return "(null)";
    }

    assert(false && "Invalid move");
}

move_list_t generate_random_move_scramble(int length) {
    move_list_t scramble = {};

    // TODO: array_reserve?
    move_e prev_base_move = MOVE_NULL;
    for (int i = 0; i < length; i++) {
        // TODO: Better random number generation
        while (true) {
            move_e random_base_move = 3 * (rand() % 6);

            if (random_base_move == prev_base_move) continue;
            if (prev_base_move == MOVE_D && random_base_move == MOVE_U) continue;
            if (prev_base_move == MOVE_B && random_base_move == MOVE_F) continue;
            if (prev_base_move == MOVE_L && random_base_move == MOVE_R) continue;

            move_e random_move = random_base_move + rand() % 3;
            array_append(scramble, random_move);

            prev_base_move = random_base_move;
            break;
        }
    }

    return scramble;
}
