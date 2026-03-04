#include "os.h"


static unsigned long long OS_SEED = 1;


int os_getpid() {
    #if defined(__OS_WINDOWS__)
        return (int)GetCurrentProcessId();
    #elif defined(__OS_UNIX__)
        return (int)getpid();
    #endif
}


int os_access(char *path) {
    FILE *f = fopen(path, "r");
    if (f) {
        fclose(f);
        return 1;
    } else {
        if (errno == ENOENT) return 0;
        return -1;
    }
}


char *os_readfile(char *path, int range_start, int range_end) {
    if (range_start == 0 && range_end == -1) {
        FILE *f = fopen(path, "r");
        if (!f) return NULL;

        fseek(f, 0, SEEK_END);
        long length = ftell(f);
        rewind(f);

        char *buffer = (char *)malloc(sizeof(char) * (length + 1));
        if (!buffer) {
            fclose(f);
            return NULL;
        }

        buffer[fread(buffer, 1, length, f)] = '\0';
        fclose(f);
        return buffer;
    } else {
        FILE *f = fopen(path, "r");
        if (!f) return NULL;

        int length = range_end - range_start + 1;
        fseek(f, range_start, SEEK_SET);
        char *buffer = (char *)malloc(sizeof(char) * (length + 1));
        if (!buffer) {
            fclose(f);
            return NULL;
        }
        buffer[fread(buffer, 1, length, f)] = '\0';
        fclose(f);
        return buffer;
    }
}


double os_time() {
    #if defined(__OS_UNIX__)
        struct timespec t;
        // `CLOCK_MONOTONIC` prevents system time from being tampered with.
        clock_gettime(CLOCK_MONOTONIC, &t);
        return (double)t.tv_sec + (double)t.tv_nsec / 1e9;
    #elif defined(__OS_WINDOWS__)
        LARGE_INTEGER frequency;
        LARGE_INTEGER counter;
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&counter);
        return (double)counter.QuadPart / frequency.QuadPart;
    #else
        return (double)time(NULL);
    #endif
}


char *os_basename(char *path) {
    char *base = path;
    if (path == NULL || *path == '\0') return NULL;
    for (char *p = path; *p; p++) if (*p == '/' || *p == '\\') base = p + 1;
    if (*base == '\0') return NULL;
    return base;
}


void os_sleep(double second) {
    if (second <= 0.0) return;
    #if defined(__OS_UNIX__)
        struct timespec requested_time;
        struct timespec remaining_time;
        requested_time.tv_sec = (long)second;
        requested_time.tv_nsec = (long)(1e9 * (second - requested_time.tv_sec));
        while (nanosleep(&requested_time, &remaining_time) == -1 && errno == EINTR) requested_time = remaining_time;
    #elif defined(__OS_WINDOWS__)
        DWORD millisecond = (DWORD)(second * 1000.0);
        if (millisecond == 0 && second > 0) millisecond = 1;
        Sleep(millisecond);
    #endif
}


int os_isdir(char *path) {
    if (path == NULL || *path == '\0') return 0;

    #if defined(__OS_UNIX__)
        struct stat s;
        if (stat(path, &s) != 0) return 0;
        return S_ISDIR(s.st_mode);
    #elif defined(__OS_WINDOWS__)
        DWORD attrs = GetFileAttributesA(path);
        if (attrs == INVALID_FILE_ATTRIBUTES) return 0;
        if (attrs & FILE_ATTRIBUTE_DIRECTORY) return 1;
        return 0;
    #endif
}


int os_mkdir(char *dir) {
    int result = 0;
    errno = 0;
    #if defined(__OS_UNIX__)
        result = mkdir(dir, 0755);
    #elif defined(__OS_WINDOWS__)
        result = _mkdir(dir);
    #endif
    if (result == 0) return 0;
    else {
        if (errno == EEXIST) return 0;
        else return 1;
    }
}


char *os_getpwd(char *buffer, int size) {
    #if defined(__OS_UNIX__)
        return getcwd(buffer, size);
    #elif defined(__OS_WINDOWS__)
        return _getcwd(buffer, size);
    #endif
}


void os_getexec(char *buffer, int size) {
    if (size == 0) return;
    memset(buffer, 0, size);

    #if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
        if (GetModuleFileName(NULL, buffer, (DWORD)size) == 0) {
            fprintf(stderr, "Error getting module file name.\n");
            return;
        }
    #elif defined(__linux__)
        int n = readlink("/proc/self/exe", buffer, size - 1);
        if (n == -1) {
            fprintf(stderr, "Error reading /proc/self/exe.\n");
            return;
        }
        if (n > 0 && n < size) buffer[n] = '\0';
    #elif defined(__APPLE__)
        unsigned int uint32size = (unsigned int)size;
        if (_NSGetExecutablePath(buffer, &uint32size) != 0) {
            fprintf(stderr, "Buffer too small; need size %u\n", uint32size);
            return;
        }
    #endif

    #if defined(__OS_UNIX__)
        char *last_slash = strrchr(buffer, '/');
    #elif defined(__OS_WINDOWS__)
        char *last_slash = strrchr(buffer, '\\');
    #endif

    if (last_slash != NULL) *last_slash = '\0';
    else fprintf(stderr, "Warning: No path separator found.\n");
}


void os_srand() {
    unsigned int i = (unsigned int)time(NULL);
    OS_SEED = (((long long int)i) << 16) | rand();
}


double os_random(double low, double high) {
    OS_SEED = (0x5DEECE66DLL * OS_SEED + 0xB16) & 0xFFFFFFFFFFFFLL;
    return low + (high - low) * ((double)(OS_SEED >> 16) / (double)0x100000000LL);
}


unsigned long long os_filesize(char *filepath) {
    #if defined(__OS_UNIX__)
        struct stat file;
        if (stat(filepath, &file) == -1) return 0;
        return (long long)file.st_size;
    #elif defined(__OS_WINDOWS__)
        struct _stat file;
        if (_stat(filepath, &file) == -1) return 0;
        return (long long)file.st_size;
    #endif
}


MapFile *os_mmap(char *filepath, usize length) {
    MapFile *f = malloc(sizeof(MapFile));
    f->size = length;
    #if defined(__OS_UNIX__)
        f->fd = open(filepath, O_RDONLY);
        if (f->fd == -1) return NULL;
        f->data = mmap(NULL, length, PROT_READ, MAP_PRIVATE, f->fd, 0);
        if (f->data == MAP_FAILED) {
            close(f->fd);
            return NULL;
        }
    #elif defined(__OS_WINDOWS__)
        f->hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (f->hFile == INVALID_HANDLE_VALUE) return NULL;
        f->hMapping = CreateFileMapping(f->hFile, NULL, PAGE_READONLY, 0, 0, NULL);
        if (f->hMapping == NULL) {
            CloseHandle(f->hFile);
            return NULL;
        }
        f->data = MapViewOfFile(f->hMapping, FILE_MAP_READ, 0, 0, length);
    #endif
    return f;
}


void os_munmap(MapFile *f) {
    if (!f) return;
    #if defined(__OS_UNIX__)
        munmap(f->data, f->size);
        close(f->fd);
    #elif defined(__OS_WINDOWS__)
        UnmapViewOfFile(f->data);
        CloseHandle(f->hMapping);
        CloseHandle(f->hFile);
    #endif
    free(f);
}
