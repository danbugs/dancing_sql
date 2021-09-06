/** \file
    An SQLite Clone compiled to WebAssembly.
    Users interact with it through the \c execute_sql , 
    \c open_table , and \c close_table functions,
    together with the \c sql_t and \c SQL macros in dcs.h. 
*/

#include <stdio.h> // sprintf, sscanf, printf
#include <stdlib.h> // EXIT_FAILURE, free, malloc, and exit
#include <string.h> // strlen, strncmp, memcpy
#include <emscripten.h> // EMSCRIPTEN_KEEPALIVE, EM_ASM_
#include <errno.h> // errno
#include <fcntl.h> // open, O_RDWR, S_IWUSR, and I_RUSR
#include <unistd.h> // lseek, read, write, and close
#include "dcs.h" // TABLE_MAX_PAGES, sql_t, Pager, and Table

/** \mainpage
    \n
    To view the source code, refer to: https://github.com/danbugs/dancing_sql \n
    For instructions on using DCS in your own project, refer to:
   https://github.com/danbugs/dancing_sql#readme \n
   Huge thanks to Connor Stack for their "writing a sqlite clone from scratch in C" 
   tutorial — this project is heavily based on it.
   For more info, view: https://cstack.github.io/db_tutorial/.
*/

/** Helper macro for getting the size of a struct
    attribute.
    Note: We are casting 0 as a specific struct 
    pointer to get the offset to one of
    that struct's attributes.
*/
#define size_of_attribute(Struct, Attribute) sizeof(((Struct *)0)->Attribute)

/** Size of the \c id attribute of the \c Row struct.
*/
#define ID_SIZE (int)size_of_attribute(Row, id)

/** Size of the \c content attribute of the \c Row struct.
*/
#define CONTENT_SIZE (int)size_of_attribute(Row, content)

/** Size of the \c Row struct.
*/
#define ROW_SIZE (int)(ID_SIZE + CONTENT_SIZE)

/** Memory offset for the \c id attribute 
    relative to the \c Row struct.
*/
#define ID_OFFSET (int)0

/** Memory offset for the \c content attribute 
    relative to the \c Row struct.
*/
#define CONTENT_OFFSET (int)(ID_OFFSET + ID_SIZE)

/** Convert to compact representation in memory.
*/
void serialize_row(Row *source, void *destination)
{
    memcpy(destination + ID_OFFSET, &(source->id), ID_SIZE);
    strncpy(destination + CONTENT_OFFSET, source->content, CONTENT_SIZE);
}

/** Convert from compact representation in memory.
*/
void deserialize_row(void *source, Row *destination)
{
    memcpy(&(destination->id), source + ID_OFFSET, ID_SIZE);
    memcpy(&(destination->content), source + CONTENT_OFFSET, CONTENT_SIZE);
}

/** Possible results for the \c prepare_statement function.
    - \c PREPARE_SUCCESSFUL : indicates parsing worked.
    - \c PREPARE_UNRECOGNIZED_COMMAND : indicates the given command
    was not recognized (note: commands are case-sensitive).
    - \c PREPARE_SYNTAX_ERROR : indicates the command was recognized
    but there were syntactical errors (e.g., too few arguments passed
    to the in the case of a \c STATEMENT_INSER ).
*/
typedef enum
{
    PREPARE_SUCCESSFUL,
    PREPARE_UNRECOGNIZED_COMMAND,
    PREPARE_SYNTAX_ERROR,
} PrepareResult;

/** Recognizes \c STATEMENT_INSERT and \c STATEMENT_SELECT SQL commands 
    and sets the \c type attribute of the \c Statement struct. In case of
    a \c STATEMENT_INSERT , parses statement into the \c row_to_insert
    attribute of the \c Statement struct.
*/
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
            (char *)&(statement->row_to_insert.content));

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

/** Possible results for the \c execute_* functions.
    - \c EXECUTE_SUCCESS : SQL command was executed.
    - \c EXECUTE_TABLE_FULL : SQL \c STATEMENT_INSERT could
    not be executed because the table is full 
    (1 Table = 4096 Bytes/Page (i.e., 4KB) * 100 Pages = 400KB).
*/
typedef enum
{
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL,
} ExecuteResult;

/** The number of rows per page is determined by the \c PAGE_SIZE
    divided by the number size of the \c Row struct.
*/
#define ROWS_PER_PAGE (int)(PAGE_SIZE / ROW_SIZE)

