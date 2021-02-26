#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "heap.h"
struct heap_t memory_manager={0,0,0};
int heap_setup(void)
{
    errno=0;

        memory_manager.head=sbrk(PAGESIZE);
        if(errno)
        {
            return -1;
        }
        memory_manager.tail=(struct block_t*)((uint8_t*)memory_manager.head+PAGESIZE-sizeof(struct block_t));
        memory_manager.size=PAGESIZE;

        //HEAD
        memory_manager.head->p_next=memory_manager.tail;
        memory_manager.head->p_prev=NULL;
        memory_manager.head->size=0;
        memory_manager.head->free=0;
        memory_manager.head->control=sum_control(memory_manager.head);

        //TAIL
        memory_manager.tail->p_next=NULL;
        memory_manager.tail->p_prev=memory_manager.head;
        memory_manager.tail->size=0;
        memory_manager.tail->free=0;
        memory_manager.tail->control=sum_control(memory_manager.tail);


    return 0;
}
void heap_clean(void)
{
    if(memory_manager.size<=0 || memory_manager.head==NULL)
    {
        return;
    }
    sbrk(-memory_manager.size);
    memory_manager.size=0;
    memory_manager.head=NULL;
    memory_manager.tail=NULL;
    return;
}
size_t sum_control(struct block_t* block)
{
    size_t sum=0;
    sum=block->size+block->free+(size_t)block->p_prev+(size_t)block->p_next;
    return sum;
}
void set_fence(struct block_t* block)
{
    *(uint32_t *)((uint8_t *)block+sizeof(struct block_t))=FENCE;
    *(uint32_t *)((uint8_t *)block+block->size+sizeof(struct block_t)-sizeof(uint32_t))=FENCE;
    block->size=block->size-sizeof(uint32_t)*2;

    return;
}
int check_block(struct block_t* block)
{
    if(block->control!=sum_control(block))
    {
        return 3;
    }
    if(*(uint32_t*)((uint8_t *)block+sizeof(struct block_t))!=FENCE || *(uint32_t*)((uint8_t *)block+sizeof(struct block_t)+block->size+sizeof(uint32_t))!=FENCE) //Naruszenie płotków(do przeanalizowania)
    {
        return 1;
    }
    return 0;
}
long long int empty_space(struct block_t *block)
{
    if(block->p_next==NULL)
    {
        return 0;
    }
    if(block->p_next==memory_manager.tail && block==memory_manager.head)
    {
        return memory_manager.size-sizeof(struct block_t)*2;
    }
    int size;
    size=((uint8_t *)block->p_next-(uint8_t *)block-2*sizeof(uint32_t)-sizeof(struct block_t)-block->size);
    return size;
}

