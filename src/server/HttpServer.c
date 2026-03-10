/* ============================================================
   PayDay | src/server/HttpServer.c
   All API logic lives here. PHP is only a proxy.

   Routes:
     POST /auth       → login, forgot-password, reset-password
     POST /employees  → list, add, update, deactivate
     POST /payroll    → calculate, list, get, release
     POST /report     → summary report for a period
   ============================================================ */

#include "HttpServer.h"
#include "../core/PayrollTypes.h"
#include "../core/PayrollEngine.h"
#include "../utils/CSVManager.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
  #pragma comment(lib, "ws2_32.lib")
  #include <process.h>
  typedef HANDLE thread_t;
  #define THREAD_FUNC unsigned __stdcall
  #define THREAD_RET  return 0
#else
  #include <pthread.h>
  typedef pthread_t thread_t;
  #define THREAD_FUNC void*
  #define THREAD_RET  return NULL
#endif

char g_data_dir[512] = "data";

/* ── Reset token store (in-memory, simple) ─────────────────── */
static char g_reset_username[64] = {0};
static char g_reset_token[16]    = {0};

/* ── Send all bytes ──────────────────────────────────────────── */
static void send_all(sock_t fd, const char* data, int len) {
    int sent = 0;
    while (sent < len) {
        int n = (int)send(fd, data + sent, (size_t)(len - sent), 0);
        if (n <= 0) break;
        sent += n;
    }
}

/* ── Handle one HTTP connection ──────────────────────────────── */
typedef struct { sock_t fd; } ClientArgs;

static THREAD_FUNC handle_client(void* arg) {
    ClientArgs* ca = (ClientArgs*)arg;
    sock_t fd = ca->fd;
    free(ca);

    char buf[65536];
    int  total = 0;

    while (total < (int)sizeof(buf) - 1) {
        int n = (int)recv(fd, buf + total, (int)sizeof(buf) - 1 - total, 0);
        if (n <= 0) break;
        total += n;
        buf[total] = '\0';

        char* hdr_end = strstr(buf, "\r\n\r\n");
        if (hdr_end) {
            /* Find content-length */
            char lower[8192];
            int  hdr_len = (int)(hdr_end - buf);
            if (hdr_len > (int)sizeof(lower) - 1) hdr_len = (int)sizeof(lower) - 1;
            strncpy(lower, buf, (size_t)hdr_len);
            lower[hdr_len] = '\0';
            /* lowercase header */
            for (int i = 0; i < hdr_len; i++)
                lower[i] = (char)tolower((unsigned char)lower[i]);

            char* cl_pos = strstr(lower, "content-length:");
            if (!cl_pos) break;
            int cl = atoi(cl_pos + 15);
            int body_start = (int)(hdr_end - buf) + 4;
            if (total >= body_start + cl) break;
        }
    }
    if (total <= 0) { sock_close(fd); THREAD_RET; }

    /* Parse request line */
    char method[16] = {0}, path[256] = {0};
    sscanf(buf, "%15s %255s", method, path);

    /* Build response helper */
    #define RESPOND(body_str) do { \
        const char* _b = (body_str); \
        int   _bl = (int)strlen(_b); \
        char  _hdr[512]; \
        int   _hl = snprintf(_hdr, sizeof(_hdr), \
            "HTTP/1.1 200 OK\r\n" \
            "Content-Type: application/json\r\n" \
            "Access-Control-Allow-Origin: *\r\n" \
            "Access-Control-Allow-Methods: POST, OPTIONS\r\n" \
            "Access-Control-Allow-Headers: Content-Type\r\n" \
            "Content-Length: %d\r\n" \
            "Connection: close\r\n\r\n", _bl); \
        send_all(fd, _hdr, _hl); \
        send_all(fd, _b, _bl); \
        free((void*)_b); \
    } while(0)

    if (strcmp(method, "OPTIONS") == 0) {
        const char* opts =
            "HTTP/1.1 200 OK\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: POST, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n\r\n";
        send_all(fd, opts, (int)strlen(opts));
        sock_close(fd);
        THREAD_RET;
    }

    /* Extract body */
    char* hdr_end = strstr(buf, "\r\n\r\n");
    const char* body = hdr_end ? hdr_end + 4 : "";

    char* result = server_dispatch(path, body);
    RESPOND(result);

    sock_close(fd);
    THREAD_RET;
}

