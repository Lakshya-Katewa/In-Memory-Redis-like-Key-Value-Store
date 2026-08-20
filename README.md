# In-Memory Key-Value Store

A lightweight **in-memory key-value database built in C++**, designed to provide fast data access with average **O(1)** operations. The project uses a modular OOP architecture with support for key expiration and file-based persistence for database recovery.

## Features

* **Fast Operations** — Uses `std::unordered_map` for average O(1) key-value operations.
* **Modular Architecture** — Separates command parsing, storage, expiration, and database management.
* **Key Expiration** — Supports TTL-based key expiration using `std::chrono`.
* **File Persistence** — Stores database data on disk for recovery across application restarts.
* **OOP Design** — Uses classes and encapsulation to keep components modular and maintainable.

## Architecture

```text
User Command
     │
     ▼
┌─────────────────┐
│  Command Parser │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Storage Engine  │
└───────┬─────────┘
        │
   ┌────┴─────┐
   ▼          ▼
Expiry      Database
Manager     Persistence
   │          │
   └────┬─────┘
        ▼
   In-Memory Store
   (unordered_map)
```

### Components

**Command Parser**

* Parses user commands and arguments.
* Validates command structure before execution.
* Routes commands to the appropriate database operations.

**Storage Engine**

* Handles key-value storage and retrieval.
* Uses `std::unordered_map` for efficient average-case access.

**Expiry Manager**

* Maintains expiration information for keys.
* Uses `std::chrono` to track TTL and expiration times.
* Removes or invalidates expired keys during access.

**Database**

* Coordinates the storage and expiration components.
* Handles file persistence and recovery.
* Restores previously stored data when the application restarts.

## Supported Operations

The database supports operations such as:

```text
SET key value
GET key
DELETE key
EXISTS key
```

Keys can also be configured with an expiration time using TTL functionality.

> Update this section if your implementation uses different command names or supports additional commands.

## Complexity

| Operation       | Average Complexity |
| --------------- | -----------------: |
| Insert / SET    |               O(1) |
| Lookup / GET    |               O(1) |
| Delete          |               O(1) |
| Existence Check |               O(1) |

These are **average-case complexities** based on `std::unordered_map`. Hash collisions can result in worse-case behavior.

## Persistence

The database supports file-based persistence so that stored data can be recovered after restarting the application.

```text
Application Running
       │
       ▼
In-Memory Data
       │
       ▼
  Save to File
       │
       ▼
Application Restart
       │
       ▼
Load from File
       │
       ▼
Database Recovery
```

## Tech Stack

* **Language:** C++
* **Data Structures:** `unordered_map`, `unordered_set`
* **Concepts:** OOP, Hashing, File I/O, TTL, Persistence
* **Libraries:** C++ Standard Library, `chrono`, file streams

## Getting Started

### Clone the Repository

```bash
git clone <repository-url>
cd <repository-name>
```

### Compile

Using `g++`:

```bash
g++ -std=c++17 -O2 -o kvstore main.cpp
```

### Run

```bash
./kvstore
```

On Windows:

```bash
kvstore.exe
```

## Example

```text
> SET name Lakshya
OK

> GET name
Lakshya

> EXISTS name
1

> DELETE name
OK

> GET name
NULL
```

## Project Structure

```text
.
├── main.cpp
├── CommandParser.*
├── StorageEngine.*
├── ExpiryManager.*
├── Database.*
├── data/
└── README.md
```

> Adjust the file structure above to match your actual repository.

## Learning Outcomes

This project helped develop practical understanding of:

* Hash-table based data storage
* Average vs. worst-case time complexity
* Object-oriented software design
* Modular component architecture
* C++ `std::chrono` and time-based operations
* File I/O and data persistence
* Database recovery across application restarts

## Future Improvements

* Add concurrent client support using threads
* Implement background cleanup for expired keys
* Add more database commands
* Improve persistence with safer serialization
* Add unit and integration tests
* Introduce command-line configuration options

## License

This project is intended for educational and personal development purposes.
