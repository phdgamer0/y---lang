# y-- Language

![WhatsApp Image 2026-01-28 at 15 01 36](https://github.com/user-attachments/assets/71d3098c-122f-46dc-8e58-dd65d92b0c03)

## Overview

y-- is an **interpreted language** written in **modern C++**. It is currently at **v1.0** and designed as a **personal language project** tailored for maximum performance and explicit control. 

## 1.0 Introduction
**y--** is a language built on a specific philosophy: **"Reach the limit of high-level abstraction, then dig deeper."** Unlike C++, which builds *up* from low-level roots, `y--` starts with the developer experience of a high-level scripting language (`y`) and provides the operators and memory controls to go lower (`--`) when performance is critical.

### 1.1 Design Goals
* **Explicit Authority:** The language does not "guess" your intent. If you want a deep copy, you ask for it (`$`). If you want a reference, you declare it (`@`).
* **Clear Syntax:** A bracket-optional, whitespace-agnostic structure that prioritizes readability without enforcing indentation rules.
* **Systems & Graphics Ready:** Built with a custom C++ backend, featuring native bindings for **Raylib** out of the box, with full support for **multithreading** and **GPU draws**.
* **C++ FFI:** Seamless Foreign Function Interface support, allowing you to interface directly with C++ libraries and execution streams.

---

Primary target domains:

* **Game development**
* **Robotics and simulations**
* **Terminal-based utilities**

Execution is explicit and controlled. The language **requires a `main()` function** and does not execute code outside of it.

---

> **Version:** 1.0 (Beta)  
> **Extension:** `.ymm`  
> **Paradigm:** Multi-paradigm (Imperative, Object-Oriented, Functional)  
> **Architecture:** Bytecode VM
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
  * Classes
  * Preprocessor Commands
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
* **Indentation:** **Optional.** Indentation is purely for visual clarity.
* **Blocks:** * **Single-line:** Braces `{}` are optional.
  * **Multi-line:** Braces `{}` are **required**.

    ```javascript
    ~ Valid Single Line
    if x > 10: return true

    ~ Valid Multi-Line
    if x > 10: {
        print("Greater")
        return true
    }
    ```

### 2.3 Preprocessor Directives
`y--` features a lightweight, zero-allocation preprocessor that executes before tokenization.

* **Text Replacement Macros:** You can define strict text replacements using the `<<replace>>` syntax. 
* **Syntax:**
```
<<replace>> "TARGET" with "REPLACEMENT"
  ```
* **Important:** The quotes `""` and the `with` keyword are strictly necessary for the tokenizer to lock onto the strings. Since these act as raw, blind text replacers before the tokens are built, use them with caution to avoid unintentionally mutating string literals.

### 2.4 Operators & Logic

`y--` features a comprehensive suite of logical gates. The compiler supports **short-circuit evaluation** on all logical operators (except `xor` and `nxor`, which must evaluate both sides by definition).

| Operator | Description | Truth Table (A, B) -> Output |
| --- | --- | --- |
| `and` | True if **both** are true. | T,T -> **T** &#124; T,F -> **F** &#124; F,T -> **F** &#124; F,F -> **F** |
| `or` | True if **at least one** is true. | T,T -> **T** &#124; T,F -> **T** &#124; F,T -> **T** &#124; F,F -> **F** |
| `not` | Inverts the value (Unary). | T -> **F** &#124; F -> **T** |
| `nand` | True if **at least one** is false. | T,T -> **F** &#124; T,F -> **T** &#124; F,T -> **T** &#124; F,F -> **T** |
| `nor` | True if **both** are false. | T,T -> **F** &#124; T,F -> **F** &#124; F,T -> **F** &#124; F,F -> **T** |
| `xor` | True if **exactly one** is true. | T,T -> **F** &#124; T,F -> **T** &#124; F,T -> **T** &#124; F,F -> **F** |
| `nxor` | True if **both are equal** (XNOR). | T,T -> **T** &#124; T,F -> **F** &#124; F,T -> **F** &#124; F,F -> **T** |

---

### **3.0 Type System & Variables**

`y--` utilizes a hybrid typing system that supports both dynamic typing (re-assignable types) and static type locking.

#### **3.1 Variable Declaration & Mutability**

Variables are declared using the `let` keyword. `y--` supports multi-declarations and multi-assignments on a single line, provided values are separated. 

* *Multi-assign example:* `let x, y, z = 1, 2, 3`

| Syntax | Description | Type Behavior |
| --- | --- | --- |
| `let x` | **Declaration only.** Variable initializes to `NoType`. | **Unbound.** Adapts to the type of the first assignment. |
| `let x = 10` | **Dynamic Assignment.** | **Mutable.** The variable `x` can be reassigned to a value of a *different* type later. |
| `let x := 10` | **Type-Locked Assignment.** | **Type-Constrained.** The variable `x` is locked to type `int`. Reassigning to a different type throws a `ConstError`. |
| `let const x = 10` | **Constant Declaration.** | **Immutable.** The compiler heavily optimizes constants, performing automatic **constant folding and propagation** at compile time. |

#### **3.2 Primitive Data Types**

* **Integer (`int`):** Supports arbitrary precision. Automatically promotes to `BigInt` if bounds are exceeded.
* **Float (`float`):** Double-precision floating-point numbers.
* **String (`string`):** Character sequences enclosed in single (`'`) or double (`"`) quotes.
* **Boolean (`bool`):** Represents truthy/falsy values.
* **NoneType (`None`):** Represents the absence of value.

#### **3.3 Composite Data Structures**

| Type | Syntax | Properties | Constructor |
| --- | --- | --- | --- |
| **List** | `[1, 2]` | Ordered, mutable sequence. | `list(args...)` |
| **Tuple** | `(1, 2)` | **Immutable**, ordered sequence. Freezes all contents upon creation. | `tuple(args...)` |
| **Set** | `{1, 2}` | Unordered collection of unique elements. Random access supported. | `set(args...)` |
| **Dictionary**| `{k: v}` | Key-value pairs where keys are immutable (frozen). | `dict(args...)` |
| **Range** | `0...10...1` | Arithmetic progression generator. Smart step deduction (1 or -1). | `range(start, end, step)` |
| **Vector** | `<x, y, z>` | N-dimensional mathematical vector (Int/Float only). *Requires Vector module.*| `vector(args...)` |

**Dictionary Unpacking Helper (`pair`)**
* **Construction:** `dict(k1: v1, k2: v2)` is standard. 
* **Note on `pair()`:** The `pair()` object is a private intermediate structure used for unpacking. If you explicitly pass a `pair()` object into a dictionary constructor, you **must only pass one**.

#### **3.4 Advanced Collection Features**

* **Slicing:** `container[start : end : step]`
* **Comprehensions:**
    ```javascript
    let squares = [x * x for x in range(10)]
    let map = {k: v for k, v in other_dict}
    ```

#### **3.5 Scoping & Lifecycle**

* **Lexical Scoping:** `y--` employs strict lexical scoping. Inner scopes (functions, conditionals, loops) securely inherit access to variables defined in outer/global scopes, while variables declared *inside* a block remain entirely isolated from the outside.
* **Global Access:** Variables defined at the top level of the file are globally accessible.
* **Memory Management:** Driven by **Automatic Reference Counting (ARC)** via C++ shared pointers. 
* **Lifecycle:** When execution exits a block scope (like the end of a function or an `if` statement), all local variables within that block have their reference counts decremented and are instantly deallocated from memory, ensuring zero memory leaks during heavy operations.

---

### **4.0 Control Flow**

#### **4.1 Conditionals**
```javascript
if condition: { ... }
elif condition: { ... }
else: { ... }
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


**Optimization Strategy:**
The compiler selects the backend structure based on case density:
* **Dense Integers:** Jump Table O(1)
* **Sparse Integers:** Red-Black Tree O(logn)
* **Strings:** Hash Map avg O(1)
* **Mixed/Complex:** Linear Search O(n)



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
Functions are declared using the `define function` syntax. 
```typescript
define [cached] function name(params...) -> DefaultReturn: { ... }

```

**The `cached` Keyword:** When `cached` is applied, the interpreter automatically stores the results of function calls in a hash map. Subsequent calls with the exact same arguments retrieve the result in O(1) time, optimizing recursive algorithms without manual caching logic.

#### **5.2 Parameter Semantics**

`y--` offers precise control over how arguments are passed into the function scope using prefix modifiers and inline assignments.

| Symbol / Keyword | Semantics | Description |
| --- | --- | --- |
| `a` (Standard) | **Shallow Copy** | The default behavior. Copies primitive values; copies references for containers. |
| `const b` | **Immutable** | The argument cannot be modified within the function scope. |
| `@c` | **Reference** | Pass-by-reference. Changes to `c` inside the function affect the original variable. |
| `$d` | **Deep Copy** | "Recursive Expansive Copy." Clones the object and all nested structures entirely. |
| `e:int()` | **Type Locked** | Enforces type safety. Passing a non-integer raises a `ConstError`. |
| `f:string("val")` | **Type Locked Default** | Type-locked to string. If omitted during the call, initializes to `"val"`. |
| `g = 123` | **Untyped Default** | Initializes to the value if omitted, but remains dynamic and unbound to a specific type. |
| `*args` | **Variadic Tuple** | Collects excess positional arguments into a `tuple`. |
| `**kwargs` | **Variadic Dict** | Collects excess keyword arguments into a `dict`. |

#### **5.3 Return Values & Defaults (`->`)**

The arrow syntax `->` serves a dual purpose: it specifies the **Return Type** and the **Default Return Value**. If a function execution ends without a `return` statement, the value defined after `->` is returned automatically. Explicitly returning a value of a mismatched type will raise a `TypeError`.

#### **5.4 Calling Convention & Overloading**

* **Named Arguments:** When calling **user-defined functions**, parameters can be assigned explicitly by name in any order (e.g., `my_func(y=10, x=5)`).
* **The `omit` Keyword:** Used to skip specific arguments in a function call, forcing them to use their default values (e.g., `foo(10, omit, 30)`).
* **Overloading Resolution:** `y--` supports robust function overloading. When multiple functions share the same name, the compiler resolves the best overload using this strict priority order:
1. **Exact Type Match:** The argument matches the parameter's locked type exactly (e.g., passing an `int` to an `int`).
2. **Castable Type Match:** The argument can be safely cast to the parameter's type (e.g., passing an `int` to a `float`).
3. **Type Unlocked (Any) Match:** The parameter is fully dynamic (e.g., `let a`).



#### **5.5 Lambdas (Anonymous Functions)**

Lambdas are syntactically similar to standard functions but must be assigned to a variable or passed directly. They also support the `cached` keyword.

```typescript
let is_ten = define lambda function(x) -> bool(): { return x === 10 }

```
---

### **6.0 Object-Oriented Programming (Classes)**

Classes in `y--` are the fundamental building blocks of the language ("God Objects"). The system enforces strict encapsulation while providing a rich meta-programming interface via "Magic Methods" (Dunders).

#### **6.1 Class Structure & Encapsulation**

Classes are defined using `define class`. The language enforces a **Strict Access Policy**: every class definition *must* explicitly declare three sections: `#public`, `#private`, and `#protected`. Omitting any of these triggers a compilation error.

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

#### **6.2 Methods & Implicit Context (`self` vs `obj`)**

`y--` fully supports both **instance methods** and **class/static methods**. Unlike Python, `y--` does not require explicit `self` arguments in method definitions. Context is injected implicitly into every scope.

* **`self` (Instance Context):** Refers to the specific instance created. Used for instance methods and standard object properties.
* *Usage:* `let self.name = "Jake"`


* **`obj` (Class Context):** Refers to the Class Object itself (Static/Shared state). Shared across all instances.
* *Usage:* `let obj.count = 110`



#### **6.3 Inheritance**

* **Syntax:** `define class Child inherits(Mom, Dad): { ... }`
* **Resolution:** `y--` supports **Multiple Inheritance**. The Method Resolution Order (MRO) is calculated using the **C3 Linearization** algorithm (standard in Python/Dylan) to solve the "Diamond Problem" deterministically.


#### **6.4 Magic Methods (Dunder Methods)**

Objects interact with operators and built-in functions via predefined "Magic Methods." These are categorized by their functionality below:

**1. Lifecycle, Identity & Representation**
| Method | Description |
| :--- | :--- |
| `__construct__` | Called on object creation (Constructor). |
| `__destruct__` | Called on cleanup (Destructor). |
| `__copy__` | Performs a shallow copy. |
| `__clone__` | Performs a deep copy. |
| `__ref__` | Creates a reference copy. |
| `__mro__` | Returns the Method Resolution Order. |
| `__display__` | User-friendly string representation (Display). |
| `__inspect__` | Programmer-focused raw representation (Debug). |

**2. Arithmetic Operators**
| Operator | Normal Method | Reverse Method | In-Place Assignment |
| :--- | :--- | :--- | :--- |
| `+` | `__plus__` | `__r_plus__` | `__plus_eq__` |
| `-` | `__minus__` | `__r_minus__` | `__minus_eq__` |
| `*` | `__times__` | `__r_times__` | `__times_eq__` |
| `/` | `__divide__` | `__r_divide__` | `__divide_eq__` |
| `//` | `__int_divide__` | `__r_int_divide__` | `__int_divide_eq__` |
| `**` | `__power__` | `__r_power__` | `__power_eq__` |
| `%` | `__modulo__` | `__r_modulo__` | `__modulo_eq__` |

**3. Logical & Bitwise Operators**
| Operator | Normal Method | Reverse Method | In-Place Assignment |
| :--- | :--- | :--- | :--- |
| `and` / `&` | `__and__` | `__r_and__` | `__and_equals__` |
| `or` / `\|` | `__or__` | `__r_or__` | `__or_equals__` |
| `xor` / `^` | `__xor__` | `__r_xor__` | `__xor_equals__` |
| `nand` | `__nand__` | `__r_nand__` | *N/A* |
| `nor` | `__nor__` | `__r_nor__` | *N/A* |
| `nxor` | `__nxor__` | `__r_nxor__` | *N/A* |

**4. Unary & State Modifiers**
| Syntax | Method | Description |
| :--- | :--- | :--- |
| `+x` | `__positive__` | Unary positive |
| `-x` | `__negative__` | Unary negative |
| `x++` | `__increment__` | Increment |
| `x--` | `__decrement__` | Decrement |
| `!!x` | `__invert__` | Bitwise NOT |

**5. Comparison**
| Operator | Method | Description |
| :--- | :--- | :--- |
| `==` | `__equals__` | Equality |
| `!=` | `__differs__` | Inequality |
| `===` | `__identical__` | Strict equality |
| `!==` | `__distinct__` | Strict inequality |
| `<` | `__less__` | Less than |
| `<=` | `__less_eq__` | Less than or equal |
| `>` | `__greater__` | Greater than |
| `>=` | `__greater_eq__` | Greater than or equal |

**6. Container, Access & Iteration**
| Syntax / Action | Method | Description |
| :--- | :--- | :--- |
| `length(obj)` | `__count__` | Size of the container |
| `obj[key]` | `__at__` | Get item at key |
| `obj[key] = val` | `__put__` | Set item at key |
| `obj()` | `__call__` | Call object like a function |
| `x in obj` | `__has__` | Check if item is in obj |
| `x not in obj` | `__lacks__` | Check if item is not in obj |
| Missing attribute | `__missing__` / `__not__missing__` | Handles missing attributes/items |
| `obj.attr = val` | `__assign__` | Called when setting a property |
| `iter(obj)` | `__traverse__` | Returns the iterator object |
| `next(obj)` | `__advance__` | Moves to the next item |

**7. Type Casting**
| Caster | Method | Caster | Method |
| :--- | :--- | :--- | :--- |
| `int(obj)` | `__to_int__` | `set(obj)` | `__to_set__` |
| `float(obj)` | `__to_float__` | `dict(obj)` | `__to_dict__` |
| `string(obj)`| `__to_string__` | `tuple(obj)` | `__to_tuple__` |
| `bool(obj)` | `__to_bool__` | `vector(obj)`| `__to_vector__` |
| `list(obj)` | `__to_list__` | `range(obj)` | `__to_range__` |

**8. Class Info & Reflection**
| Category | Associated Methods |
| :--- | :--- |
| **Variables** | `__var_count__`, `__var_names__`, `__var_values__`, `__var_pairs__`, `__var_reverse_pairs__` |
| **Functions** | `__function_count__`, `__function_names__` |
| **All Members** | `__all_count__`, `__all_names__` |
---



### **7.0 Modules & The Standard Library**

`y--` uses a runtime-resolved import system. To prevent global scope pollution, importing a module assigns it strictly to its own namespace (e.g., `Math.sin()`). You can bypass this and dump everything into the global scope by using `import * from Module`.

#### **7.1 Import Syntax**

```javascript
import Math                     ~ Imports the Math namespace
import func1, func2 from Math   ~ Imports specific functions globally
import * from Math              ~ Dumps all Math functions globally
import "my_module.ymm"          ~ Imports a local file

```

#### **7.2 Dependency Resolution (The "Blind Function" Rule)**

`y--` does not perform automatic dependency resolution inside imported files. If `funcA` relies on `funcB` inside the same module, you **must** import both.

```javascript
~ Correct usage for dependent functions
import parentFunc, dependentChild from "logic.ymm"

```

#### **7.3 Standard Library**

| Module | Description |
| --- | --- |
| **Math** | Advanced mathematical functions and constants. |
| **Vector** | N-dimensional vector operations. |
| **OpenNN** | Cpu acclerated Neural network and machine learning library. |
| **Raylib** | Native bindings for 2D/3D hardware-accelerated graphics. |
| **Ncurses** | Terminal UI graphics handling (Linux targets only). |
| **QRgen** | Native QR code generation. |
| **Json** | Fast JSON parsing, serialization, and deserialization. |
| **Http** | Native web requests and network communication. |
| **Random** | Pseudo-random number generation. |
| **Time** | Clock access, delays, and delta-time calculations. |
| **System** | Interpreter status and garbage collection controls. |
| **Os** | Operating System interaction (Env vars, Shell commands). |
| **FileStream** | Read/Write access to the file system. |

---

### **8.0 Tooling & Ecosystem**

* **File Extension:** Source files use the `.ymm` extension.
* **Interactive REPL:** A fully functional REPL shell is available alongside the main executable for live testing.
* **IDE Support:** A Visual Studio Code extension providing syntax highlighting and snippets is fully available.
* **Runtime:** The interpreter relies on late binding; declarations are not checked until execution reaches them.
* **C++ FFI:** `y--` supports Foreign Function Interfaces, allowing it to seamlessly tap into native C++ libraries, multi-threaded execution streams, and direct GPU drawing operations.

---

## 9.0 Roadmap

* [x] Core Interpreter & Memory Management
* [x] Standard Library (IO, Math, Vector)
* [x] Raylib Integration
* [x] VS Code Extension (Syntax Highlighting)
* [ ] Package Manager

---

## 10.0 Installation & Usage

`y--` provides pre-compiled, standalone binaries. You do not need to install C++ compilers, link Raylib, or manage dependencies to start writing code.

### 10.1 Download the Binaries
1. Navigate to the `y--` GitHub repository.
2. Open the `build` directory.
3. Download the pre-compiled executable for your specific operating system (e.g., `y_lang.exe` for Windows, or the `y_lang` binary for Linux).

### 10.2 Adding to Global Path (Recommended)
To easily run the language from any folder in your terminal, you should add the binary to your system's global path.

**For Windows:**
1. Move the `y_lang_win64.exe` file to a permanent folder (e.g., `C:\Program Files\y_lang\`).
2. Open the Windows Start menu, type **"env"**, and select **"Edit the system environment variables"**.
3. Click the **"Environment Variables"** button at the bottom.
4. Under "System variables", find the `Path` variable, select it, and click **Edit**.
5. Click **New** and paste the folder path where you placed the `.exe` (e.g., `C:\Program Files\y_lang\`).
6. Click **OK** on all windows. Restart your terminal for the changes to take effect.

**For Linux:**
1. Open your terminal.
2. Move the downloaded binary to your user binaries folder and grant it execution permissions:
```bash
   sudo mv /path/to/downloaded/y_lang_linux /usr/local/bin/
   sudo chmod +x /usr/local/bin/y_lang_linux
```

### 10.3 Running the Interpreter

**Executing a Script (VM Mode):**
To run a `y--` program, call the binary and pass the **absolute path** to your `.ymm` file as the argument.

```bash
y_lang /absolute/path/to/your/script.ymm

```

*(Note: Windows users using the command prompt or PowerShell can use standard Windows paths like `y_lang C:\Projects\script.ymm`)*

**Interactive REPL Mode:**
If you launch the binary without providing any file path arguments, it will automatically boot into the interactive REPL shell for live coding and testing.

```bash
y_lang

```

### 10.4 Example Programs

If you want to see what `y--` is capable of, navigate to the `example programs` folder in the GitHub repository. It contains ready-to-run `.ymm` scripts showcasing core syntax, algorithms, and native Raylib graphics implementations!
