# DanCing SQL 💃📂 (DCS)

This is an **incoming** SQLite clone compiled to WebAssembly — heavily inspired by [this](https://cstack.github.io/db_tutorial/).

## Getting Started

Dancing SQL is now distributed with the [Tarantella Package Manager](https://github.com/danbugs/tarantella) — a tool I've made to simplify setup of projects like these!

To install the Tarantella Package Manager, you need the Rust toolchain — for instructions on how to install it, see [this](https://www.rust-lang.org/tools/install). Once you're done with that, you can install Tarantella by running: `cargo install tarantella`.

To start-off, setup a new main module WASM project with: `tapm new my-first-project --server`. This will start a new C WASM project with:

- a build folder,
- a dependencies folder,
- an index.js,
- a `Makefile`,
- a releases folder,
- a src folder with some starting code in `main.c`, and a
- `Tarantella.toml` file.

Next, add the latest version of Dancing SQL as a dependency with: `tapm add "danbugs/dancing_sql"`. This will automatically download DCS to your dependencies folder, add it to your `Tarantella.toml` list of dependencies, and append `dependencies/dcs/dcs.o` to the `DEPENDENCIES` variable of your `Makefile` to facilitate compilation.

Now, replace the content of `src/main.c`with:

```C
#include <stdio.h>
#include <emscripten.h>
#include "../dependencies/dcs/dcs.h"
// ^^^ adds necessary structs and definitions from DCS

extern void execute_sql(sql_t raw_sql, Table *table);
extern Table *open_table(char* table_name);
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
    execute_sql(select_s, table);
    // ^^^ executes SQL statements and
    // prints results to console.

    close_table(table);
    // ^^^ frees malloc-ed table and its' 
    // attributes and saves data to database file.
}
```

Next, in your `Makefile`, change the `EMCC_CFLAGS` to:
```Makefile
EMCC_CFLAGS=-s MAIN_MODULE=1 -lnodefs.js -s EXIT_RUNTIME=1
```

Now, to finish off, run `tapm build` to compile our code and `tapm run` to test it out. You should see the following in your console:
![getting-started-result](https://i.imgur.com/mhiF2iC.png)

Your information will be persisted to a file at `db/mytable.db`.

You can view the `mytable.db` file directly w/ a hex editor. In VSCode, you can install [this](https://marketplace.visualstudio.com/items?itemName=slevesque.vscode-hexdump) extension, right click the `mytable.db` file and select "Show hexdump". You should see:

![getting-started-hexdump-of-db-file](https://i.imgur.com/QxhZhtg.png)