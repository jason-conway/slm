#include "slm.h"

static void *xmalloc(size_t size)
{
    void *ptr = malloc(size);
    if (unlikely(!ptr)) {
        abort();
    }
    return ptr;
}

static void *xcalloc(size_t count, size_t size)
{
    void *ptr = calloc(count, size);
    if (unlikely(!ptr)) {
        abort();
    }
    return ptr;
}

static void *xrealloc(void *ptr, size_t size)
{
    void *ptr = realloc(ptr, size);
    if (unlikely(!ptr)) {
        abort();
    }
    return ptr;
}

static void xfree(void *ptr)
{
    free(ptr);
}

slm_vec_t *slm_vec_new(void)
{
    return xcalloc(1, sizeof(slm_vec_t));
}

slm_elem_t *slm_elem_new(void)
{
    return xcalloc(1, sizeof(slm_elem_t));
}

slm_matrix_t *slm_matrix_new(void)
{
    return xcalloc(1, sizeof(slm_matrix_t));
}

void slm_row_free(slm_vec_t *row)
{
    for_each_element_in_row_safe(elem, row) {
        xfree(elem);
    }
    xfree(row);
}

void slm_col_free(slm_vec_t *col)
{
    for_each_element_in_col_safe(elem, col) {
        xfree(elem);
    }
    xfree(col);
}

void slm_matrix_free(slm_matrix_t *matrix)
{
    for_each_row_in_matrix_safe(row, matrix) {
        slm_row_free(row);
    }
    for_each_col_in_matrix_safe(col, matrix) {
        col->last = NULL;
        col->first = NULL;
        slm_col_free(col);
    }

    xfree(matrix->rows);
    xfree(matrix->cols);
    xfree(matrix);
}

slm_vec_t *slm_get_row(slm_matrix_t *matrix, size_t m)
{
    return unlikely(m >= matrix->rows_size) ? NULL : matrix->rows[m];
}

slm_vec_t *slm_get_col(slm_matrix_t *matrix, size_t n)
{
    return unlikely(n >= matrix->cols_size) ? NULL : matrix->cols[n];
}

slm_vec_t *slm_row_dupl(slm_vec_t *row)
{
    slm_vec_t *new_row = slm_vec_new();
    for_each_element_in_row(cell, row) {
        slm_row_insert(new_row, cell->j);
    }
    return new_row;
}

// Insert `element` into the row `row` at index (column) `n`
static inline slm_elem_t *slm_insert_into_row(slm_vec_t *row, size_t n, slm_elem_t *element)
{
    slm_elem_t *elem = element;
    if (!row->last) {
        element->j = n;
        row->first = element;
        row->last = element;
        element->next_col = NULL;
        element->prev_col = NULL;
        row->length++;
    }
    else if (row->last->j < n) {
        element->j = n;
        row->last->next_col = element;
        element->prev_col = row->last;
        row->last = element;
        element->next_col = NULL;
        row->length++;
    }
    else if (row->first->j > n) {
        element->j = n;
        row->first->prev_col = element;
        element->next_col = row->first;
        row->first = element;
        element->prev_col = NULL;
        row->length++;
    }
    else {
        slm_elem_t *itr = row->first;
        for (; itr->j < n; itr = itr->next_col);
        if (itr->j > n) {
            element->j = n;
            itr = itr->prev_col;
            itr->next_col->prev_col = element;
            element->next_col = itr->next_col;
            itr->next_col = element;
            element->prev_col = itr;
            row->length++;
        }
        else {
            element = itr;
        }
    }
    if (unlikely(elem != element)) {
        xfree(elem);
        elem = NULL;
    }
    return elem;
}

// Insert `element` into the column `col` at index (row) `m`
static inline slm_elem_t *slm_insert_into_col(slm_vec_t *col, size_t m, slm_elem_t *element)
{
    slm_elem_t *elem = element;
    if (!col->last) {
        element->i = m;
        col->first = element;
        col->last = element;
        element->next_row = NULL;
        element->prev_row = NULL;
        col->length++;
    }
    else if (col->last->i < m) {
        element->i = m;
        col->last->next_row = element;
        element->prev_row = col->last;
        col->last = element;
        element->next_row = NULL;
        col->length++;
    }
    else if (col->first->i > m) {
        element->i = m;
        col->first->prev_row = element;
        element->next_row = col->first;
        col->first = element;
        element->prev_row = NULL;
        col->length++;
    }
    else {
        slm_elem_t *itr = col->first;
        for (; itr->i < m; itr = itr->next_row);
        if (itr->i > m) {
            element->i = m;
            itr = itr->prev_row;
            itr->next_row->prev_row = element;
            element->next_row = itr->next_row;
            itr->next_row = element;
            element->prev_row = itr;
            col->length++;
        }
        else {
            element = itr;
        }
    }
    if (unlikely(elem != element)) {
        xfree(elem);
        elem = NULL;
    }
    return elem;
}

