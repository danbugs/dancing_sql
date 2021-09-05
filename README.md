# DanCing SQL 💃📂 (DCS)

This is an **incoming** SQLite clone compiled to WebAssembly — heavily inspired by [this](https://cstack.github.io/db_tutorial/).

## Getting Started

Dancing SQL is now distributed with the [Tarantella Package Manager](https://github.com/danbugs/tarantella) — a tool I've made to simplify setup of projects like these!

To install the Tarantella Package Manager, you need the Rust toolchain — for instructions on how to install it, see [this](https://www.rust-lang.org/tools/install). Once you're done with that, you can install Tarantella by running: `cargo install tarantella`.

To start-off, setup a new main module WASM project with: `tapm new my-first-project`. This will start a new C WASM project with:

- a build folder,
- a dependencies folder,
- an index.html,
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
extern Table *new_table();
extern void free_table(Table *table);
// ^^^ group of functions coming from ../dependencies/dcs/dcs.o

int main()
{
    Table *table = new_table();
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

    free_table(table);
    // ^^^ frees table post-use.
}
```

To finish off, run `tapm build` to compile our code and `tapm run` to test it out. If you navigate to `http://127.0.0.1:4000/`, you should see:

![getting-started](https://i.imgur.com/48jNzE3.png)