/** The maximum number of rows is defined by the maximum number of
    pages multiplied by the number of rows per page.
*/
#define TABLE_MAX_ROWS (int)(ROWS_PER_PAGE * TABLE_MAX_PAGES)

/** Access pager page at given offset or malloc
    new page if necessary while handling cache misses.
*/
void *get_page(Pager *pager, int page_offset)
{
    if (page_offset > TABLE_MAX_PAGES)
    {
        printf("[ERROR ] Tried to fetch page number out of bounds. %d > %d.\n", page_offset, TABLE_MAX_PAGES);
        exit(EXIT_FAILURE);
    }

    if (pager->pages[page_offset] == NULL) // page isn't cached
    {
        void *page = malloc(PAGE_SIZE);
        int num_pages = pager->file_length / PAGE_SIZE;

        if (pager->file_length % PAGE_SIZE)
        {
            num_pages += 1;
        }
        // ^^^ if file_length is a multiple of page_size,
        // that means all our current pages are full.

        if (page_offset <= num_pages)
        {
            lseek(pager->file_descriptor, page_offset * PAGE_SIZE, SEEK_SET);
            // ^^^ set file offset to page currently being read.
            int bytes_read = (int)read(pager->file_descriptor, page, PAGE_SIZE);
            // ^^^ cache current page.
            if (bytes_read == -1)
            {
                printf("[ERROR ] Error reading table: %d.\n", errno);
                exit(EXIT_FAILURE);
            }
        }
        // ^^^ only cache the page if the page is partially full

        pager->pages[page_offset] = page;
        // ^^^ if it is full, we just malloc a new page
    }

    return pager->pages[page_offset];
}

/** Helper function to figure out where to write to.
*/
void *row_slot(Table *table, int row_num)
{
    int page_offset = row_num / ROWS_PER_PAGE;
    void *page = get_page(table->pager, page_offset);
    int row_offset = row_num % ROWS_PER_PAGE;
    int byte_offset = row_offset * ROW_SIZE;
    return page + byte_offset;
}

/** Opens database file and keeps track of its' size.
*/
Pager *pager_open(const char *filename)
{
    int fd = open(filename,
                  O_RDWR,       // READ/WRITE permission
                  S_IWUSR |     // WRITE persmission for user
                      S_IRUSR); // READ permission for user

    if (fd == -1)
    {
        printf("[ERROR ] Unable to open file.\n");
        exit(EXIT_FAILURE);
    }

    int file_length = (int)lseek(fd, 0, SEEK_END);
    // ^^^ will get file_length

    Pager *pager = malloc(sizeof(Pager));
    pager->file_descriptor = fd;
    pager->file_length = file_length;

    for (int i = 0; i < TABLE_MAX_PAGES; i++)
    {
        pager->pages[i] = NULL;
    }
    // ^^^ clear junk data from cache

    return pager;
}