void* heap_malloc(size_t size)
{
    if(size<=0 )
    {
        return NULL;
    }
    int check;
    check=heap_validate();
    if(check)
    {
        return NULL;
    }

    struct block_t *block=memory_manager.head;

    while(block->p_next!=NULL)
    {
        if((long long int)((empty_space(block)-sizeof(struct block_t)-sizeof(uint32_t)*2))>=(long long int)size)
        {
            struct block_t *given;
            if(block==memory_manager.head)
            {
                given=(struct block_t*)((uint8_t *)memory_manager.head+sizeof(struct block_t));
                given->p_prev=memory_manager.head;
                memory_manager.head->p_next=given;
                given->p_next=memory_manager.tail;
                memory_manager.tail->p_prev=given;
                given->free=0;
                given->size=size+sizeof(uint32_t)*2;
                set_fence(given);
                given->control=sum_control(given);
                memory_manager.tail->control=sum_control(memory_manager.tail);
                memory_manager.head->control=sum_control(memory_manager.head);
                return (uint8_t *)given+sizeof(struct block_t)+sizeof(uint32_t);
            }
            else
            {
                given=(struct block_t *)((uint8_t *)block+sizeof(struct block_t)+block->size+sizeof(uint32_t)*2);
                given->p_prev=block;
                given->p_next=block->p_next;
                block->p_next=given;
                given->p_next->p_prev=given;
                given->free=0;
                given->size=size+sizeof(uint32_t)*2;
                set_fence(given);
                given->control=sum_control(given);
                block->control=sum_control(block);
                given->p_next->control=sum_control(given->p_next);
                return (uint8_t *)given+sizeof(struct block_t)+sizeof(uint32_t);
            }
        }
        block=block->p_next;
    }
        errno=0;

    sbrk(PAGESIZE);
        if(errno)
        {
            return NULL;
        }
        memory_manager.size=memory_manager.size+PAGESIZE;
        struct block_t *new_tail,*tail2;
        new_tail=(struct block_t*)((uint8_t*)memory_manager.head+memory_manager.size-sizeof(struct block_t));
        new_tail->size=0;
        new_tail->p_next=NULL;
        new_tail->free=0;
        tail2=memory_manager.tail;
        memory_manager.tail=new_tail;
        new_tail->p_prev=tail2->p_prev;
        tail2->p_prev->p_next=new_tail;
        tail2->p_prev->control=sum_control(tail2->p_prev);
        new_tail->control=sum_control(new_tail);
        return heap_malloc(size);

}
void* heap_calloc(size_t number, size_t size)
{
    if(number<=0)
    {
        return NULL;
    }
    if(heap_validate())
    {
        return NULL;
    }
    void* tmp;
    tmp=heap_malloc(size*number);
    if(tmp==NULL)
    {
        return NULL;
    }
    memset(tmp,0,size*number);
    return tmp;
}
void* heap_realloc(void* memblock, size_t count)
{

    if(heap_validate())
    {
        return NULL;
    }
    if(memblock==NULL)
    {
        return heap_malloc(count);
    }
    if(pointer_valid!=get_pointer_type(memblock))
    {
         return NULL;
    }
    if(count==0)
    {
        heap_free(memblock);
        return NULL;
    }
    struct block_t *block;
    block=(struct block_t*)((uint8_t*)memblock-sizeof(struct block_t)-sizeof(uint32_t));
    if(count<block->size)
    {
        block->size=count+sizeof(uint32_t)*2;
        set_fence(block);
        block->control=sum_control(block);
        return memblock;
    }
    else if(count==block->size)
    {
        return memblock;
    }
    else
    {
        if((unsigned long)empty_space(block)+block->size>=count)
        {
            block->size=sizeof(uint32_t)*2+count;
            set_fence(block);
            block->control=sum_control(block);
            return memblock;
        }
        else
        {
            if(block->p_next==memory_manager.tail && empty_space(block)+block->size<count)
            {
                errno=0;

                sbrk(PAGESIZE);

                if(errno)
                {
                    return NULL;
                }

                memory_manager.size=memory_manager.size+PAGESIZE;
                struct block_t *new_tail,*tail2;
                new_tail=(struct block_t*)((uint8_t*)memory_manager.head+memory_manager.size-sizeof(struct block_t));
                new_tail->size=0;
                new_tail->p_next=NULL;
                new_tail->free=0;
                tail2=memory_manager.tail;
                memory_manager.tail=new_tail;
                new_tail->p_prev=tail2->p_prev;
                tail2->p_prev->p_next=new_tail;
                tail2->p_prev->control=sum_control(tail2->p_prev);
                new_tail->control=sum_control(new_tail);
                return heap_realloc(memblock,count);
            }
            struct block_t *ptr=heap_malloc(count);

            if(ptr==NULL)
            {
                return NULL;
            }

            memcpy(ptr,memblock,block->size);
            heap_free(memblock);
            return ptr;

        }
    }
}
void heap_free(void* memblock)
{
    if(pointer_valid!=get_pointer_type(memblock))
    {
        return;
    }

    struct block_t *block,*prev,*next;
    block=(struct block_t*)((uint8_t *)memblock-sizeof(struct block_t)-sizeof(uint32_t));

    next=block->p_next;
    prev=block->p_prev;
    next->p_prev=prev;
    prev->p_next=next;
    next->control=sum_control(next);
    prev->control=sum_control(prev);

    return;
}
size_t heap_get_largest_used_block_size(void)
{
    size_t zmienna=0;
    if(heap_validate())
    {
        return 0;
    }
    for(struct block_t *block=memory_manager.head;block->p_next!=NULL;block=block->p_next)
    {
        if(zmienna<=block->size)
        {
            zmienna=block->size;
        }
    }
    return zmienna;
}
enum pointer_type_t get_pointer_type(const void* const pointer)
{
    if(pointer==NULL)
    {
        return pointer_null;
    }
    struct block_t *block;
    if(heap_validate())
    {
        return pointer_heap_corrupted;
    }
    block=memory_manager.head->p_next;
    while(block->p_next!=NULL)
    {
        if((uint8_t *)block<=(uint8_t *)pointer && (uint8_t *)block->p_next>(uint8_t *)pointer)
        {
            if((uint8_t*)block<=(uint8_t *)pointer && (uint8_t*)block+sizeof(struct block_t)>(uint8_t *)pointer)
            {
                return pointer_control_block;
            }
            else if((uint8_t*)block+sizeof(struct block_t)<=(uint8_t *)pointer && (uint8_t*)block+sizeof(struct block_t)+sizeof(uint32_t)>(uint8_t *)pointer)
            {
                return pointer_inside_fences;
            }
            else if((uint8_t*)block+sizeof(struct block_t)+sizeof(uint32_t)==(uint8_t*)pointer)
            {
                return pointer_valid;
            }
            else if((uint8_t*)block+sizeof(struct block_t)+sizeof(uint32_t)<(uint8_t*)pointer && (uint8_t*)block+sizeof(struct block_t)+sizeof(uint32_t)+block->size>(uint8_t*)pointer)
            {
                return pointer_inside_data_block;
            }
            else if((uint8_t*)block+sizeof(struct block_t)+sizeof(uint32_t)+block->size<=(uint8_t*)pointer && (uint8_t*)block+sizeof(struct block_t)+sizeof(uint32_t)*2+block->size>(uint8_t*)pointer)
            {
                return pointer_inside_fences;
            }
        }
        block=block->p_next;
    }