/* ── Main server loop ────────────────────────────────────────── */
void server_run(int port) {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);
#endif

    sock_t srv = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons((unsigned short)port);

    if (bind(srv, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        fprintf(stderr, "[PayDay] bind() failed on port %d\n", port);
        return;
    }
    listen(srv, 128);
    printf("[PayDay] C backend listening on port %d\n", port);

    for (;;) {
        sock_t client = accept(srv, NULL, NULL);
        if (client == SOCK_INVALID_VAL) continue;

        ClientArgs* ca = (ClientArgs*)malloc(sizeof(ClientArgs));
        ca->fd = client;

#ifdef _WIN32
        _beginthreadex(NULL, 0, handle_client, ca, 0, NULL);
#else
        pthread_t t;
        pthread_create(&t, NULL, handle_client, ca);
        pthread_detach(t);
#endif
    }
}

/* ── Dispatcher ──────────────────────────────────────────────── */
char* server_dispatch(const char* path, const char* body) {
    if (strncmp(path, "/auth",      5) == 0) return route_auth(body);
    if (strncmp(path, "/employees", 10)== 0) return route_employees(body);
    if (strncmp(path, "/payroll",   8) == 0) return route_payroll(body);
    if (strncmp(path, "/report",    7) == 0) return route_report(body);
    return json_err("Unknown endpoint.");
}

/* ============================================================
   /auth
   actions: login | forgot_password | reset_password
   ============================================================ */
char* route_auth(const char* body) {
    char action[32], username[64], password[64], token[16], new_pass[64];
    json_get_str(body, "action",   action,   sizeof(action));
    json_get_str(body, "username", username, sizeof(username));
    json_get_str(body, "password", password, sizeof(password));

    char path[512];
    snprintf(path, sizeof(path), "%s/employees.csv", g_data_dir);

    Employee employees[MAX_EMPLOYEES];
    int count = csv_load_employees(path, employees, MAX_EMPLOYEES);

    /* ── login ─────────────────────────────────────────────── */
    if (strcmp(action, "login") == 0) {
        for (int i = 0; i < count; i++) {
            Employee* e = &employees[i];
            if (!e->active) continue;
            if (strcmp(e->username, username) == 0 &&
                strcmp(e->password, password) == 0) {

                char esc_name[256], esc_dept[128], esc_pos[128], esc_email[256];
                json_escape(e->full_name,  esc_name,  sizeof(esc_name));
                json_escape(e->department, esc_dept,  sizeof(esc_dept));
                json_escape(e->position,   esc_pos,   sizeof(esc_pos));
                json_escape(e->email,      esc_email, sizeof(esc_email));

                char buf[1024];
                snprintf(buf, sizeof(buf),
                    "\"id\":%d,\"username\":\"%s\",\"full_name\":\"%s\","
                    "\"department\":\"%s\",\"position\":\"%s\","
                    "\"email\":\"%s\",\"role\":%d,"
                    "\"base_salary\":%.2f,\"hourly_rate\":%.2f",
                    e->id, e->username, esc_name,
                    esc_dept, esc_pos, esc_email,
                    (int)e->role, e->base_salary, e->hourly_rate);
                return json_ok(buf);
            }
        }
        return json_err("Invalid username or password.");
    }

    /* ── forgot_password ───────────────────────────────────── */
    if (strcmp(action, "forgot_password") == 0) {
        char email[128];
        json_get_str(body, "email", email, sizeof(email));
        for (int i = 0; i < count; i++) {
            if (strcmp(employees[i].username, username) == 0 &&
                strcmp(employees[i].email,    email)    == 0 &&
                employees[i].active) {
                payroll_generate_token(g_reset_token, sizeof(g_reset_token));
                strncpy(g_reset_username, username, sizeof(g_reset_username)-1);
                /* In production: email the token. Here we return it directly. */
                char buf[128];
                snprintf(buf, sizeof(buf), "\"token\":\"%s\"", g_reset_token);
                return json_ok(buf);
            }
        }
        return json_err("No account found with that username and email.");
    }

    /* ── reset_password ────────────────────────────────────── */
    if (strcmp(action, "reset_password") == 0) {
        json_get_str(body, "token",    token,    sizeof(token));
        json_get_str(body, "new_pass", new_pass, sizeof(new_pass));

        if (strcmp(g_reset_token, token) != 0 ||
            strcmp(g_reset_username, username) != 0)
            return json_err("Invalid or expired reset token.");

        if (strlen(new_pass) < 3)
            return json_err("Password must be at least 3 characters.");

        for (int i = 0; i < count; i++) {
            if (strcmp(employees[i].username, username) == 0) {
                strncpy(employees[i].password, new_pass,
                        sizeof(employees[i].password)-1);
                csv_save_employees(path, employees, count);
                g_reset_token[0]    = '\0';
                g_reset_username[0] = '\0';
                return json_ok("\"message\":\"Password updated.\"");
            }
        }
        return json_err("User not found.");
    }

    return json_err("Unknown auth action.");
}

