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

    emp_t *emp1 = XCALLOC(1, emp_t);
    emp_t *emp2 = XCALLOC(1, emp_t);
    emp_t *emp3 = XCALLOC(1, emp_t);
    emp_t *emp4 = XCALLOC(1, emp_t);

    student_t *stud1 = XCALLOC(1, student_t);
    student_t *stud2 = XCALLOC(1, student_t);
    
    //mm_print_memory_usage(0);

    mm_print_memory_usage(0);
    xfree(emp1);
    xfree(emp2);
    //mm_print_block_usage();
    
    #if 1
    for(int i=0;i<50;i++){
        XCALLOC(1,student_t);
    }
    #endif

    xfree(stud1);

    mm_print_memory_usage(0);
    mm_print_block_usage();
    return 0;
}