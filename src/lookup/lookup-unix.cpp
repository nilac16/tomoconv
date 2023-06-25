#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include "lookup.h"
#include "../log.h"

using namespace std::literals;


static void throw_errno(const char *ctx, int erno)
{
    char buf[256];
    const char *msg = buf;
    std::stringstream ss;

#if _GNU_SOURCE
    msg = strerror_r(erno, buf, sizeof buf);
#else
    strerror_r(erno, buf, sizeof buf);
#endif
    ss << ctx << ": " << msg;
    throw std::runtime_error(ss.str());
}


static void throw_errno(const char *ctx)
{
    throw_errno(ctx, errno);
}


static void throw_gai(int code)
{
    std::string str = "Cannot resolve host: "s;

    str += gai_strerror(code);
    throw std::runtime_error(str);
}


class connection {
    int sock;

public:
    connection(const char *host, const char *svc, uint16_t port);
    ~connection();

    void send(const void *data, size_t len);
    size_t recv(void *dst, size_t len);
};


static void set_port(struct sockaddr *addr, uint16_t port)
{
    switch (addr->sa_family) {
    case AF_INET:
        reinterpret_cast<struct sockaddr_in *>(addr)->sin_port = htons(port);
        break;
    case AF_INET6:
        reinterpret_cast<struct sockaddr_in6 *>(addr)->sin6_port = htons(port);
        break;
    default:
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
        return "Unsupported address family"s;
    }
    return std::string(buf);
}


connection::connection(const char *host, const char *svc, uint16_t port):
    sock(-1)
{
    struct addrinfo hints{ }, *ai, *node;
    int res;

    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags    = AI_ADDRCONFIG;
    res = getaddrinfo(host, svc, &hints, &ai);
    if (res) {
        throw_gai(res);
    }
    for (node = ai; node; node = node->ai_next) {
        sock = socket(node->ai_family, SOCK_STREAM, IPPROTO_TCP);
        if (sock == -1) {
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
        close(sock);
        sock = -1;
    }
    freeaddrinfo(ai);
    if (res) {
        throw_errno("Connection to MRN server failed");
    }
}


connection::~connection()
{
    close(sock);
}


void connection::send(const void *buf, size_t len)
{
    ssize_t count;

    count = ::send(sock, buf, len, 0);
    if (count < 0) {
        throw_errno("Failed sending data to MRN server");
    }
}


size_t connection::recv(void *buf, size_t len)
{
    ssize_t count;

    count = ::recv(sock, buf, len, 0);
    if (count < 0) {
        throw_errno("Failed receiving data from MRN server");
    }
    return static_cast<size_t>(count);
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