/* ============================================================
   /employees
   actions: list | add | update | deactivate | get
   ============================================================ */
char* route_employees(const char* body) {
    char action[32];
    json_get_str(body, "action", action, sizeof(action));

    char path[512];
    snprintf(path, sizeof(path), "%s/employees.csv", g_data_dir);

    Employee employees[MAX_EMPLOYEES];
    int count = csv_load_employees(path, employees, MAX_EMPLOYEES);

    /* ── list ──────────────────────────────────────────────── */
    if (strcmp(action, "list") == 0) {
        char* arr = (char*)malloc(65536);
        strcpy(arr, "[");
        int first = 1;
        for (int i = 0; i < count; i++) {
            Employee* e = &employees[i];
            if (!e->active) continue;
            if (!first) strcat(arr, ",");
            first = 0;
            char esc_name[256], esc_dept[128], esc_pos[128];
            json_escape(e->full_name,  esc_name, sizeof(esc_name));
            json_escape(e->department, esc_dept, sizeof(esc_dept));
            json_escape(e->position,   esc_pos,  sizeof(esc_pos));
            char entry[512];
            snprintf(entry, sizeof(entry),
                "{\"id\":%d,\"username\":\"%s\",\"full_name\":\"%s\","
                "\"department\":\"%s\",\"position\":\"%s\","
                "\"base_salary\":%.2f,\"hourly_rate\":%.2f,"
                "\"email\":\"%s\",\"phone\":\"%s\","
                "\"date_hired\":\"%s\",\"role\":%d}",
                e->id, e->username, esc_name,
                esc_dept, esc_pos,
                e->base_salary, e->hourly_rate,
                e->email, e->phone, e->date_hired, (int)e->role);
            strcat(arr, entry);
        }
        strcat(arr, "]");
        char* result = (char*)malloc(strlen(arr) + 32);
        sprintf(result, "\"employees\":%s", arr);
        free(arr);
        char* ret = json_ok(result);
        free(result);
        return ret;
    }

    /* ── get (single employee) ─────────────────────────────── */
    if (strcmp(action, "get") == 0) {
        int id = json_get_int(body, "id", 0);
        for (int i = 0; i < count; i++) {
            if (employees[i].id == id) {
                Employee* e = &employees[i];
                char esc_name[256], esc_dept[128], esc_pos[128];
                json_escape(e->full_name,  esc_name, sizeof(esc_name));
                json_escape(e->department, esc_dept, sizeof(esc_dept));
                json_escape(e->position,   esc_pos,  sizeof(esc_pos));
                char buf[1024];
                snprintf(buf, sizeof(buf),
                    "\"employee\":{\"id\":%d,\"username\":\"%s\","
                    "\"full_name\":\"%s\",\"department\":\"%s\","
                    "\"position\":\"%s\",\"base_salary\":%.2f,"
                    "\"hourly_rate\":%.2f,\"email\":\"%s\","
                    "\"phone\":\"%s\",\"date_hired\":\"%s\",\"role\":%d}",
                    e->id, e->username, esc_name,
                    esc_dept, esc_pos,
                    e->base_salary, e->hourly_rate,
                    e->email, e->phone, e->date_hired, (int)e->role);
                return json_ok(buf);
            }
        }
        return json_err("Employee not found.");
    }

    /* ── add ───────────────────────────────────────────────── */
    if (strcmp(action, "add") == 0) {
        Employee e;
        memset(&e, 0, sizeof(e));
        e.id = csv_next_employee_id(employees, count);
        json_get_str(body, "username",   e.username,   sizeof(e.username));
        json_get_str(body, "password",   e.password,   sizeof(e.password));
        json_get_str(body, "full_name",  e.full_name,  sizeof(e.full_name));
        json_get_str(body, "department", e.department, sizeof(e.department));
        json_get_str(body, "position",   e.position,   sizeof(e.position));
        json_get_str(body, "email",      e.email,      sizeof(e.email));
        json_get_str(body, "phone",      e.phone,      sizeof(e.phone));
        json_get_str(body, "date_hired", e.date_hired, sizeof(e.date_hired));
        e.base_salary = json_get_dbl(body, "base_salary", 0.0);
        e.hourly_rate = json_get_dbl(body, "hourly_rate", 0.0);
        e.role        = (UserRole)json_get_int(body, "role", 1);
        e.active      = 1;

        if (!e.date_hired[0]) csv_today(e.date_hired, sizeof(e.date_hired));

        /* Check duplicate username */
        for (int i = 0; i < count; i++) {
            if (strcmp(employees[i].username, e.username) == 0 && employees[i].active)
                return json_err("Username already taken.");
        }

        char err_msg[256];
        if (!payroll_validate_employee(&e, err_msg, sizeof(err_msg)))
            return json_err(err_msg);

        employees[count++] = e;
        csv_save_employees(path, employees, count);

        char buf[128];
        snprintf(buf, sizeof(buf), "\"id\":%d,\"message\":\"Employee added.\"", e.id);
        return json_ok(buf);
    }

    /* ── update ────────────────────────────────────────────── */
    if (strcmp(action, "update") == 0) {
        int id = json_get_int(body, "id", 0);
        for (int i = 0; i < count; i++) {
            if (employees[i].id == id) {
                Employee* e = &employees[i];
                char tmp[128];
                if (json_get_str(body, "full_name",  tmp, sizeof(tmp)) && tmp[0])
                    strncpy(e->full_name,  tmp, sizeof(e->full_name)-1);
                if (json_get_str(body, "department", tmp, sizeof(tmp)) && tmp[0])
                    strncpy(e->department, tmp, sizeof(e->department)-1);
                if (json_get_str(body, "position",   tmp, sizeof(tmp)) && tmp[0])
                    strncpy(e->position,   tmp, sizeof(e->position)-1);
                if (json_get_str(body, "email",      tmp, sizeof(tmp)) && tmp[0])
                    strncpy(e->email,      tmp, sizeof(e->email)-1);
                if (json_get_str(body, "phone",      tmp, sizeof(tmp)) && tmp[0])
                    strncpy(e->phone,      tmp, sizeof(e->phone)-1);
                double bs = json_get_dbl(body, "base_salary", -1.0);
                if (bs > 0) e->base_salary = bs;
                double hr = json_get_dbl(body, "hourly_rate", -1.0);
                if (hr > 0) e->hourly_rate = hr;
                if (json_get_str(body, "password", tmp, sizeof(tmp)) && strlen(tmp) >= 3)
                    strncpy(e->password, tmp, sizeof(e->password)-1);

                csv_save_employees(path, employees, count);
                return json_ok("\"message\":\"Employee updated.\"");
            }
        }
        return json_err("Employee not found.");
    }

    /* ── deactivate ────────────────────────────────────────── */
    if (strcmp(action, "deactivate") == 0) {
        int id = json_get_int(body, "id", 0);
        for (int i = 0; i < count; i++) {
            if (employees[i].id == id) {
                employees[i].active = 0;
                csv_save_employees(path, employees, count);
                return json_ok("\"message\":\"Employee deactivated.\"");
            }
        }
        return json_err("Employee not found.");
    }

    return json_err("Unknown employees action.");
}

