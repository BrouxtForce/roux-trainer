#include "common.h"
#include <stdio.h>
#include <string.h>

// TODO: Revisit and tune this to the actual temporary memory usage of the program
#define MIN_ALLOCATION_BLOCK_SIZE (1 << 20)

typedef struct {
    void* data;
    size_t size;
    size_t offset;
} allocation_block_t;

typedef struct {
    allocation_block_t* data;
    size_t size;
    size_t capacity;
    allocator_e allocator;
} allocation_block_list_t;

static allocation_block_list_t temp_allocation_block_list = { .allocator = INTERNAL_ALLOCATOR };

static allocation_block_t* temp_alloc_append_block(size_t size) {
    array_append(temp_allocation_block_list, (allocation_block_t){});

    allocation_block_t* block = &temp_allocation_block_list.data[temp_allocation_block_list.size - 1];

    block->size   = max_u64(size, MIN_ALLOCATION_BLOCK_SIZE);
    block->data   = malloc(block->size);
    block->offset = 0;

    return block;
}

static void* temp_alloc_from_block(allocation_block_t* allocation_block, size_t size) {
    if (size > allocation_block->size - allocation_block->offset) {
        return NULL;
    }

    void* out = (uint8_t*)allocation_block->data + allocation_block->offset;
    allocation_block->offset += size;
    return out;
}

static void* temp_alloc(size_t size) {
    for (size_t i = 0; i < temp_allocation_block_list.size; i++) {
        allocation_block_t* block = &temp_allocation_block_list.data[i];
        void* ptr = temp_alloc_from_block(block, size);
        if (ptr != NULL) {
            return ptr;
        }
    }

    allocation_block_t* block = temp_alloc_append_block(size);
    void* ptr = temp_alloc_from_block(block, size);
    assert(ptr != NULL);
    return ptr;
}

static bool is_pointer_in_block(void* ptr, allocation_block_t* block) {
    return ptr >= block->data && (uint8_t*)ptr < (uint8_t*)block->data + block->size;
}

static void* temp_realloc(size_t old_size, void* data, size_t size) {
    if (old_size == 0 || data == NULL) {
        assert(old_size == 0 && data == NULL);
        return temp_alloc(size);
    }

    // Attempt to reallocate in place
    for (size_t i = 0; i < temp_allocation_block_list.size; i++) {
        allocation_block_t* block = &temp_allocation_block_list.data[i];
        if (!is_pointer_in_block(data, block)) {
            continue;
        }
        if ((uint8_t*)data + old_size != (uint8_t*)block->data + block->offset) {
            break;
        }
        assert(block->offset >= old_size);
        block->offset -= old_size;

        void* ptr = temp_alloc_from_block(block, size);
        if (ptr != NULL) {
            return ptr;
        }
    }

    // The allocation needs to be relocated
    void* new_ptr = temp_alloc(size);
    memcpy(new_ptr, data, old_size);
    return new_ptr;
}

typedef struct {
    void* ptr;
    size_t size;
    source_location_t caller_location;
} allocation_record_t;

typedef struct {
    allocation_record_t* data;
    size_t size;
    size_t capacity;
    allocator_e allocator;
} allocation_record_list_t;

static allocation_record_list_t allocation_records = { .allocator = INTERNAL_ALLOCATOR };

static void record_allocation(void* ptr, size_t size, source_location_t caller_location) {
    assert(ptr != NULL);
    allocation_record_t allocation_record = {
        .ptr = ptr,
        .size = size,
        .caller_location = caller_location
    };
    array_append(allocation_records, allocation_record);
}

static void record_reallocation([[maybe_unused]] size_t old_size, void* old_ptr, size_t new_size, void* new_ptr, source_location_t caller_location) {
    if (old_ptr == NULL) {
        record_allocation(new_ptr, new_size, caller_location);
        return;
    }

    for (size_t i = 0; i < allocation_records.size; i++) {
        allocation_record_t* record = &allocation_records.data[i];
        if (record->ptr != old_ptr) {
            continue;
        }

        assert(record->size == old_size && "Reallocation size does not match");

        record->ptr = new_ptr;
        record->size = new_size;
        record->caller_location = caller_location;
        return;
    }

    assert(false && "Could not find original allocation");
}

static void record_deallocation(void* ptr) {
    if (ptr == NULL) {
        return;
    }

    assert(allocation_records.size > 0);
    for (size_t i = 0; i < allocation_records.size; i++) {
        if (ptr != allocation_records.data[i].ptr) {
            continue;
        }

        size_t last_index = allocation_records.size - 1;

        allocation_record_t swap = allocation_records.data[i];
        allocation_records.data[i] = allocation_records.data[last_index];
        allocation_records.data[last_index] = swap;

        allocation_records.size--;
        return;
    }

    assert(false && "ptr was already freed");
}

static void print_leaked_allocations() {
    if (allocation_records.size == 0) {
        return;
    }

    printf("--- LEAKED ALLOCATIONS ---\n");
    for (size_t i = 0; i < allocation_records.size; i++) {
        allocation_record_t record = allocation_records.data[i];
        source_location_t location = record.caller_location;
        printf("%zu bytes at %s:%i\n", record.size, location.file, location.line);
    }
}

static void register_atexits() {
    static bool has_registered = false;
    if (!has_registered) {
        has_registered = true;
        atexit(print_leaked_allocations);
    }
}

void* alloc(size_t size, allocator_e allocator, source_location_t caller_location) {
    register_atexits();
    switch (allocator) {
        case MAIN_ALLOCATOR: case INTERNAL_ALLOCATOR: {
            void* ptr = malloc(size);
            if (allocator != INTERNAL_ALLOCATOR) {
                record_allocation(ptr, size, caller_location);
            }
            return ptr;
        }
        case TEMP_ALLOCATOR: return temp_alloc(size);
        default: assert(false && "Invalid allocator");
    }
    return NULL;
}

