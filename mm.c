//Linux memory manager

#include <stdio.h>
#include <memory.h>
#include <unistd.h>     /*for getpage size*/
#include <sys/mman.h>   /*For using mmap()*/
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include "mm.h"
#include "gluethread/glthread.h"


static size_t SYSTEM_PAGE_SIZE = 0;
static vm_page_for_families_t *first_vm_page_for_families = NULL;

void mm_init()
{
    SYSTEM_PAGE_SIZE = getpagesize();   /* This will return the size of 1 VM Page*/
}

//Request a virtual mem page from the virtual add space of a process from the kernel
//mmap to allocate & deallocate the vm pages
//api to allocate vm page from kernel space to user space
static void* mm_get_new_vm_page_from_kernel(int units)
{
    //add of the page
    char *vm_page = mmap(
        0,                          
        units * SYSTEM_PAGE_SIZE,   
        PROT_READ|PROT_WRITE|PROT_EXEC, 
        MAP_ANON|MAP_PRIVATE,           
        0, 0                                            
    );

    if(vm_page == MAP_FAILED){
        printf("Error : VM Page allocation Failed\n");
        return NULL;
    }
    memset(vm_page, 0, units * SYSTEM_PAGE_SIZE);
    return (void *)vm_page;
}

//to deallocate the vm pages back to the kernel
static void mm_return_vm_page_to_kernel(void *vm_page, int units)
{    
    if(munmap(vm_page, units * SYSTEM_PAGE_SIZE))
    {
        printf("Error : Could not munmap VM page to kernel");
    }
}


// to instatiate a new page family
void mm_instantiate_new_page_family(char *struct_name, uint32_t struct_size)
{
    printf("Struct Name - %s\n", struct_name);
    vm_page_family_t *vm_page_family_curr = NULL;
    vm_page_for_families_t *new_vm_page_for_famililes = NULL;

    if(struct_size > SYSTEM_PAGE_SIZE)
    {
        //printf("Error : %s () Structure %s size exceeds system page size\n" \
                __FUNCTION__, struct_size);
        printf("Error : %s () Structure %d size exceeds system page size\n", struct_name, struct_size);
        return;
    }

    //Case 1: Registering the first page family with the Linux Memory Manager
    if(!first_vm_page_for_families)
    {
        first_vm_page_for_families =  (vm_page_for_families_t *) mm_get_new_vm_page_from_kernel(1);
        first_vm_page_for_families->next = NULL;    
        strncpy(first_vm_page_for_families->vm_page_family[0].struct_name, struct_name, MM_MAX_STRUCT_NAME);
        first_vm_page_for_families->vm_page_family[0].struct_size = struct_size;
        first_vm_page_for_families->vm_page_family[0].first_page = NULL;
        init_glthread(&first_vm_page_for_families->vm_page_family[0].free_block_priority_list_head,
                        offsetof(vm_page_family_t, free_block_priority_list_head));
    }
    else{
        uint32_t count = 0;

        ITERATE_PAGE_FAMILIES_BEGIN(first_vm_page_for_families, vm_page_family_curr){

            //printf("vm_page_family_curr=%s\n",vm_page_family_curr->struct_name);
            if(strncmp(vm_page_family_curr->struct_name, struct_name, MM_MAX_STRUCT_NAME) != 0){
                count++;
                continue;
            }
            assert(0);

        }ITERATE_PAGE_FAMILIES_END(first_vm_page_for_families, vm_page_family_curr)
    

        if(count == MAX_FAMILIES_PER_VM_PAGE)
        {
            new_vm_page_for_famililes = (vm_page_for_families_t *) mm_get_new_vm_page_from_kernel(1);
            new_vm_page_for_famililes->next = first_vm_page_for_families;
            first_vm_page_for_families = new_vm_page_for_famililes;
            vm_page_family_curr = &first_vm_page_for_families->vm_page_family[0];
        }
    

        strncpy(vm_page_family_curr->struct_name, struct_name, MM_MAX_STRUCT_NAME);
        vm_page_family_curr->struct_size = struct_size;
        vm_page_family_curr->first_page = NULL;
        init_glthread(&first_vm_page_for_families->vm_page_family[0].free_block_priority_list_head,
                        offset_of(vm_page_family_t, free_block_priority_list_head));
    }
    //vm_page_family_curr->first_page = NULL;
}

