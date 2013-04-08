//
//  prafrentex.c
//  PrafrentexVM
//
//  Created by Café Artes Visuais on 28/03/13.
//  Copyright (c) 2013 Café Artes Visuais. All rights reserved.
//

#include <stdlib.h>
#include "prafrentex.h"

static size_t init_stack_sz = 4096, init_ret_stack_sz = 256;
static size_t init_int_stack_sz = 4096;

int pf_stack_create(t_stack *stack, size_t stack_size) {
    stack->stack_end = calloc(sizeof(t_double_atom)*stack_size, 0);
    stack->stack_begin = stack->stack = stack->stack_end+stack_size;
    if (stack->stack_end) {
        return 1;
    }
    return 0;
}

t_context* pf_context_create() {
    t_context *ctx = malloc(sizeof(t_context));
    
    ctx->stack_end = calloc(sizeof(t_double_atom)*init_stack_sz, 0);
    ctx->stack_begin = ctx->stack = ctx->stack_end+init_stack_sz;
    
    ctx->ret_stack_end = calloc(sizeof(t_double_atom)*init_ret_stack_sz, 0);
    ctx->ret_stack_begin = ctx->ret_stack = ctx->ret_stack_end+init_ret_stack_sz;
    
    ctx->int_stack = calloc(sizeof(int32_t)*init_int_stack_sz, 0);
    ctx->int_stack_begin = ctx->int_stack = ctx->int_stack_end+init_int_stack_sz;
    return ctx;
}

void pf_context_destroy(t_context *ctx) {
    free(ctx->int_stack_end);
    free(ctx->ret_stack_end);
    free(ctx->stack_end);
}

int pf_exec(t_context *ctx, t_double_atom *code, t_stack *closure) {
    bool wasnull = false;
    if (ctx == NULL && code != NULL) {
        ctx = pf_context_create();
        wasnull = true;
    }
    
    ctx->execptr = code;
    while (true) {
        if (ctx->execptr[0].d != ctx->execptr[0].d) {
            // the test above is the same as isnan(execptr[0].d) but looks less
            // likely to translate to a function - even though the compiler might.
            
            uint64_t u64 = ctx->execptr[0].u64 - UPTR_MASK; // clear the NANQ bits, still 50-odd bits left for us
            if (u64&1) {
                // it's an integer push - 31bits. TODO: deal with endianess
                int32_t i = ctx->execptr[0].i32._1>>1;
                (--ctx->int_stack)[0] = i;
            } else {
                uintptr_t uptr = u64>>1;
                t_atom_union *atom = (t_atom_union*)uptr;
                switch (atom->atom.flags.type) {
                    case A_NULL:
                        // TODO: push null object into stack
                        break;
                    case A_WORD:
                    {
                        atom->word.code(ctx, ctx->execptr, atom->word.data);
                    }
                        break;
                    case A_USER_WORD:
                    {
                        int ret = pf_exec(ctx, atom->user_word.code, &atom->user_word.closure);
                        if (ret != 0) {
                            return ret;
                        }
                    }
                        break;
                    case A_STRING:
                        // TODO: push constant string -- override constant flag from source?
                        break;
                    default:
                        return -1; // not executable
                }
            }
        } else {
            // push double constant to stack.
            (--ctx->stack)[0] = ctx->execptr[0];
        }
        ctx->execptr++;
    }
    
    if (wasnull) {
        pf_context_destroy(ctx);
    }
    return 0;
}

t_double_atom pf_make_double_for_atom(t_atom *atom) {
    t_double_atom da;
    da.u64 = ((uint64_t)((uintptr_t)atom))+UPTR_MASK;
    return da;
}


t_atom *pf_unref_double(t_double_atom d) {
    return (t_atom *)(d.u64-UPTR_MASK);
}

//// STACK MANIPULATION

// dup: (... d1 d0 => ... d1 d0 d0)
static void dup_op(t_context *ctx, t_double_atom *atom, void *data) {
    ctx->stack--;
    ctx->stack[0] = ctx->stack[1];
}

// dup: (... d1 d0 => ... d1)
static void drop_op(t_context *ctx, t_double_atom *atom, void *data) {
    if (ctx->stack[0].d != ctx->stack[0].d) {
        // test if it's actually a quiet NaN.
        if (!(ctx->stack[0].u64 & QNAN_NEGMASK)) {
            // right now I don't know if this test is enough
            // anyways, just zero it out, GC will care for the rest.
            ctx->stack[0].d = 0;
        } // else just drop
    }
    ctx->stack++;
}



//// INT-INDEXED STACK MANIPULATION

// pick: (dn ... d0, ... i1 i0 => dn ... d0 dn, ... i1)
static void pick_op(t_context *ctx, t_double_atom *atom, void *data) {
    int i = ctx->int_stack[0];
    ctx->int_stack++; // pop int
    t_double_atom a = ctx->stack[i];
    ctx->stack--;
    ctx->stack[0] = a;
}


//// INTEGER STACK MANIPULATION

// idup: (... i1 i0 => ... i1 i0 i0)
static void idup_op(t_context *ctx, t_double_atom *atom, void *data) {
    ctx->int_stack--;
    ctx->int_stack[0] = ctx->int_stack[1];
}

// idrop: (... i1 i0 => ... i1)
static void idrop_op(t_context *ctx, t_double_atom *atom, void *data) {
    ctx->int_stack++;
}

// iadd: (... i1 i0 => ... i1 i0 i1+i0)
static void iadd_op(t_context *ctx, t_double_atom *atom, void *data) {
    ctx->int_stack--;
    ctx->int_stack[0] = ctx->int_stack[1]+ctx->int_stack[2];
}

// isub: (... i1 i0 => ... i1 i0 i1-i0)
static void isub_op(t_context *ctx, t_double_atom *atom, void *data) {
    ctx->int_stack--;
    ctx->int_stack[0] = ctx->int_stack[2]-ctx->int_stack[1];
}

