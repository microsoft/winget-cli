---
applyTo: "src/AppInstallerRepositoryCore/Microsoft/Schema/*, src/Microsoft.Management.Configuration/Database/Schema/*"
---

## SQLite Statement Builder

**Always use `AppInstaller::SQLite::Builder::StatementBuilder` when writing SQLite database code. Never write raw SQL strings.**

The builder is in `<winget/SQLiteStatementBuilder.h>` (namespace `AppInstaller::SQLite::Builder`). It generates type-safe, parameterized SQL and ensures symbolic names are used for tables and columns throughout.

### Key conventions

- Define table and column names as `constexpr std::string_view` constants, then pass them to the builder.
- Use `ColumnBuilder` with chained modifiers (`.NotNull()`, `.Unique()`, `.Default(value)`) when creating tables.
- Use `IntegerPrimaryKey()` for auto-increment rowid primary keys.
- Use `Unbound` as a placeholder and bind values later via `Statement::Bind()`, or pass values directly to have them bound automatically.

### Common operations

```cpp
using namespace AppInstaller::SQLite::Builder;

// Symbolic names
static constexpr std::string_view s_MyTable      = "my_table"sv;
static constexpr std::string_view s_Col_Id       = "id"sv;
static constexpr std::string_view s_Col_Name     = "name"sv;
static constexpr std::string_view s_Col_Value    = "value"sv;

// CREATE TABLE
StatementBuilder builder;
builder.CreateTable(s_MyTable).Columns({
    IntegerPrimaryKey(),
    ColumnBuilder(s_Col_Name, Type::Text).NotNull().Unique(),
    ColumnBuilder(s_Col_Value, Type::Int64).NotNull().Default(0),
});
builder.Execute(connection);

// SELECT
StatementBuilder builder;
builder.Select({ s_Col_Id, s_Col_Name })
    .From(s_MyTable)
    .Where(s_Col_Name).Equals(nameValue);
auto stmt = builder.Prepare(connection);
while (stmt.Step()) { /* stmt.GetColumn<T>(index) */ }

// INSERT
StatementBuilder builder;
builder.InsertInto(s_MyTable)
    .Columns({ s_Col_Name, s_Col_Value })
    .Values(nameValue, intValue);
builder.Execute(connection);

// UPDATE
StatementBuilder builder;
builder.Update(s_MyTable).Set()
    .Column(s_Col_Value).AssignValue(newValue)
    .Where(s_Col_Id).Equals(rowId);
builder.Execute(connection);

// DELETE
StatementBuilder builder;
builder.DeleteFrom(s_MyTable)
    .Where(s_Col_Id).Equals(rowId);
builder.Execute(connection);

// ALTER TABLE – add a column
StatementBuilder builder;
builder.AlterTable(s_MyTable).Add(s_Col_NewCol, Type::Text).NotNull().Default(0);
builder.Execute(connection);
```

### Nullable values: `Equals()` vs `AssignValue()`

Both accept `std::optional<T>`, but they behave differently when the optional is empty and must be used in the right context:

| Method | Empty optional emits | Use in |
|---|---|---|
| `Equals(optional<T>)` | `IS NULL` | **WHERE** / filter clauses |
| `AssignValue(optional<T>)` | `= ?` (binds NULL) | **UPDATE SET** assignments |

Using `Equals(optional)` in an UPDATE SET clause is a bug — SQLite does not accept `col = IS NULL`.

```cpp
// ✅ Correct: Equals for WHERE filter, AssignValue for SET assignment
std::optional<int64_t> maybeEpoch = ...;
std::optional<std::string> maybeNote = ...;

builder.Update(s_MyTable).Set()
    .Column(s_Col_Name).AssignValue(requiredName)    // non-optional: AssignValue in SET
    .Column(s_Col_Epoch).AssignValue(maybeEpoch)     // nullable: must use AssignValue in SET
    .Column(s_Col_Note).AssignValue(maybeNote)       // nullable: must use AssignValue in SET
    .Where(s_Col_Id).Equals(rowId);                  // filter: Equals is correct here

// ✅ Correct: Equals(optional) in WHERE — emits "IS NULL" when empty
builder.Select(s_Col_Id).From(s_MyTable)
    .Where(s_Col_Note).Equals(maybeNote);            // → "WHERE note IS NULL" when empty
```

### Execution

- `builder.Execute(connection)` — prepares, binds, and runs a statement that returns no rows.
- `builder.Prepare(connection)` — returns a `SQLite::Statement`; call `.Step()` to iterate, `.GetColumn<T>(index)` to read values.

## Naming Conventions

- **Namespace structure**: `AppInstaller::<Area>[::<Subarea>]`
  - `AppInstaller::CLI::Execution` - CLI execution context
  - `AppInstaller::CLI::Workflow` - Workflow functions
  - `AppInstaller::Repository` - Repository/source logic
  - `AppInstaller::Manifest` - Manifest types
  - `AppInstaller::Settings` - User/admin settings

- **Macros**: Prefixed with `AICLI_` for CLI, `WINGET_` for general
- **Data keys**: ExecutionContextData uses enum keys to type-safely store/retrieve data
