#ifndef PAYDAY_PAYROLL_TYPES_H
#define PAYDAY_PAYROLL_TYPES_H

#include <stddef.h>

typedef enum {
    ROLE_ADMIN    = 0,
    ROLE_EMPLOYEE = 1
} UserRole;

typedef struct {
    int    id;
    char   username[64];
    char   password[64];     
    char   full_name[128];
    char   department[64];
    char   position[64];
    double base_salary;        
    double hourly_rate;        
    char   email[128];
    char   phone[32];
    char   date_hired[16];   
    UserRole role;
    int    active;          
} Employee;

typedef struct {
    int    id;
    int    employee_id;
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
    double net_pay;
    char   generated_date[16];
    char   status[16];  
} PayrollRecord;

typedef struct {
    double tax_rate;       
    double sss_rate;
    double philhealth_rate;
    double pagibig_fixed;     
} DeductionRates;

#define MAX_EMPLOYEES   256
#define MAX_PAYROLL     1024
#define MAX_NAME        128
#define MAX_PATH        512

#endif
