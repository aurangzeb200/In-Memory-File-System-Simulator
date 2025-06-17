# 🗂️ In-Memory File System Simulator

This C++ project simulates a Unix-like file system in memory, using a tree-based structure to manage directories and files. It supports commands like `mkdir`, `cd`, `ls`, `touch`, `cat`, `mv`, `cp`, `grep`, and more, with features like symbolic links, permissions, and file content search. Perfect for learning about file system design and tree data structures.

## Features
- **Tree-Based Structure**: Organizes files and directories using a tree with parent, child, and sibling pointers.
- **Unix-Like Commands**: Implements `mkdir`, `cd`, `ls`, `pwd`, `touch`, `cat`, `rm`, `mv`, `cp`, `rename`, `rmdir`, `chmod`, `chown`, `find`, and `grep`.
- **File Metadata**: Tracks creation/modification times, owner, permissions, and file size.
- **Symbolic Links**: Supports creating and managing symbolic links.
- **File Content Search**: Implements `grep` to search file contents and `find` for case-sensitive/insensitive name matching.
- **File I/O**: Saves and loads file content to/from disk using serialization.

## Interesting Techniques Used
Let’s walk through some of the coolest parts of this project, step-by-step, like we’re drawing it out on a whiteboard. I’ll break it down so you can follow along, even if you’re new to some of these concepts.

### 1. Tree-Based File System Representation
The file system is modeled as a [tree data structure](https://en.wikipedia.org/wiki/Tree_(data_structure)), where each `Node` represents a file or directory. This is a natural ascend-descend hierarchy, where directories have `firstChild` and `nextSibling` pointers, and files have a `parent` pointer.

- **What’s happening?**: Each `Node` has a `name`, `isDirectory` flag, and pointers to its `firstChild` (for directories), `nextSibling`, and `parent`. The root node (`/`) starts the tree, and paths like `/home/docs` are resolved by traversing these pointers.
- **How it works**: To find a node for a path (e.g., `/home/docs`), we tokenize the path into components (`home`, `docs`), start at the root (or current directory for relative paths), and follow `firstChild` and `nextSibling` pointers to locate the target node. For example, if `home` is a child of `/`, and `docs` is a sibling of another node, we check each component until we find the node or return `nullptr`.
- **Why it’s cool**: This structure mimics real file systems (like ext4). It’s efficient for navigation (`cd`, `ls`) and supports hierarchical operations like recursive deletion (`rmdir`) or copying (`cp`). The time complexity for path resolution is O(n), where n is the number of path components.

### 2. Path Tokenization for Navigation
To handle paths like `/home/docs/file.txt`, the `tokenize` function splits the path into components using `/` as a delimiter.

- **What’s happening?**: We use a `stringstream` to split the path string into a `vector<string>`. For example, `/home/docs/file.txt` becomes `["home", "docs", "file.txt"]`.
- **How it works**: The `findNode` function processes each token. If it’s `..`, we move to the `parent`. If it’s `.`, we stay put. Otherwise, we search the `firstChild` list for a matching `name`. This handles both absolute (`/home`) and relative (`docs/file.txt`) paths.
- **Why it’s cool**: This approach is robust and supports complex paths, including edge cases like empty tokens or navigating up directories. It’s a clean way to parse paths without regex.

### 3. Recursive Tree Operations
Operations like `cp`, `rmdir`, and `grep` use recursive traversal to handle directory hierarchies.

- **What’s happening?**: For `cp`, we use `copyNode` to recursively copy a node and its children. For `rmdir`, `deleteTree` recursively deletes all children before the directory itself. For `grep`, we use a [BFS (Breadth-First Search)](https://en.wikipedia.org/wiki/Breadth-first_search) with a `queue` to search file contents.
- **How it works**: In `copyNode`, we create a new `Node`, copy its properties, and recursively copy its `firstChild` if it’s a directory. For `grep`, we push the `currentDirectory` into a `queue`, then process each node, checking its `content` for the pattern, and add its `firstChild` and `nextSibling` to the queue.
- **Why it’s cool**: Recursion makes operations on nested directories intuitive, and BFS ensures we visit all nodes efficiently. For `grep`, BFS guarantees we check every file without getting stuck in deep directory trees.

## Non-Obvious Libraries/Tools Used
- **[sstream](https://en.cppreference.com/w/cpp/header/sstream)**: Used for tokenizing paths (e.g., splitting `/home/docs` into components). It’s great for parsing strings without manual character-by-character iteration.
- **[ctime](https://en.cppreference.com/w/cpp/header/ctime)**: Provides `time(nullptr)` to track file creation and modification times, mimicking real file system metadata.
- **[queue](https://en.cppreference.com/w/cpp/container/queue)**: Used in `find` and `grep` for BFS traversal of the file system tree, ensuring efficient exploration of all nodes.
- **[fstream](https://en.cppreference.com/w/cpp/header/fstream)**: Handles file I/O for `saveToFile` and `loadFromFile`, enabling persistent storage of file content.

## Project Folder Structure

- **File_System.cpp**: The main source file containing the `Node`, `FileSystem`, and CLI logic.
- **README.md**: This documentation file.

## Build and Run
1. **Prerequisites**: A C++ compiler (e.g., g++ on Windows with MinGW or MSVC).