    return pointer_unallocated;
}
int heap_validate(void)
{
    if(memory_manager.head==NULL && memory_manager.tail==NULL)
    {
        return 2;
    }
    struct block_t *ptr;

    if(memory_manager.head->p_next!=memory_manager.tail)
    {
        for(ptr=memory_manager.head->p_next;ptr!=memory_manager.tail;ptr=ptr->p_next)
        {

            int check;
            check=check_block(ptr);
            if(check)
            {
                return check;
            }
        }
    }

    if(memory_manager.head->control!=sum_control(memory_manager.head) || memory_manager.tail->control!=sum_control(memory_manager.tail))
    {
        return 3;
    }


    return 0;
}
void* get_aligned_pointer(struct block_t* block)
{
    int i=0;
    if(block==memory_manager.head)
    {
        uint8_t * ptr=(uint8_t *)block+sizeof(struct block_t);
        while(ptr+i!=(uint8_t *)block->p_next)
        {
            if((((intptr_t)(ptr+i)) & (intptr_t)(PAGESIZE-1))==0)
            {
                return ptr+i;
            }
            i++;
        }
    }
    else
    {
        uint8_t * ptr=(uint8_t *)block+sizeof(struct block_t)+sizeof(uint32_t)*2+block->size;
        while(ptr+i!=(uint8_t *)block->p_next)
        {
            if((((intptr_t)(ptr+i)) & (intptr_t)(PAGESIZE-1))==0)
            {
                return ptr+i;
            }
            i++;
        }
    }

    return NULL;
}
void* heap_malloc_aligned(size_t count)
{
    if(count<=0)
    {
        return NULL;
    }
    if(heap_validate())
    {
        return NULL;
    }

    struct block_t *block=memory_manager.head;

    while(block->p_next!=NULL)
    {
        if((long long int)((empty_space(block)-sizeof(struct block_t)-sizeof(uint32_t)*2))>=(long long int)count)
        {
            uint8_t *ptr=get_aligned_pointer(block);
            if(ptr!=NULL)
            {
                if(((long int)(uint8_t *)ptr-(long int)(uint8_t *)block-(long int)sizeof(struct block_t)-(long int)block->size-(long int)sizeof(uint32_t)*2>=(long int)(sizeof(struct block_t)+sizeof(uint32_t))) && (long int)(uint8_t *)block->p_next-(long int)(uint8_t*)ptr>=(long int)(sizeof(uint32_t)+count))
                {
                    struct block_t *given;
                    given=(struct block_t*)((uint8_t *)ptr-sizeof(uint32_t)-sizeof(struct block_t));
                    given->p_prev=block;
                    given->p_next=block->p_next;
                    block->p_next=given;
                    given->p_next->p_prev=given;
                    given->free=0;
                    given->size=count+sizeof(uint32_t)*2;
                    set_fence(given);
                    given->control=sum_control(given);
                    block->control=sum_control(block);
                    given->p_next->control=sum_control(given->p_next);
                    return ptr;
                }
            }
        }
        block=block->p_next;
    }

    errno=0;
    sbrk(PAGESIZE);
    if(errno)
    {
        return NULL;
    }
    memory_manager.size=memory_manager.size+PAGESIZE;
    struct block_t *new_tail,*tail2;
    new_tail=(struct block_t*)((uint8_t*)memory_manager.head+memory_manager.size-sizeof(struct block_t));
    new_tail->size=0;
    new_tail->p_next=NULL;
    new_tail->free=0;
    tail2=memory_manager.tail;
    memory_manager.tail=new_tail;
    new_tail->p_prev=tail2->p_prev;
    tail2->p_prev->p_next=new_tail;
    tail2->p_prev->control=sum_control(tail2->p_prev);
    new_tail->control=sum_control(new_tail);
    return heap_malloc_aligned(count);
}
void* heap_calloc_aligned(size_t number, size_t size)
{
    if(number<=0)
    {
        return NULL;
    }
    if(heap_validate())
    {
        return NULL;
    }
    void* tmp;
    tmp=heap_malloc_aligned(size*number);
    if(tmp==NULL)
    {
        return NULL;
    }
    memset(tmp,0,size*number);
    return tmp;

}
void* heap_realloc_aligned(void* memblock, size_t size)
{
    if(heap_validate())
    {
        return NULL;
    }
    if(memblock==NULL)
    {
        return heap_malloc_aligned(size);
    }
    if(pointer_valid!=get_pointer_type(memblock))
    {
        return NULL;
    }
    if(size==0)
    {
        heap_free(memblock);
        return NULL;
    }
    struct block_t *block;
    block=(struct block_t*)((uint8_t*)memblock-sizeof(struct block_t)-sizeof(uint32_t));
    if(size<block->size)
    {
        block->size=size+sizeof(uint32_t)*2;
        set_fence(block);
        block->control=sum_control(block);
        return memblock;
    }
    else if(size==block->size)
    {
        return memblock;
    }
    else
    {
        if((unsigned long)empty_space(block)+block->size>=size)
        {
            block->size=sizeof(uint32_t)*2+size;
            set_fence(block);
            block->control=sum_control(block);
            return memblock;
        }
        else
        {
            if(block->p_next==memory_manager.tail && empty_space(block)+block->size<size)
            {
                errno=0;

                sbrk(PAGESIZE);

                if(errno)
                {
                    return NULL;
                }

                memory_manager.size=memory_manager.size+PAGESIZE;
                struct block_t *new_tail,*tail2;
                new_tail=(struct block_t*)((uint8_t*)memory_manager.head+memory_manager.size-sizeof(struct block_t));
                new_tail->size=0;
                new_tail->p_next=NULL;
                new_tail->free=0;
                tail2=memory_manager.tail;
                memory_manager.tail=new_tail;
                new_tail->p_prev=tail2->p_prev;
                tail2->p_prev->p_next=new_tail;
                tail2->p_prev->control=sum_control(tail2->p_prev);
                new_tail->control=sum_control(new_tail);
                return heap_realloc_aligned(memblock,size);
            }
            struct block_t *ptr=heap_malloc_aligned(size);

            if(ptr==NULL)
            {
                return NULL;
            }

            memcpy(ptr,memblock,block->size);
            heap_free(memblock);
            return ptr;

        }
    }
}
