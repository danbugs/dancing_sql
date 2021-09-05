#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <emscripten.h>
#include "dcs.h"

typedef enum
{
    STATEMENT_INSERT,
    STATEMENT_SELECT,
} StatementType;

#define COLUMN_CONTENT_SIZE (int)255
typedef struct
{
    int id;
    char content[COLUMN_CONTENT_SIZE + 1];
} Row;

#define size_of_attribute(Struct, Attribute) sizeof(((Struct *)0)->Attribute)
#define ID_SIZE (int)size_of_attribute(Row, id)
#define CONTENT_SIZE (int)size_of_attribute(Row, content)
#define ROW_SIZE (int)(ID_SIZE + CONTENT_SIZE)
#define ID_OFFSET (int)0
#define CONTENT_OFFSET (int)(ID_OFFSET + ID_SIZE)
typedef struct
{
    StatementType type;
    Row row_to_insert;
} Statement;

void serialize_row(Row *source, void *destination)
{
    memcpy(destination + ID_OFFSET, &(source->id), ID_SIZE);
    memcpy(destination + CONTENT_OFFSET, &(source->content), CONTENT_SIZE);
}

void deserialize_row(void *source, Row *destination)
{
    memcpy(&(destination->id), source + ID_OFFSET, ID_SIZE);
    memcpy(&(destination->content), source + CONTENT_OFFSET, CONTENT_SIZE);
}

typedef enum
{
    PREPARE_SUCCESSFUL,
    PREPARE_UNRECOGNIZED_COMMAND,
    PREPARE_SYNTAX_ERROR,
} PrepareResult;

PrepareResult prepare_statement(char *rs, Statement *statement)
{
    if (strncmp(rs, "INSERT", 6) == 0)
    {
        printf("[INFO ] 'INSERT' command received\n");
        statement->type = STATEMENT_INSERT;
        int args_assigned = sscanf(
            rs,
            "INSERT %d '%255[^\t\n]'", // truncates input to COLUM_CONTENT_SIZE (255)
            &(statement->row_to_insert.id),
            &(statement->row_to_insert.content));

        if (args_assigned < 2)
        {
            return PREPARE_SYNTAX_ERROR;
        }
        return PREPARE_SUCCESSFUL;
    }

    if (strncmp(rs, "SELECT", 6) == 0)
    {
        printf("[INFO ] 'SELECT' command received\n");
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESSFUL;
    }

    return PREPARE_UNRECOGNIZED_COMMAND;
}

typedef enum
{
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL,
} ExecuteResult;

#define PAGE_SIZE (int)4096
#define ROWS_PER_PAGE (int)(PAGE_SIZE / ROW_SIZE)
#define TABLE_MAX_ROWS (int)(ROWS_PER_PAGE * TABLE_MAX_PAGES)
void* row_slot(Table* table, int row_num) {
    int page_num = row_num / ROWS_PER_PAGE;
    void* page = table->pages[page_num];
    if (page == NULL) {
        page = table->pages[page_num] = malloc(PAGE_SIZE);
    }

    int row_offset = row_num % ROWS_PER_PAGE;
    int byte_offset = row_offset * ROW_SIZE;
    return page + byte_offset;
}

ExecuteResult execute_insert(Statement *statement, Table *table)
{
    if (table->num_rows >= TABLE_MAX_ROWS) {
        return EXECUTE_TABLE_FULL;
    }

    Row* row_to_insert = &(statement->row_to_insert);
    serialize_row(row_to_insert, row_slot(table, table->num_rows));
    table->num_rows += 1;

    return EXECUTE_SUCCESS;
}

void print_row(Row* row) {
    printf("(%d, %s)\n", row->id, row->content);
}

ExecuteResult execute_select(Statement *statement, Table *table)
{
    Row row;
    for (int i = 0; i < table->num_rows; i++) {
        deserialize_row(row_slot(table, i), &row);
        print_row(&row);
    }

    return EXECUTE_SUCCESS;
}

ExecuteResult execute_statement(Statement *statement, Table *table)
{
    switch (statement->type)
    {
    case (STATEMENT_INSERT):
        return execute_insert(statement, table);
    case (STATEMENT_SELECT):
        return execute_select(statement, table);
    }
}

EMSCRIPTEN_KEEPALIVE
void execute_sql(sql_t raw_sql, Table *table)
{
    Statement statement;
    switch (prepare_statement(raw_sql, &statement))
    {
    case (PREPARE_SUCCESSFUL):
        printf("[INFO ] Successfully parsed the statement.\n");
        break;
    case (PREPARE_UNRECOGNIZED_COMMAND):
        printf("[ERROR ] Unrecognized keyword at start of '%s'.\n", raw_sql);
        return;
    case (PREPARE_SYNTAX_ERROR):
        printf("[ERROR ] Syntax error. Could not parse statement.\n");
        return;
    }

    switch (execute_statement(&statement, table))
    {
    case (EXECUTE_SUCCESS):
        printf("[INFO ] Successfully executed the command.\n");
        break;
    case (EXECUTE_TABLE_FULL):
        printf("[ERROR ]  Table full.\n");
        return;
    }
}

EMSCRIPTEN_KEEPALIVE
Table *new_table()
{
    Table *table = malloc(sizeof(Table));
    table->num_rows = 0;
    for (int i = 0; i < TABLE_MAX_PAGES; i++)
    {
        table->pages[i] = NULL;
    }
    return table;
}

EMSCRIPTEN_KEEPALIVE
void free_table(Table *table)
{
    for (int i = 0; table->pages[i]; i++)
    {
        free(table->pages[i]);
    }
    free(table);
}