void* resize_alloc(size_t old_size, void* data, size_t size, allocator_e allocator, source_location_t caller_location) {
    register_atexits();
    switch (allocator) {
        case MAIN_ALLOCATOR: case INTERNAL_ALLOCATOR: {
            void* ptr = realloc(data, size);
            if (allocator != INTERNAL_ALLOCATOR) {
                record_reallocation(old_size, data, size, ptr, caller_location);
            }
            return ptr;
        }
        case TEMP_ALLOCATOR: return temp_realloc(old_size, data, size);
        default: assert(false && "Invalid allocator");
    }
    return NULL;
}

void free_alloc(void* ptr, allocator_e allocator) {
    if (allocator == TEMP_ALLOCATOR) {
        return;
    }
    assert(allocator == MAIN_ALLOCATOR || allocator == INTERNAL_ALLOCATOR);
    if (allocator != INTERNAL_ALLOCATOR) {
        record_deallocation(ptr);
    }
    free(ptr);
}

void temp_allocator_free_all() {
    for (size_t i = 0; i < temp_allocation_block_list.size; i++) {
        temp_allocation_block_list.data[i].offset = 0;
    }
}

// TODO: Better random number generation
uint64_t random_u64() {
    uint64_t a = (uint64_t)rand(), b = (uint64_t)rand(), c = (uint64_t)rand();
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

const char* g_move_to_string(move_t move, allocator_e allocator) {
    const char* base_move = move_to_string(move.move);
    if (move.width == 1) {
        return base_move;
    }
    if (move.width == 2) {
        size_t length = strlen(base_move);
        char* wide_move = alloc(length + 2, allocator, SOURCE_LOCATION);

        wide_move[0] = base_move[0];
        wide_move[1] = 'w';
        memcpy(&wide_move[2], &base_move[1], length - 1);
        wide_move[length + 1] = '\0';

        return wide_move;
    }

    assert(false);
    return "[INVALID MOVE]";
}

move_composition_t decompose_move(move_e move) {
    return (move_composition_t){
        .face_index = move / 3,
        .count_cw   = (move % 3) + 1
    };
}

move_list_t generate_random_move_scramble(int length, allocator_e allocator) {
    move_list_t scramble = { .allocator = allocator };

    // TODO: array_reserve?
    move_e prev_base_move = MOVE_NULL;
    for (int i = 0; i < length; i++) {
        // TODO: Better random number generation
        while (true) {
            move_e random_base_move = (move_e)(3 * (rand() % 6));

            if (random_base_move == prev_base_move) continue;
            if (prev_base_move == MOVE_D && random_base_move == MOVE_U) continue;
            if (prev_base_move == MOVE_B && random_base_move == MOVE_F) continue;
            if (prev_base_move == MOVE_L && random_base_move == MOVE_R) continue;

            move_e random_move = (move_e)(random_base_move + rand() % 3);
            array_append(scramble, random_move);

            prev_base_move = random_base_move;
            break;
        }
    }

    return scramble;
}

g_move_list_t generate_random_move_scramble_4(int length, allocator_e allocator) {
    g_move_list_t scramble = { .allocator = allocator };

    move_e prev_base_move = MOVE_NULL;
    bool prev_was_wide = false;

    for (int i = 0; i < length; i++) {
        while (true) {
            move_e base_move;
            bool   is_wide = rand() % 2 == 0;
            if (is_wide) {
                base_move = (move_e)(6 * (rand() % 3));
            } else {
                base_move = (move_e)(3 * (rand() % 6));
            }

            if (base_move == prev_base_move && (prev_was_wide == is_wide || !prev_was_wide)) continue;
            if (prev_base_move == MOVE_D && base_move == MOVE_U) continue;
            if (prev_base_move == MOVE_B && base_move == MOVE_F) continue;
            if (prev_base_move == MOVE_L && base_move == MOVE_R) continue;

            move_t move = { .move = (move_e)(base_move + rand() % 3), .width = is_wide ? 2 : 1 };
            array_append(scramble, move);

            prev_base_move = base_move;
            prev_was_wide  = is_wide;
            break;
        }
    }

    return scramble;
}

void distribution_add(distribution_t* distribution, int value) {
    for (size_t i = 0; i < distribution->size; i++) {
        if (distribution->data[i].value == value) {
            distribution->data[i].count++;
            return;
        }
    }

    distribution_node_t node = {
        .value = value,
        .count = 1
    };
    array_append(*distribution, node);
}

int distribution_node_compare(const void* left, const void* right) {
    const distribution_node_t* a = left;
    const distribution_node_t* b = right;

    return a->value - b->value;
}

void distribution_print(distribution_t* distribution, int bar_length) {
    if (distribution->size == 0) return;

    char bar_buffer[bar_length] = {};
    memset(bar_buffer, '#', (size_t)bar_length);

    qsort(distribution->data, distribution->size, sizeof(*distribution->data), distribution_node_compare);

    int total_count = 0;
    for (size_t i = 0; i < distribution->size; i++) {
        total_count += distribution->data[i].count;
    }

    for (size_t i = 0; i < distribution->size; i++) {
        distribution_node_t node = distribution->data[i];

        int bar_size = node.count * bar_length / total_count;

        // TODO: Make left number width dynamic depending on the input distribution
        printf("%4i: %.*s %i\n", node.value, bar_size, bar_buffer, node.count);
    }
}