/* ============================================================
   /payroll
   actions: calculate | list | get | release
   ============================================================ */
char* route_payroll(const char* body) {
    char action[32];
    json_get_str(body, "action", action, sizeof(action));

    char emp_path[512], pay_path[512];
    snprintf(emp_path, sizeof(emp_path), "%s/employees.csv", g_data_dir);
    snprintf(pay_path, sizeof(pay_path), "%s/payroll.csv",   g_data_dir);

    Employee      employees[MAX_EMPLOYEES];
    PayrollRecord payroll[MAX_PAYROLL];
    int emp_count = csv_load_employees(emp_path, employees, MAX_EMPLOYEES);
    int pay_count = csv_load_payroll  (pay_path, payroll,   MAX_PAYROLL);

    /* ── calculate ─────────────────────────────────────────── */
    if (strcmp(action, "calculate") == 0) {
        int    emp_id    = json_get_int(body, "employee_id", 0);
        char   ps[16], pe[16];
        double ot_hours  = json_get_dbl(body, "overtime_hours",  0.0);
        double other_ded = json_get_dbl(body, "other_deductions", 0.0);
        json_get_str(body, "period_start", ps, sizeof(ps));
        json_get_str(body, "period_end",   pe, sizeof(pe));

        Employee* emp = NULL;
        for (int i = 0; i < emp_count; i++)
            if (employees[i].id == emp_id) { emp = &employees[i]; break; }
        if (!emp) return json_err("Employee not found.");

        PayrollRecord rec;
        payroll_calculate(&rec, emp, ps, pe, ot_hours, other_ded, NULL);
        rec.id = csv_next_payroll_id(payroll, pay_count);

        payroll[pay_count++] = rec;
        csv_save_payroll(pay_path, payroll, pay_count);

        char buf[1024];
        snprintf(buf, sizeof(buf),
            "\"payroll\":{\"id\":%d,\"employee_id\":%d,"
            "\"period_start\":\"%s\",\"period_end\":\"%s\","
            "\"base_salary\":%.2f,\"overtime_hours\":%.2f,"
            "\"overtime_pay\":%.2f,\"gross_pay\":%.2f,"
            "\"tax_deduction\":%.2f,\"sss_deduction\":%.2f,"
            "\"philhealth_deduction\":%.2f,\"pagibig_deduction\":%.2f,"
            "\"other_deductions\":%.2f,\"total_deductions\":%.2f,"
            "\"net_pay\":%.2f,\"status\":\"%s\"}",
            rec.id, rec.employee_id,
            rec.period_start, rec.period_end,
            rec.base_salary, rec.overtime_hours,
            rec.overtime_pay, rec.gross_pay,
            rec.tax_deduction, rec.sss_deduction,
            rec.philhealth_deduction, rec.pagibig_deduction,
            rec.other_deductions, rec.total_deductions,
            rec.net_pay, rec.status);
        return json_ok(buf);
    }

    /* ── list ──────────────────────────────────────────────── */
    if (strcmp(action, "list") == 0) {
        int filter_id = json_get_int(body, "employee_id", 0);
        char* arr = (char*)malloc(131072);
        strcpy(arr, "[");
        int first = 1;

        for (int i = 0; i < pay_count; i++) {
            PayrollRecord* p = &payroll[i];
            if (filter_id > 0 && p->employee_id != filter_id) continue;

            /* Find employee name */
            const char* emp_name = "";
            for (int j = 0; j < emp_count; j++)
                if (employees[j].id == p->employee_id) {
                    emp_name = employees[j].full_name; break;
                }

            if (!first) strcat(arr, ",");
            first = 0;
            char entry[1024];
            snprintf(entry, sizeof(entry),
                "{\"id\":%d,\"employee_id\":%d,\"employee_name\":\"%s\","
                "\"period_start\":\"%s\",\"period_end\":\"%s\","
                "\"gross_pay\":%.2f,\"total_deductions\":%.2f,"
                "\"net_pay\":%.2f,\"status\":\"%s\","
                "\"generated_date\":\"%s\"}",
                p->id, p->employee_id, emp_name,
                p->period_start, p->period_end,
                p->gross_pay, p->total_deductions,
                p->net_pay, p->status, p->generated_date);
            strcat(arr, entry);
        }
        strcat(arr, "]");
        char* result = (char*)malloc(strlen(arr) + 32);
        sprintf(result, "\"records\":%s", arr);
        free(arr);
        char* ret = json_ok(result);
        free(result);
        return ret;
    }

    /* ── get (single record) ───────────────────────────────── */
    if (strcmp(action, "get") == 0) {
        int id = json_get_int(body, "id", 0);
        for (int i = 0; i < pay_count; i++) {
            PayrollRecord* p = &payroll[i];
            if (p->id != id) continue;
            const char* emp_name = "";
            for (int j = 0; j < emp_count; j++)
                if (employees[j].id == p->employee_id) {
                    emp_name = employees[j].full_name; break;
                }
            char buf[2048];
            snprintf(buf, sizeof(buf),
                "\"payroll\":{\"id\":%d,\"employee_id\":%d,"
                "\"employee_name\":\"%s\","
                "\"period_start\":\"%s\",\"period_end\":\"%s\","
                "\"base_salary\":%.2f,\"overtime_hours\":%.2f,"
                "\"overtime_rate\":%.4f,\"overtime_pay\":%.2f,"
                "\"tax_deduction\":%.2f,\"sss_deduction\":%.2f,"
                "\"philhealth_deduction\":%.2f,\"pagibig_deduction\":%.2f,"
                "\"other_deductions\":%.2f,\"gross_pay\":%.2f,"
                "\"total_deductions\":%.2f,\"net_pay\":%.2f,"
                "\"generated_date\":\"%s\",\"status\":\"%s\"}",
                p->id, p->employee_id, emp_name,
                p->period_start, p->period_end,
                p->base_salary, p->overtime_hours,
                p->overtime_rate, p->overtime_pay,
                p->tax_deduction, p->sss_deduction,
                p->philhealth_deduction, p->pagibig_deduction,
                p->other_deductions, p->gross_pay,
                p->total_deductions, p->net_pay,
                p->generated_date, p->status);
            return json_ok(buf);
        }
        return json_err("Payroll record not found.");
    }

    /* ── release ───────────────────────────────────────────── */
    if (strcmp(action, "release") == 0) {
        int id = json_get_int(body, "id", 0);
        for (int i = 0; i < pay_count; i++) {
            if (payroll[i].id == id) {
                strncpy(payroll[i].status, "released", sizeof(payroll[i].status)-1);
                csv_save_payroll(pay_path, payroll, pay_count);
                return json_ok("\"message\":\"Payroll released.\"");
            }
        }
        return json_err("Payroll record not found.");
    }

    return json_err("Unknown payroll action.");
}

