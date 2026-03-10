#ifndef PAYDAY_PAYROLL_ENGINE_H
#define PAYDAY_PAYROLL_ENGINE_H

#include "PayrollTypes.h"

#define DEFAULT_TAX_RATE         0.15
#define DEFAULT_SSS_RATE         0.045
#define DEFAULT_PHILHEALTH_RATE  0.025
#define DEFAULT_PAGIBIG_FIXED    100.0
#define DEFAULT_OVERTIME_RATE    1.25  

void payroll_calculate(
    PayrollRecord*      out,
    const Employee*     emp,
    const char*         period_start,
    const char*         period_end,
    double              overtime_hours,
    double              other_deductions,
    const DeductionRates* rates    
);

double payroll_overtime_pay    (double hourly_rate, double hours, double multiplier);
double payroll_tax             (double gross, double rate);
double payroll_sss             (double gross, double rate);
double payroll_philhealth      (double gross, double rate);
double payroll_net             (double gross, double total_deductions);

int  payroll_validate_employee (const Employee* e, char* err_out, int err_len);

void payroll_generate_token    (char* out, int len);

#endif 
