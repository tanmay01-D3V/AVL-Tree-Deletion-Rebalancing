# AVL Studio

![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)
![CMake](https://img.shields.io/badge/CMake-3.24%2B-064F8C?logo=cmake&logoColor=white)
![SQLite](https://img.shields.io/badge/storage-SQLite-003B57?logo=sqlite&logoColor=white)
![Frontend](https://img.shields.io/badge/frontend-server--rendered%20HTML%20%2B%20CSS-46685B)

**AVL Studio** is an educational AVL-tree visualizer and reservation manager
powered by the same C++17 AVL implementation. The native C++ server handles
HTTP requests, application logic, and HTML/SVG generation. The browser uses
HTML forms and CSS only—there is no JavaScript, TypeScript, or frontend
framework.

> Learn AVL rotations by interacting with the tree, then see the same data
> structure index real reservation records.

**Pages:** [AVL visualizer](#avl-visualizer) ·
[Reservation manager](#reservation-manager) · [Build and run](#build-and-run) ·
[Tests](#tests)

## Features

- Insert, search, and delete unique integer keys with LL, RR, LR, and RL
  rebalancing. Deletion rebalances every affected ancestor.
- Explore the live tree, node heights, balance factors, sorted keys, tree
  height, root balance, and operation history in the AVL visualizer.
- Create, view, search, update, filter, and delete reservations indexed by
  reservation ID in the existing AVL tree.
- Persist reservation records in SQLite and rebuild/verify the in-memory AVL
  index at startup.
- Generate tree diagrams from live C++ AVL snapshots as server-rendered SVG.
- Run a reproducible reservation demonstration, including all rotation
  patterns and deletion cases.
- Validate the AVL tree and reservation/index synchronization with automated
  tests, including randomized comparisons with `std::set`.

A null subtree has height `0`, a leaf has height `1`, and balance factor is
`height(left) - height(right)`. A balanced node has a factor from `-1` through
`1`.

## Architecture

```text
Browser: server-rendered HTML forms + CSS
                  │ GET / POST
                  ▼
          C++ HTTP application
           ┌──────┴──────┐
           ▼             ▼
      Existing AVL   Reservation service
                          │
                          ▼
                    SQLite database
```

The AVL tree is implemented once in the shared C++ library. The visualizer
keeps its demonstration tree in memory; the reservation service persists
metadata in SQLite and reconstructs its AVL index from reservation IDs at
startup. Tree diagrams are generated as SVG from snapshots of the live C++
tree.

## Team

This project was developed as a DSA-II group project. The GitHub issue
assignments were:

- **Tanmay Vijay Sherkar:** project architecture ([#1](https://github.com/tanmay01-D3V/AVL-Tree-Deletion-Rebalancing/issues/1)),
  console application/integration ([#6](https://github.com/tanmay01-D3V/AVL-Tree-Deletion-Rebalancing/issues/6)),
  and shared validation/documentation ([#7](https://github.com/tanmay01-D3V/AVL-Tree-Deletion-Rebalancing/issues/7)).
- **Alina Patel:** rotations and balancing ([#2](https://github.com/tanmay01-D3V/AVL-Tree-Deletion-Rebalancing/issues/2)).
- **Shubham Mane:** insertion/search ([#3](https://github.com/tanmay01-D3V/AVL-Tree-Deletion-Rebalancing/issues/3))
  and traversals/utilities ([#5](https://github.com/tanmay01-D3V/AVL-Tree-Deletion-Rebalancing/issues/5)).
- **Aditya Kadam:** deletion and upward rebalancing ([#4](https://github.com/tanmay01-D3V/AVL-Tree-Deletion-Rebalancing/issues/4)).
- **All members:** shared testing and documentation ([#7](https://github.com/tanmay01-D3V/AVL-Tree-Deletion-Rebalancing/issues/7)).

Assignments describe the work plan, not exact file authorship. For example,
[PR #15](https://github.com/tanmay01-D3V/AVL-Tree-Deletion-Rebalancing/pull/15)
was authored by Aditya Kadam and contains work across several assigned
modules.

## Project structure

```text
include/AVLNode.h           AVL node definition
include/AVLTree.h           AVL API, snapshots, and rotation events
include/Reservation.h       Reservation model and validation helpers
include/ReservationStore.h  SQLite-backed service and AVL index
include/WebApplication.h    HTTP routes and server-rendered page interface
src/AVLTree.cpp             Ownership, heights, balance factors, cleanup
src/AVLBalance.cpp          Rotations and rebalancing
src/AVLInsert.cpp           Insertion and search
src/AVLDelete.cpp           Removal and upward rebalancing
src/AVLTraversal.cpp        Inorder, preorder, and postorder traversals
src/Reservation.cpp         Reservation rules and verified demo dataset
src/ReservationStore.cpp    SQLite persistence and index synchronization
src/WebApplication.cpp     C++ routes, HTML, and SVG generation
web/styles.css              Responsive visual design
web/public/favicon.svg      Favicon
web/DESIGN.md               Visual direction and responsive layout
tests/tests.cpp             AVL behavior and invariant tests
tests/reservation_tests.cpp Reservation, persistence, and randomized tests
main.cpp                    Interactive console application
web_server_main.cpp         Native HTTP server entry point
cmake/                      Shared AVL source manifest
CMakeLists.txt              App and test build configuration
```

## Prerequisites

- CMake 3.24 or newer
- A C++17-compatible compiler
- SQLite3 development headers and library
- Internet access for the first configure, so CMake can fetch the pinned
  cpp-httplib v0.24.0 source archive (unless it is already cached)

Example setup commands:

```sh
# Ubuntu / Debian
sudo apt-get update
sudo apt-get install build-essential cmake libsqlite3-dev

# macOS (Xcode Command Line Tools provide the compiler and SQLite headers)
xcode-select --install
brew install cmake
```

No Node.js, npm, Emscripten, or JavaScript runtime is required.

## Build and run

From the repository root:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/avl_server
```

The server listens on `127.0.0.1:8080` by default. Visit
<http://127.0.0.1:8080/> for the visualizer or
<http://127.0.0.1:8080/reservations> for the reservation manager. Stop the
server with **Ctrl+C**.

The persistent SQLite database defaults to `data/avl-studio.sqlite3`. Server
options can be passed as command-line arguments or set through environment
variables (`AVL_STUDIO_HOST`, `AVL_STUDIO_PORT`, and `AVL_STUDIO_DB_PATH`).
Command-line arguments take precedence:

```sh
./build/avl_server --host 127.0.0.1 --port 8081 --db ./data/demo.sqlite3
./build/avl_server --help
```

Use `--db :memory:` for a temporary, non-persistent database. The separate
interactive console app is also available:

```sh
./build/avl_demo
```

## Pages and interactions

### AVL visualizer

The visualizer supports insertion, search, deletion, a resettable sample tree,
and LL/RR/LR/RL examples. It displays the current SVG tree, stored node heights,
balance factors, sorted keys, tree height, root balance factor, and recent
operations. The visualizer uses the same AVL library as the console app and
reservation manager. All state-changing actions are C++ routes handling POST
forms.

### Reservation manager

Each reservation has a unique positive integer ID, customer name,
service/resource, date, time, number of guests/resources, and status
(`Confirmed`, `Pending`, or `Cancelled`). The dashboard supports creating,
viewing, searching, editing, deleting, and filtering reservations. It also
shows counts by status, AVL height/balance, the live ID tree, and operation
history.

Reservation metadata is stored in SQLite; reservation IDs are indexed by the
existing C++ AVL tree. At startup, the index is rebuilt from the database and
checked against the records. Changing a reservation ID updates the record and
AVL index together. The SVG tree and rotation history reflect actual C++
operations, not a separate demo model.

### Verified demonstration

On an empty reservation database, choose **Load sample reservations**, then
**Run verified demo sequence**. The twelve-record sample triggers all four
rotation patterns. The sequence searches for `#1011`, deletes leaf `#1004`
(causing two ancestor rotations), then deletes `#1006` (one child) and `#1008`
(two children). The app will not overwrite existing reservations; use a new
database path or `--db :memory:` to run the sample independently.

## HTTP routes

All data-changing routes accept POST requests from native HTML forms. Search
and navigation use GET requests.

- `GET /` — AVL visualizer; `GET /?search=<key>` searches the current tree.
- `POST /visualizer/insert`, `/visualizer/delete`, `/visualizer/reset`, and
  `/visualizer/example` — mutate or demonstrate the visualizer tree.
- `GET /reservations` — reservation dashboard; optionally filter with
  `?status=Confirmed`, `?status=Pending`, or `?status=Cancelled`.
- `GET /reservations/search?id=<id>`, `/reservations/view?id=<id>`, and
  `/reservations/edit?id=<id>` — find, view, or edit a reservation.
- `POST /reservations/create`, `/reservations/update`, and
  `/reservations/delete` — reservation CRUD actions.
- `POST /reservations/demo/load` and `/reservations/demo/run` — load and run
  the verified demonstration.

## Tests

Run the CTest suite after building:

```sh
ctest --test-dir build --output-on-failure
```

Tests cover all rotation patterns, insertion and duplicate handling, search,
leaf/one-child/two-child and root deletion, deletion requiring rebalancing at
multiple ancestors, traversal order, empty-tree behavior, randomized
`std::set` comparisons, reservation validation/CRUD, SQLite persistence, index
synchronization, and the verified demonstration sequence. Invariant checks
cover BST ordering, stored heights, and every node's AVL balance.

## Known limitations

- The HTTP application is a local development/demo server, not a production
  deployment: it has no authentication, TLS, or multi-user access control.
- The visualizer tree and operation history are in memory for the lifetime of
  the server; reservation records persist in SQLite.
- The console application operates on integer keys and does not persist its
  tree after exit.
- The tree is intended for single-threaded mutation.
