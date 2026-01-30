# y-- Language

![WhatsApp Image 2026-01-28 at 15 01 36](https://github.com/user-attachments/assets/71d3098c-122f-46dc-8e58-dd65d92b0c03)

## Overview

y-- is a **tree‑walker interpreted language** written in **modern C++**. It is currently at **v1.0** and designed as a **personal language project** with a long‑term roadmap toward a **Bytecode VM** and eventually **LLVM**.

The language aims to be:

* **Beginner‑friendly** and forgiving
* **Explicit and controllable** for advanced users
* **Readable**, resembling spoken English mixed with Python/TypeScript
* **Structurally familiar**, borrowing execution rules from C

Primary target domains:

* **Game development**
* **Robotics and simulations**

Execution is explicit and controlled. The language **requires a `main()` function** and does not execute code outside of it.

---

## Architecture

* **Interpreter type**: AST / Tree Walker
* **Input**: Source file is read as a text document and passed through a lexer → parser → runtime
* **Future plans**:

  1. Bytecode Virtual Machine
  2. Object‑Oriented Programming
  3. LLVM backend (final target)

Currently supported platforms:

* Windows (tested)
* Linux / macOS (not yet tested)

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

## Types & Hybrid Typing

y-- uses **hybrid typing**, allowing dynamic usage with optional type locking.

### Built‑in Types

* `int`
* `float`
* `bool`
* `string`
* `list`
* `tuple`
* `set`
* `dict`
* `None`
* `vector` (requires module import)

All types are **value‑typed**, internally backed by **C++ shared pointers** and freed automatically when out of scope.

---

## Variable Declarations

```y--
let x                ~ NoType
let x := int()       ~ type‑locked int, initialized to 0
let x = int("123")   ~ cast string → int
let x, y, z = [], {}, ""
let const x = "data"
```

* `:=` locks the variable type
* `const` makes variables immutable
* Mutating a const throws `ConstError`

---

## Constructors & Casting

All types have constructors:

```y--
int() bool() float() string()
list() tuple() set() dict()
range() vector()
```

Rules:

* Empty constructors create the **false state** of the type
* Casting is allowed **only if logically valid**
* Invalid casts throw `TypeError`

---

## Ownership & Copy Semantics

| Operator | Meaning               |
| -------- | --------------------- |
| `@`      | Reference assignment  |
| `$`      | Deep copy (recursive) |
| none     | Shallow copy          |

Examples:

```y--
let x = $y   ~ deep copy
let x = @y   ~ reference
let x = y    ~ shallow copy
```

Deep copy is **safe but expensive**.
Reference passing is **explicit by design**.

---

## Operators

### Arithmetic

```
+  -  *  /  %  **
++  --  (postfix only)
```

### Assignment

```
+=  -=  *=  /=  %=  **=
:=   (type lock)
```

### Boolean

```
not and nand xor nxor or nor
```

### Boolean Assignment

```
&=  |=  ^=
```

### Comparison

```
<  <=  >  >=
==   !=
===  !==
```

* `==` loose equality (truthiness)
* `===` strict equality (type + value)

```y--
[1,2,3] == "hello"   ~ true
[1,2,3] === "hello"  ~ false
[1,2,[3]] === [1,2,[3]]  ~true 
```

Short‑circuiting is supported.

---

## Ternary

```y--
A if condition else B
```

---

## Control Flow

### If / Elif / Else

```y--
if cond : print(1)
if cond:{ }
elif cond:{ }
else:{ }
```

Everything except `false`, `None`, `NoType`, `0`, `0.0`, empty containers and invalid ranges are true.

---

### Loops

C-style loops (For-Do, Do-For) support the `omit` keyword, which means that part is ignored.

#### For‑Do

```y--
for let i = 0 if cond then i++ do:{ } ~Normal loop.
for let i, j, k = 0, 1, 2 if cond then i++, j--, k*=2 do: { } ~Multi variable loop.
for omit if cond then omit do: { } ~Can omit one or both deceleration and incremention.
```

#### Do‑For

```y--
for let i = 0 if cond do:{ } then i++ ~Normal loop.
for let i,j,k = 0,1,2 if cond do: { } then i++, j--, k*=2 ~Multi variable loop.
for omit if cond do: { } then omit ~Can omit one or both deceleration and incremention.
```

#### While / Do‑While

```y--
while cond do:{ }
do:{ } while cond
```

#### For‑Each

```y--
for i in collection do:{ }
for i, j, k in collection1, collection2, collection3 do:{ } 
```

* Supports `break` and `continue`
* `for‑each` supports `skip n`
* Loop variables declared inside loop are destroyed afterward

---

## Switch

```y--
switch (x):{
  case a then do:
  case b then do:
  case c do:
  default do:
}
```

* Uses **strict equality** (`===`)
* No fallthrough
* Loop‑based execution
* Missing `break` repeats the switch
* `default` is mandatory
---

## Error Handling

```y--
try:{ }
catch ErrA, ErrB then do:{ }
catch ErrC do:{ }
else do:{ }
finally do:{ }
```

Rules:

* Sub‑errors are caught by parent errors
* `else` runs only if nothing was caught
* `finally` always runs
* `finally` return overrides other returns

---

## Assertions

Assertions are **runtime checks** intended for debugging.

* Failed assertions throw `AssertionError`
* They immediately stop execution unless caught

---

## Truthiness Rules

Falsy values:

* `False`
* `0`, `0.0`
* `None`
* `NoType`
* Empty containers
* Invalid ranges

Everything else is truthy.

---

## Identity Operators

```y--
a is b
```

* Checks if both refer to the **same object in memory**
* Will do this for **ALL** types.

```y--
a is in b
```

* Membership test for containers

---

## Functions

```y--
define function foo(const a, b, c:int(), @d, $e,
                     f:string("abc"), *args, **kwargs) -> int(3):{
}
```

Rules:

* Functions must be defined outside `main()`
* Function overloading exists
* Default return type is `NoType`
* Empty `return` returns default type
* `-> int(3)` enforces return type and default

---

## Function Calls & Ownership

```y--
foo(11, (1,2), 5, @m, d, omit, 10, 12, "origin":(0,0))
```

Rules:

* `@` **must be explicit** in calls
* `$` not needed twice
* `*args` becomes a tuple
* `**kwargs` becomes a dict
* Missing args throw argument errors
* `omit` throws ArgumentError if used on an argument with no default value.
---

## Lambdas

```y--
let x = define lambda function sq(n:int())->int():{
  return n*n
}

x(10)
```

Rules:

* Must be assigned to a variable
* Same rules as functions

---

## Cached Keyword in lambdas and functions.

```y--
define cached function fib(n:int())->int():{ }
```

* Uses internal C++ hashmap
* Ideal for recursion
* Converts exponential time to linear
---

## Modules

Pure C++ standard modules:

* Math
* FileStream
* OS
* System
* Random
* Vector

---

## Comments

### Single‑line

```y--
~ this is a comment
```

### Multi‑line

```y--
`
Anything inside here is ignored
`
```

---

## Status

* Feature‑complete for v1.0
* Currently stress‑testing
* Personal project
* Bytecode VM is next milestone

---

**y-- is explicit by design.**
**Nothing is hidden.**
**Control is always in the developer’s hands.**
