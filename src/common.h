#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

#define array_append(array, element) \
    do { \
        if ((array).size >= (array).capacity) { \
            if ((array).capacity == 0) { \
                (array).capacity = 8; \
            } else { \
                (array).capacity *= 2; \
            } \
            (array).data = realloc((array).data, (array).capacity * sizeof *(array).data); \
        } \
        (array).data[(array).size++] = (element); \
    } while (false)

#define array_pop(array) \
    do { \
        assert((array).size > 0); \
        (array).size--; \
    } while (false)

#define array_copy(src_array, copy_array) \
    do { \
        (copy_array).data = malloc((src_array).size); \
        (copy_array).size = (src_array).size; \
        (copy_array).capacity = (src_array).capacity; \
        memcpy((copy_array).data, (src_array).data, (src_array).size * sizeof *(copy_array).data); \
    } while (false)

#define array_free(array) \
    do { \
        free((array).data); \
        (array).data = NULL; \
        (array).size = 0; \
        (array).capacity = 0; \
    } while (false)

static inline int byte_popcount(uint8_t value) {
    return __builtin_popcountg(value);
}

static inline int byte_ctz(uint8_t value) {
    return __builtin_ctzg(value);
}

static inline bool byte_has_single_bit(uint8_t value) {
    return byte_popcount(value) == 1;
}

static inline uint8_t byte_rotate_left(uint8_t value, uint8_t amount) {
    return __builtin_rotateleft8(value, amount);
}

static inline uint8_t byte_rotate_right(uint8_t value, uint8_t amount) {
    return __builtin_rotateright8(value, amount);
}

static inline uint16_t rotate_left_u16(uint16_t value, uint16_t amount) {
    return __builtin_rotateleft16(value, amount);
}

static inline uint16_t rotate_right_u16(uint16_t value, uint16_t amount) {
    return __builtin_rotateright16(value, amount);
}

uint64_t random_u64();

typedef enum : uint8_t {
    FACE_INDEX_U,
    FACE_INDEX_D,
    FACE_INDEX_F,
    FACE_INDEX_B,
    FACE_INDEX_R,
    FACE_INDEX_L
} face_index_e;

typedef enum : uint8_t {
    MOVE_U, MOVE_U2, MOVE_U_PRIME,
    MOVE_D, MOVE_D2, MOVE_D_PRIME,
    MOVE_F, MOVE_F2, MOVE_F_PRIME,
    MOVE_B, MOVE_B2, MOVE_B_PRIME,
    MOVE_R, MOVE_R2, MOVE_R_PRIME,
    MOVE_L, MOVE_L2, MOVE_L_PRIME,
    MOVE_NULL
} move_e;

const char* move_to_string(move_e move);

typedef struct {
    move_e* data;
    size_t size;
    size_t capacity;
} move_list_t;

move_list_t generate_random_move_scramble(int length);

typedef struct {
    move_list_t* data;
    size_t size;
    size_t capacity;
} solution_list_t;
