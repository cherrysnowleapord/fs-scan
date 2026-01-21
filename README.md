# 🗂️ FileSystem Scanner (fsscan)

## 📝 Overview

A flexible, configurable filesystem scanning library written in C that provides recursive directory traversal with advanced filtering capabilities. Designed for security tools, file analysis, and system monitoring applications.
This was made start to finish in 2 hours so there might still be some bugs, however from my testing the main functionality is working. Any untested conditions can cause a crash though.

## ✨ Features

- **Recursive Directory Scanning**: Deep filesystem traversal with nesting limits
- **Advanced Filtering**: Blacklist/whitelist support for paths and file types
- **Resource Management**: Configurable limits for memory and file descriptor usage
- **Flexible Callback System**: Custom processing functions for scanned files
- **Cross-Platform Ready**: Standard C library implementation

## 🛠️ Technical Highlights

### Key Capabilities

- **Path Filtering**: Exclude/include specific directories and file types
- **Resource Limits**: Control memory usage, nesting depth, and scan limits
- **File Type Detection**: Filter by file type (regular files, directories, etc.)
- **Access Control**: Permission-based filtering
- **Custom Processing**: Pluggable callback functions for file handling

## 📋 API Reference

### Configuration Structure

```c
typedef struct {
    int mode;           // Access mode filter (0 = no filter)
    int ftype;          // File type filter (S_IFREG, S_IFDIR, etc.)
    
    size_t fptr_limit;  // Maximum file pointers (use -1 for unlimited)
    size_t nest_limit;  // Maximum directory nesting depth (use -1 for unlimited)
    size_t scan_limit;  // Maximum files to scan (use -1 for unlimited)
    
    const char **blacklist;     // Paths to exclude
    size_t blacklist_size;      // Size of blacklist array
    
    const char **whitelist;     // Paths to include (if specified)
    size_t whitelist_size;      // Size of whitelist array
    
    bool start_enabled;         // Whether to use custom start paths
    const char **start;         // Custom starting paths
    size_t start_size;          // Size of start array
    
    fsscan_f func;              // Callback function for processing files
} fsscan_config_t;
```

### Main Function

```c
int fsscan(fsscan_config_t *config);
```

Returns the total number of files scanned.

### Usage Example

```c
// Simple file scanner
int ret = fsscan(&(fsscan_config_t){
    .mode = 0,              // No access mode filtering
    .ftype = S_IFREG,       // Only regular files
    .nest_limit = 3,        // Maximum 3 levels deep
    .scan_limit = -1,       // No scan limit
    .fptr_limit = -1,       // No file pointer limit
    .func = my_callback,    // Your custom processing function
});

printf("Scanned %d files\n", ret);
```

## ⚙️ Configuration Options

### Filtering Options
- **File Type Filtering**: `S_IFREG` (regular files), `S_IFDIR` (directories), etc.
- **Access Mode Filtering**: `R_OK`, `W_OK`, `X_OK` for read/write/execute permissions
- **Path Blacklisting**: Automatically excludes `/proc`, `/dev/fd` and custom paths
- **Path Whitelisting**: Include only specified paths

### Resource Limits
- **Nesting Limit**: Control recursion depth to prevent infinite loops
- **Scan Limit**: Stop after processing N files
- **File Pointer Limit**: Control memory usage for path storage

## 🚀 Performance Characteristics

- **Memory Efficient**: Dynamic allocation with cleanup
- **Bounds Checking**: Prevents buffer overflows and resource exhaustion
- **Early Termination**: Respects all configured limits
- **Standard Library**: Portable across Unix-like systems

## 🔧 Internal Architecture

### Core Components
1. **Directory Traversal**: Recursive `opendir()`/`readdir()` implementation
2. **Filter Engine**: Multi-layer filtering (blacklist, whitelist, permissions, file type)
3. **Resource Manager**: Memory and file descriptor limit enforcement
4. **Callback Processor**: Custom file processing via function pointers

## 💡 Use Cases

- **Security Scanning**: Malware detection, configuration audits
- **File Analysis**: Inventory systems, duplicate detection
- **Backup Tools**: Selective file copying with filtering
- **System Monitoring**: Track file changes and access patterns
- **Development Tools**: Code search, project analysis

## ⚠️ Important Notes

- Currently excludes `/proc` and `/dev/fd` by default for safety
- Uses standard POSIX functions for portability
- Includes automatic memory management to prevent leaks
- Designed for Unix-like systems (Linux, macOS, BSD)
