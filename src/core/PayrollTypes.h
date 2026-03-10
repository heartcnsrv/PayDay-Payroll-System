/* ============================================================
   PayDay | src/core/PayrollTypes.h
   Core structs — mirrors ThinkFast's GameTypes.h pattern.
   Pure C (C99). No C++ at all.
   ============================================================ */
#ifndef PAYDAY_PAYROLL_TYPES_H
#define PAYDAY_PAYROLL_TYPES_H

#include <stddef.h>

/* ── Roles ─────────────────────────────────────────────────── */
typedef enum {
    ROLE_ADMIN    = 0,
    ROLE_EMPLOYEE = 1
} UserRole;

/* ── Employee record (maps to employees.csv) ────────────────── */
typedef struct {
    int    id;
    char   username[64];
    char   password[64];       /* plain for now, same as ThinkFast */
    char   full_name[128];
    char   department[64];
    char   position[64];
    double base_salary;        /* monthly base */
    double hourly_rate;        /* for overtime calc */
    char   email[128];
    char   phone[32];
    char   date_hired[16];     /* YYYY-MM-DD */
    UserRole role;
    int    active;             /* 1 = active, 0 = deactivated */
} Employee;

/* ── Payroll record (maps to payroll.csv) ───────────────────── */
typedef struct {
    int    id;
    int    employee_id;
    char   period_start[16];   /* YYYY-MM-DD */
    char   period_end[16];
    double base_salary;
    double overtime_hours;
    double overtime_rate;      /* multiplier, e.g. 1.25 */
    double overtime_pay;
    double tax_deduction;
    double sss_deduction;
    double philhealth_deduction;
    double pagibig_deduction;
    double other_deductions;
    double gross_pay;
    double total_deductions;
    double net_pay;
    char   generated_date[16];
    char   status[16];         /* "generated", "released" */
} PayrollRecord;

/* ── Deduction presets ──────────────────────────────────────── */
typedef struct {
    double tax_rate;           /* e.g. 0.15 for 15% */
    double sss_rate;
    double philhealth_rate;
    double pagibig_fixed;      /* flat amount */
} DeductionRates;

/* ── Sizes ──────────────────────────────────────────────────── */
#define MAX_EMPLOYEES   256
#define MAX_PAYROLL     1024
#define MAX_NAME        128
#define MAX_PATH        512

#endif /* PAYDAY_PAYROLL_TYPES_H */
