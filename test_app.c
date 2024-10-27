#include <stdio.h>
#include <stdlib.h>
#include "uapi_mm.h"

typedef struct emp_ {
    char name[32];
    uint32_t emp_id;
} emp_t;

typedef struct student_ {
    char name[32];
    uint32_t rollno;
    uint32_t marks_phys;
    uint32_t marks_chem;
    uint32_t marks_maths;
    struct student_ *next;
} student_t;

typedef struct student2_ {
    char name[32];
    uint32_t rollno;
    uint32_t marks_phys;
    uint32_t marks_chem;
    uint32_t marks_maths;
    struct student_ *next;
} student2_t;

int main(int argc, char **argv)
{
    mm_init();
    MM_REG_STRUCT(emp_t);
    MM_REG_STRUCT(student_t);
    MM_REG_STRUCT(student2_t);
    mm_print_registered_page_families();

    XCALLOC(1, empt_t);
    XCALLOC(1, empt_t);
    XCALLOC(1, empt_t);

    XCALLOC(1, student_t);
    XCALLOC(1, student_t);
    
    /*
    for(int i=0;i<500;i++){
        XCALLOC(1,emp_t);
        XCALLOC(1,student_t);
    }
    */

    scanf("\n");
    mm_print_memory_usage(0);
    mm_print_block_usage();
    return 0;
}