/*Function to print the registered Page families
    1. Assign a pointer to first_vm_page_for_families and run a for loop to iterate all the families
    2. Second for loop to travese each family*/
void mm_print_registered_page_families()
{
    vm_page_family_t *vm_page_family_curr = NULL;  
    vm_page_for_families_t *vm_page_for_families_curr = NULL;
    for(vm_page_for_families_curr=first_vm_page_for_families;vm_page_for_families_curr; \
                            vm_page_for_families_curr=vm_page_for_families_curr->next ) 
    {
                                
        ITERATE_PAGE_FAMILIES_BEGIN(vm_page_for_families_curr, vm_page_family_curr){

        printf("Page Family : %s, Size=%d\n",vm_page_family_curr->struct_name, vm_page_family_curr->struct_size);

        }ITERATE_PAGE_FAMILIES_END(vm_page_for_families_curr, vm_page_family_curr)
    }
}

/*Function to return the pointer to the block with struct_name*/
vm_page_family_t* lookup_page_family_by_name(char *struct_name)
{
    vm_page_family_t *vm_page_family_curr = NULL;   
    vm_page_for_families_t *vm_page_for_families_curr = NULL;

    for(vm_page_for_families_curr=first_vm_page_for_families; vm_page_for_families_curr ;
                                vm_page_for_families_curr=vm_page_for_families_curr->next)
    {
        ITERATE_PAGE_FAMILIES_BEGIN(first_vm_page_for_families, vm_page_family_curr){

            if(strncmp(vm_page_family_curr->struct_name, struct_name, MM_MAX_STRUCT_NAME) == 0)
                return vm_page_family_curr;

        }ITERATE_PAGE_FAMILIES_END(first_vm_page_for_families, vm_page_family_curr)
    }

    return NULL;
}

/*To merge two consecutive free memory blocks*/
static void mm_union_free_blocks(block_meta_data_t *first, block_meta_data_t *second)
{
    assert(first->is_free == MM_TRUE && second->is_free == MM_TRUE);

    first->block_size = first->block_size + sizeof(block_meta_data_t)+
                                            second->block_size;

    first->next_block = second->next_block;

    if(second->next_block)
        second->next_block->prev_block = first;
}

/*To check if a vm page is empty or not*/
vm_bool_t mm_is_vm_page_empty(vm_page_t *vm_page)
{
    if(vm_page->block_meta_data.next_block == NULL &&
        vm_page->block_meta_data.prev_block == NULL &&
        vm_page->block_meta_data.is_free == MM_TRUE){
        
            return MM_TRUE;
        }
    return MM_FALSE;
}

/*Return the size of Free Data block of an Empty VM Page*/
static inline uint32_t mm_max_page_allocatable_memory (int units){
    return (uint32_t)
            ((SYSTEM_PAGE_SIZE * units) - offset_of(vm_page_t, page_memory));
}

/*Allocate a new VM page to the page family*/
vm_page_t *allocate_vm_page(vm_page_family_t *vm_page_family)
{
    vm_page_t *vm_page = mm_get_new_vm_page_from_kernel(1);

    /*Initialize lower most meta block of the vm page*/
    MARK_VM_PAGE_EMPTY(vm_page);

    vm_page->block_meta_data.block_size = mm_max_page_allocatable_memory(1);
    vm_page->block_meta_data.offset = offset_of(vm_page_t, block_meta_data);
    init_glthread(&vm_page->block_meta_data.priority_thread_glue,
                    offset_of(block_meta_data_t, priority_thread_glue));

    vm_page->next = NULL;
    vm_page->prev = NULL;

    /*Set the back pointer to page family*/
    vm_page->pg_family = vm_page_family;

    /*First VM data page for a given page family*/
    if(!vm_page_family->first_page)
    {
        vm_page_family->first_page = vm_page;
        return vm_page;
    }

    /*Insert new VM Page to the head of the doubly linked list*/
    vm_page->next = vm_page_family->first_page;
    vm_page_family->first_page->prev = vm_page;
    vm_page_family->first_page = vm_page;

    return vm_page;
}

