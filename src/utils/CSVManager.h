/* ============================================================
   PayDay | src/utils/CSVManager.h
   CSV persistence layer — pure C99.
   Mirrors ThinkFast's CSVManager pattern.
   ============================================================ */
#ifndef PAYDAY_CSV_MANAGER_H
#define PAYDAY_CSV_MANAGER_H

#include "../core/PayrollTypes.h"

/* ── Employee CSV ───────────────────────────────────────────── */
int  csv_load_employees (const char* path, Employee* out, int max);
int  csv_save_employees (const char* path, const Employee* arr, int count);
int  csv_next_employee_id(const Employee* arr, int count);

/* ── Payroll CSV ────────────────────────────────────────────── */
int  csv_load_payroll   (const char* path, PayrollRecord* out, int max);
int  csv_save_payroll   (const char* path, const PayrollRecord* arr, int count);
int  csv_next_payroll_id(const PayrollRecord* arr, int count);

/* ── Helpers ────────────────────────────────────────────────── */
void csv_today(char* buf, int buflen);  /* writes YYYY-MM-DD */

#endif /* PAYDAY_CSV_MANAGER_H */
