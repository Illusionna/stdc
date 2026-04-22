#include "socket.h"


Socket socket_init() {
    #ifdef __OS_WINDOWS__
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return SOCKET_INVALID;
    #endif
    return 0;
}


Socket socket_create(int domain, int type, int protocol) {
    return socket(domain, type, protocol);
}


void socket_close(Socket s) {
    #if defined(__OS_WINDOWS__)
        if (s == SOCKET_INVALID) return;
        else closesocket(s);
    #elif defined(__OS_UNIX__)
        if (s == SOCKET_INVALID) return;
        else close(s);
    #endif
}


void socket_destroy() {
    #ifdef __OS_WINDOWS__
        WSACleanup();
    #endif
}


void socket_config(struct sockaddr_in *server, int domain, char *ip, int port) {
    #if defined(__OS_UNIX__)
        server->sin_family = domain;
        server->sin_addr.s_addr = inet_addr(ip);
        server->sin_port = socket_htons(port);
    #elif defined(__OS_WINDOWS__)
        server->sin_family = domain;
        server->sin_addr.S_un.S_addr = inet_addr(ip);
        server->sin_port = socket_htons(port);
    #endif
}


Socket socket_connect(Socket s, struct sockaddr_in *server, int size) {
    int status = connect(s, (struct sockaddr *)server, size);
    return status < 0 ? SOCKET_INVALID : 0;
}


long socket_send(Socket s, char *buffer, int length, int flag) {
    return send(s, buffer, length, flag);
}


long socket_sendto(Socket s, void *buffer, int length, int flag, struct sockaddr_in *to, int size) {
    return sendto(s, buffer, length, flag, (struct sockaddr *)to, size);
}


long socket_recv(Socket s, char *buffer, int length, int flag) {
    return recv(s, buffer, length, flag);
}


long socket_recvfrom(Socket s, void *buffer, int length, int flag, struct sockaddr_in *from, int *size) {
    #if defined(__OS_UNIX__)
        return recvfrom(s, buffer, length, flag, (struct sockaddr *)from, (socklen_t *)size);
    #elif defined(__OS_WINDOWS__)
        return recvfrom(s, buffer, length, flag, (struct sockaddr *)from, size);
    #endif
}


Socket socket_bind(Socket s, struct sockaddr_in *address_name, int size) {
    int status = bind(s, (struct sockaddr *)address_name, size);
    return status < 0 ? SOCKET_INVALID : 0;
}


Socket socket_listen(Socket s, int backlog) {
    int status = listen(s, backlog);
    return status < 0 ? SOCKET_INVALID : 0;
}


Socket socket_accept(Socket s, struct sockaddr_in *address, int *size_pointer) {
    #if defined(__OS_UNIX__)
        return accept(s, (struct sockaddr *)address, (socklen_t *)size_pointer);
    #elif defined(__OS_WINDOWS__)
        return accept(s, (struct sockaddr *)address, size_pointer);
    #endif
}


Socket socket_setopt(Socket s, int level, int optname, void *ctx, int size) {
    int status = 0;
    if (ctx == NULL && size == 0) {
        int opt = 1;
        #if defined(__OS_UNIX__)
            status = setsockopt(s, level, optname, (void *)&opt, sizeof(opt));
        #elif defined(__OS_WINDOWS__)
            status = setsockopt(s, level, optname, (char *)&opt, sizeof(opt));
        #endif
    } else {
        #if defined(__OS_UNIX__)
            status = setsockopt(s, level, optname, ctx, size);
        #elif defined(__OS_WINDOWS__)
            status = setsockopt(s, level, optname, (char *)ctx, size);
        #endif
    }
    return status < 0 ? SOCKET_INVALID : 0;
}


unsigned int socket_ntohl(unsigned int value) {
    return ntohl(value);
}


unsigned int socket_htonl(unsigned int value) {
    return htonl(value);
}


unsigned short socket_ntohs(unsigned short value) {
    return ntohs(value);
}


unsigned short socket_htons(unsigned short value) {
    return htons(value);
}


void socket_ipv4(char *buffer, int size) {
    if (buffer && size > 0) buffer[0] = '\0';

    #if defined(__OS_UNIX__)
        struct ifaddrs *ifa;
        struct ifaddrs *ifaddr;
        if (getifaddrs(&ifaddr) == -1) return;
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr == NULL) continue;
            if (ifa->ifa_addr->sa_family != AF_INET) continue;
            if (ifa->ifa_flags & IFF_LOOPBACK) continue;
            int s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), buffer, size, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                buffer[0] = '\0';
                continue;
            }
            freeifaddrs(ifaddr);
            return;
        }
        freeifaddrs(ifaddr);
    #elif defined(__OS_WINDOWS__)
        WSADATA wsaData;
        char hostname[256];
        struct addrinfo hints;
        struct addrinfo *res = NULL;
        struct addrinfo *ptr = NULL;

        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return;

        if (gethostname(hostname, sizeof(hostname)) != 0) {
            WSACleanup();
            return;
        }

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        if (getaddrinfo(hostname, NULL, &hints, &res) != 0) {
            WSACleanup();
            return;
        }

        for (ptr = res; ptr != NULL; ptr = ptr->ai_next) {
            if (buffer && size > 0) buffer[0] = '\0';
            if (ptr->ai_family != AF_INET) continue;
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)ptr->ai_addr;
            char *ip = inet_ntoa(ipv4->sin_addr);
            if (ip == NULL) continue;
            if (strcmp(ip, "127.0.0.1") == 0) continue;
            strncpy(buffer, ip, size);
            buffer[size - 1] = '\0';
            break;
        }

        freeaddrinfo(res);
        WSACleanup();
    #endif
}


