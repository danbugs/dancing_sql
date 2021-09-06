#include <stdio.h>
#include <emscripten.h>
#include "../dependencies/dcs/dcs.h"
// ^^^ adds necessary structs and definitions from DCS

extern void execute_sql(sql_t raw_sql, Table *table);
extern Table *open_table();
extern void close_table(Table *table);
// ^^^ group of functions coming from ../dependencies/dcs/dcs.o

int main()
{
    Table *table = open_table("test");
    // ^^^ creating a new default table.
    // Default Structure:
    // - INTEGER id, and
    // - VARCHAR(255) content ~ meant to contain all necessary info in sort of a JSON.stringify-ed way.
    // Notes:
    // - content will be trucated at 255 characters.
    // - content must be wrapped around single-quotes.

    sql_t insert_s = SQL(INSERT 0 '{"name":"dan"}');
    // ^^^ un-executed insert statement.
    // Note: 'INSERT' must be upper-case, the VM
    // is case-sensitive.

    sql_t select_s = SQL(SELECT);
    // ^^^ un-executed select statement.
    // Note: 'SELECT' must be upper-case, the VM
    // is case-sensitive.

    execute_sql(insert_s, table);
    execute_sql(select_s, table);
    // ^^^ executes SQL statements and
    // prints results to console.

    close_table(table);
    // ^^^ frees table post-use.
}