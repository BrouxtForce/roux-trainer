// This header provides a template for a hash table implementation. I originally wrote a
// more general version of this, using void* and size_t to allow for any types and a fixed
// set of functions that can be used for any hash table. However, that implementation caused
// a significant decrease in performance (about 4x), so instead I went for the C version of
// templates: macros. It's not as clean looking as the void* implementation, but it is much
// faster, which is paramount.
//
// The use of this header file is very simple. #define the required macros, then include this
// header file. This header file can be included multiple times in a single source file (hence
// no #pragma once), and the macros are cleaned up (#undef'ed) at the end of the source file to
// avoid pollution.

#include <stdint.h>
#include <assert.h>
#include <stddef.h>

#if !defined(HASH_TABLE_NAME) || !defined(STATE_TYPE) || !defined(STATE_EQUALS) || \
    !defined(STATE_TO_U64)    || !defined(STATE_NULL) || !defined(HASH_TABLE_SIZE)
    #define HASH_TABLE_NAME unnamed
    #define STATE_TYPE uint64_t
    #define STATE_EQUALS(a, b) (a == b)
    #define STATE_TO_U64(state) (state)
    #define STATE_NULL ((uint64_t)0)
    #define HASH_TABLE_SIZE 1
    #error "You must provide the parameters listed above to the hash table implementation"
#endif

#define CONCAT2(a, b) a##b
#define CONCAT(a, b) CONCAT2(a, b)

#define HASH_TABLE_NODE_TYPE CONCAT(HASH_TABLE_NAME, _node_t)
#define HASH_TABLE_TYPE CONCAT(HASH_TABLE_NAME, _t)

#define HASH_TABLE_NAME_ CONCAT(HASH_TABLE_NAME, _)
#define HASH_TABLE_FUNCTION(RETURN_TYPE, NAME, ...) static RETURN_TYPE CONCAT(HASH_TABLE_NAME_, NAME)(__VA_ARGS__)
#define HASH_TABLE_CALL(NAME, ...) CONCAT(HASH_TABLE_NAME_, NAME)(__VA_ARGS__)

typedef struct HASH_TABLE_NODE_TYPE {
    STATE_TYPE state;
    int next_index;
    int distance_from_solved;
} HASH_TABLE_NODE_TYPE;

typedef struct HASH_TABLE_TYPE {
    HASH_TABLE_NODE_TYPE entries[HASH_TABLE_SIZE];
    uint32_t count;
    uint32_t next_free;
    uint64_t magic;
} HASH_TABLE_TYPE;

HASH_TABLE_FUNCTION(int, get_next_free, HASH_TABLE_TYPE* table) {
    while (table->next_free < HASH_TABLE_SIZE) {
        if (STATE_EQUALS(table->entries[table->next_free].state, STATE_NULL)) {
            return table->next_free++;
        }
        table->next_free++;
    }
    unreachable();
}

HASH_TABLE_FUNCTION(int, get_index, STATE_TYPE state, uint64_t magic) {
    uint64_t num = STATE_TO_U64(state);
    // TODO: What should this be shifted by, and should it very for different tables?
    return ((num * magic) >> 32) % HASH_TABLE_SIZE;
}

HASH_TABLE_FUNCTION(bool, insert, HASH_TABLE_TYPE* table, STATE_TYPE state, int distance_from_solved) {
    int index = HASH_TABLE_CALL(get_index, state, table->magic);

    HASH_TABLE_NODE_TYPE* entry = &table->entries[index];

    // Case 1: The entry is empty
    if (STATE_EQUALS(entry->state, STATE_NULL)) {
        entry->state = state;
        entry->next_index = -1;
        entry->distance_from_solved = distance_from_solved;

        table->count++;
        return true;
    }

    // Case 2: g0_state collides with an entry already present in the table
    int entry_index = HASH_TABLE_CALL(get_index, entry->state, table->magic);
    if (entry_index == index) {
        while (true) {
            if (STATE_EQUALS(entry->state, state)) {
                if (entry->distance_from_solved > distance_from_solved) {
                    entry->distance_from_solved = distance_from_solved;
                    return true;
                }
                return false;
            }
            if (entry->next_index == -1) {
                break;
            }
            entry = &table->entries[entry->next_index];
        }

        int free_entry_index = HASH_TABLE_CALL(get_next_free, table);
        entry->next_index = free_entry_index;

        HASH_TABLE_NODE_TYPE* free_entry = &table->entries[free_entry_index];
        free_entry->state = state;
        free_entry->next_index = -1;
        free_entry->distance_from_solved = distance_from_solved;

        table->count++;
        return true;
    }

    // Case 3: The entry is filled with a linked list node that collided with another entry
    HASH_TABLE_NODE_TYPE* prev_entry = NULL;
    HASH_TABLE_NODE_TYPE* other_entry = &table->entries[entry_index];
    while (true) {
        if (STATE_EQUALS(other_entry->state, entry->state)) {
            break;
        }
        if (other_entry->next_index == -1) {
            assert(false);
        }
        prev_entry = other_entry;
        other_entry = &table->entries[other_entry->next_index];
    }

    if (prev_entry != NULL) {
        prev_entry->next_index = other_entry->next_index;
    }

    STATE_TYPE missing_state = other_entry->state;
    int missing_distance_from_solved = other_entry->distance_from_solved;

    other_entry->state = state;
    other_entry->next_index = -1;
    other_entry->distance_from_solved = distance_from_solved;

    [[maybe_unused]] bool result = HASH_TABLE_CALL(insert, table, missing_state, missing_distance_from_solved);
    assert(result);

    return true;
}

HASH_TABLE_FUNCTION(int, lookup, const HASH_TABLE_TYPE* table, STATE_TYPE state) {
    int index = HASH_TABLE_CALL(get_index, state, table->magic);

    const HASH_TABLE_NODE_TYPE* entry = &table->entries[index];
    while (true) {
        if (STATE_EQUALS(entry->state, state)) {
            return entry->distance_from_solved;
        }
        if (entry->next_index == -1) {
            break;
        }
        entry = &table->entries[entry->next_index];
    }

    return -1;
}

#undef HASH_TABLE_NAME
#undef STATE_TYPE
#undef STATE_EQUALS
#undef STATE_TO_U64
#undef STATE_NULL
#undef HASH_TABLE_SIZE

#undef CONCAT2
#undef CONCAT

#undef HASH_TABLE_NODE_TYPE
#undef HASH_TABLE_TYPE

#undef HASH_TABLE_NAME_
#undef HASH_TABLE_FUNCTION
#undef HASH_TABLE_CALL