slm_elem_t *slm_row_insert(slm_vec_t *row, size_t n)
{
    slm_elem_t *element = slm_elem_new();
    return slm_insert_into_row(row, n, element);
}

void slm_row_remove(slm_vec_t *row, size_t index)
{
    slm_elem_t *cell = row->first;
    for (; cell && cell->j < index; cell = cell->next_col);
    if (cell && (cell->j == index)) {
        if (!cell->prev_col) {
            row->first = cell->next_col;
        }
        else {
            cell->prev_col->next_col = cell->next_col;
        }
        if (!cell->next_col) {
            row->last = cell->prev_col;
        }
        else {
            cell->next_col->prev_col = cell->prev_col;
        }
        row->length--;
        xfree(cell);
    }
}

void slm_matrix_resize_row(slm_matrix_t *matrix, size_t m)
{
    const size_t size = (2 * matrix->rows_size) > (m + 1) ? (2 * matrix->rows_size) : (m + 1);
    matrix->rows = xrealloc(matrix->rows, sizeof(slm_vec_t *) * size);
    for (size_t i = matrix->rows_size; i < size; i++) {
        matrix->rows[i] = NULL;
    }
    matrix->rows_size = size;
}

void slm_matrix_resize_col(slm_matrix_t *matrix, size_t n)
{
    const size_t size = (2 * matrix->cols_size) > (n + 1) ? (2 * matrix->cols_size) : (n + 1);
    matrix->cols = xrealloc(matrix->cols, sizeof(slm_vec_t *) * size);
    for (size_t i = matrix->cols_size; i < size; i++) {
        matrix->cols[i] = NULL;
    }
    matrix->cols_size = size;
}

void slm_matrix_resize(slm_matrix_t *matrix, size_t m, size_t n)
{
    if (m >= matrix->rows_size) {
        slm_matrix_resize_row(matrix, m);
    }
    if (n >= matrix->cols_size) {
        slm_matrix_resize_col(matrix, n);
    }
}

slm_matrix_t *slm_matrix_dupl(slm_matrix_t *matrix)
{
    slm_matrix_t *dupl = slm_matrix_new();
    if (matrix->last_row) {
        slm_matrix_resize(dupl, matrix->last_row->index, matrix->last_col->index);
        for_each_row_in_matrix(row, matrix) {
            for_each_element_in_row(cell, row) {
                slm_matrix_insert(dupl, cell->i, cell->j);
            }
        }
    }
    return dupl;
}

// Add a new row, `row` to `matrix` at index `m`
static inline void slm_add_row(slm_matrix_t *matrix, slm_vec_t *row, size_t m)
{
    if (!matrix->last_row) {
        row->index = m;
        matrix->first_row = row;
        matrix->last_row = row;
        row->next = NULL;
        row->prev = NULL;
        matrix->m++;
    }
    else if (matrix->last_row->index < m) {
        row->index = m;
        matrix->last_row->next = row;
        row->prev = matrix->last_row;
        matrix->last_row = row;
        row->next = NULL;
        matrix->m++;
    }
    else if (matrix->first_row->index > m) {
        row->index = m;
        matrix->first_row->prev = row;
        row->next = matrix->first_row;
        matrix->first_row = row;
        row->prev = NULL;
        matrix->m++;
    }
    else {
        slm_vec_t *itr = matrix->first_row;
        for (; itr->index < m; itr = itr->next);
        if (itr->index > m) {
            row->index = m;
            itr = itr->prev;
            itr->next->prev = row;
            row->next = itr->next;
            itr->next = row;
            row->prev = itr;
            matrix->m++;
        }
    }
}

// Add a new column, `col` to `matrix` at index `n`
static inline void slm_add_col(slm_matrix_t *matrix, slm_vec_t *col, size_t n)
{
    if (!matrix->last_col) {
        col->index = n;
        matrix->first_col = col;
        matrix->last_col = col;
        col->next = NULL;
        col->prev = NULL;
        matrix->n++;
    }
    else if (matrix->last_col->index < n) {
        col->index = n;
        matrix->last_col->next = col;
        col->prev = matrix->last_col;
        matrix->last_col = col;
        col->next = NULL;
        matrix->n++;
    }
    else if (matrix->first_col->index > n) {
        col->index = n;
        matrix->first_col->prev = col;
        col->next = matrix->first_col;
        matrix->first_col = col;
        col->prev = NULL;
        matrix->n++;
    }
    else {
        slm_vec_t *itr = matrix->first_col;
        for (; itr->index < n; itr = itr->next);
        if (itr->index > n) {
            col->index = n;
            itr = itr->prev;
            itr->next->prev = col;
            col->next = itr->next;
            itr->next = col;
            col->prev = itr;
            matrix->n++;
        }
    }
}

