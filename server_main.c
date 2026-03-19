/* ============================================================
   C HTTP backend — port 8081.

   Data lives in data/employees.csv and data/payroll.csv.

   Usage (Windows):
     PayDayServer.exe
   Usage (Linux/Mac):
     ./PayDayServer

   Browser → Apache/WAMP/XAMPP → backend/proxy.php → :8081
   ============================================================ */

#include "src/server/HttpServer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/*
 * Server entry point.
 * It prepares the CSV-backed storage, checks required files, then starts the
 * HTTP backend that the browser frontend talks to through proxy.php.
 */
#ifdef _WIN32
  #include <direct.h>
  #define mkdir_compat(p) _mkdir(p)
#else
  #include <sys/stat.h>
  #define mkdir_compat(p) mkdir(p, 0755)
#endif

static void on_signal(int s) {
    (void)s;
    printf("\n[PayDay] Shutting down...\n");
    exit(0);
}

static void ensure_payroll_header(void) {
    /* Creates payroll.csv with the expected columns on first startup. */
    char path[512];
    snprintf(path, sizeof(path), "%.490s/payroll.csv", g_data_dir);
    FILE* f = fopen(path, "r");
    if (f) { fclose(f); return; }
    f = fopen(path, "w");
    if (!f) { fprintf(stderr, "[PayDay] Cannot create %s\n", path); return; }
    fprintf(f,
        "id,employee_id,period_start,period_end,base_salary,"
        "overtime_hours,overtime_rate,overtime_pay,"
        "tax_deduction,sss_deduction,philhealth_deduction,"
        "pagibig_deduction,other_deductions,"
        "gross_pay,total_deductions,net_pay,generated_date,status\n");
    fclose(f);
    printf("[PayDay] Created empty payroll.csv\n");
}

int main(int argc, char* argv[]) {
    signal(SIGINT,  on_signal);
    signal(SIGTERM, on_signal);

    int port = 8081;
    if (argc > 1) port = atoi(argv[1]);

    /* Share the storage directory with every route in HttpServer.c. */
    strncpy(g_data_dir, "data", sizeof(g_data_dir) - 1);
    mkdir_compat(g_data_dir);
    ensure_payroll_header();

    char emp_path[512];
    snprintf(emp_path, sizeof(emp_path), "%.490s/employees.csv", g_data_dir);
    FILE* ef = fopen(emp_path, "r");
    if (!ef) {
        fprintf(stderr, "[PayDay] ERROR: %s not found.\n", emp_path);
        fprintf(stderr, "[PayDay] Place employees.csv in the data/ folder.\n");
        return 1;
    }
    fclose(ef);

    printf("[PayDay] Employee Payroll System — C Backend\n");
    printf("[PayDay] Data    : %s/\n", g_data_dir);
    printf("[PayDay] Port    : %d\n",  port);
    printf("[PayDay] Endpoints:\n");
    printf("   POST http://localhost:%d/auth\n",      port);
    printf("   POST http://localhost:%d/employees\n", port);
    printf("   POST http://localhost:%d/payroll\n",   port);
    printf("   POST http://localhost:%d/report\n",    port);
    printf("[PayDay] Press Ctrl+C to stop.\n\n");

    /* From this point on, requests are routed into CSV load/save operations. */
    server_run(port);
    return 0;
}
