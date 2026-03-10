/* ============================================================
   PayDay | src/utils/CSVManager.c
   ============================================================ */
#include "CSVManager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ── Internal: strip surrounding quotes and whitespace ──────── */
static void strip(char* s) {
    if (!s || !*s) return;
    /* trailing */
    int len = (int)strlen(s);
    while (len > 0 && (s[len-1] == '\r' || s[len-1] == '\n' ||
                       s[len-1] == ' '  || s[len-1] == '"')) {
        s[--len] = '\0';
    }
    /* leading */
    char* p = s;
    while (*p == ' ' || *p == '"') p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
}

/* ── Internal: parse one CSV line into tokens ───────────────── */
static int parse_csv_line(char* line, char** tokens, int max_tok) {
    int n = 0;
    char* p = line;
    while (*p && n < max_tok) {
        tokens[n++] = p;
        /* find next comma (skip quoted fields) */
        int in_quote = 0;
        while (*p) {
            if (*p == '"') in_quote = !in_quote;
            else if (*p == ',' && !in_quote) { *p = '\0'; p++; break; }
            p++;
        }
    }
    /* strip each token */
    for (int i = 0; i < n; i++) strip(tokens[i]);
    return n;
}

/* ── Employee CSV ───────────────────────────────────────────── */

int csv_load_employees(const char* path, Employee* out, int max) {
    FILE* f = fopen(path, "r");
    if (!f) return 0;

    char line[1024];
    int  count = 0;
    int  first = 1;

    while (fgets(line, sizeof(line), f) && count < max) {
        if (first) { first = 0; continue; } /* skip header */
        char* tok[16];
        char  copy[1024];
        strncpy(copy, line, sizeof(copy)-1);
        int n = parse_csv_line(copy, tok, 16);
        if (n < 12) continue;

        Employee* e = &out[count++];
        memset(e, 0, sizeof(*e));
        e->id           = atoi(tok[0]);
        strncpy(e->username,   tok[1],  sizeof(e->username)-1);
        strncpy(e->password,   tok[2],  sizeof(e->password)-1);
        strncpy(e->full_name,  tok[3],  sizeof(e->full_name)-1);
        strncpy(e->department, tok[4],  sizeof(e->department)-1);
        strncpy(e->position,   tok[5],  sizeof(e->position)-1);
        e->base_salary  = atof(tok[6]);
        e->hourly_rate  = atof(tok[7]);
        strncpy(e->email,      tok[8],  sizeof(e->email)-1);
        strncpy(e->phone,      tok[9],  sizeof(e->phone)-1);
        strncpy(e->date_hired, tok[10], sizeof(e->date_hired)-1);
        e->role         = (UserRole)atoi(tok[11]);
        e->active       = (n > 12) ? atoi(tok[12]) : 1;
    }
    fclose(f);
    return count;
}

int csv_save_employees(const char* path, const Employee* arr, int count) {
    FILE* f = fopen(path, "w");
    if (!f) return 0;
    fprintf(f, "id,username,password,full_name,department,position,"
               "base_salary,hourly_rate,email,phone,date_hired,role,active\n");
    for (int i = 0; i < count; i++) {
        const Employee* e = &arr[i];
        fprintf(f, "%d,\"%s\",\"%s\",\"%s\",\"%s\",\"%s\","
                   "%.2f,%.2f,\"%s\",\"%s\",\"%s\",%d,%d\n",
            e->id, e->username, e->password, e->full_name,
            e->department, e->position,
            e->base_salary, e->hourly_rate,
            e->email, e->phone, e->date_hired,
            (int)e->role, e->active);
    }
    fclose(f);
    return 1;
}

int csv_next_employee_id(const Employee* arr, int count) {
    int max = 0;
    for (int i = 0; i < count; i++)
        if (arr[i].id > max) max = arr[i].id;
    return max + 1;
}

/* ── Payroll CSV ────────────────────────────────────────────── */

int csv_load_payroll(const char* path, PayrollRecord* out, int max) {
    FILE* f = fopen(path, "r");
    if (!f) return 0;

    char line[1024];
    int  count = 0;
    int  first = 1;

    while (fgets(line, sizeof(line), f) && count < max) {
        if (first) { first = 0; continue; }
        char* tok[24];
        char  copy[1024];
        strncpy(copy, line, sizeof(copy)-1);
        int n = parse_csv_line(copy, tok, 24);
        if (n < 17) continue;

        PayrollRecord* p = &out[count++];
        memset(p, 0, sizeof(*p));
        p->id                    = atoi(tok[0]);
        p->employee_id           = atoi(tok[1]);
        strncpy(p->period_start,    tok[2],  sizeof(p->period_start)-1);
        strncpy(p->period_end,      tok[3],  sizeof(p->period_end)-1);
        p->base_salary           = atof(tok[4]);
        p->overtime_hours        = atof(tok[5]);
        p->overtime_rate         = atof(tok[6]);
        p->overtime_pay          = atof(tok[7]);
        p->tax_deduction         = atof(tok[8]);
        p->sss_deduction         = atof(tok[9]);
        p->philhealth_deduction  = atof(tok[10]);
        p->pagibig_deduction     = atof(tok[11]);
        p->other_deductions      = atof(tok[12]);
        p->gross_pay             = atof(tok[13]);
        p->total_deductions      = atof(tok[14]);
        p->net_pay               = atof(tok[15]);
        strncpy(p->generated_date,  tok[16], sizeof(p->generated_date)-1);
        if (n > 17) strncpy(p->status, tok[17], sizeof(p->status)-1);
        else        strncpy(p->status, "generated", sizeof(p->status)-1);
    }
    fclose(f);
    return count;
}

int csv_save_payroll(const char* path, const PayrollRecord* arr, int count) {
    FILE* f = fopen(path, "w");
    if (!f) return 0;
    fprintf(f, "id,employee_id,period_start,period_end,base_salary,"
               "overtime_hours,overtime_rate,overtime_pay,"
               "tax_deduction,sss_deduction,philhealth_deduction,"
               "pagibig_deduction,other_deductions,"
               "gross_pay,total_deductions,net_pay,generated_date,status\n");
    for (int i = 0; i < count; i++) {
        const PayrollRecord* p = &arr[i];
        fprintf(f, "%d,%d,\"%s\",\"%s\",%.2f,%.2f,%.4f,%.2f,"
                   "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,\"%s\",\"%s\"\n",
            p->id, p->employee_id,
            p->period_start, p->period_end,
            p->base_salary, p->overtime_hours,
            p->overtime_rate, p->overtime_pay,
            p->tax_deduction, p->sss_deduction,
            p->philhealth_deduction, p->pagibig_deduction,
            p->other_deductions, p->gross_pay,
            p->total_deductions, p->net_pay,
            p->generated_date, p->status);
    }
    fclose(f);
    return 1;
}

int csv_next_payroll_id(const PayrollRecord* arr, int count) {
    int max = 0;
    for (int i = 0; i < count; i++)
        if (arr[i].id > max) max = arr[i].id;
    return max + 1;
}

/* ── Date helper ────────────────────────────────────────────── */
void csv_today(char* buf, int buflen) {
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    strftime(buf, (size_t)buflen, "%Y-%m-%d", tm);
}
