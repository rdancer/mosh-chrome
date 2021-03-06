// pepper_wrapper.cc - C wrapper functions to interface to PepperPOSIX.

// Copyright 2013, 2014, 2015 Richard Woodbury
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "pepper_wrapper.h"

#include "make_unique.h"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <langinfo.h>
#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <map>
#include <memory>
#include <string>

using std::map;
using std::move;
using std::string;
using std::unique_ptr;
using util::make_unique;

// Literal strings are "const char *", but many of these old functions want to
// return "char *". To avoid compiler warnings, we generate non-const strings
// as needed, by way of calling Get("some_string"). We use C++ static
// initialization (by putting the object in the global scope) to manage the
// lifecycle of the backing memory.
class BadInterning {
 public:
  char* Get(const string &str) {
    if (strings_.count(str) == 0) {
      int size = str.size();
      auto buf = make_unique<char[]>(size+1);
      str.copy(buf.get(), size);
      buf.get()[size] = '\0';
      strings_[str] = move(buf);
    }
    return strings_[str].get();
  }

 private:
  map<string, unique_ptr<char[]>> strings_;
};

BadInterning strings;

extern "C" {

#define UNUSED __attribute__((unused))

// These are used to avoid CORE dumps. Should be OK to stub them out. However,
// it seems that on x86_32 with glibc, pthread_create() calls this with
// RLIMIT_STACK. It needs to return an error at least, otherwise the thread
// cannot be created. This does not seem to be an issue on x86_64 nor with
// newlib (which doesn't have RLIMIT_STACK in the headers).
#ifndef USE_NEWLIB
int getrlimit(int resource, UNUSED struct rlimit *rlim) {
  if (resource == RLIMIT_STACK) {
    errno = EAGAIN;
    return -1;
  }
  return 0;
}
#else
int getrlimit(UNUSED int resource, UNUSED struct rlimit *rlim) {
  return 0;
}
#endif
int setrlimit(UNUSED int resource, UNUSED const struct rlimit *rlim) {
  return 0;
}

// sigprocmask() isn't meaningful in NaCl; stubbing out.
int sigprocmask(int how, UNUSED const sigset_t *set, UNUSED sigset_t *oldset) {
  Log("sigprocmask(%d, ...)", how);
  return 0;
}

// kill() is used to send a SIGSTOP on Ctrl-Z, which is not useful for NaCl.
// This shouldn't be called, but it is annoying to see a linker warning about
// it not being implemented.
int kill(pid_t pid, int sig) {
  Log("kill(%d, %d)", pid, sig);
  return 0;
}

// Stubbing out getpid() by just returning a bogus PID.
pid_t getpid(void) {
  Log("getpid()");
  return 0;
}

// Stub these out. In the NaCl glibc, locale support is terrible (and we don't
// get UTF-8 because of it). In newlib, there seems to be some crashiness with
// nl_langinfo(). This will do for both cases (although no UTF-8 in glibc can
// cause a bit of a mess).
#ifndef USE_NEWLIB
char *setlocale(int category, const char *locale) {
  Log("setlocale(%d, \"%s\")", category, locale);
  return strings.Get("NaCl");
}
#endif
char *nl_langinfo(nl_item item) {
  switch (item) {
    case CODESET:
      Log("nl_langinfo(CODESET)");
      return strings.Get("UTF-8");
    default:
      Log("nl_langinfo(%d)", item);
      return strings.Get("Error");
  }
}

// We don't really care about terminal attributes.
int tcgetattr(int fd, UNUSED struct termios *termios_p) {
  Log("tcgetattr(%d, ...)", fd);
  return 0;
}
int tcsetattr(
    int fd, int optional_actions, UNUSED const struct termios *termios_p) {
  Log("tcsetattr(%d, %d, ...)", fd, optional_actions);
  return 0;
}

//
// Wrap fopen() and friends to capture access to stderr and /dev/urandom.
//

FILE *fopen(const char *path, const char *mode) {
  int flags = 0;
  if (mode[1] == '+') {
    flags = O_RDWR;
  } else if (mode[0] == 'r') {
    flags = O_RDONLY;
  } else if (mode[0] == 'w' || mode[0] == 'a') {
    flags = O_WRONLY;
  } else {
    errno = EINVAL;
    return nullptr;
  }

  FILE *stream = new FILE;
  memset(stream, 0, sizeof(*stream));
  // TODO: Consider the mode param of open().
#ifdef USE_NEWLIB
  stream->_file = open(path, flags);
#else
  stream->_fileno = open(path, flags);
#endif
  return stream;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
#ifdef USE_NEWLIB
  int fd = stream->_file;
#else
  int fd = stream->_fileno;
#endif
  return read(fd, ptr, size*nmemb);
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
#ifdef USE_NEWLIB
  int fd = stream->_file;
#else
  int fd = stream->_fileno;
#endif
  return write(fd, ptr, size*nmemb);
}

int fileno(FILE *stream) {
#ifdef USE_NEWLIB
  return stream->_file;
#else
  return stream->_fileno;
#endif
}

// For some reason, there are linking errors when trying to override fclose().
// However, nothing calls it, so omitting it is safe. Still, leaving this right
// here in case it is needed.
//
// int fclose(FILE *stream) {
//   Log("fclose: fd=%d", fileno(stream));
//   int result = close(fileno(stream));
//   if (result == 0) {
//     delete stream;
//     return 0;
//   }
//   return result;
// }

// Fake getaddrinfo(), as we expect it will always be an IP address and numeric
// port.
int getaddrinfo(const char *node, const char *service,
    const struct addrinfo *hints, struct addrinfo **res) {
  if (hints->ai_flags & AI_CANONNAME) {
    Log("getaddrinfo(): AI_CANONNAME not implemented.");
    return EAI_FAIL;
  }

  struct sockaddr *addr;
  size_t addr_size;
  uint32_t ip4_addr;
  unsigned char ip6_addr[16];

  if (inet_pton(AF_INET, node, &ip4_addr) == 1) {
    struct sockaddr_in *addr_in = new sockaddr_in;
    memset(addr_in, 0, sizeof(*addr_in));
    addr_in->sin_family = AF_INET;
    addr_in->sin_addr.s_addr = ip4_addr;
    addr_in->sin_port = htons(atoi(service));
    addr = (struct sockaddr *)addr_in;
    addr_size = sizeof(*addr_in);
  } else if (inet_pton(AF_INET6, node, &ip6_addr) == 1) {
    struct sockaddr_in6 *addr_in = new sockaddr_in6;
    memset(addr_in, 0, sizeof(*addr_in));
    addr_in->sin6_family = AF_INET6;
    memcpy(addr_in->sin6_addr.s6_addr, ip6_addr,
        sizeof(addr_in->sin6_addr.s6_addr));
    addr_in->sin6_port = htons(atoi(service));
    addr = (struct sockaddr *)addr_in;
    addr_size = sizeof(*addr_in);
  } else {
    Log("getaddrinfo(): Cannot parse address.");
    return EAI_FAIL;
  }

  struct addrinfo *ai = new struct addrinfo;
  memset(ai, 0, sizeof(*ai));
  ai->ai_addr = addr;
  ai->ai_addrlen = addr_size;
  ai->ai_family = addr->sa_family;
  if (hints != nullptr) {
    ai->ai_protocol = hints->ai_protocol;
    ai->ai_socktype = hints->ai_socktype;
  }

  *res = ai;
  return 0;
}

void freeaddrinfo(struct addrinfo *res) {
  while (res != nullptr) {
    struct addrinfo *last = res;
    delete res->ai_addr;
    res = res->ai_next;
    delete last;
  }
}

char *gai_strerror(UNUSED int errcode) {
  Log("gai_strerror(): Not implemented.");
  return strings.Get("gai_strerror not implemented");
}

//
// Wrap all unistd functions to communicate via the Pepper API.
//

// There is a pseudo-overload that includes a third param |mode_t|.
int open(const char *pathname, int flags, ...) {
  // TODO: For now, ignoring |mode_t| param.
  return GetPOSIX().Open(pathname, flags, 0);
}

ssize_t read(int fd, void *buf, size_t count) {
  return GetPOSIX().Read(fd, buf, count);
}

ssize_t write(int fd, const void *buf, size_t count) {
  return GetPOSIX().Write(fd, buf, count);
}

int close(int fd) {
  return GetPOSIX().Close(fd);
}

int socket(int domain, int type, int protocol) {
  return GetPOSIX().Socket(domain, type, protocol);
}

int bind(
    int sockfd, UNUSED const struct sockaddr *addr, UNUSED socklen_t addrlen) {
  Log("bind(%d, ...): Not implemented", sockfd);
  errno = ENOMEM;
  return -1;
}

// Most socket options aren't supported by PPAPI, so just stubbing out.
int setsockopt(UNUSED int sockfd, UNUSED int level, UNUSED int optname,
    UNUSED const void *optval, UNUSED socklen_t optlen) {
  return 0;
}

// This is needed to return TCP connection status.
int getsockopt(int sockfd, int level, int optname,
    void *optval, socklen_t *optlen) {
  return GetPOSIX().GetSockOpt(sockfd, level, optname, optval, optlen);
}

int dup(int oldfd) {
  return GetPOSIX().Dup(oldfd);
}

int pselect(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
    const struct timespec *timeout, const sigset_t *sigmask) {
  return GetPOSIX().PSelect(
     nfds, readfds, writefds, exceptfds, timeout, sigmask);
}

int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
    struct timeval *timeout) {
  return GetPOSIX().Select(nfds, readfds, writefds, exceptfds, timeout);
}

int poll(struct pollfd *fds, nfds_t nfds, int timeout) {
  return GetPOSIX().Poll(fds, nfds, timeout);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
  return GetPOSIX().Recv(sockfd, buf, len, flags);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
  return GetPOSIX().RecvMsg(sockfd, msg, flags);
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags){
  return GetPOSIX().Send(sockfd, buf, len, flags);
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
    const struct sockaddr *dest_addr, socklen_t addrlen) {
  return GetPOSIX().SendTo(sockfd, buf, len, flags, dest_addr, addrlen);
}

int fcntl(int fd, int cmd, ...) {
  va_list argp;
  va_start(argp, cmd);
  int result = GetPOSIX().FCntl(fd, cmd, argp);
  va_end(argp);
  return result;
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
  return GetPOSIX().Connect(sockfd, addr, addrlen);
}

} // extern "C"
