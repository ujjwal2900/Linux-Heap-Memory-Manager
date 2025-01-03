#ifndef __UAPI_MM__
#define __UAPI_MM__

#include <stdint.h>

void *
xcalloc(char *struct_name, int units);

void xfree(void *app_data);

#define XCALLOC(units, struct_name)    \
        (xcalloc(#struct_name, units))

#define XFREE(pointer_to_be_freed)      \
        xfree(pointer_to_be_freed)

/*Initialization Functions*/
void mm_init();

/*Registration Function*/
void mm_instantiate_new_page_family(char *struc_name, uint32_t struct_size);

/*To print the Registered families*/
void mm_print_registered_page_families();

#define MM_REG_STRUCT(struct_name)  \
        (mm_instantiate_new_page_family(#struct_name, sizeof(struct_name)))

void mm_print_memory_usage(char *struct_name);
void mm_print_registered_page_families();
void mm_print_block_usage();


#endif /* __UAPI_MM__ */

