#ifndef BLOCK_T
#define BLOCK_T
struct block_t{

    struct block_t *p_next;
    struct block_t *p_prev;

    size_t free; // 1==free 0==busy
    size_t size; // block size
    size_t control; // sum control

};

#ifndef POINTER_TYPE_T
#define POINTER_TYPE_T
enum pointer_type_t
{
    pointer_null,
    pointer_heap_corrupted,
    pointer_control_block,
    pointer_inside_fences,
    pointer_inside_data_block,
    pointer_unallocated,
    pointer_valid
};
int heap_setup(void);
void heap_clean(void);

size_t sum_control(struct block_t* block);
void set_fence(struct block_t* block);
int check_block(struct block_t* block);
long long int empty_space(struct block_t *block);

void* heap_malloc(size_t size);
void* heap_calloc(size_t number, size_t size);
void* heap_realloc(void* memblock, size_t count);
void  heap_free(void* memblock);

size_t   heap_get_largest_used_block_size(void);
enum pointer_type_t get_pointer_type(const void* const pointer);

int heap_validate(void);

void* get_aligned_pointer(struct block_t* block);

void* heap_malloc_aligned(size_t count);
void* heap_calloc_aligned(size_t number, size_t size);
void* heap_realloc_aligned(void* memblock, size_t size);

#endif //HEAP_T
#endif //BLOCK_T