/* Delete a VM page from the page family */
void mm_vm_page_delete_and_free(vm_page_t *vm_page)
{
    vm_page_family_t *vm_page_family = vm_page->pg_family;

    /* If the first page (first node) is being deleted*/
    if(vm_page_family->first_page == vm_page){
        vm_page_family->first_page = vm_page->next;
        if(vm_page->next)
            vm_page->next->prev = NULL;
        
    }

    /* If the middle node is being deleted */
    vm_page->prev->next = vm_page->next;
    if(vm_page->next)
        vm_page->next->prev = vm_page->prev;

    vm_page->next = NULL;
    vm_page->prev = NULL;
    mm_return_vm_page_to_kernel((void *)vm_page, 1);
    return;

}

/* Free block Data Comparison Function:
   Return -1 : if_block_meta_data1 block size is greater
   Return 1 : if block_meta_data2 block size is greater
   Return 0 : if block sizes are equal */
static int free_blocks_comparison_function( void *_block_meta_data1, void *_block_meta_data2){

    block_meta_data_t *block_meta_data1 = (block_meta_data_t *)_block_meta_data1;
    block_meta_data_t *block_meta_data2 = (block_meta_data_t *)_block_meta_data2;

    if(block_meta_data1->block_size > block_meta_data2->block_size)
        return -1;
    else if(block_meta_data1->block_size < block_meta_data2->block_size)
        return 1;
    return 0;   
}

/* Insert free meta block into the priority queues of the page family */
static void mm_add_free_block_meta_data_to_free_block_list(
                vm_page_family_t *vm_page_family,
                block_meta_data_t *free_block){

    if(free_block->is_free == MM_TRUE)
    {
        printf("The block to be added to free_blocks is not free");
        assert(1);
    }
    glthread_priority_insert(&vm_page_family->free_block_priority_list_head,
                                &free_block->priority_thread_glue,
                                free_blocks_comparison_function,
                                offset_of(block_meta_data_t, priority_thread_glue));
}

/* Returns a pointer to the first node present in the priority queue*/
static inline block_meta_data_t *mm_get_biggest_free_block_page_family(vm_page_family_t *vm_page_family){
    return vm_page_family->free_block_priority_list_head.head;
}

/*  Function to mark block_meta_data as being allocated for 
    size bytes of application data. Return TRUE if 
    block allocation succeeds */
static vm_bool_t
mm_split_free_data_block_for_allocation(vm_page_family_t *vm_page_family,
                                        block_meta_data_t *block_meta_data,
                                        uint32_t size){
    
    block_meta_data_t *next_block_meta_data = NULL;
    
    assert(block_meta_data->is_free == MM_TRUE);

    if(block_meta_data->block_size < size){
        return MM_FALSE;
    }

    uint32_t remaining_size = block_meta_data->block_size - size;

    block_meta_data->is_free = MM_FALSE;
    block_meta_data->block_size = size;
    /*block_meta_data->offset = remains same */
    //remove_glthread(&block_meta_data->priority_thread_glue);
    glthread_remove(&block_meta_data->priority_thread_glue,
                    &vm_page_family->free_block_priority_list_head.head);

    /*Case 1 : No Split */
    if( remaining_size == 0){
        return MM_TRUE;
    }
    
    /*Case 3 : Partial Split : Soft Internal Fragmentation */
    else if( remaining_size > sizeof(block_meta_data_t) && 
             remaining_size < (sizeof(block_meta_data_t) +vm_page_family->struct_size)){

        next_block_meta_data = NEXT_META_BLOCK_BY_SIZE(block_meta_data);
        next_block_meta_data->is_free = MM_TRUE;
        next_block_meta_data->offset = block_meta_data->offset+
                                        sizeof(block_meta_data_t)+block_meta_data->block_size;
        init_glthread(&next_block_meta_data->priority_thread_glue,
                        offset_of(block_meta_data_t, priority_thread_glue));
        mm_add_free_block_meta_data_to_free_block_list(
                         vm_page_family, next_block_meta_data);
        mm_bind_blocks_for_allocation(block_meta_data, next_block_meta_data);
        //next_block_meta_data->block_size = remaining_size-sizeof(block_meta_data_t);
        //next_block_meta_data->prev_block = block_meta_data;
        //next_block_meta_data->next_block = block_meta_data->next_block;
        //block_meta_data->next_block = next_block_meta_data;

    }
    /*Case 3 : Partial Split : Hard Internal Fragmentation */
    else if( remaining_size < sizeof(block_meta_data_t)){
        /* No need to do anything */
    }
    
    /*Case 2 : Full Split : New Meta block is created  Same as of Soft Internal Fragmentation */
    else{
        next_block_meta_data = NEXT_META_BLOCK_BY_SIZE(block_meta_data);
        next_block_meta_data->is_free = MM_TRUE;
        next_block_meta_data->offset = block_meta_data->offset+
                                        sizeof(block_meta_data_t)+block_meta_data->block_size;
        init_glthread(&next_block_meta_data->priority_thread_glue,
                        offset_of(block_meta_data_t, priority_thread_glue));
        mm_add_free_block_meta_data_to_free_block_list(
                         vm_page_family, next_block_meta_data);
        mm_bind_blocks_for_allocation(block_meta_data, next_block_meta_data);
        //next_block_meta_data->block_size = remaining_size-sizeof(block_meta_data_t);
        //next_block_meta_data->prev_block = block_meta_data;
        //next_block_meta_data->next_block = block_meta_data->next_block;
        //block_meta_data->next_block = next_block_meta_data;
    }
    return MM_TRUE;
}