Socket socket_setopt_timeout(Socket c, int type, double second) {
    int s = (int)second;
    int ms = (int)((second - s) * 1000 + 0.5);
    #if defined(__OS_UNIX__)
        struct timeval timeout = {.tv_sec = s, .tv_usec = ms * 1000};
    #elif defined(__OS_WINDOWS__)
        int timeout = s * 1000 + ms;
    #endif
    if (type == 0) return socket_setopt(c, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    else return socket_setopt(c, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
}


int socket_valid_ipv4(char *buffer) {
    int a, b, c, d;
    char extra;
    if (sscanf(buffer, "%d.%d.%d.%d%c", &a, &b, &c, &d, &extra) == 4) {
        if (
            a >= 0 && a <= 255
            &&
            b >= 0 && b <= 255
            &&
            c >= 0 && c <= 255
            &&
            d >= 0 && d <= 255
        ) return 1;
    }
    return 0;
}


Socket socket_connect_timeout(Socket s, struct sockaddr_in *server, int size, double second) {
    // Set to non-blocking mode.
    #if defined(__OS_UNIX__)
        int flags = fcntl(s, F_GETFL, 0);
        fcntl(s, F_SETFL, flags | O_NONBLOCK);
    #elif defined(__OS_WINDOWS__)
        unsigned long mode = 1;
        ioctlsocket(s, FIONBIO, &mode);
    #endif

    // Initiate standard C connection.
    int condition = connect(s, (struct sockaddr *)server, size);
    Socket status = 0;

    if (condition < 0) {
        // It must be verified if socket that responded actually connected or just failed instantly.
        fd_set writefds;
        FD_ZERO(&writefds);
        FD_SET(s, &writefds);

        int ms = (int)((second - (int)second) * 1000 + 0.5);
        struct timeval tv = {.tv_sec = (int)second, .tv_usec = ms * 1000};

        condition = select((int)(s + 1), NULL, &writefds, NULL, &tv);

        if (condition > 0) {
            int error = 0;
            int len = sizeof(error);
            #if defined(__OS_UNIX__)
                getsockopt(s, SOL_SOCKET, SO_ERROR, (void *)&error, (socklen_t *)&len);
            #elif defined(__OS_WINDOWS__)
                getsockopt(s, SOL_SOCKET, SO_ERROR, (char *)&error, &len);
            #endif
            status = (error != 0) ? SOCKET_INVALID : 0;
        } else {
            // `status = 0` for timeout; `status < 0` for `select()` error.
            status = SOCKET_INVALID;
        }
    }

    // Restore socket to standard blocking mode.
    #if defined(__OS_UNIX__)
        fcntl(s, F_SETFL, flags);
    #elif defined(__OS_WINDOWS__)
        mode = 0;
        ioctlsocket(s, FIONBIO, &mode);
    #endif

    return status;
}


long socket_send_nowait(Socket s, char *buffer, int length) {
    #if defined(__OS_UNIX__)
        return send(s, buffer, length, MSG_DONTWAIT);
    #elif defined(__OS_WINDOWS__)
        u_long mode = 1;    // `1` for non-blocking.
        if (ioctlsocket(s, FIONBIO, &mode) != 0) return send(s, buffer, length, 0);
        long result = send(s, buffer, length, 0);
        mode = 0;   // `0` for blocking.
        ioctlsocket(s, FIONBIO, &mode);
        return result;
    #endif
}


long long socket_sendfile(Socket s, FILE *f, long long offset, long long size) {
    #if defined(__linux__)
        int fd = fileno(f);
        long long sent = 0;
        long off = offset;
        while (sent < size) {
            long n = sendfile(s, fd, &off, (unsigned long)(size - sent));
            if (n < 0) {
                if (errno == EINTR || errno == EAGAIN) continue;
                return -1;
            }
            sent = sent + n;
        }
        return sent;
    #elif defined(__APPLE__)
        int fd = fileno(f);
        long long sent = 0;
        long long off = offset;
        while (sent < size) {
            long long length = size - sent;
            int n = sendfile(fd, s, off, &length, NULL, 0);
            if (length > 0) {
                off = off + length;
                sent = sent + length;
            }
            if (n < 0) {
                if (errno == EINTR || errno == EAGAIN) continue;
                return -1;
            }
        }
        return sent;
    #else
        char buffer[65536];
        size = 0;
        unsigned long long length;
        socket_fseek(f, offset, SEEK_SET);
        while ((length = fread(buffer, 1, sizeof(buffer), f)) > 0) {
            char *p = buffer;
            unsigned long long left = length;
            while (left > 0) {
                long n = socket_send(s, p, left, 0);
                if (n <= 0) return -1;
                p = p + n;
                left = left - (unsigned long long)n;
                size = size + n;
            }
        }
        return size;
    #endif
}
