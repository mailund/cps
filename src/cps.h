#ifndef closure_h
#define closure_h

#include <assert.h> // FIXME: remove when I have persistent storage
#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

// MARK: STACK -------------------
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

// MARK: POOLS -------------------

enum cps_closure_pool_types
{
    CPS_STACK = 0,
    CPS_PERSISTENT = 1,
};

struct cps_memory_pool
{
    // This is not standard compliant, but is very likely to work.
    // We are relying on pointers having alignment > 1, so the first bit is always zero
    // for a true pointer, which means we can use it for a tag.
    uintptr_t pool;
};

static inline enum cps_closure_pool_types cps_pool_type(struct cps_memory_pool pool)
{
    return pool.pool & 1;
}

static inline void *cps_pool_address(struct cps_memory_pool pool)
{
    return (void *)(pool.pool & ~(uintptr_t)1);
}

static inline struct cps_memory_pool cps_new_memory_pool(enum cps_closure_pool_types type, void *pool)
{
    return (struct cps_memory_pool){.pool = (uintptr_t)pool | type};
}

static inline void **cps_base_from_stack(struct cps_stack *stack)
{
    return &stack->data;
}

static inline struct cps_stack *cps_stack_from_base(void **base)
{
#pragma GCC diagnostic ignored "-Wcast-align"
    return (struct cps_stack *)((char *)base - offsetof(struct cps_stack, data));
}

struct cps_memory
{
    // We cannot hold a direct pointer to a frame, if the memory pool needs
    // to realloc memory. Instead, we hold a pointer to the memory, so we
    // can get at the current block of storage.
    void **base;
    // First bit in pool_id holds the pool type. The rest is an id
    // used to access memory from the pool (to avoid issues with
    // reallocated memory).
    size_t pool_id;
};

static inline struct cps_memory cps_memory(struct cps_memory_pool pool, size_t pool_id)
{
    switch (cps_pool_type(pool))
    {
    case CPS_STACK:
        return (struct cps_memory){
            .base = cps_base_from_stack(cps_pool_address(pool)),
            .pool_id = (pool_id << 1) | cps_pool_type(pool)};
    case CPS_PERSISTENT:
        assert(0); // FIXME
    }
}

static inline struct cps_memory cps_allocate_memory(struct cps_memory_pool pool, size_t size, size_t align)
{
    switch (cps_pool_type(pool))
    {
    case CPS_STACK:
        return cps_memory(pool, cps_stack_alloc_frame_aligned(cps_pool_address(pool), size, align));
    case CPS_PERSISTENT:
        assert(0); // FIXME
    }
}

static inline enum cps_closure_pool_types cps_memory_pool_type(struct cps_memory mem)
{
    return mem.pool_id & 1;
}

static inline size_t cps_memory_id(struct cps_memory mem)
{
    return mem.pool_id >> 1;
}

static inline void *cps_memory_address(struct cps_memory mem)
{
    return (char *)(*mem.base) + cps_memory_id(mem);
}

static inline struct cps_memory_pool cps_memory_pool_from_memory(struct cps_memory mem)
{
    switch(cps_memory_pool_type(mem)) {
        case CPS_STACK:
            return cps_new_memory_pool(cps_memory_pool_type(mem), cps_stack_from_base(mem.base));
        case CPS_PERSISTENT:
            assert(0);
    }
}

#define cps_stack_alloc_frame(STACK, TYPE) \
    cps_stack_alloc_frame_aligned(STACK, sizeof(TYPE), alignof(TYPE))

// MARK: CLOSURES -------------------
typedef struct cps_memory cps_closure;

struct cps_closure_frame;
static inline struct cps_closure_frame *cps_get_closure_frame(cps_closure cl)
{
    return cps_memory_address(cl);
}

static inline void cps_decref_closure(cps_closure cl)
{
    switch (cps_memory_pool_type(cl))
    {
    case CPS_STACK:
        cps_stack_free_frame(cps_stack_from_base(cl.base), cps_memory_id(cl));
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
    cps_closure cl;
};

#define cps_enter_closure(TYPE)                                                   \
    struct TYPE##_frame _ = *((struct TYPE##_frame *)cps_get_closure_frame(cl_)); \
    cps_decref_closure(cl_);

#define cps_args(...) __VA_ARGS__
#define cps_define_closure_type(NAME, RET, ARGS) \
    typedef RET (*NAME##_func_)(cps_closure, ARGS);

#define cps_define_closure_frame(NAME, RET, ARGS, ...)                                            \
    struct NAME##_frame                                                                           \
    {                                                                                             \
        struct cps_closure_frame cf_;                                                             \
        __VA_ARGS__;                                                                              \
    };                                                                                            \
    /* forward decl. */                                                                           \
    RET NAME(cps_closure cl_, ARGS);                                                              \
    static inline cps_closure                                                                     \
        push_new_##NAME##_closure(struct cps_memory_pool pool,                                    \
                                  RET (*fn)(cps_closure, ARGS),                                   \
                                  struct NAME##_frame data)                                       \
    {                                                                                             \
        cps_closure cl =                                                                          \
            cps_allocate_memory(pool, sizeof(struct NAME##_frame), alignof(struct NAME##_frame)); \
        struct NAME##_frame *frame = (struct NAME##_frame *)cps_get_closure_frame(cl);            \
        *frame = data;                                                                            \
        ((struct cps_closure_frame *)frame)->fn = (cps_closure_func)fn;                           \
        ((struct cps_closure_frame *)frame)->cl = cl;                                             \
        return cl;                                                                                \
    }

#define cps_closure_function(FNAME, ...) \
    FNAME(cps_closure cl_, __VA_ARGS__)

// FIXME: handle if closure is in persistant storage
// FIXME: dispatch on id
#define cps_call_closure(TYPE, CL, ...) \
    ((TYPE##_func_)(cps_get_closure_frame(CL)->fn))(CL, __VA_ARGS__)

#define cps_push_closure_to_pool(F, POOL, ...) \
    push_new_##F##_closure(POOL, F, (struct F##_frame){__VA_ARGS__})
#define cps_push_closure(F, ...) \
    cps_push_closure_to_pool(F, cps_memory_pool_from_memory(cl_), __VA_ARGS__)

#endif /* closure_h */
