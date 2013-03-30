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
    
}

int pf_exec(t_context *ctx, t_double_atom *code,
            t_double_atom *closure_stack,
            t_double_atom *closure_stack_begin,
            t_double_atom *closure_stack_end) {
    bool wasnull = false;
    if (ctx == NULL && code != NULL) {
        ctx = pf_context_create();
        wasnull = true;
    }
    
    t_double_atom *execptr = code;
    while (true) {
        if (execptr[0].d != execptr[0].d) {
            // the test above is the same as isnan(execptr[0].d) but looks less
            // likely to translate to a function - even though the compiler might.
            
            uint64_t u64 = execptr[0].u64 - UPTR_MASK; // clear the NANQ bits, still 50-odd bits left for us
            if (u64&1) {
                // it's an integer push - 31bits. TODO: deal with endianess
                int32_t i = execptr[0].i32._1>>1;
                (--ctx->int_stack)[0] = i;
            } else {
                uintptr_t uptr = u64>>1;
                t_atom_union *atom = (t_atom_union*)uptr;
                switch (atom->atom.flags.type) {
                    case A_NULL:
                        // TODO: store null object into stack
                        break;
                    case A_WORD:
                        // TODO: execute native word
                        break;
                    case A_USER_WORD:
                        // TODO: execute PFVM word
                    {
                        int ret = pf_exec(ctx, atom->user_word.code,
                                          atom->user_word.closure,
                                          atom->user_word.closure_begin,
                                          atom->user_word.closure_end);
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
            (--ctx->stack)[0] = execptr[0];
        }
        execptr++;
    }
    
    if (wasnull) {
        pf_context_destroy(ctx);
    }
    return 0;
}

static void dup_op(t_context *ctx, t_double_atom *atom, void *data) {
    ctx->stack--;
    ctx->stack[0] = ctx->stack[1];
}

static void drop_op(t_context *ctx, t_double_atom *atom, void *data) {
    ctx->stack++;
}