/* ============================================================
   /report
   action: summary — total gross/net/deductions for a period
   ============================================================ */
char* route_report(const char* body) {
    char ps[16], pe[16];
    json_get_str(body, "period_start", ps, sizeof(ps));
    json_get_str(body, "period_end",   pe, sizeof(pe));

    char emp_path[512], pay_path[512];
    snprintf(emp_path, sizeof(emp_path), "%s/employees.csv", g_data_dir);
    snprintf(pay_path, sizeof(pay_path), "%s/payroll.csv",   g_data_dir);

    Employee      employees[MAX_EMPLOYEES];
    PayrollRecord payroll[MAX_PAYROLL];
    int emp_count = csv_load_employees(emp_path, employees, MAX_EMPLOYEES);
    int pay_count = csv_load_payroll  (pay_path, payroll,   MAX_PAYROLL);

    double total_gross = 0, total_deductions = 0, total_net = 0;
    int    record_count = 0;

    char* rows_buf = (char*)malloc(131072);
    strcpy(rows_buf, "[");
    int first = 1;

    for (int i = 0; i < pay_count; i++) {
        PayrollRecord* p = &payroll[i];
        /* Filter by period if given */
        if (ps[0] && strcmp(p->period_start, ps) < 0) continue;
        if (pe[0] && strcmp(p->period_end,   pe) > 0) continue;

        const char* emp_name = "Unknown";
        const char* dept     = "";
        for (int j = 0; j < emp_count; j++)
            if (employees[j].id == p->employee_id) {
                emp_name = employees[j].full_name;
                dept     = employees[j].department;
                break;
            }

        total_gross       += p->gross_pay;
        total_deductions  += p->total_deductions;
        total_net         += p->net_pay;
        record_count++;

        if (!first) strcat(rows_buf, ",");
        first = 0;
        char row[512];
        snprintf(row, sizeof(row),
            "{\"id\":%d,\"employee_name\":\"%s\",\"department\":\"%s\","
            "\"period_start\":\"%s\",\"period_end\":\"%s\","
            "\"gross_pay\":%.2f,\"total_deductions\":%.2f,"
            "\"net_pay\":%.2f,\"status\":\"%s\"}",
            p->id, emp_name, dept,
            p->period_start, p->period_end,
            p->gross_pay, p->total_deductions,
            p->net_pay, p->status);
        strcat(rows_buf, row);
    }
    strcat(rows_buf, "]");

    char* result = (char*)malloc(strlen(rows_buf) + 256);
    sprintf(result,
        "\"total_gross\":%.2f,\"total_deductions\":%.2f,"
        "\"total_net\":%.2f,\"record_count\":%d,"
        "\"rows\":%s",
        total_gross, total_deductions, total_net, record_count, rows_buf);
    free(rows_buf);
    char* ret = json_ok(result);
    free(result);
    return ret;
}

