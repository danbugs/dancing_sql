/** \file 
    Useful macros and types intended to be used by the end-user.
*/

#ifndef DCS_H
#define DCS_H
/** A function macro to allow embedding SQL within C files.
*/
#define SQL(raw_sql) #raw_sql

/** A Page (i.e., block of memory) has 4096 Bytes (or, 4KB) —
    this value was chosen because its' the same size of a 
    page in most processor architectures.
*/
#define PAGE_SIZE (int)4096

/** An arbitrarily decided number for the maximum number of pages
    in a table.
*/
#define TABLE_MAX_PAGES (int)100

/** A macro that expands to the \c char* type — 
    created to distinguish between normal strings and 
    SQL statements.
*/
#define sql_t char *

/** Pager holds the page cache and our database file.
*/
typedef struct
{
    int file_descriptor;
    int file_length;
    void *pages[TABLE_MAX_PAGES];
} Pager;

/** Table holds the number of rows in it (for easy insertion), and
    the a \c Pager that holds the actual database.
*/
typedef struct
{
    int num_rows;
    Pager *pager;
} Table;

/** Currently supported SQL statements.
*/
typedef enum
{
    STATEMENT_INSERT,
    STATEMENT_SELECT,
} StatementType;

/** The size of the content row — equivalent
    to VARCHAR(255).
*/
#define COLUMN_CONTENT_SIZE (int)255

/** Default table row. DCS does not currently support
    custom rows. The content attribute is intended
    to hold JSON-stringified type of content.
*/
typedef struct
{
    int id;
    char content[COLUMN_CONTENT_SIZE + 1];
} Row;

/** General SQL statement — includes a \c type 
    ( \c STATEMENT_INSERT or \c STATEMENT_SELECT ),
    and a \c row_to_insert in case of it being of
    \c type \c STATEMENT_INSERT .
*/
typedef struct
{
    StatementType type;
    Row row_to_insert;
    char select_result[PAGE_SIZE * TABLE_MAX_PAGES]; // maximum select size
} Statement;
#endif