#include <stdio.h>
#include <emscripten.h>
#include "../dependencies/dcs/dcs.h"

extern void execute_sql(sql_t raw_sql, Table *table);
extern Table *new_table();
extern void free_table(Table *table);

int main()
{
    Table *table1 = new_table();
    Table *table2 = new_table();
    sql_t insert1_s = SQL(INSERT -1 '{"name":"dan"}');
    sql_t insert3_s = SQL(INSERT 1 '{"name":"dab"}');
    sql_t select1_s = SQL(SELECT);
    sql_t insert2_s = SQL(INSERT 0 '{"name":"bia"}');
    sql_t select2_s = SQL(SELECT);
    execute_sql(insert1_s, table1);
    execute_sql(insert3_s, table1);
    execute_sql(insert2_s, table2);
    execute_sql(select1_s, table1);
    execute_sql(select2_s, table2);
    free_table(table1);
    free_table(table2);
}