/* ============================================================
   JSON Helpers
   ============================================================ */

char* json_get_str(const char* json, const char* key, char* out, int outlen) {
    out[0] = '\0';
    if (!json || !key) return out;
    char needle[128];
    snprintf(needle, sizeof(needle), "\"%s\"", key);
    const char* pos = strstr(json, needle);
    if (!pos) return out;
    pos += strlen(needle);
    while (*pos == ' ' || *pos == ':') pos++;
    if (*pos == '"') {
        pos++;
        int i = 0;
        while (*pos && *pos != '"' && i < outlen-1) {
            if (*pos == '\\' && *(pos+1)) {
                pos++;
                switch (*pos) {
                    case '"':  out[i++] = '"';  break;
                    case '\\': out[i++] = '\\'; break;
                    case 'n':  out[i++] = '\n'; break;
                    default:   out[i++] = *pos; break;
                }
            } else {
                out[i++] = *pos;
            }
            pos++;
        }
        out[i] = '\0';
    } else {
        int i = 0;
        while (*pos && *pos != ',' && *pos != '}' && *pos != ' ' && i < outlen-1)
            out[i++] = *pos++;
        out[i] = '\0';
    }
    return out;
}

int json_get_int(const char* json, const char* key, int def) {
    char buf[32];
    json_get_str(json, key, buf, sizeof(buf));
    if (!buf[0]) return def;
    return atoi(buf);
}

