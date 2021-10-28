//
//  closure.h
//  cps-iter
//
//  Created by Thomas Mailund on 26/10/2021.
//

#ifndef closure_h
#define closure_h

#include <assert.h>
#include <stdlib.h>

// --- STACK -------------------

struct cps_stack
{
    size_t size, sp;
    void *data;
};

void cps_init_stack(struct cps_stack *stack);
void cps_resize_stack(struct cps_stack *stack, size_t new_size);
void cps_free_stack(struct cps_stack *stack);

// move an address up to the nearest that matches the alignment
// constraints.
static inline size_t cps_align_address(size_t addr, size_t align)
{
    // align is a power of two, so   00...000100.
    // thus m becomes                00...000011.
    // If addr is zero on the lower bits and we
    // add m, we leave the upper bits alone.
    // However, if addr has set lower bits:
    // add m to addr: if addr = 10...011001
    //                     +m   00...000011
    //               addr + m = 10...100101
    // will add 1 to the bits above align.
    // masking out the lower bits and we have
    // rounded up.
    size_t m = align - 1;
    return (addr + m) & ~m;
}

static inline size_t cps_stack_alloc_frame_aligned(struct cps_stack *stack,
                                                   size_t frame_size,
                                                   size_t frame_align)
{
    stack->sp = cps_align_address(stack->sp, frame_align);
    if (stack->sp + frame_size > stack->size)
    {
        cps_resize_stack(stack, 2 * (stack->sp + frame_size));
    }

    size_t new_frame = stack->sp;
    stack->sp += frame_size;

    return new_frame;
}

#define cps_stack_alloc_frame(STACK, TYPE) \
    cps_stack_alloc_frame_aligned(STACK, sizeof(TYPE), _Alignof(TYPE))

static inline void cps_stack_free_frame(struct cps_stack *stack,
                                        size_t fp)
{
    stack->sp = fp;
    if (stack->sp < stack->size / 4)
    {
        cps_resize_stack(stack, stack->size / 2);
    }
}

static inline void *cps_get_stack_frame(struct cps_stack *stack, size_t offset)
{
    return (char *)stack->data + offset;
}

// --- CLOSURES -------------------

enum cps_closure_pool
{
    CPS_STACK,
    CPS_PERSISTENT
};
struct cps_closure
{
    enum cps_closure_pool pool;
    size_t id;
};

static inline void cps_decref_closure(struct cps_stack *stack, struct cps_closure cl)
{
    switch (cl.pool)
    {
    case CPS_STACK:
        cps_stack_free_frame(stack, cl.id);
        break;
    case CPS_PERSISTENT:
        assert(0); // FIXME
        break;
    }
}

typedef void (*cps_closure_func)(void);
struct cps_closure_frame
{
    cps_closure_func fn;
    struct cps_closure cl;
};

#define cps_decref_frame(POOL, FRAME) \
    cps_decref_closure(POOL, ((struct cps_closure_frame *)(FRAME))->cl)

#define cps_enter_closure(TYPE)                               \
    struct TYPE##_frame _ = *((struct TYPE##_frame *)frame_); \
    cps_decref_frame(pool_, frame_);

#define cps_args(...) __VA_ARGS__
#define cps_define_closure_type(NAME, RET, ARGS) \
    typedef RET (*NAME##_func_)(struct cps_stack *, void *, ARGS);

#define cps_define_closure_frame(NAME, RET, ARGS, ...)                                                       \
    struct NAME##_frame                                                                                      \
    {                                                                                                        \
        struct cps_closure_frame cf_;                                                                        \
        __VA_ARGS__;                                                                                         \
    };                                                                                                       \
    /* forward decl. */                                                                                      \
    RET NAME(struct cps_stack *, void *, ARGS);                                                              \
    static inline struct cps_closure alloc_new_##NAME##_closure(struct cps_stack *stack,                     \
                                                                RET (*fn)(struct cps_stack *, void *, ARGS), \
                                                                struct NAME##_frame data)                    \
    {                                                                                                        \
        struct cps_closure cl = {CPS_STACK, cps_stack_alloc_frame(stack, struct NAME##_frame)};              \
        struct NAME##_frame *frame = cps_get_stack_frame(stack, cl.id);                                      \
        *frame = data;                                                                                       \
        ((struct cps_closure_frame *)frame)->fn = (cps_closure_func)fn;                                      \
        ((struct cps_closure_frame *)frame)->cl = cl;                                                        \
        return cl;                                                                                           \
    }

#define cps_closure_function(FNAME, CLTYPE, ...) \
    FNAME(struct cps_stack *pool_, void *frame_, __VA_ARGS__)

#define cps_closure_frame(POOL, CL) \
    ((struct cps_closure_frame *)cps_get_stack_frame(POOL, (CL).id))
#define cps_closure_func(POOL, CL) \
    (cps_closure_frame(POOL, CL)->fn)
#define cps_call_closure_with_pool(TYPE, POOL, CL, ...) \
    ((TYPE##_func_)cps_closure_func(POOL, CL))(POOL, cps_get_stack_frame(POOL, (CL).id), __VA_ARGS__);
#define cps_call_closure(TYPE, CL, ...) \
    cps_call_closure_with_pool(TYPE, pool_, CL, __VA_ARGS__)

#define cps_push_closure_to_pool(F, POOL, ...) \
    alloc_new_##F##_closure(POOL, F, (struct F##_frame){__VA_ARGS__})
#define cps_push_closure(F, ...) \
    alloc_new_##F##_closure(pool_, F, (struct F##_frame){__VA_ARGS__})

int fib(int n);

#endif /* closure_h */
