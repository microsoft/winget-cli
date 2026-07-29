# Coding Standards

This document describes the coding conventions used in the WinGet CLI codebase. It is a companion to:

- [`CONTRIBUTING.md`](../CONTRIBUTING.md) — workflow and process guidance
- [`doc/Developing.md`](./Developing.md) — build, test, and localization instructions

The codebase is primarily C++/WinRT. Brief notes for .NET (C#) components appear at the end.

---

## File Formatting

All source files must conform to the rules in [`.editorconfig`](../.editorconfig):

- **Line endings**: CRLF
- **Encoding**: UTF-8
- **Final newline**: required
- **Trailing whitespace**: must be trimmed
- **YAML files** (`.yml`/`.yaml`): 2-space indentation
- **Markdown files** (`.md`): tab indentation

Configure your editor to apply these settings on save, or run a check before committing.

---

## File Headers

Every C++ source and header file must begin with:

```cpp
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
```

Every C# source file must begin with:

```csharp
// -----------------------------------------------------------------------------
// <copyright file="FileName.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------
```

---

## Brace Style

The codebase uses **Allman style** (also known as BSD style): the opening brace of every block goes on its **own new line**, at the same indentation level as the statement that introduced it.

```cpp
namespace AppInstaller::CLI::Workflow
{
    void SomeWorkflowTask(Execution::Context& context)
    {
        if (condition)
        {
            DoSomething();
        }
        else if (otherCondition)
        {
            DoSomethingElse();
        }
        else
        {
            Fallback();
        }

        for (const auto& item : items)
        {
            Process(item);
        }

        switch (value)
        {
        case SomeEnum::First:
            HandleFirst();
            break;
        case SomeEnum::Second:
            HandleSecond();
            break;
        default:
            THROW_HR(E_UNEXPECTED);
        }
    }
}
```

Key points:

- `else` and `else if` go on their own line after the closing `}` — never on the same line as `}`.
- `case` labels inside a `switch` are **not** indented relative to the `switch` keyword; they sit at the same level.
- **Trivial single-expression bodies** (simple getters, forwarding constructors, one-liner lambdas) may be written inline:
  ```cpp
  bool IsTerminated() const { return m_isTerminated; }
  SomeClass(std::string name) : m_name(std::move(name)) {}
  ```
  Use judgment: if the body is anything more than a single expression, use the full Allman form.

---

## Naming Conventions

### C++

| Element | Convention | Example |
|---------|-----------|---------|
| Types (classes, structs, enums, type aliases) | `PascalCase` | `ExecutionContext`, `ContextFlag` |
| Functions and methods | `PascalCase` | `GetErrorCode()`, `IsTerminated()` |
| Local variables and parameters | `camelCase` | `installerType`, `packageId` |
| Non-static member variables | `m_` prefix + `camelCase` | `m_name`, `m_flags` |
| Static member variables | `s_` prefix + `camelCase` | `s_disabledReason` |
| Namespaces | `PascalCase` | `AppInstaller::CLI::Workflow` |
| Macros | `AICLI_` or `WINGET_` prefix, `ALL_CAPS` | `AICLI_TERMINATE_CONTEXT`, `WINGET_CATCH_STORE` |
| Enum members | `PascalCase` | `ContextFlag::InstallerTrusted` |

### C\#

Follow standard .NET naming conventions (PascalCase for public members, camelCase for local variables and private fields). StyleCop is configured in `src/stylecop.json`.

---

## Error Handling

### C++: WIL macros (preferred)

The codebase uses the [Windows Implementation Library (WIL)](https://github.com/microsoft/wil) for HRESULT-based error handling. Prefer WIL macros over manual `if (FAILED(hr))` checks:

```cpp
// Throw on failure — use in code where exceptions are acceptable
THROW_IF_FAILED(SomeWin32OrComApi());
THROW_HR(E_UNEXPECTED);
THROW_HR_IF(E_POINTER, ptr == nullptr);
THROW_HR_MSG(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), "Stage %d is not valid here", stage);

// Return the HRESULT on failure — use in COM methods or HRESULT-returning functions
RETURN_IF_FAILED(SomeWin32OrComApi());

// Log without propagating — use for non-fatal side effects
LOG_IF_FAILED(CleanupTempFiles());
```

Prefer the most specific macro. For example, `THROW_HR_IF` is cleaner than `if (condition) { THROW_HR(...); }`.

### C++: Workflow context termination

Inside workflow functions (functions that take `Execution::Context& context`), use the `AICLI_*` family of macros rather than throwing:

```cpp
void WorkflowTask(Execution::Context& context)
{
    // Guard against an already-terminated context at the top of every task
    AICLI_RETURN_IF_TERMINATED(context);

    // Terminate the context and return from the current function
    if (failed)
    {
        AICLI_TERMINATE_CONTEXT(HRESULT_VALUE);
    }

    // Terminate but return a specific value (useful in non-void helpers)
    // AICLI_TERMINATE_CONTEXT_RETURN(HRESULT_VALUE, returnValue);
}
```

`AICLI_TERMINATE_CONTEXT` records the file and line of the failure, sets the context's termination HRESULT, and returns from the current function. Do not throw exceptions out of workflow functions — use context termination instead.

### C++: Unexpected cases must not be swallowed

Every `switch` statement over an enum or integer must have an explicit `default` branch. That branch must either:

1. **Terminate with `E_UNEXPECTED`** if reaching it indicates a programming error (an enum value was added but the switch was not updated, or the caller passed an invalid value):
   ```cpp
   switch (installerType)
   {
   case InstallerTypeEnum::Exe:     /* ... */ break;
   case InstallerTypeEnum::Msi:     /* ... */ break;
   default:
       THROW_HR(E_UNEXPECTED);
   }
   ```

2. **Have a clearly intentional and documented fallback** when a default behavior is genuinely correct:
   ```cpp
   switch (result)
   {
   case ConfigurationTestResult::Positive: return Resource::StringId::ConfigPositive;
   case ConfigurationTestResult::Negative: return Resource::StringId::ConfigNegative;
   default: return Resource::StringId::Empty(); // Unknown/not yet tested
   }
   ```

The same principle applies outside of switches: use `THROW_HR_IF(E_UNEXPECTED, condition)` to assert runtime invariants that should never be violated:

```cpp
THROW_HR_IF(E_UNEXPECTED, entries.size() != expectedCount);
```

**Do not leave an empty `default:` or an empty `else` branch** that silently discards an unexpected case. Silent failures are harder to debug than explicit ones.

---

## Casts

**Never use C-style casts.** C-style casts (e.g., `(int)value`) bypass the type system silently. Use the named C++ cast operators instead:

| Situation | Use |
|-----------|-----|
| Safe numeric or enum conversions | `static_cast<T>(value)` |
| Reinterpreting pointer/integer bytes | `reinterpret_cast<T>(value)` |
| Removing `const` (rare; justify in a comment) | `const_cast<T>(value)` |
| Downcasting via virtual dispatch | `dynamic_cast<T>(value)` |
| WinRT interface conversion | `.as<T>()` from C++/WinRT |
| Converting `ToIntegral` for enums | Use the project helper `ToIntegral(enumValue)` where available |

---

## `std::move()`

`std::move()` casts a value to an rvalue reference so its resources can be transferred rather than copied. Use it only where ownership transfer is clearly intended.

### When to use

- **Passing to a constructor or function that takes by value or `&&`** when you no longer need the source:
  ```cpp
  context.Add<Data::Manifest>(std::move(manifest));
  m_items.push_back(std::move(item));
  ```
- **Initializing members from constructor parameters**:
  ```cpp
  MyClass(std::string name) : m_name(std::move(name)) {}
  ```

### When NOT to use

- **On `return` statements.** Named Return Value Optimization (NRVO) can eliminate the copy/move entirely, but only if you return the variable directly. `std::move()` on a return statement suppresses NRVO and can result in an extra move that would not otherwise occur:

  ```cpp
  // ✗ Suppresses NRVO
  std::string BuildResult() { return std::move(result); }

  // ✓ Allows NRVO
  std::string BuildResult() { return result; }
  ```

- **On trivially copyable types** (`int`, `HRESULT`, raw pointers, enums, etc.). Moving these is no cheaper than copying, and `std::move()` just adds noise.

- **On `const` objects.** The move constructor cannot be selected for a `const` object; the call silently falls back to a copy.

- **On an object you still need after the call.** After a move, the source is in a valid but unspecified state. Accessing it without re-assignment is undefined behavior.

---

## Resource Strings

All user-visible strings must be added to the English resource file:

```
src/AppInstallerCLIPackage/Shared/Strings/en-us/winget.resw
```

**Do not edit** any file under `Localization/Resources/<locale>/`; those files are owned by Microsoft's localization pipeline and will be overwritten automatically.

Every new or modified string entry must include a `<comment>` element that gives translators enough context. This is especially important for:

- Short or single-word values (column headers, status labels) where the word has multiple meanings
- Technical jargon that may have a different colloquial meaning in other languages
- Strings with placeholders — document what each `{0}`, `{1}`, etc. represents

See [Localization in `doc/Developing.md`](./Developing.md#localization) for an example.

---

## .NET (C#) Components

The PowerShell modules (`src/PowerShell`) and configuration tests are written in C#. In addition to the file header and naming guidance above:

- StyleCop Analyzers are configured in `src/stylecop.json`. Build warnings from StyleCop must not be suppressed without justification.
- Follow standard .NET exception handling — avoid swallowing exceptions silently.
- Match the namespace structure of the surrounding project (e.g., `Microsoft.WinGet.Client.Engine.Commands`).