static block_meta_data_t *
mm_allocate_free_data_block(vm_page_family_t *vm_page_family, uint32_t req_size){

    vm_bool_t status = MM_FALSE;
    vm_page_t *vm_page = NULL;
    block_meta_data_t *biggest_block_meta_data = 
                        mm_get_biggest_free_block_page_family(vm_page_family);
    
    if(!biggest_block_meta_data || biggest_block_meta_data->block_size < req_size){

        /* Add a new page to the page family to satify the request */
        vm_page = mm_family_new_page_add(vm_page_family);

        /* Allocate the free block from theis page*/
        status = mm_split_free_data_data_block_for_allocation(vm_page_family, 
                        &vm_page->block_meta_data, req_size);
        
        if(status)
            return &vm_page->block_meta_data;

        return NULL;
    }
    /* The biigels free meta data block can staisfy the request */
    if(biggest_block_meta_data){
        status = mm_split_free_data_block_for_allocation(vm_page_family,
                        &vm_page->block_meta_data, req_size);
    }

    if(status)
        return biggest_block_meta_data;

    return NULL;

}

/* The public function to be invoked by the application for Dynamic Memory Allocations */
void *
xcalloc(char *struct_name, int units){

    /*Step 1 - search for a page family with struct name provided by the application*/
    vm_page_family_t *pg_family = lookup_page_family_by_name(struct_name);

    if(!pg_family){

        printf("Error : Structure %s is not registered with Memory Manager \n", struct_name);
        return NULL;
    }

    /* Check if application is demanding a chunk of size which exceeds the limit of empty VM data page*/
    if(units * pg_family->struct_size > MAX_PAGE_ALLOCATABLE_MEMORY(1)){
        
        printf("Error : Memory Requested Exceeds Page Size\n");
        return NULL;
    }

    /* Find the page which can satisfy the request */
    block_meta_data_t *free_block_meta_data = NULL;

    free_block_meta_data = mm_allocate_free_data_block(pg_family, units * pg_family->struct_size);

    // this meta block is guardian of the data block which is allocated to the application
    if(free_block_meta_data){
        memset((char *)(free_block_meta_data+1), 0, free_block_meta_data->block_size);
        return (void *)(free_block_meta_data + 1);
    }

    return NULL;
}

//void remove_glthread()

/*
int main(int argc, char **argv)
{
    mm_init();
    printf("VM Page size = %lu\n", SYSTEM_PAGE_SIZE);
    void *addr1 = mm_get_new_vm_page_from_kernel(1);
    void *addr2 = mm_get_new_vm_page_from_kernel(1);
    printf("page 1 = %p, page 2 = %p\n", addr1, addr2);
    return 0;

}
*/