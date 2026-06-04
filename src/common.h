#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

typedef struct {
    const char* file;
    int line;
} source_location_t;

#define SOURCE_LOCATION (source_location_t){ .file = __FILE__, .line = __LINE__ }

#define array_reserve(array, amount) \
    do { \
        if ((amount) > (array).capacity) { \
            size_t old_capacity = (array).capacity; \
            (array).capacity *= 2; \
            if ((array).capacity < 8)      (array).capacity = 8; \
            if ((array).capacity < amount) (array).capacity = (amount); \
            (array).data = resize_alloc( \
                old_capacity * sizeof *(array).data, (array).data, (array).capacity * sizeof *(array).data, \
                (array).allocator, SOURCE_LOCATION); \
        } \
    } while (false)

#define array_append(array, element) \
    do { \
        array_reserve((array), (array).size + 1); \
        (array).data[(array).size++] = (element); \
    } while (false)

#define array_append_array(array, other) \
    do { \
        static_assert(sizeof *(array).data == sizeof *(other).data); \
        array_reserve((array), (array).size + (other).size); \
        memcpy((array).data + (array).size, (other).data, (other.size) * sizeof *(other).data); \
        (array).size += (other).size; \
    } while (false)

#define array_pop(array) \
    do { \
        assert((array).size > 0); \
        (array).size--; \
    } while (false)

#define array_copy(src_array, copy_array) \
    do { \
        static_assert(sizeof *(src_array).data == sizeof *(copy_array).data); \
        size_t byte_length = (src_array).size * sizeof *(src_array).data; \
        (copy_array).data = alloc(byte_length, (copy_array).allocator, SOURCE_LOCATION); \
        (copy_array).size = (src_array).size; \
        (copy_array).capacity = (src_array).capacity; \
        memcpy((copy_array).data, (src_array).data, byte_length); \
    } while (false)

#define array_free(array) \
    do { \
        free_alloc((array).data, (array).allocator); \
        (array).data = NULL; \
        (array).size = 0; \
        (array).capacity = 0; \
    } while (false)

#define ARRAY(TYPE) struct { \
    TYPE* data; \
    size_t size; \
    size_t capacity; \
    allocator_e allocator; \
}

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

static inline int clz_u32(uint32_t value) {
    return __builtin_clz(value);
}

static inline int popcount_u32(uint32_t value) {
    return __builtin_popcount(value);
}

typedef enum {
    INVALID_ALLOCATOR,

    // Standard general-purpose allocator that is just a wrapper for malloc()
    MAIN_ALLOCATOR,

    // Arena allocator for temporary allocations
    TEMP_ALLOCATOR,

    // The allocator used internally by the custom alloc() functions
    INTERNAL_ALLOCATOR
} allocator_e;

void* alloc(size_t size, allocator_e allocator, source_location_t caller_location);
void* resize_alloc(size_t old_size, void* data, size_t size, allocator_e allocator, source_location_t caller_location);

void free_alloc(void* ptr, allocator_e allocator);
void temp_allocator_free_all();

uint64_t random_u64();

inline static int32_t max_i32(int32_t a, int32_t b) {
    return a > b ? a : b;
}

inline static uint64_t max_u64(uint64_t a, uint64_t b) {
    return a > b ? a : b;
}

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
    face_index_e face_index;
    int          count_cw;
} move_composition_t;

move_composition_t decompose_move(move_e move);

typedef struct {
    move_e  move;
    uint8_t width;
} move_t;

const char* g_move_to_string(move_t move, allocator_e allocator);

typedef ARRAY(move_e)        move_list_t;
typedef ARRAY(move_list_t)   solution_list_t;
typedef ARRAY(move_t)        g_move_list_t;
typedef ARRAY(g_move_list_t) g_solution_list_t;

move_list_t generate_random_move_scramble(int length, allocator_e allocator);
g_move_list_t generate_random_move_scramble_4(int length, allocator_e allocator);

typedef struct {
    int value;
    int count;
} distribution_node_t;

typedef ARRAY(distribution_node_t) distribution_t;

// NOTE: This function takes O(n) time for n=distribution.size
// TODO: Improve the time complexity
void distribution_add(distribution_t* distribution, int value);

// NOTE: This sorts the array
void distribution_print(distribution_t* distribution, int bar_length);
