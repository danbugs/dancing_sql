#pragma once

#define SQL(raw_sql) #raw_sql
#define TABLE_MAX_PAGES (int) 100
#define sql_t char*

typedef struct
{
    int num_rows;
    void* pages[TABLE_MAX_PAGES];
} Table;