/** Flushes all malloc-ed pages to the table database file
*/
void pager_flush(Pager *pager, int page_offset, int size)
{
    if (pager->pages[page_offset] == NULL)
    {
        printf("[ERROR ] Tried to save null page.\n");
        exit(EXIT_FAILURE);
    }

    int offset = (int)lseek(pager->file_descriptor, page_offset * PAGE_SIZE, SEEK_SET);
    // ^^^ set offset where to save page to in file

    if (offset == -1)
    {
        printf("[ERROR ] Error seeking: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    int bytes_written = (int)write(pager->file_descriptor, pager->pages[page_offset], size);
    // ^^^ write page at page_offset to file

    if (bytes_written == -1)
    {
        printf("[ERROR ] Error writing: %d.\n", errno);
        exit(EXIT_FAILURE);
    }
}

/** Handles a \c STATEMENT_INSERT .
*/
ExecuteResult execute_insert(Statement *statement, Table *table)
{
    if (table->num_rows >= TABLE_MAX_ROWS)
    {
        return EXECUTE_TABLE_FULL;
    }

    Row *row_to_insert = &(statement->row_to_insert);
    serialize_row(row_to_insert, row_slot(table, table->num_rows));
    table->num_rows += 1;

    return EXECUTE_SUCCESS;
}

/** Handles a \c STATEMENT_SELECT .
*/
ExecuteResult execute_select(Statement *statement, Table *table)
{
    Row row;
    for (int i = 0; i < table->num_rows; i++)
    {
        deserialize_row(row_slot(table, i), &row);
        sprintf(statement->select_result, "%s\n%d, %s", statement->select_result, row.id, row.content);
    }

    return EXECUTE_SUCCESS;
}

/** Handles a generic SQL statement
    and determines whether it is a
    \c STATEMENT_INSERT or \c STATEMENT_SELECT .
*/
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

/** Parses and executes some SQL at a given table.
*/
EMSCRIPTEN_KEEPALIVE
Statement execute_sql(sql_t raw_sql, Table *table)
{
    Statement statement;
    switch (prepare_statement(raw_sql, &statement))
    {
    case (PREPARE_SUCCESSFUL):
        printf("[INFO ] Successfully parsed the statement.\n");
        break;
    case (PREPARE_UNRECOGNIZED_COMMAND):
        printf("[ERROR ] Unrecognized keyword at start of '%s'.\n", raw_sql);
        exit(EXIT_FAILURE);
    case (PREPARE_SYNTAX_ERROR):
        printf("[ERROR ] Syntax error. Could not parse statement.\n");
        exit(EXIT_FAILURE);
    }

    switch (execute_statement(&statement, table))
    {
    case (EXECUTE_SUCCESS):
        printf("[INFO ] Successfully executed the command.\n");  
        return statement;
    case (EXECUTE_TABLE_FULL):
        printf("[ERROR ]  Table full.\n");
        exit(EXIT_FAILURE);
    }
}

/** Opens a connection to a table or creates it if it doesn't exist.
*/
EMSCRIPTEN_KEEPALIVE
Table *open_table(char *table_name)
{
    char filename[strlen(table_name) + 8];
    // ^^^ + 8 to account for "/db/", ".db", and '\0'.
    sprintf(filename, "/db/%s.db", table_name);
    // ^^^ convert table name to table file name

    EM_ASM_(
        {
            let tableName = UTF8ToString($0);
            var fs = require("fs");
            if (!fs.existsSync("./db"))
            {
                fs.mkdirSync("./db");
            }
            if (!fs.existsSync("./db/" + tableName + ".db"))
            {
                fs.writeFileSync("./db/" + tableName + ".db", "");
            }
            FS.mkdir("/db");
            FS.mount(NODEFS, {root : "./db"}, "/db");
        },
        table_name);
    // ^^^ does the following:
    // - creates ./db folder if it doesn't exist,
    // - creates tableName.db if it doesn't exist,
    // - create corresponding /db folder in Emscripten, and
    // - mount NODEFS for persistence tying ./db to /db.

    Pager *pager = pager_open(filename);
    Table *table = malloc(sizeof(Table));
    table->num_rows = pager->file_length / ROW_SIZE;
    table->pager = pager;
    return table;
}

/** Flushes the malloc-ed page to disk, closes the
    table file, and free the memory for the
    Pager and Table data structures.
*/
EMSCRIPTEN_KEEPALIVE
void close_table(Table *table)
{
    Pager *pager = table->pager;
    int num_full_pages = table->num_rows / ROWS_PER_PAGE;

    for (int i = 0; i < num_full_pages; i++)
    {
        if (pager->pages[i] == NULL)
        {
            continue;
        }
        pager_flush(pager, i, PAGE_SIZE);
        // ^^^ writes to file
        free(pager->pages[i]);
        pager->pages[i] = NULL;
        // ^^^ defensively set to NULL
    }

    int num_additional_rows = table->num_rows % ROWS_PER_PAGE;
    // ^^^ not full-page
    if (num_additional_rows > 0)
    {
        int page_offset = num_full_pages;
        if (pager->pages[page_offset] != NULL)
        {
            pager_flush(pager, page_offset, num_additional_rows * ROW_SIZE);
            // ^^^ write non-full page to file
            free(pager->pages[page_offset]);
            pager->pages[page_offset] = NULL;
            // ^^^ defensively set to NULL
        }
    }

    int result = close(pager->file_descriptor);
    // ^^^ close table file
    if (result == -1)
    {
        printf("[ERROR ] Error closing table.\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < TABLE_MAX_PAGES; i++)
    {
        void *page = pager->pages[i];
        if (page)
        {
            free(page);
            pager->pages[i] = NULL;
        }
    }
    // ^^^ make sure full-cache is cleared
    free(pager);
    free(table);
}