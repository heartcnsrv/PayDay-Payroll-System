#ifndef PAYDAY_PAYROLL_TYPES_H
#define PAYDAY_PAYROLL_TYPES_H

#include <stddef.h>

/*
 * These structs are the in-memory
 * version of rows stored in the CSV files under data.
 */
typedef enum {
    ROLE_ADMIN    = 0,
    ROLE_EMPLOYEE = 1
} UserRole;

typedef struct {
    int    id;               /* Primary key from employees.csv. */
    char   username[64];     /* Login identifier and duplicate-check field. */
    char   password[64];     /* Stored directly in CSV in this demo project. */
    char   full_name[128];
    char   department[64];
    char   position[64];
    double base_salary;      /* Fixed pay used as the payroll base amount. */
    double hourly_rate;      /* Used when computing overtime pay. */
    char   email[128];
    char   phone[32];
    char   date_hired[16];   /* YYYY-MM-DD string. */
    UserRole role;           /* Determines admin vs employee access. */
    int    active;           /* Soft-delete flag; inactive rows stay in CSV. */
} Employee;

typedef struct {
    int    id;                    /* Primary key from payroll.csv. */
    int    employee_id;           /* Links back to Employee.id. */
    char   period_start[16];
    char   period_end[16];
    double base_salary;
    double overtime_hours;
    double overtime_rate;
    double overtime_pay;
    double tax_deduction;
    double sss_deduction;
    double philhealth_deduction;
    double pagibig_deduction;
    double other_deductions;
    double gross_pay;
    double total_deductions;
    double net_pay;               /* Final take-home pay after deductions. */
    char   generated_date[16];
    char   status[16];            /* "generated" or "released". */
} PayrollRecord;

typedef struct {
    double tax_rate;
    double sss_rate;
    double philhealth_rate;
    double pagibig_fixed;     /* Fixed amount instead of a percentage. */
} DeductionRates;

/* Fixed-capacity arrays replace dynamic database result sets. */
#define MAX_EMPLOYEES   256
#define MAX_PAYROLL     1024
#define MAX_NAME        128
#define MAX_PATH        512

#endif
