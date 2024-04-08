#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

typedef struct slm_elem_t slm_elem_t;
struct slm_elem_t {
    size_t i; // entry row index
    size_t j; // entry column index
    slm_elem_t *next_row;
    slm_elem_t *prev_row;
    slm_elem_t *next_col;
    slm_elem_t *prev_col;
};

typedef struct slm_vec_t slm_vec_t;
struct slm_vec_t {
    slm_vec_t *next;
    slm_vec_t *prev;
    slm_elem_t *first;
    slm_elem_t *last;
    size_t index; // row / column number
    size_t length; // total row / column elements
    bool flag; // indicate reachability
};

typedef struct slm_matrix_t slm_matrix_t;
struct slm_matrix_t {
    slm_vec_t *first_row;
    slm_vec_t *last_row;
    slm_vec_t *first_col;
    slm_vec_t *last_col;
    slm_vec_t **rows; // list of row-list heads
    slm_vec_t **cols; // list of column-list heads
    size_t rows_size; // current memory allocation for all rows
    size_t cols_size; // current memory allocation for all columns
    size_t m; // number of rows
    size_t n; // number of columns
};

// Optionally supply custom allocator functions
void slm_init_allocator(void *(*calloc)(size_t, size_t), void *(*realloc)(void *, size_t), void (*free)(void *));

// Create an empty matrix
slm_matrix_t *slm_matrix_new(void);

// Free a matrix, including all allocated rows, columns, and elements
void slm_matrix_free(slm_matrix_t *matrix);

// Create a new vector, capable of being either a row or column
slm_vec_t *slm_vec_new(void);

// Free a row, including all allocated elements
void slm_row_free(slm_vec_t *row);

// Free a column, including all allocated elements
void slm_col_free(slm_vec_t *row);

// Create a new matrix element and insert into row `row` at column index `n`
// Returns a pointer to the created element
slm_elem_t *slm_row_insert(slm_vec_t *row, size_t n);

// Remove (and free memory allocated for) the element at column index `n` from the row `row`
void slm_row_remove(slm_vec_t *row, size_t n);

// Duplicate row `row`
slm_vec_t *slm_row_dupl(slm_vec_t *row);

// Create a new matrix element
slm_elem_t *slm_elem_new(void);

// Return the `m`th row of matrix `matrix`
slm_vec_t *slm_get_row(slm_matrix_t *matrix, size_t m);

// Return the `n`th column of matrix `matrix`
slm_vec_t *slm_get_col(slm_matrix_t *matrix, size_t n);

// Duplicate matrix `matrix`
slm_matrix_t *slm_matrix_dupl(slm_matrix_t *matrix);

// Remove (and free memory allocated for) the row at index `m` from matrix `matrix`
void slm_matrix_remove_row(slm_matrix_t *matrix, size_t m);

// Remove (and free memory allocated for) the column at index `n` from matrix `matrix`
void slm_matrix_remove_col(slm_matrix_t *matrix, size_t n);

// Create a new matrix element and insert into matrix `matrix` at
// row index `m` and column index `n`
void slm_matrix_insert(slm_matrix_t *matrix, size_t m, size_t n);

// Return the total number of elements in matrix `matrix`
size_t slm_total_elements(slm_matrix_t *matrix);

// Dump matrix to file `f`
void slm_matrix_print(FILE *f, slm_matrix_t *matrix);

// Perform a partitioning of matrix `matrix` into a diagonal block matrix of the form
// | `A` 0 |
// | 0 `B` |
// with `A` being the maximal block reduction of `matrix` and `B` being the remainder
bool slm_diagonal_partition(slm_matrix_t *matrix, slm_matrix_t **restrict A, slm_matrix_t **restrict B);

#define for_each_element_in_vec_safe(elem, vec, next_field) \
    for (slm_elem_t *elem = (vec)->first, *next_##elem = NULL; \
         (elem) && ((next_##elem) = ((elem)->next_##next_field), 1); \
         (elem) = (next_##elem))

#define for_each_element_in_row_safe(elem, vec) \
    for (slm_elem_t *elem = (vec)->first, *next_##elem = NULL; \
         (elem) && ((next_##elem) = ((elem)->next_col), 1); \
         (elem) = (next_##elem))

#define for_each_element_in_col_safe(elem, vec) \
    for (slm_elem_t *elem = (vec)->first, *next_##elem = NULL; \
         (elem) && ((next_##elem) = ((elem)->next_row), 1); \
         (elem) = (next_##elem))

#define for_each_col_in_matrix_safe(col, matrix) \
    for (slm_vec_t *col = (matrix)->first_col, *next_##col = NULL; \
         (col) && ((next_##col) = ((col)->next), 1); \
         (col) = (next_##col))

#define for_each_row_in_matrix_safe(row, matrix) \
    for (slm_vec_t *row = (matrix)->first_row, *next_##row = NULL; \
         (row) && ((next_##row) = ((row)->next), 1); \
         (row) = (next_##row))

#define for_each_row_in_matrix(row, matrix) \
    for (slm_vec_t *row = (matrix)->first_row; row; row = row->next)

#define for_each_col_in_matrix(col, matrix) \
    for (slm_vec_t *col = (matrix)->first_col; col; col = col->next)

#define for_each_element_in_row(elem, row) \
    for (slm_elem_t *elem = (row)->first; elem; elem = elem->next_col)

#define for_each_element_in_col(elem, col) \
    for (slm_elem_t *elem = (col)->first; elem; elem = elem->next_row)

#define for_each_stack_element_in_row(elem, row) \
    for (elem = (row)->first; elem; elem = elem->next_col)

#define for_each_stack_element_in_col(elem, col) \
    for (elem = (col)->first; elem; elem = elem->next_row)
