//
//  prafrentex.h
//  PrafrentexVM
//
//  Created by Café Artes Visuais on 27/03/13.
//  Copyright (c) 2013 Café Artes Visuais. All rights reserved.
//

#ifndef PrafrentexVM_prafrentex_h
#define PrafrentexVM_prafrentex_h

#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "uthash.h"

/*
 * Prafrentex Virtual Machine is an experimental VM (and language?)
 * with likeness to Forth, but using doubles as numeric representation
 * and NaN-boxing (nun-boxing to be more correct, as we privilege
 * doubles over pointers).
 * It is also a direct-threaded VM, where each word is an address to either
 * the code
 */


typedef union {
    double d;
    uint64_t u64;
    int64_t i64;
    struct t_u32_overlay {
        uint32_t _0;
        uint32_t _1;
    } u32;
    struct t_i32_overlay {
        int32_t _0;
        int32_t _1;
    } i32;
} t_double_atom;

// top bits masked off of a non-signaling NaN
#define PF_PTR_MASK 0x7ff8000000000000ul
#define PF_INT_MASK PF_PTR_MASK + 0x10000000001ul

#define PF_MAKE_INT_DOUBLE(i) ((t_double_atom){.u64 = PF_INT_MASK + ((unsigned)i << 1)})
#define PF_MAKE_PTR_DOUBLE(p) ((t_double_atom){.u64 = PF_PTR_MASK + (p << 1)})
#define PF_GET_DOUBLE_PTR(d) (t_atom*)((d.u64 - PF_PTR_MASK)>>1)
#define PF_GET_DOUBLE_INT(d) (int)((d.u64 - PF_INT_MASK)>>1)
#define PF_GET_DOUBLE_UINT(d) (unsigned)((d.u64 - PF_INT_MASK)>>1)

// quiet NaN negative mask
#define QNAN_NEGMASK 0x0008000000000000ul

enum AtomTypes {
    A_NULL = 0,
    A_WORD = 1,
    A_USER_WORD,
    A_STRING,
    A_BOXED_INT,
    A_BOXED_DOUBLE,
    
    A_USER
};

typedef union _atom {
    uint32_t u32f;
    struct _flags {
        unsigned gc_flags:2;
        unsigned extra:10;
        unsigned type:20;
    } flags;
} t_atom;

extern t_atom *null_atom;

typedef struct _context t_context;

typedef struct _stack {
    t_double_atom *stack;
    t_double_atom *stack_begin;
    t_double_atom *stack_end;
} t_stack;

typedef void(*t_word_native_code_ptr)(t_context *ctx, t_double_atom *code_ptr, void *data);
typedef struct _word {
    t_atom atom;
    t_word_native_code_ptr code;
    void *data;
} t_word;

typedef struct _user_word {
    t_atom atom;
    t_double_atom *code;
    t_stack closure;
} t_user_word;

typedef struct {
    t_atom atom;
    bool constant;
    size_t size;
    char *chars;
} t_string;

typedef union _atom_union {
    t_atom atom;
    t_word word;
    t_user_word user_word;
    t_string string;
} t_atom_union;

typedef struct _dictionary_entry {
    char *name;
    t_atom *atom;
    struct _dictionary_entry *next;
    UT_hash_handle hh;
} t_dictionary_entry;

struct _context {
    t_double_atom *stack;
    t_double_atom *stack_begin;
    t_double_atom *stack_end;
    t_double_atom *ret_stack;
    t_double_atom *ret_stack_begin;
    t_double_atom *ret_stack_end;
    
    t_double_atom *execptr;
    
    int32_t *int_stack;
    int32_t *int_stack_begin;
    int32_t *int_stack_end;
    void *heap_base;
    
    t_dictionary_entry *dictionary;
};

t_context *pf_context_create();
void pf_context_destroy(t_context *ctx);
int pf_stack_create(t_stack *stack, size_t stack_size);
int pf_exec(t_context *ctx, t_double_atom *program, t_stack *closure_stack);

t_double_atom pf_find_word(t_context *ctx, const char* name);
void pf_add_word(t_context *ctx, const char *name, t_atom *atom);
void pf_forget_word(t_context *ctx, const char *name);

t_double_atom pf_make_double_for_atom(t_atom *atom);
t_atom *pf_unref_double(t_double_atom d);

t_double_atom pf_get_val(t_context *ctx, int index);

#endif
