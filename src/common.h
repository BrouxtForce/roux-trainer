#pragma once

#include <stdint.h>
#include <stdbool.h>

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

typedef enum : uint8_t {
    FACE_INDEX_U,
    FACE_INDEX_D,
    FACE_INDEX_F,
    FACE_INDEX_B,
    FACE_INDEX_R,
    FACE_INDEX_L
} face_index_e;
