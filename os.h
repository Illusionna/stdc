#ifndef _OS_H_
#define _OS_H_


#if !defined(__OS_WINDOWS__) && !defined(__OS_UNIX__)
    #if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
        #define __OS_WINDOWS__
    #elif defined(__linux__) || defined(__APPLE__)
        #define __OS_UNIX__
        #define _GNU_SOURCE
    #else
        #error "Unsupported platforms."
    #endif
#endif


#if defined(__OS_WINDOWS__)
    #include <direct.h>
    #include <windows.h>
#elif defined(__OS_UNIX__)
    #include <fcntl.h>
    #include <dirent.h>
    #include <unistd.h>
    #include <limits.h>
    #include <sys/mman.h>
#endif


#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>


#include "type.h"


#if defined(__OS_UNIX__)
    #define os_ftell ftello
    #define os_fseek fseeko
    #define os_flockfile flockfile
    #define os_funlockfile funlockfile
#elif defined(__OS_WINDOWS__)
    #define os_ftell _ftelli64
    #define os_fseek _fseeki64
    #define os_flockfile _lock_file
    #define os_funlockfile _unlock_file
#endif



#if defined(__OS_UNIX__)
    typedef struct {
        void *data;
        usize size;
        int fd;
    } MapFile;
#elif defined(__OS_WINDOWS__)
    typedef struct {
        void *data;
        size_t size;
        HANDLE hFile;
        HANDLE hMapping;
    } MapFile;
#endif


typedef void (*_ListdirCallback)(char *dir, char *name, bool folder);


/**
 * @brief Get process ID.
 * @return PID.
**/
int os_getpid();


/**
 * @brief Judge whether the file exists.
 * @param path The pointer of file path string.
 * @return `1` for existence, `0` for nonentity, `-1` for other errors (such as permission denied, etc.)
**/
int os_access(char *path);


/**
 * @brief Read file and convert it to string (note: the memory allocated must call free).
 * @param path The pointer of file path string.
 * @param range_start The start of range.
 * @param range_end The end of range.
 * @return The string content of file.
**/
char *os_readfile(char *path, int range_start, int range_end);


/**
 * @brief Get current time (`unit: s`).
 * @return Timestamp.
**/
double os_time();


/**
 * @brief Get the file name from a path.
 * @param path The path of file.
 * @return File name (note: `/home/Desktop/` returns `NULL`).
**/
char *os_basename(char *path);


/**
 * @brief Wait for a time (`unit: s`).
 * @param second You can set `3s` or `0.02s`.
**/
void os_sleep(double second);


/**
 * @brief Determine whether the path is a directory.
 * @param path The path.
 * @return `1` for directory, `0` for file or inexistence.
**/
int os_isdir(char *path);


/**
 * @brief Make a directory.
 * @param dir Name of folder.
 * @return `0` for success, `1` for failure.
**/
int os_mkdir(char *dir);


/**
 * @brief Get the current directory.
 * @param buffer Store the current directory.
 * @param size The size of buffer.
 * @return `NULL` for failure.
**/
char *os_getpwd(char *buffer, int size);


/**
 * @brief Get the executable directory.
 * @param buffer Store the executable directory.
 * @param size The size of buffer.
**/
void os_getexec(char *buffer, int size);


/**
 * @brief Initialize the seed for the rand function.
**/
void os_srand();


/**
 * @brief Generate a random value in zone `[low, high]`.
 * @param low The low value of zone.
 * @param high The high value of zone.
 * @return The random value.
**/
double os_random(double low, double high);


/**
 * @brief Get the size of file.
 * @param filepath The path of file.
 * @return The size of file (`-1` for failure).
**/
int64 os_filesize(char *filepath);


/**
 * @brief Map a file into the process's virtual address space.
 * @param filepath The path of the file to be mapped.
 * @param length The number of bytes to map from the beginning of the file (`usize` is `size_t`).
 * @return A pointer to a `MapFile` structure containing the memory address (`NULL` for failure).
 * @example
 * @code
MapFile *f = os_mmap("demo.txt", 12);
if (f && f->data) {
    printf("%c\n", ((char *)f->data)[7]);
    os_munmap(f);
} else {
    printf("Failure!\n");
}
 * @endcode
**/
MapFile *os_mmap(char *filepath, usize length);


/**
 * @brief Unmap a previously mapped file and release associated system resources.
 * @param f The pointer to the `MapFile` structure to be released.
**/
void os_munmap(MapFile *f);


/**
 * @brief List the contents of a directory.
 * @param path The path of directory.
 * @param func The callback function like `void recurse(char *dir, char *name, bool folder)`. Go to the `os.h` declaration to see how to use `_ListdirCallback func`.
 * @example
 * @code
void recurse(char *dir, char *name, bool folder) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    printf("%s\n", path);
    if (folder) os_listdir(path, recurse);
}
 * @endcode
**/
void os_listdir(char *path, _ListdirCallback func);


/**
 * @brief Check whether the path exceeds the boundary.
 * @param path A path like `"./documents/../main.tex"`.
 * @return `1` for `True`, `0` for `False`.
**/
bool os_traversal(char *path);


#endif
