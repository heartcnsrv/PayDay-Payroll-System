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

/* Shared data folder used by all HTTP routes when accessing CSV files. */
extern char g_data_dir[512];

void server_run(int port);

/* Central router that maps each request path to the correct handler. */
char* server_dispatch(const char* path, const char* body);

char* route_auth       (const char* body);
char* route_employees  (const char* body);
char* route_payroll    (const char* body);
char* route_report     (const char* body);

char* json_get_str (const char* json, const char* key, char* out, int outlen);
int   json_get_int (const char* json, const char* key, int def);
double json_get_dbl(const char* json, const char* key, double def);
char* json_ok      (const char* body);   /* Caller frees returned JSON buffer. */
char* json_err     (const char* msg);    /* Caller frees returned JSON buffer. */
char* json_escape  (const char* s, char* out, int outlen);

#endif 
