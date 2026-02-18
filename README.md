# y-- Language

![WhatsApp Image 2026-01-28 at 15 01 36](https://github.com/user-attachments/assets/71d3098c-122f-46dc-8e58-dd65d92b0c03)

## Overview

y-- is an **interpreted language** written in **modern C++**. It is currently at **v1.0** and designed as a **personal language project** with a long‑term roadmap toward a **LLVM** and eventually **JIT and AOT**.

## 1.0 Introduction
**y--** is a language built on a specific philosophy: **"Reach the limit of high-level abstraction, then dig deeper."** Unlike C++, which builds *up* from low-level roots, `y--` starts with the developer experience of a high-level scripting language (`y`) and provides the operators and memory controls to go lower (`--`) when performance is critical.

### 1.1 Design Goals
* **Explicit Authority:** The language does not "guess" your intent. If you want a deep copy, you ask for it (`$`). If you want a reference, you declare it (`@`).
* **Clear Syntax:** A bracket-optional, whitespace-agnostic structure that prioritizes readability without enforcing indentation rules.
* **Systems Ready:** Built with a custom C++ backend, featuring native bindings for **Raylib** and  soon **SDL2** concepts out of the box.

---

Primary target domains:

* **Game development**
* **Robotics and simulations**

Execution is explicit and controlled. The language **requires a `main()` function** and does not execute code outside of it.

---

> **Version:** 1.0 (Alpha)  
> **Extension:** `.ymm`  
> **Paradigm:** Multi-paradigm (Imperative, Object-Oriented, Functional)  
> **Architecture:** Bytecode VM (LLVM / Hybrid AOT-JIT planned)
---

## Core Philosophy

* Everything is **explicit**
* Invalid usage is forgiven **if logically sound**
* Errors fail fast unless explicitly handled
* Ownership, copying, and mutability are **always visible in syntax**

---

## Entry Point & Execution Rules

* Code is parsed top‑to‑bottom
* **Only declarations are allowed outside `main()`**

  * Functions
  * Variables
* **Executable statements only run inside `main()`**

---

## 2.0 Lexical Structure
`y--` is case-sensitive and format-flexible. It does not enforce specific coding styles (camelCase vs snake_case), leaving that to the user's preference.

### 2.1 Comments
* **Single Line:** Uses the tilde `~`.
    ```javascript
    ~ This is a comment
    let x = 10 ~ Comments can be inline
    ```
* **Multi-Line:** Enclosed in backticks `` ` ``.
    ```javascript
    `
      This is a multi-line comment block.
      Useful for documentation or temporarily disabling code.
    `
    ```

### 2.2 Whitespace & Formatting
* **Semicolons:** **Not required.** The parser automatically detects statement termination.
* **Indentation:** **Optional.** Unlike Python, indentation is purely for visual clarity.
* **Blocks:** * **Single-line:** Braces `{}` are optional.
* **Multi-line:** Braces `{}` are **required**.

    ```javascript
    ~ Valid Single Line
    if x > 10: return true

    ~Valid Multi-Line
    if x > 10: {
        print("Greater")
        return true
    }
    ```
### 2.4 Operators & Logic

`y--` features a comprehensive suite of logical gates, including native support for complex operations often missing in other high-level languages.

| Type | Operator | Description | Truth Table (A, B) |
| --- | --- | --- | --- |
| **AND** | `and` / `&&` | True if **both** are true. | T, T  T |
| **OR** | `or` / ` |  | ` |
| **NOT** | `not` / `!` | Inverts the value. | T  F |
| **NAND** | `nand` | True if **at least one** is false. | T, T  F |
| **NOR** | `nor` | True if **both** are false. | F, F  T |
| **XOR** | `xor` | True if **exactly one** is true. | T, F  T |
| **NXOR** | `nxor` | True if **both are equal** (XNOR). | T, T  T |

---

### **3.0 Type System & Variables**

`y--` utilizes a hybrid typing system that supports both dynamic typing (re-assignable types) and static type locking. The memory management model relies on automatic reference counting (via C++ shared pointers), ensuring efficient resource deallocation when variables go out of scope.

#### **3.1 Variable Declaration & Mutability**

Variables are declared using the `let` keyword. The language supports three distinct assignment behaviors regarding mutability and type safety.

| Syntax | Description | Type Behavior |
| --- | --- | --- |
| `let x` | **Declaration only.** Variable initializes to `NoType`. | **Unbound.** Adapts to the type of the first assignment. |
| `let x = 10` | **Dynamic Assignment.** | **Mutable.** The variable `x` can be reassigned to a value of a *different* type later (e.g., `x = "string"` is valid). |
| `let x := 10` | **Type-Locked Assignment.** | **Type-Constrained.** The variable `x` is locked to type `int`. Reassigning it to a different type throws a `ConstError`. |
| `let const x = 10` | **Constant Declaration.** | **Immutable.** The value of `x` cannot be changed after initialization. |

**The `NoType` State**

* Variables declared without an assignment (e.g., `let z`) exist in a state of `NoType`.
* This is a transient state; it is strictly for initialization logic and cannot be explicitly assigned to a variable.
* Upon the first valid assignment, `NoType` is erased and replaced by the value's type.

#### **3.2 Primitive Data Types**

`y--` provides a robust set of built-in primitives with automatic casting and promotion.

* **Integer (`int`):** Supports arbitrary precision. Standard integers automatically promote to `BigInt` if the value exceeds .
* *Initializer:* `int()` (Defaults to 0). Accepts numeric strings or floats.


* **Float (`float`):** Double-precision floating-point numbers.
* *Initializer:* `float()` (Defaults to 0.0).


* **String (`string`):** specific sequences of characters enclosed in single (`'`) or double (`"`) quotes.
* *Initializer:* `string()` (Converts arguments to string representation).


* **Boolean (`bool`):** Represents truthy/falsy values.
* *Initializer:* `bool()` (Defaults to `false`). Evaluates the truthiness of the argument.


* **NoneType (`None`):** Represents the absence of value.
* *Initializer:* `None()` (Always returns `None`).



#### **3.3 Composite Data Structures**

| Type | Syntax | Properties | Constructor |
| --- | --- | --- | --- |
| **List** | `[1, 2]` | Ordered, mutable sequence. | `list(args...)` |
| **Tuple** | `(1, 2)` | **Immutable**, ordered sequence. Freezes all contents upon creation. | `tuple(args...)` |
| **Set** | `{1, 2}` | Unordered collection of unique elements. Random access supported. | `set(args...)` |
| **Range** | `0...10...1` | Arithmetic progression generator. Smart step deduction (1 or -1) if omitted. | `range(start, end, step)` |
| **Vector** | `<x, y, z>` | N-dimensional mathematical vector (Int/Float only). *Requires `Vector` library import.* | `vector(args...)` |

**Dictionary (`dict`)**
Key-value pairs where keys are immutable (frozen).

* **Syntax:** `{}`
* **Construction:**
1. `dict(k1: v1, k2: v2)`: Direct keyword argument style.
2. `dict(pair(k, v), ...)`: Using the internal `pair()` unpacking helper.


* **Note:** The `pair()` object is a private intermediate structure used exclusively for unpacking into a dictionary.

### 3.3 Advanced Collection Features

`y--` supports Pythonic slicing and powerful comprehensions for concise data manipulation.

#### **Slicing**

Sequences (Lists, Strings, Tuples, Vectors) support slicing syntax to extract subsets.

* **Syntax:** `container[start : end : step]`
* **Behavior:**
* `list[1:4]` - Elements from index 1 up to (but not including) 4.
* `list[::2]` - Every second element.
* `list[::-1]` - Reverses the container.



#### **Comprehensions**

Construct new collections by iterating over existing ones in a single expressive line.

* **List Comprehension:**
```javascript
let squares = [x * x for x in range(10)]

```


* **Set Comprehension:**
```javascript
let evens = {x for x in numbers if x % 2 == 0}

```


* **Dictionary Comprehension:**
```javascript
let map = {k: v for k, v in other_dict}

```

#### **3.5 Scoping & Lifecycle**

* **Lexical Scoping:** `y--` employs standard lexical scoping rules. Inner scopes (functions, blocks) inherit access to variables defined in outer/global scopes.
* **Global Access:** Variables defined at the top level are accessible globally.
* **Memory Management:** Variables are managed via **Automatic Reference Counting (ARC)**. When a scope (function or block) exits, local variables are automatically deallocated.

---

### **4.0 Control Flow**

`y--` distinguishes itself with a state-aware control flow system. Conditional logic supports both traditional branching and advanced, re-evaluating state selection.

#### **4.1 Conditionals**

Standard branching uses a colon-delimited syntax. Curly braces `{}` are optional for single-line statements but required for multi-line blocks.

* **Syntax:**
```javascript
if condition: { ... }
elif condition: { ... }
else: { ... }

```


* **Ternary Operator:**
```python
result = value_a if condition else value_b

```



#### **4.2 Loops & Iteration**

Loops in `y--` support both C-style counter manipulation and modern iterator patterns.

**C-Style (`for`)**
Explicit initialization, condition, and increment steps.

* **Syntax:** `for let i=0 if i < 10 then i++ do: { ... }`
* **The `omit` Keyword:** Used to bypass specific loop stages (equivalent to C's `;;`).
* *Example (Infinite Loop):* `for omit if true then omit do: { ... }`



**Iterator-Style (`for in`)**
Supports "Zipping" (iterating multiple containers simultaneously). The loop terminates when the *shortest* container is exhausted.

* **Single Container:** `for i in list do: { ... }`
* **Dictionary Unpacking:** `for k, v in my_dict do: { ... }` (Special 2-arg unpacking).
* **Multi-Container (Zip):** `for x, y, z in list1, list2, list3 do: { ... }`
* *Constraint:* The number of loop variables must match the number of containers (unless unpacking a single Dictionary).



**Control Keywords:**

* `break`: Exits the loop immediately.
* `continue`: Skips to the next iteration.
* `skip(n)`: Advances the iterator by `n` steps instantly.

**While / Do-While**

* `while cond do: { ... }`
* `do: { ... } while cond`

#### **4.3 The Re-evaluating Switch**

The `switch` statement in `y--` is dynamic. Unlike traditional C-switches, if a case block does not `break`, 
the switch **re-evaluates the switch value** and restarts the matching process. This allows for complex state-machine logic within a single block.

* **Syntax Keywords:**
* `case X then do:` Indicates subsequent cases follow.
* `case Y do:` Indicates this is the final case before `default`.
* `default do:` Fallback execution.


* **Optimization Strategy:**
The compiler selects the backend structure based on case density:
* **Dense Integers:** Jump Table ()
* **Sparse Integers:** Red-Black Tree ()
* **Strings:** Hash Map ( avg)
* **Mixed/Complex:** Linear Search ()



#### **4.4 Error Handling (`try-catch`)**

Error handling mirrors the `switch` syntax and logic.

* **Structure:**
```javascript
try: { ... }
catch ErrorType1, ErrorType2 then do: { ... }
catch ErrorType3 do: { ... }
else do: { ... }     ~ Executes if NO error occurs
finally do: { ... }  ~ ALWAYS executes

```

* **Behavior:**
* `throw RuntimeError, "message"`: Initiates an error.
* **Uncaught Errors:** Terminate the program immediately.
* **Finally Priority:** A `return` statement inside a `finally` block overrides any other returns from `try` or `catch` blocks.

---

### **5.0 Functions & Procedures**

`y--` treats functions as first-class citizens. They support advanced parameter handling (deep copies, references, type locking), automatic memoization, and a unique default-return system.

#### **5.1 Function Definition**

Functions are declared using the `define function` syntax. The language supports both standard and **cached** (memoized) functions.

* **Syntax:**
```typescript
define [cached] function name(params...) -> DefaultReturn: { ... }

```

**The `cached` Keyword**

* When `cached` is applied, the interpreter automatically stores the results of function calls in a hash map (C++ `unordered_map`).
* Subsequent calls with the same arguments retrieve the result in  time, optimizing recursive algorithms (e.g., Fibonacci) to linear time without manual caching logic.

#### **5.2 Parameter Semantics**

`y--` offers precise control over how arguments are passed into the function scope using prefix modifiers.

| Symbol / Keyword | Semantics | Description |
| --- | --- | --- |
| `a` (Standard) | **Shallow Copy** | The default behavior. Copies primitive values; copies references for containers. |
| `const b` | **Immutable** | The argument cannot be modified within the function scope. |
| `@c` | **Reference** | Pass-by-reference. Changes to `c` inside the function affect the original variable. |
| `$d` | **Deep Copy** | "Recursive Expansive Copy." Clones the object and all nested structures entirely. Expensive but safe. |
| `e:int()` | **Type Locked** | Enforces type safety. Passing a non-integer raises a `ConstError`. |
| `f:string("val")` | **Default Value** | Type-locked to string. If `f` is omitted during the call, it initializes to `"val"`. |
| `*args` | **Variadic Tuple** | Collects excess positional arguments into a `tuple`. |
| `**kwargs` | **Variadic Dict** | Collects excess keyword arguments into a paired object and finally into a `dict`. |

#### **5.3 Return Values & Defaults (`->`)**

The arrow syntax `->` in `y--` serves a dual purpose: it specifies the **Return Type** and the **Default Return Value**.

* **Behavior:** If a function execution ends without a `return` statement, or encounters an empty `return`, the value defined after `->` is returned automatically.
* **Examples:**
* `-> int()`: Returns `0` (default int) if no return is specified.
* `-> string("error")`: Returns `"error"` if no return is specified.
* `(No arrow)`: Returns `NoType`.


* **Constraint:** If `->` is present, explicitly returning a value of a mismatched type will raise a `TypeError`.

#### **5.4 Calling Convention**

* **The `omit` Keyword:** Used to skip specific arguments in a function call, forcing them to use their default values.
* *Example:* `foo(10, omit, 30)` (Skips the 2nd argument).


* **Overloading:** `y--` supports value-based overloading. The runtime determines the specific function implementation to execute based on the pattern of values passed.

#### **5.5 Lambdas (Anonymous Functions)**

Lambdas are syntactically similar to standard functions but must be assigned to a variable or passed directly.

* **Syntax:**
```typescript
let is_ten = define lambda function(x) -> bool(): { x === 10 }

```


* **Caching:** Lambdas also support the `cached` keyword (`define cached lambda function...`).

---

### **6.0 Object-Oriented Programming (Classes)**

Classes in `y--` are the fundamental building blocks of the language ("God Objects"). The system enforces strict encapsulation while providing a rich meta-programming interface via "Magic Methods" (Dunders).

#### **6.1 Class Structure & Encapsulation**

Classes are defined using `define class`. The language enforces a **Strict Access Policy**: every class definition *must* explicitly declare three sections: `#public`, `#private`, and `#protected`.

* **Syntax:**
```javascript
define class Animal: {
    #public:
        ~ Accessible everywhere
    #protected:
        ~ Accessible by class and subclasses
    #private:
        ~ Accessible only within this class
}

```


* **Constraint:** Omitting any of these three keywords triggers a compilation error, ensuring developers consciously decide on visibility.

#### **6.2 Implicit Context (`self` vs `obj`)**

Unlike Python, `y--` does not require explicit `self` arguments in method definitions. Context is injected implicitly into every scope.

* **`self` (Instance Context):** Refers to the specific instance created.
* *Usage:* `let self.name = "Jake"`


* **`obj` (Class Context):** Refers to the Class Object itself (Static/Shared state).
* *Usage:* `let obj.count = 110` (Shared across all instances).



#### **6.3 Inheritance**

* **Syntax:** `define class Child inherits(Mom, Dad): { ... }`
* **Resolution:** `y--` supports **Multiple Inheritance**. The Method Resolution Order (MRO) is calculated using the **C3 Linearization** algorithm (standard in Python/Dylan) to solve the "Diamond Problem" deterministically.

#### **6.4 Magic Methods (Dunder Methods)**

Objects interact with operators and built-in functions via predefined "Magic Methods."

**Lifecycle & Introspection**
| Method | Description |
| :--- | :--- |
| `__construct__` | Constructor. Creates and returns the instance. |
| `__destruct__` | Destructor. Handles cleanup before memory release. |
| `__copy__` / `__clone__` | Handles shallow copy vs. deep copy logic. |
| `__ref__` | Creates an alias/reference to the instance. |
| `__inspect__` | Debugging hook for nested object printing. |
| `__mro__` | Returns the Method Resolution Order list. |
| `__display__` | Called by `print()`. |

**Arithmetic Operators**
*Supported for both standard (`a + b`) and reverse (`b + a`) operations.*
| Operator | Normal Method | Reverse Method | Assignment (`+=`) |
| :--- | :--- | :--- | :--- |
| `+` | `__plus__` | `__r_plus__` | `__plus_eq__` |
| `-` | `__minus__` | `__r_minus__` | `__minus_eq__` |
| `*` | `__times__` | `__r_times__` | `__times_eq__` |
| `/` | `__divide__` | `__r_divide__` | `__divide_eq__` |
| `//` | `__int_divide__` | `__r_int_divide__` | `__int_divide_eq__` |
| `**` | `__power__` | `__r_power__` | `__power_eq__` |
| `%` | `__modulo__` | `__r_modulo__` | `__modulo_eq__` |
| Unary | `__positive__`, `__negative__`, `__increment__` (`++`), `__decrement__` (`--`) | | |

**Comparison & Logic**
| Operator | Method |
| :--- | :--- |
| `==` / `!=` | `__equals__` / `__differs__` |
| `===` / `!==` | `__identical__` / `__distinct__` (Strict type/ref check) |
| `<` / `<=` | `__less__` / `__less_eq__` |
| `>` / `>=` | `__greater__` / `__greater_eq__` |

**Bitwise Operators**
*Full suite including NAND/NOR/NXOR.*
| Op | Method | Reverse | Assignment |
| :--- | :--- | :--- | :--- |
| `&` | `__bit_and__` | `__r_bit_and__` | `__and_equals__` |
| `\|` | `__bit_or__` | `__r_bit_or__` | `__or_equals__` |
| `^` | `__bit_xor__` | `__r_bit_xor__` | `__xor_equals__` |
| `~` | `__invert__` | N/A | N/A |
| `nand` | `__bit_nand__` | `__r_bit_nand__` | N/A |

**Container & Access Emulation**
| Syntax | Method |
| :--- | :--- |
| `obj[key]` | `__at__` (Get) |
| `obj[key] = val` | `__put__` (Set) |
| `x in obj` | `__has__` |
| `x not in obj` | `__missing__` |
| `obj.attr = val` | `__assign__` |
| `length(obj)` | `__count__` |
| `iter(obj)` | `__traverse__` |
| `next(obj)` | `__advance__` |

**Type Casting**
| Caster | Method |
| :--- | :--- |
| `int(obj)` | `__to_int__` |
| `string(obj)` | `__to_string__` |
| `bool(obj)` | `__to_bool__` |
| `list(obj)` | `__to_list__` |
| `dict(obj)` | `__to_dict__` |
| `vector(obj)` | `__to_vector__` |

---

### **7.0 Modules & The Standard Library**

`y--` uses a runtime-resolved import system. It includes a powerful standard library with built-in bindings for graphics and system operations.

#### **7.1 Import Syntax**

Modules are imported using the `import` keyword. Paths are string literals for local files, or bare words for Standard Library headers.

* **Full Import:** Loads the entire module namespace.
```javascript
import Math
import "my_module.ymm"

```


* **Partial Import:** Loads specific symbols into the current scope.
```javascript
import func1, func2 from Math
import func1, func2 from "my_module.ymm"

```



#### **7.2 Dependency Resolution (The "Blind Function" Rule)**

`y--` does not perform static linking or automatic dependency resolution inside imported files.

* **Constraint:** If you partially import a function `funcA`, and `funcA` relies on `funcB` inside the same module, you **must** import both.
* **Failure Case:** Importing only `funcA` will cause a `RuntimeError` when it attempts to call the missing `funcB`.
```javascript
// Correct usage for dependent functions
import parentFunc, dependentChild from "logic.ymm"

```



#### **7.3 Standard Library**

The following modules are built into the interpreter:

| Module | Description |
| --- | --- |
| **Math** | Advanced mathematical functions and constants. |
| **Vector** | N-dimensional vector operations (requires `vector()` constructor). |
| **Raylib** | Native bindings for the Raylib graphics library. |
| **Random** | Pseudo-random number generation. |
| **Time** | Clock access, delays, and delta-time calculations. |
| **System** | Interpreter status and garbage collection controls. |
| **Os** | Operating System interaction (Environment variables, Shell commands). |
| **FileStream** | Read/Write access to the file system. |

---

### **8.0 Tooling & Ecosystem**

* **File Extension:** Source files use the `.ymm` extension.
* **IDE Support:** A Visual Studio Code extension (syntax highlighting, snippets) is currently in development.
* **Runtime:** The interpreter relies on late binding; declarations are not checked until execution reaches them, removing the need for `extern` headers.

---

## 9.0 Roadmap

* [x] Core Interpreter & Memory Management
* [x] Standard Library (IO, Math, Vector)
* [x] Raylib Integration
* [ ] VS Code Extension (Syntax Highlighting)
* [ ] Package Manager
* [ ] LLVM JIT

---


## 10.0 Installation & Usage

### 10.1 Prerequisites

* **C++ Compiler:** GCC/Clang/MSVC supporting C++17 or later.
* **Raylib:** Must be installed and correctly linked to your compiler.
* **Dependencies:** Ensure `pystring.h` and `old.h` are present in the root source folder.

### 10.2 Setup Guide

1. **Clone the Repository:**
```bash
git clone https://github.com/phdgamer0/y--lang.git
cd y--

```


2. **Configure the Entry Point:**
* Open `yrun.cpp` in your text editor.
* Locate the source path string variable.
* Change the path to point to your target `.ymm` file.
```cpp
// Example inside yrun.cpp
std::string path = "path/to/your/project/main.ymm";

```

3. **Compile & Link:**
Compile `yrun.cpp` ensuring Raylib is linked.
```bash
# Example (g++):
g++ yrun.cpp -o y_lang -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

```

4. **Run the Interpreter:**
Execute the binary. You will be prompted to select an execution mode.
```bash
./y_lang

```

### 10.3 Execution Modes

When prompted, **ALWAYS select Option 1**.

* **[0] Tree Walker:** *Deprecated/Unsupported.* Do not use.
* **[1] Virtual Machine (VM):** The standard, optimized runtime for `y--`. Use this for all development.
* **[2] Debugger:** *Experimental.* Extremely slow; currently limited to AST and OpCode printing and basic inspection.
