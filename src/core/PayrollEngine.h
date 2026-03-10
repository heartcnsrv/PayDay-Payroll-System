/* ============================================================
   PayDay | src/core/PayrollEngine.h
   All payroll calculation logic — pure C99.
   ============================================================ */
#ifndef PAYDAY_PAYROLL_ENGINE_H
#define PAYDAY_PAYROLL_ENGINE_H

#include "PayrollTypes.h"

/* ── Default Philippine deduction rates ─────────────────────── */
#define DEFAULT_TAX_RATE         0.15
#define DEFAULT_SSS_RATE         0.045
#define DEFAULT_PHILHEALTH_RATE  0.025
#define DEFAULT_PAGIBIG_FIXED    100.0
#define DEFAULT_OVERTIME_RATE    1.25   /* 125% of hourly rate */

/* ── Calculate a full payroll record for one employee ────────── */
/*    Fills all computed fields: gross, deductions, net.         */
void payroll_calculate(
    PayrollRecord*      out,
    const Employee*     emp,
    const char*         period_start,
    const char*         period_end,
    double              overtime_hours,
    double              other_deductions,
    const DeductionRates* rates       /* pass NULL for defaults */
);

/* ── Individual calculators (also usable standalone) ─────────── */
double payroll_overtime_pay    (double hourly_rate, double hours, double multiplier);
double payroll_tax             (double gross, double rate);
double payroll_sss             (double gross, double rate);
double payroll_philhealth      (double gross, double rate);
double payroll_net             (double gross, double total_deductions);

/* ── Validate employee fields ────────────────────────────────── */
int  payroll_validate_employee (const Employee* e, char* err_out, int err_len);

/* ── Password reset token (simple 6-digit code) ──────────────── */
void payroll_generate_token    (char* out, int len);

#endif /* PAYDAY_PAYROLL_ENGINE_H */