void slm_matrix_insert(slm_matrix_t *matrix, size_t m, size_t n)
{
    if ((m >= matrix->rows_size) || (n >= matrix->cols_size)) {
        slm_matrix_resize(matrix, m, n);
    }

    slm_vec_t *row = matrix->rows[m];
    if (!row) {
        matrix->rows[m] = slm_vec_new();
        row = matrix->rows[m];
        row->index = m;
        slm_add_row(matrix, row, m);
    }

    slm_vec_t *col = matrix->cols[n];
    if (!col) {
        matrix->cols[n] = slm_vec_new();
        col = matrix->cols[n];
        col->index = n;
        slm_add_col(matrix, col, n);
    }

    slm_elem_t *element = slm_elem_new();
    if (slm_insert_into_row(row, n, element)) {
        slm_insert_into_col(col, m, element);
    }
}

void slm_matrix_remove_row(slm_matrix_t *matrix, size_t m)
{
    slm_vec_t *row = slm_get_row(matrix, m);
    if (row) {
        for_each_element_in_row_safe(elem, row) {
            slm_vec_t *col = slm_get_col(matrix, elem->j);
            if (!elem->prev_row) {
                col->first = elem->next_row;
            }
            else {
                elem->prev_row->next_row = elem->next_row;
            }
            if (!elem->next_row) {
                col->last = elem->prev_row;
            }
            else {
                elem->next_row->prev_row = elem->prev_row;
            }
            col->length--;

            xfree(elem);
            if (!col->first) {
                matrix->cols[col->index] = NULL;
                if (!col->prev) {
                    matrix->first_col = col->next;
                }
                else {
                    col->prev->next = col->next;
                }
                if (!col->next) {
                    matrix->last_col = col->prev;
                }
                else {
                    col->next->prev = col->prev;
                }
                matrix->n--;
                col->last = NULL;
                col->first = NULL;
                slm_col_free(col);
            }
        }
        matrix->rows[m] = NULL;

        if (!row->prev) {
            matrix->first_row = row->next;
        }
        else {
            row->prev->next = row->next;
        }
        if (!row->next) {
            matrix->last_row = row->prev;
        }
        else {
            row->next->prev = row->prev;
        }
        matrix->m--;

        row->last = NULL;
        row->first = NULL;
        slm_row_free(row);
    }
}

void slm_matrix_remove_col(slm_matrix_t *matrix, size_t n)
{
    slm_vec_t *col = slm_get_col(matrix, n);
    if (col) {
        for_each_element_in_col_safe(elem, col) {
            slm_vec_t *row = slm_get_row(matrix, elem->i);
            if (!elem->prev_col) {
                row->first = elem->next_col;
            }
            else {
                elem->prev_col->next_col = elem->next_col;
            }
            if (!elem->next_col) {
                row->last = elem->prev_col;
            }
            else {
                elem->next_col->prev_col = elem->prev_col;
            }
            row->length--;

            xfree(elem);
            if (!row->first) {
                matrix->rows[row->index] = NULL;

                if (!row->prev) {
                    matrix->first_row = row->next;
                }
                else {
                    row->prev->next = row->next;
                }
                if (!row->next) {
                    matrix->last_row = row->prev;
                }
                else {
                    row->next->prev = row->prev;
                }
                matrix->m--;

                row->last = NULL;
                row->first = NULL;
                slm_row_free(row);
            }
        }
        matrix->cols[n] = NULL;

        if (!col->prev) {
            matrix->first_col = col->next;
        }
        else {
            col->prev->next = col->next;
        }
        if (!col->next) {
            matrix->last_col = col->prev;
        }
        else {
            col->next->prev = col->prev;
        }
        matrix->n--;

        col->last = NULL;
        col->first = NULL;
        slm_col_free(col);
    }
}

static slm_elem_t *slm_row_find(slm_vec_t *row, size_t n)
{
    slm_elem_t *cell = row->first;
    for (; cell && cell->j < n; cell = cell->next_col);
    return (cell && cell->j == n) ? cell : NULL;
}

