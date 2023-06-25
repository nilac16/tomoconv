#include "lookup.h"
#include "../log.h"

#include <WS2tcpip.h>

#undef min
#undef max


static class wsa_init {
    WSADATA wsdata;

public:
    wsa_init()
    {
        WSAStartup(MAKEWORD(2, 2), &wsdata);
    }

} initializer;


static void throw_win32_error(DWORD err)
{
    char message[256], *crlf;

    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
                   NULL,
                   err,
                   LANG_USER_DEFAULT,
                   message,
                   sizeof message,
                   NULL);
    crlf = strstr(message, "\r\n");
    if (crlf) {
        *crlf = '\0';
    }
    throw std::runtime_error(message);
}


static void throw_win32_error(void)
{
    DWORD err;

    err = GetLastError();
    throw_win32_error(err);
}


class connection {
    SOCKET sock;

public:
    connection(const char *host, const char *svc, u_short port);
    ~connection();

    void send(const void *data, size_t len);
    size_t recv(void *dst, size_t len);
};


static void set_port(struct sockaddr *addr, u_short port)
{
    switch (addr->sa_family) {
    case AF_INET:
        reinterpret_cast<struct sockaddr_in *>(addr)->sin_port = htons(port);
        break;
    case AF_INET6:
        reinterpret_cast<struct sockaddr_in6 *>(addr)->sin6_port = htons(port);
        break;
    default:
        throw std::runtime_error("Bad address family");
        break;
    }
}


static std::string sockaddr_str(const struct sockaddr *addr)
{
    const struct sockaddr_in *in4 = (const struct sockaddr_in *)addr;
    const struct sockaddr_in6 *in6 = (const struct sockaddr_in6 *)addr;
    const int af = addr->sa_family;
    uint16_t port;
    char ip[80], buf[128];

    switch (af) {
    case AF_INET:
        port = ntohs(in4->sin_port);
        inet_ntop(af, &in4->sin_addr, ip, sizeof ip);
        snprintf(buf, sizeof buf, "%s:%u", ip, (unsigned)port);
        break;
    case AF_INET6:
        port = ntohs(in6->sin6_port);
        inet_ntop(af, &in6->sin6_addr, ip, sizeof ip);
        snprintf(buf, sizeof buf, "[%s]:%u", ip, (unsigned)port);
        break;
    default:
        return "Unsupported address family";
    }
    return std::string(buf);
}


connection::connection(const char *host, const char *svc, u_short port):
    sock(INVALID_SOCKET)
{
    ADDRINFO hints{ }, *ai, *node;
    DWORD lasterr;
    int res;

    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags    = AI_ADDRCONFIG;
    res = getaddrinfo(host, svc, &hints, &ai);
    if (res) {
        throw_win32_error((DWORD)WSAGetLastError());
    }
    for (node = ai; node; node = node->ai_next) {
        sock = socket(node->ai_family, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET) {
            lasterr = WSAGetLastError();
            res = 1;
            break;
        }
        if (!svc) {
            set_port(node->ai_addr, port);
        }
        tomo::log << tomo::log::DEBUG << "Attempting to connect to " << sockaddr_str(node->ai_addr);
        res = connect(sock, node->ai_addr, node->ai_addrlen);
        if (!res) {
            break;
        }
        lasterr = WSAGetLastError();
        closesocket(sock);
        sock = INVALID_SOCKET;
    }
    freeaddrinfo(ai);
    if (res) {
        throw_win32_error(lasterr);
    }
}


connection::~connection()
{
    closesocket(sock);
}


void connection::send(const void *data, size_t len)
{
    int count;
            /* type "void *" not compatible with "char *"? */
    count = ::send(sock, (const char *)data, len, 0);
    if (count < 0) {
        throw_win32_error((DWORD)WSAGetLastError());
    }
}


size_t connection::recv(void *buf, size_t len)
{
    int count;
                    /* Fuck you, C++ */
    count = ::recv(sock, (char *)buf, len, 0);
    if (count < 0) {
        throw_win32_error((DWORD)WSAGetLastError());
    }
    return (size_t)count;
}


void tomo::lookup_name(const char *host, uint16_t port, char *name, size_t len)
{
    connection conn(host, NULL, port);
    size_t recd;

    conn.send(name, strlen(name));
    recd = conn.recv(name, len);
    name[std::min(recd, len - 1)] = '\0';
    if (!strcmp(name, "NOT FOUND")) {
        throw std::runtime_error("Patient name not found in MRN database");
    }
}
