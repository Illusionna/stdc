#include "socket.h"


int socket_init() {
    #ifdef __OS_WINDOWS__
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;
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


int socket_connect(Socket s, struct sockaddr_in *server, int size) {
    return connect(s, (struct sockaddr *)server, size);
}


int socket_send(Socket s, char *buffer, int length, int flag) {
    return send(s, buffer, length, flag);
}


int socket_sendto(Socket s, void *buffer, int length, int flag, struct sockaddr_in *to, int size) {
    return sendto(s, buffer, length, flag, (struct sockaddr *)to, size);
}


int socket_recv(Socket s, char *buffer, int length, int flag) {
    return recv(s, buffer, length, flag);
}


int socket_recvfrom(Socket s, void *buffer, int length, int flag, struct sockaddr_in *from, int *size) {
    #if defined(__OS_UNIX__)
        return recvfrom(s, buffer, length, flag, (struct sockaddr *)from, (socklen_t *)size);
    #elif defined(__OS_WINDOWS__)
        return recvfrom(s, buffer, length, flag, (struct sockaddr *)from, size);
    #endif
}


int socket_bind(Socket s, struct sockaddr_in *address_name, int size) {
    return bind(s, (struct sockaddr *)address_name, size);
}


int socket_listen(Socket s, int backlog) {
    return listen(s, backlog);
}


Socket socket_accept(Socket s, struct sockaddr_in *address, int *size_pointer) {
    #if defined(__OS_UNIX__)
        return accept(s, (struct sockaddr *)address, (socklen_t *)size_pointer);
    #elif defined(__OS_WINDOWS__)
        return accept(s, (struct sockaddr *)address, size_pointer);
    #endif
}


int socket_setopt(Socket s, int level, int optname, void *ctx, int size) {
    if (ctx == NULL && size == 0) {
        int opt = 1;
        #if defined(__OS_UNIX__)
            return setsockopt(s, level, optname, (void *)&opt, sizeof(opt));
        #elif defined(__OS_WINDOWS__)
            return setsockopt(s, level, optname, (char *)&opt, sizeof(opt));
        #endif
    } else {
        #if defined(__OS_UNIX__)
            return setsockopt(s, level, optname, ctx, size);
        #elif defined(__OS_WINDOWS__)
            return setsockopt(s, level, optname, (char *)ctx, size);
        #endif
    }
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
    #if defined(__OS_UNIX__)
        struct ifaddrs *ifa;
        struct ifaddrs *ifaddr;
        if (getifaddrs(&ifaddr) == -1) return;
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr == NULL) continue;
            if (ifa->ifa_addr->sa_family == AF_INET) {
                if (ifa->ifa_flags & IFF_LOOPBACK) continue;
                int s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), buffer, size, NULL, 0, NI_NUMERICHOST);
                if (s == 0) {
                    freeifaddrs(ifaddr);
                    return;
                }
            }
        }
        freeifaddrs(ifaddr);
    #elif defined(__OS_WINDOWS__)
        WSADATA wsaData;
        char hostname[256];
        struct addrinfo hints;
        struct addrinfo *res = NULL;
        struct addrinfo *ptr = NULL;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return;
        if (gethostname(hostname, sizeof(hostname)) == 0) {
            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            if (getaddrinfo(hostname, NULL, &hints, &res) == 0) {
                for (ptr = res; ptr != NULL; ptr = ptr->ai_next) {
                    struct sockaddr_in *ipv4 = (struct sockaddr_in *)ptr->ai_addr;
                    char *ip = inet_ntoa(ipv4->sin_addr);
                    if (ip != NULL) {
                        if (strcmp(ip, "127.0.0.1") != 0) {
                            strncpy(buffer, ip, size);
                            buffer[size - 1] = '\0';
                            break;
                        }
                    }
                }
                freeaddrinfo(res);
            }
        }
        WSACleanup();
    #endif
}


int socket_setopt_timeout(Socket c, int type, double second) {
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
