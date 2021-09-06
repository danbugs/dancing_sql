#pragma once
/** \file 
    Useful macros and types intended to be used by the end-user.
*/

/** A function macro to allow embedding SQL within C files.
*/
#define SQL(raw_sql) #raw_sql

/** An arbitrarily decided number for the maximum number of pages
    in a table.
*/
#define TABLE_MAX_PAGES (int) 100

/** A macro that expands to the \c char* type — 
    created to distinguish between normal strings and 
    SQL statements.
*/
#define sql_t char*

/** Pager holds the page cache and our database file.
*/
typedef struct {
    int file_descriptor;
    int file_length;
    void* pages[TABLE_MAX_PAGES];
} Pager;

/** Table holds the number of rows in it (for easy insertion), and
    the a \c Pager that holds the actual database.
*/
typedef struct
{
    int num_rows;
    Pager* pager;
} Table;