double json_get_dbl(const char* json, const char* key, double def) {
    char buf[32];
    json_get_str(json, key, buf, sizeof(buf));
    if (!buf[0]) return def;
    return atof(buf);
}

char* json_escape(const char* s, char* out, int outlen) {
    int i = 0, j = 0;
    while (s[i] && j < outlen-2) {
        switch (s[i]) {
            case '"':  out[j++] = '\\'; out[j++] = '"';  break;
            case '\\': out[j++] = '\\'; out[j++] = '\\'; break;
            case '\n': out[j++] = '\\'; out[j++] = 'n';  break;
            case '\r': out[j++] = '\\'; out[j++] = 'r';  break;
            default:   out[j++] = s[i]; break;
        }
        i++;
    }
    out[j] = '\0';
    return out;
}

char* json_ok(const char* body) {
    size_t len = strlen(body) + 16;
    char*  out = (char*)malloc(len);
    if (body && body[0])
        snprintf(out, len, "{\"ok\":true,%s}", body);
    else
        strcpy(out, "{\"ok\":true}");
    return out;
}

char* json_err(const char* msg) {
    char esc[512];
    json_escape(msg, esc, sizeof(esc));
    size_t len = strlen(esc) + 32;
    char*  out = (char*)malloc(len);
    snprintf(out, len, "{\"ok\":false,\"error\":\"%s\"}", esc);
    return out;
}
