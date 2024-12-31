
#include "gluethread/glthread.h"
#include <stdint.h>
#include <unistd.h> 


typedef enum{
    MM_FALSE,
    MM_TRUE
}vm_bool_t;

//Meta Block Data Structure
typedef struct block_meta_data_
{
    vm_bool_t is_free;
    uint32_t block_size;    /* Size of block */
    glthread_t priority_thread_glue;
    struct block_meta_data_ *prev_block; /* ptr to the below block */
    struct block_meta_data_ *next_block; /* ptr to the above block */
    uint32_t offset;        /* Offse of the Meta block */
}block_meta_data_t;


/*structure which represents a VM Page*/
typedef struct vm_page_ {
    struct vm_page_ *next;
    struct vm_page_ *prev;
    struct vm_page_family_ *pg_family; /* back pointer*/
    block_meta_data_t block_meta_data;
    char page_memory[0];    /*first data block in vm page*/
}vm_page_t;
GLTHREAD_TO_STRUCT(glthread_to_block_meta_data, block_meta_data_t, priority_thread_glue, glthread_ptr);

#define MM_MAX_STRUCT_NAME 64
typedef struct vm_page_family_
{
    char struct_name[MM_MAX_STRUCT_NAME];
    uint32_t struct_size;
    vm_page_t *first_page;
    glthread_t free_block_priority_list_head;
} vm_page_family_t;

typedef struct vm_page_for_families_
{
    struct vm_page_for_families_ *next;
    vm_page_family_t vm_page_family[0];
} vm_page_for_families_t;

#define MAX_FAMILIES_PER_VM_PAGE (SYSTEM_PAGE_SIZE - sizeof(vm_page_for_families_t *)) / (sizeof(vm_page_family_t *))

/*
#define ITERATE_PAGE_FAMILIES_BEGIN(vm_page_for_families_ptr, curr) \
{                                                                   \
    uint32_t count = 0;                                             \
    for(curr = (vm_page_for_families_t *)&vm_page_for_families_ptr->vm_page_family[0];  \
        curr->struct_size && count < MAX_FAMILIES_PER_VM_PAGE;                          \
        curr++, count++){                                                              \
    
#define ITERATE_PAGE_FAMILIES_END(vm_page_for_families_ptr, curr) }}
*/

#define ITERATE_PAGE_FAMILIES_BEGIN(vm_page_for_families_ptr, curr) \
{                                                                   \
    uint32_t count = 0;                                             \
    for(curr = (vm_page_family_t *)&vm_page_for_families_ptr->vm_page_family[0];  \
        curr->struct_size && count < MAX_FAMILIES_PER_VM_PAGE;                          \
        curr++, count++){                                                               \
    
#define ITERATE_PAGE_FAMILIES_END(vm_page_for_families_ptr, curr) }}

vm_page_family_t* lookup_page_family_by_name(char *struct_name);

//Returns the offset of a field in a structure
#define offset_of(container_structure, field_name) \
                (size_t)&((container_structure *)0)->field_name

//Returns the starting address of VM Page
#define MM_GET_PAGE_FROM_META_BLOCK(block_meta_data_ptr)    \
        ((vm_page_t *)((char *)(block_meta_data_ptr)-block_meta_data_ptr->offset))

//Return the next meta block ptr
#define NEXT_META_BLOCK(block_meta_data_ptr)    \
                block_meta_data_ptr->next_block

//Return the next meta block ptr using the block size
//Creates a new meta_block after fragmentation
#define NEXT_META_BLOCK_BY_SIZE(block_meta_data_ptr)    \
                (block_meta_data_t *)((char *)(block_meta_data_ptr + 1) + block_meta_data_ptr->block_size)

//Return the prev meta block ptr 
#define PREV_META_BLOCK(block_meta_data_ptr)    \
                block_meta_data_ptr->prev_block

/*
#define mm_bind_blocks_for_allocation(allocated_meta_block, free_meta_block)                \
        allocated_meta_block->prev_block = free_meta_block->prev_block;                        \
        if(free_meta_block->prev_block)                                                       \
            free_meta_block->prev_block->next_block = allocated_meta_block;                      \
        allocated_meta_block->next_block = free_meta_block;                                      \
        free_meta_block->prev_block = allocated_meta_block;                                     \
        free_meta_block->block_size = free_meta_block->block_size-allocated_meta_block->block_size;   \
        if(free_meta_block->next_block)                                                         \
            free_meta_block->next_block->prev_block = free_meta_block                           \
*/

//Function to check if vm_page_is_empty
vm_bool_t mm_is_vm_page_empty(vm_page_t *vm_page);
static vm_bool_t mm_split_free_data_block_for_allocation(vm_page_family_t *vm_page_family,
                                        block_meta_data_t *block_meta_data,
                                        uint32_t size);

//Macro to mark VM data page as empty
#define MARK_VM_PAGE_EMPTY(vm_page_t_ptr)           \
        vm_page_t_ptr->block_meta_data.next_block = NULL;   \
        vm_page_t_ptr->block_meta_data.prev_block = NULL;   \
        vm_page_t_ptr->block_meta_data.is_free = MM_TRUE    \


//Macro to iterate over all VM Data pages for a given page family
#define ITERATE_VM_PAGE_BEGIN(vm_page_family_ptr, curr)     \
    for(curr = vm_page_family_ptr->first_page; curr != NULL ; curr = curr->next){   

#define ITERATE_VM_PAGE_END(vm_page_family_ptr, curr)   }

//Macro to loop over all Meta blocks of a given VM Data Page
#define ITERATE_VM_PAGE_ALL_BLOCKS_BEGIN (vm_page_ptr, curr)    \
    for(curr = vm_page_ptr->block_meta_data; curr != NULL ; curr = curr->next_block){       \

#define ITERATE_VM_PAGE_ALL_BLOCKS_END (vm_page_ptr, curr)}                 

//Allocate a new VM page to the page family
vm_page_t *allocate_vm_page(vm_page_family_t *vm_page_family);

//Insert a free data to priority_queue of a page family
static void mm_add_free_block_meta_data_to_free_block_list(
                vm_page_family_t *vm_page_family,
                block_meta_data_t *free_block
);



#define mm_bind_blocks_for_allocation(allocated_meta_block, free_meta_block)    \
    free_meta_block->prev_block = allocated_meta_block;                         \
    free_meta_block->next_block = allocated_meta_block->next_block;             \
    allocated_meta_block->next_block = free_meta_block;                         \
    if(free_meta_block->next_block)                                             \
        free_meta_block->next_block->prev_block = free_meta_block


/*                      
        uint32_t no_free_blocks = 0;                                    
        uint32_t no_allocated_blocks = 0;                                
        block_meta_data_t largest_free_block;                           
        block_meta_data_t largest_allocated_block;                      
        uint32_t max_free_block=INT32_MIN;                              
        uint32_t max_allocated_block=INT32_MIN;                              
        for(block_meta_data_t curr = first_meta_block;      \            
            curr->next;                                             \    
            curr = curr->next){                                     \    
                if(curr->is_free)
                {
                    no_free_blocks++;
                    if(max_free_block < curr->block_size){
                        max_free_block = curr->block_size;
                        largest_free_block = curr;
                    }
                }
                else{
                    no_allocated_blocks++;
                    if(max_allocated_block < curr->block_size){
                        max_allocated_block = curr;
                    }
                }
                if(curr->prev_block){
                    if(!curr->prev_block->is_free && !curr->is_free)
                        assert(0); 
                } 
        }
*/
                   






