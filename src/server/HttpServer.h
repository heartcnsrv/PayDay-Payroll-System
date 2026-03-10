/* ============================================================
   PayDay | src/server/HttpServer.h
   Minimal HTTP/1.1 server on port 8081.
   Windows: Winsock2   Linux: POSIX sockets
   Pure C99. No external libraries.
   ============================================================ */
#ifndef PAYDAY_HTTP_SERVER_H
#define PAYDAY_HTTP_SERVER_H

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  typedef SOCKET sock_t;
  #define SOCK_INVALID_VAL INVALID_SOCKET
  #define sock_close closesocket
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <unistd.h>
  typedef int sock_t;
  #define SOCK_INVALID_VAL (-1)
  #define sock_close close
#endif

/* Path to data directory — set before calling server_run() */
extern char g_data_dir[512];

/* Start blocking HTTP server on given port */
void server_run(int port);

/* Called per-request — returns heap-allocated JSON string (caller frees) */
char* server_dispatch(const char* path, const char* body);

/* Route handlers */
char* route_auth       (const char* body);
char* route_employees  (const char* body);
char* route_payroll    (const char* body);
char* route_report     (const char* body);

/* JSON helpers */
char* json_get_str (const char* json, const char* key, char* out, int outlen);
int   json_get_int (const char* json, const char* key, int def);
double json_get_dbl(const char* json, const char* key, double def);
char* json_ok      (const char* body);   /* caller frees */
char* json_err     (const char* msg);    /* caller frees */
char* json_escape  (const char* s, char* out, int outlen);

#endif /* PAYDAY_HTTP_SERVER_H */
