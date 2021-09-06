#include <stdio.h>
#include <emscripten.h>
#include "../dependencies/dcs/dcs.h"
// ^^^ adds necessary structs and definitions from DCS

extern Statement execute_sql(sql_t raw_sql, Table *table);
extern Table *open_table(char *table_name);
extern void close_table(Table *table);
// ^^^ group of functions coming from ../dependencies/dcs/dcs.o

int main()
{
    Table *table = open_table("mytable");
    // ^^^ creating a new default table.
    // Default Structure:
    // - INTEGER id, and
    // - VARCHAR(255) content ~ meant to contain all necessary info in sort of a JSON.stringify-ed way.
    // Notes:
    // - content will be trucated at 255 characters.
    // - content must be wrapped around single-quotes.
    // - just like VARCHAR, you content does not
    // need to have 255 bytes but; regardless,
    // 255 will still be allocated for that field.

    sql_t insert_s = SQL(INSERT 0 '{"name":"dan"}');
    // ^^^ un-executed insert statement.
    // Notes:
    // - 'INSERT' must be upper-case, the VM
    // is case-sensitive.
    // - There are currently no checks to ensure
    // the ID is unique.

    sql_t select_s = SQL(SELECT);
    // ^^^ un-executed select statement.
    // Note: 'SELECT' must be upper-case, the VM
    // is case-sensitive.

    execute_sql(insert_s, table);
    Statement select_r = execute_sql(select_s, table);
    printf("select: %s\n", select_r.select_result);
    // ^^^ executes SQL statements and
    // prints results to console.

    close_table(table);
    // ^^^ frees malloc-ed table and its'
    // attributes and saves data to database file.
}