static void slm_print(FILE *fp, slm_matrix_t *matrix)
{
    for_each_row_in_matrix(row, matrix) {
        fprintf(fp, "%-7zu\t", row->index);
        for_each_col_in_matrix(col, matrix) {
            char c = slm_row_find(row, col->index) ? '1' : '-';
            putc(c, fp);
        }
        putc('\n', fp);
    }
}

void slm_matrix_print(FILE *f, slm_matrix_t *matrix)
{
    if (!matrix->m || !matrix->n) {
        return;
    }

    fprintf(f, "%zu rows by %zu cols\n", matrix->m, matrix->n);
    slm_print(f, matrix);
}

size_t slm_total_elements(slm_matrix_t *matrix)
{
    size_t count = 0;
    for_each_row_in_matrix(row, matrix) {
        count += row->length;
    }
    return count;
}

static bool slm_matrix_reachability(slm_matrix_t *matrix, slm_vec_t *row)
{
    size_t rows_visited = 0;
    size_t cols_visited = 0;
    bool out = false;

    typedef struct stack_frame_t {
        slm_vec_t *row;
        slm_vec_t *col;
        slm_elem_t *xm; // row-iterating element
        slm_elem_t *xn; // column-iterating element
        bool stat;
        void *ret_addr;
    } stack_frame_t;

    ptrdiff_t stack_depth = 0;
    ptrdiff_t frame_capacity = 4096;
    stack_frame_t *stk = xmalloc(frame_capacity * sizeof(stack_frame_t));
    stk[stack_depth] = (stack_frame_t) {
        .row = row,
        .col = NULL,
        .xm = NULL,
        .xn = NULL,
        .stat = false,
        .ret_addr = &&ret_addr_0
    };

next_frame:
    while (stack_depth >= 0) {
        if (unlikely(stack_depth + 2 >= frame_capacity)) {
            frame_capacity *= 2;
            stk = xrealloc(stk, frame_capacity * sizeof(stack_frame_t));
            fprintf(stderr, "resizing %s stack for %" PRIiPTR " frames\n", __FUNCTION__, frame_capacity);
        }
        stack_frame_t stk_top = stk[stack_depth--];
        row = stk_top.row;
        slm_vec_t *col = stk_top.col;
        slm_elem_t *xm = stk_top.xm;
        slm_elem_t *xn = stk_top.xn;
        bool stat = stk_top.stat;
        slm_vec_t *node = NULL;
        goto *stk_top.ret_addr;

ret_addr_0:
        if (!row->flag) {
            row->flag = true;
            if (++rows_visited == matrix->m) {
                out = true;
                goto next_frame;
            }
            for_each_stack_element_in_row (xm, row) {
                col = slm_get_col(matrix, xm->j);
                if (!col->flag) {
                    stat = false;
                    col->flag = true;
                    if (++cols_visited == matrix->n) {
                        stat = true;
                        goto next_col;
                    }
                    for_each_stack_element_in_col (xn, col) {
                        node = slm_get_row(matrix, xn->i);
                        if (!node->flag) {
                            stk[++stack_depth] = (stack_frame_t) {
                                .row = row,
                                .col = col,
                                .xm = xm,
                                .xn = xn,
                                .stat = stat,
                                .ret_addr = &&ret_addr_1
                            };
                            stk[++stack_depth] = (stack_frame_t) {
                                .row = node,
                                .col = NULL,
                                .xm = NULL,
                                .xn = NULL,
                                .stat = false,
                                .ret_addr = &&ret_addr_0
                            };
                            goto next_frame;
ret_addr_1:
                            if (out) {
                                stat = true;
                                goto next_col;
                            }
                        }
                    }
                    next_col:
                    if (stat) {
                        out = true;
                        goto next_frame;
                    }
                }
            }
        }
        out = false;
        goto next_frame;
    }

    free(stk);
    return out;
}

bool slm_diagonal_partition(slm_matrix_t *matrix, slm_matrix_t **restrict A, slm_matrix_t **restrict B)
{
    if (!matrix->m) {
        return false;
    }

    for_each_row_in_matrix(row, matrix) {
        row->flag = false;
    }
    for_each_col_in_matrix(col, matrix) {
        col->flag = false;
    }

    if (slm_matrix_reachability(matrix, matrix->first_row)) {
        return false;
    }

    *A = slm_matrix_new();
    *B = slm_matrix_new();

    for_each_row_in_matrix(row, matrix) {
        slm_matrix_t *blk = row->flag ? *A : *B;
        for_each_element_in_row(elem, row) {
            slm_matrix_insert(blk, elem->i, elem->j);
        }
    }

    if ((*A)->n > (*B)->n) {
        slm_matrix_t *swap = *A;
        *A = *B;
        *B = swap;
    }
    return true;
}
