const API_BASE = "http://localhost:8081";

function pageName() {
  const p = (window.location.pathname || window.location.href || "").replace(/\\/g, "/");
  const parts = p.split("/");
  const file = parts[parts.length - 1] || "";
  return file.split("?")[0].toLowerCase();
}

function isPage(name) {
  return pageName() === String(name).toLowerCase();
}

async function apiPost(path, payload) {
  const res = await fetch(`${API_BASE}${path}`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(payload),
  });
  let data;
  try {
    data = await res.json();
  } catch (_) {
    throw new Error("Invalid server response.");
  }
  return data;
}

function showMessage(id, message, isError = false) {
  const el = document.getElementById(id);
  if (!el) return;
  el.textContent = message || "";
  el.classList.toggle("is-error", !!isError);
}

// ========== AUTH ==========
function handleLoginPage() {
  if (!isPage("index.html")) return;
  const form = document.querySelector("form.form");
  if (!form) return;
  form.addEventListener("submit", async (e) => {
    e.preventDefault();
    showMessage("login-error", "");
    const username = form.username?.value.trim() || "";
    const password = form.password?.value || "";
    if (!username || !password) {
      showMessage("login-error", "Please enter both username and password.", true);
      return;
    }
    try {
      const data = await apiPost("/auth", { action: "login", username, password });
      if (!data.ok) {
        showMessage("login-error", data.error || "Login failed.", true);
        return;
      }
      sessionStorage.setItem("paydayUser", JSON.stringify(data));
      const role = typeof data.role === "number" ? data.role : 1;
      window.location.href = role === 0 ? "admin-employee-dashboard.html" : "employee-payroll.html";
    } catch (err) {
      showMessage("login-error", err.message || "Unable to reach server.", true);
    }
  });
}

// ========== ADD EMPLOYEE (admin-employee-add.html only) ==========
function handleAdminAddEmployeePage() {
  if (!isPage("admin-employee-add.html")) return;
  const form = document.querySelector("form.employee-form");
  if (!form) return;
  form.addEventListener("submit", async (e) => {
    e.preventDefault();
    showMessage("employee-add-message", "");
    const payload = {
      action: "add",
      username: form.username?.value.trim() || "",
      password: form.password?.value || "",
      full_name: form.full_name?.value.trim() || "",
      department: form.department?.value.trim() || "",
      position: form.position?.value.trim() || "",
      email: form.email?.value.trim() || "",
      phone: form.phone?.value.trim() || "",
      date_hired: form.date_hired?.value.trim() || "",
      base_salary: parseFloat(form.base_salary?.value || "0") || 0,
      hourly_rate: parseFloat(form.hourly_rate?.value || "0") || 0,
      role: 1,
    };
    try {
      const data = await apiPost("/employees", payload);
      if (!data.ok) {
        showMessage("employee-add-message", data.error || "Failed to add employee.", true);
        return;
      }
      form.reset();
      showMessage("employee-add-message", data.message || "Employee added (ID: " + (data.id ?? "?") + ").");
    } catch (err) {
      showMessage("employee-add-message", err.message || "Unable to reach server.", true);
    }
  });
}

// ========== VIEW EMPLOYEES ==========
function handleAdminEmployeeViewPage() {
  if (!isPage("admin-employee-view.html")) return;
  const tableBody = document.querySelector(".employee-table tbody");
  if (!tableBody) return;
  (async () => {
    showMessage("admin-employee-view-message", "Loading employees...");
    try {
      const data = await apiPost("/employees", { action: "list" });
      if (!data.ok) {
        showMessage("admin-employee-view-message", data.error || "Failed to load.", true);
        return;
      }
      const employees = Array.isArray(data.employees) ? data.employees : [];
      tableBody.innerHTML = "";
      for (const e of employees) {
        const tr = document.createElement("tr");
        tr.innerHTML = `<td>${e.id ?? ""}</td><td>${e.full_name ?? ""}</td><td>${e.department ?? ""}</td><td>${e.position ?? ""}</td><td>${typeof e.base_salary === "number" ? e.base_salary.toFixed(2) : ""}</td><td>${e.role ?? ""}</td>`;
        tableBody.appendChild(tr);
      }
      showMessage("admin-employee-view-message", employees.length ? "" : "No employees.");
    } catch (err) {
      showMessage("admin-employee-view-message", err.message || "Unable to reach server.", true);
    }
  })();
}

// ========== EDIT EMPLOYEE SEARCH (admin-employee-edit-search.html only) ==========
function handleAdminEmployeeEditSearchPage() {
  if (!isPage("admin-employee-edit-search.html")) return;
  const searchInput = document.querySelector(".search-input");
  if (!searchInput) return;
  let all = [];
  (async () => {
    try {
      const data = await apiPost("/employees", { action: "list" });
      if (data.ok && Array.isArray(data.employees)) all = data.employees;
    } catch (_) {}
  })();
  searchInput.addEventListener("input", (e) => {
    const q = e.target.value.trim().toLowerCase();
    const section = document.querySelector(".search-section");
    const existing = section?.querySelector(".search-results");
    if (existing) existing.remove();
    if (!q) return;
    const matches = all.filter(
      (emp) =>
        String(emp.id).includes(q) ||
        (emp.full_name || "").toLowerCase().includes(q) ||
        (emp.username || "").toLowerCase().includes(q)
    );
    if (!matches.length) return;
    const div = document.createElement("div");
    div.className = "search-results";
    matches.forEach((emp) => {
      const row = document.createElement("div");
      row.className = "search-result-item";
      row.innerHTML = `<div class="employee-info"><strong>${emp.full_name}</strong> (ID: ${emp.id})<br><small>${emp.department} - ${emp.position}</small></div><button class="select-btn" data-edit-id="${emp.id}">Edit</button>`;
      div.appendChild(row);
    });
    div.addEventListener("click", (ev) => {
      const btn = ev.target.closest?.("button[data-edit-id]");
      if (!btn) return;
      const id = parseInt(btn.dataset.editId, 10);
      const emp = all.find((e) => e.id === id);
      if (emp) {
        sessionStorage.setItem("employeeEditSelected", JSON.stringify(emp));
        window.location.href = "admin-employee-edit-found.html";
      }
    });
    section.appendChild(div);
  });
}

// ========== EDIT EMPLOYEE FORM (admin-employee-edit-found.html only) ==========
function handleAdminEmployeeEditFoundPage() {
  if (!isPage("admin-employee-edit-found.html")) return;
  const form = document.querySelector("form.employee-form");
  if (!form) return;
  const raw = sessionStorage.getItem("employeeEditSelected");
  if (!raw) {
    window.location.href = "admin-employee-edit-search.html";
    return;
  }
  let emp;
  try {
    emp = JSON.parse(raw);
  } catch (_) {
    window.location.href = "admin-employee-edit-search.html";
    return;
  }
  const si = document.querySelector(".search-input");
  if (si) si.value = String(emp.id ?? "");
  const rt = document.querySelector(".result-text");
  if (rt) rt.textContent = `User: ${emp.id} - ${emp.full_name} found!`;
  if (form.username) form.username.value = emp.username || "";
  if (form.department) form.department.value = emp.department || "";
  if (form.position) form.position.value = emp.position || "";
  if (form.full_name) form.full_name.value = emp.full_name || "";
  if (form.date_hired) form.date_hired.value = emp.date_hired || "";
  if (form.email) form.email.value = emp.email || "";
  if (form.phone) form.phone.value = emp.phone || "";
  if (form.base_salary) form.base_salary.value = emp.base_salary ?? "";
  if (form.hourly_rate) form.hourly_rate.value = emp.hourly_rate ?? "";

  form.addEventListener("submit", async (e) => {
    e.preventDefault();
    showMessage("employee-edit-message", "");
    const payload = {
      action: "update",
      id: emp.id,
      full_name: form.full_name?.value.trim() || "",
      department: form.department?.value.trim() || "",
      position: form.position?.value.trim() || "",
      email: form.email?.value.trim() || "",
      phone: form.phone?.value.trim() || "",
      base_salary: parseFloat(form.base_salary?.value || "0") || 0,
      hourly_rate: parseFloat(form.hourly_rate?.value || "0") || 0,
    };
    if (form.password?.value && form.password.value.length >= 3) payload.password = form.password.value;
    try {
      const data = await apiPost("/employees", payload);
      if (!data.ok) {
        showMessage("employee-edit-message", data.error || "Failed to update.", true);
        return;
      }
      showMessage("employee-edit-message", data.message || "Employee updated.");
    } catch (err) {
      showMessage("employee-edit-message", err.message || "Unable to reach server.", true);
    }
  });
  const cancelBtn = document.querySelector(".cancel-btn");
  if (cancelBtn) cancelBtn.addEventListener("click", () => { window.location.href = "admin-employee-edit-search.html"; });
}

// ========== DEACTIVATE SEARCH ==========
function handleAdminEmployeeDeactivateSearchPage() {
  if (!isPage("admin-employee-deactivate-search.html")) return;
  const searchInput = document.querySelector(".search-input");
  if (!searchInput) return;
  let all = [];
  (async () => {
    try {
      const data = await apiPost("/employees", { action: "list" });
      if (data.ok && Array.isArray(data.employees)) all = data.employees;
    } catch (_) {}
  })();
  searchInput.addEventListener("input", (e) => {
    const q = e.target.value.trim().toLowerCase();
    const section = document.querySelector(".search-section");
    const existing = section?.querySelector(".search-results");
    if (existing) existing.remove();
    if (!q) return;
    const matches = all.filter(
      (emp) =>
        String(emp.id).includes(q) ||
        (emp.full_name || "").toLowerCase().includes(q) ||
        (emp.username || "").toLowerCase().includes(q)
    );
    if (!matches.length) return;
    const div = document.createElement("div");
    div.className = "search-results";
    matches.forEach((emp) => {
      const row = document.createElement("div");
      row.className = "search-result-item";
      row.innerHTML = `<div class="employee-info"><strong>${emp.full_name}</strong> (ID: ${emp.id})<br><small>${emp.department} - ${emp.position}</small></div><button class="select-btn" data-deact-id="${emp.id}">Deactivate</button>`;
      div.appendChild(row);
    });
    div.addEventListener("click", (ev) => {
      const btn = ev.target.closest?.("button[data-deact-id]");
      if (!btn) return;
      const id = parseInt(btn.dataset.deactId, 10);
      const emp = all.find((e) => e.id === id);
      if (emp) {
        sessionStorage.setItem("employeeDeactivateSelected", JSON.stringify(emp));
        window.location.href = "admin-employee-deactivate-status.html";
      }
    });
    section.appendChild(div);
  });
}

// ========== DEACTIVATE CONFIRM ==========
function handleAdminEmployeeDeactivateStatusPage() {
  if (!isPage("admin-employee-deactivate-status.html")) return;
  const raw = sessionStorage.getItem("employeeDeactivateSelected");
  if (!raw) {
    window.location.href = "admin-employee-deactivate-search.html";
    return;
  }
  let emp;
  try {
    emp = JSON.parse(raw);
  } catch (_) {
    window.location.href = "admin-employee-deactivate-search.html";
    return;
  }
  const si = document.querySelector(".search-input");
  if (si) si.value = String(emp.id ?? "");
  const rt = document.querySelector(".result-text");
  if (rt) rt.textContent = `User: ${emp.id} - ${emp.full_name} found!`;
  const title = document.querySelector(".modal-body h2");
  if (title) title.textContent = `Deactivate: ${emp.id} - ${emp.full_name}`;

  const confirmBtn = document.querySelector(".confirm-btn");
  if (confirmBtn) {
    confirmBtn.addEventListener("click", async () => {
      showMessage("employee-deactivate-message", "Deactivating...");
      try {
        const data = await apiPost("/employees", { action: "deactivate", id: emp.id });
        if (!data.ok) {
          showMessage("employee-deactivate-message", data.error || "Failed.", true);
          return;
        }
        showMessage("employee-deactivate-message", data.message || "Deactivated.");
        sessionStorage.removeItem("employeeDeactivateSelected");
      } catch (err) {
        showMessage("employee-deactivate-message", err.message || "Unable to reach server.", true);
      }
    });
  }
  const cancelBtn = document.querySelector(".modal-actions .cancel-btn");
  if (cancelBtn) cancelBtn.addEventListener("click", () => { window.location.href = "admin-employee-deactivate-search.html"; });
}

// ========== EMPLOYEE PAYROLL ==========
function handleEmployeePayrollPage() {
  if (!isPage("employee-payroll.html")) return;
  const tableBody = document.querySelector(".payroll-table tbody");
  if (!tableBody) return;
  let user = null;
  try {
    const raw = sessionStorage.getItem("paydayUser");
    if (raw) user = JSON.parse(raw);
  } catch (_) {}
  if (!user || typeof user.id !== "number") {
    showMessage("employee-payroll-message", "Please sign in again.", true);
    return;
  }
  (async () => {
    showMessage("employee-payroll-message", "Loading...");
    try {
      const data = await apiPost("/payroll", { action: "list", employee_id: user.id });
      if (!data.ok) { showMessage("employee-payroll-message", data.error || "Failed.", true); return; }
      const records = Array.isArray(data.records) ? data.records : [];
      tableBody.innerHTML = "";
      for (const rec of records) {
        const tr = document.createElement("tr");
        tr.innerHTML = `<td>${rec.id ?? ""}</td><td>${rec.employee_name ?? ""}</td><td>${rec.period_start ?? ""}</td><td>${rec.period_end ?? ""}</td><td>${typeof rec.gross_pay === "number" ? rec.gross_pay.toFixed(2) : ""}</td><td>${typeof rec.net_pay === "number" ? rec.net_pay.toFixed(2) : ""}</td><td>${rec.status ?? ""}</td>`;
        tableBody.appendChild(tr);
      }
      showMessage("employee-payroll-message", records.length ? "" : "No records.");
    } catch (err) {
      showMessage("employee-payroll-message", err.message || "Unable to reach server.", true);
    }
  })();
}

// ========== ADMIN PAYROLL RECORDS ==========
function handleAdminPayrollRecordsPage() {
  if (!isPage("admin-payroll-records.html")) return;
  const tableBody = document.querySelector(".records-table tbody");
  if (!tableBody) return;
  (async () => {
    showMessage("admin-payroll-records-message", "Loading...");
    try {
      const data = await apiPost("/payroll", { action: "list" });
      if (!data.ok) { showMessage("admin-payroll-records-message", data.error || "Failed.", true); return; }
      const records = Array.isArray(data.records) ? data.records : [];
      tableBody.innerHTML = "";
      for (const rec of records) {
        const tr = document.createElement("tr");
        tr.innerHTML = `<td>${rec.id ?? ""}</td><td>${rec.employee_name ?? ""}</td><td>${rec.period_start ?? ""}</td><td>${rec.period_end ?? ""}</td><td>${typeof rec.gross_pay === "number" ? rec.gross_pay.toFixed(2) : ""}</td><td>${typeof rec.net_pay === "number" ? rec.net_pay.toFixed(2) : ""}</td><td>${rec.status ?? ""}</td>`;
        tableBody.appendChild(tr);
      }
      showMessage("admin-payroll-records-message", records.length ? "" : "No records.");
    } catch (err) {
      showMessage("admin-payroll-records-message", err.message || "Unable to reach server.", true);
    }
  })();
}

// ========== ADMIN PAYROLL REPORT ==========
function handleAdminPayrollReportPage() {
  if (!isPage("admin-payroll-report.html")) return;
  const tableBody = document.querySelector(".payroll-summary-table tbody");
  if (!tableBody) return;
  (async () => {
    showMessage("admin-payroll-report-message", "Loading...");
    try {
      const data = await apiPost("/report", { period_start: "", period_end: "" });
      if (!data.ok) { showMessage("admin-payroll-report-message", data.error || "Failed.", true); return; }
      const rows = Array.isArray(data.rows) ? data.rows : [];
      tableBody.innerHTML = "";
      for (const row of rows) {
        const period = row.period_start && row.period_end ? `${row.period_start} to ${row.period_end}` : row.period_start || row.period_end || "";
        const tr = document.createElement("tr");
        tr.innerHTML = `<td>${row.employee_name ?? ""}</td><td>${row.department ?? ""}</td><td>${period}</td><td>${typeof row.gross_pay === "number" ? row.gross_pay.toFixed(2) : ""}</td><td>${typeof row.total_deductions === "number" ? row.total_deductions.toFixed(2) : ""}</td><td>${typeof row.net_pay === "number" ? row.net_pay.toFixed(2) : ""}</td>`;
        tableBody.appendChild(tr);
      }
      showMessage("admin-payroll-report-message", rows.length ? "" : "No records.");
    } catch (err) {
      showMessage("admin-payroll-report-message", err.message || "Unable to reach server.", true);
    }
  })();
}

// ========== CALCULATE SALARY (admin-salary-search.html only) ==========
function handleAdminSalarySearchPage() {
  if (!isPage("admin-salary-search.html")) return;
  const searchInput = document.querySelector(".search-input");
  if (!searchInput) return;
  let employees = [];
  (async () => {
    try {
      const data = await apiPost("/employees", { action: "list" });
      if (data.ok && Array.isArray(data.employees)) employees = data.employees;
    } catch (_) {}
  })();
  searchInput.addEventListener("input", (e) => {
    const q = e.target.value.trim().toLowerCase();
    const section = document.querySelector(".search-section");
    const existing = section?.querySelector(".search-results");
    if (existing) existing.remove();
    if (!q) return;
    const matches = employees.filter(
      (emp) =>
        String(emp.id).includes(q) ||
        (emp.full_name || "").toLowerCase().includes(q) ||
        (emp.username || "").toLowerCase().includes(q)
    );
    if (!matches.length) return;
    const div = document.createElement("div");
    div.className = "search-results";
    matches.forEach((emp) => {
      const row = document.createElement("div");
      row.className = "search-result-item";
      row.innerHTML = `<div class="employee-info"><strong>${emp.full_name}</strong> (ID: ${emp.id})<br><small>${emp.department} - ${emp.position}</small></div><button class="select-btn" data-salary-id="${emp.id}">Select</button>`;
      div.appendChild(row);
    });
    div.addEventListener("click", (ev) => {
      const btn = ev.target.closest?.("button[data-salary-id]");
      if (!btn) return;
      const id = parseInt(btn.dataset.salaryId, 10);
      const emp = employees.find((e) => e.id === id);
      if (emp) {
        sessionStorage.setItem("selectedEmployee", JSON.stringify(emp));
        window.location.href = "admin-salary-edit.html";
      }
    });
    section.appendChild(div);
  });
}

// ========== CALCULATE SALARY EDIT (admin-salary-edit.html only) ==========
function handleAdminSalaryEditPage() {
  if (!isPage("admin-salary-edit.html")) return;
  const form = document.querySelector(".payroll-form");
  const calcBtn = document.querySelector(".calc-btn");
  const cancelBtn = document.querySelector(".cancel-btn");
  if (!form || !calcBtn) return;
  const raw = sessionStorage.getItem("selectedEmployee");
  if (raw) {
    try {
      const emp = JSON.parse(raw);
      const searchInput = document.querySelector(".search-input");
      const infoBar = document.querySelector(".employee-info-bar");
      if (searchInput) searchInput.value = emp.full_name;
      if (infoBar) infoBar.innerHTML = `<span>${emp.full_name}</span><span>Base: PHP ${(emp.base_salary || 0).toFixed(2)}</span><span>Rate: PHP ${(emp.hourly_rate || 0).toFixed(2)}/hr</span>`;
      form.dataset.employeeId = emp.id;
    } catch (_) {}
  }
  calcBtn.addEventListener("click", async (e) => {
    e.preventDefault();
    const employeeId = parseInt(form.dataset.employeeId || "0", 10);
    if (!employeeId) { showMessage("salary-calc-message", "No employee selected.", true); return; }
    const periodStart = form.elements["period_start"]?.value.trim() || "";
    const periodEnd = form.elements["period_end"]?.value.trim() || "";
    if (!periodStart || !periodEnd) { showMessage("salary-calc-message", "Period start and end required.", true); return; }
    showMessage("salary-calc-message", "Calculating...");
    try {
      const data = await apiPost("/payroll", {
        action: "calculate",
        employee_id: employeeId,
        period_start: periodStart,
        period_end: periodEnd,
        overtime_hours: parseFloat(form.elements["overtime_hours"]?.value || "0"),
        other_deductions: parseFloat(form.elements["other_deductions"]?.value || "0"),
      });
      if (!data.ok) { showMessage("salary-calc-message", data.error || "Failed.", true); return; }
      sessionStorage.setItem("payrollResult", JSON.stringify(data.payroll));
      window.location.href = "admin-salary-report.html";
    } catch (err) {
      showMessage("salary-calc-message", err.message || "Unable to reach server.", true);
    }
  });
  if (cancelBtn) cancelBtn.addEventListener("click", () => { sessionStorage.removeItem("selectedEmployee"); window.location.href = "admin-salary-search.html"; });
}

// ========== SALARY REPORT ==========
function handleAdminSalaryReportPage() {
  if (!isPage("admin-salary-report.html")) return;
  const table = document.querySelector(".report-table");
  if (!table) return;
  const raw = sessionStorage.getItem("payrollResult");
  if (!raw) return;
  try {
    const payroll = JSON.parse(raw);
    const periodRow = table.querySelector(".period-row");
    if (periodRow) periodRow.textContent = `Period: ${payroll.period_start} to ${payroll.period_end}`;
    const header = table.querySelector(".emp-name-header");
    if (header) header.textContent = payroll.employee_name || "";
    const cells = table.querySelectorAll(".amount");
    if (cells.length >= 9) {
      cells[0].textContent = (payroll.base_salary || 0).toFixed(2);
      cells[1].textContent = (payroll.overtime_pay || 0).toFixed(2);
      cells[2].textContent = (payroll.gross_pay || 0).toFixed(2);
      cells[3].textContent = (payroll.tax_deduction || 0).toFixed(2);
      cells[4].textContent = (payroll.sss_deduction || 0).toFixed(2);
      cells[5].textContent = (payroll.philhealth_deduction || 0).toFixed(2);
      cells[6].textContent = (payroll.pagibig_deduction || 0).toFixed(2);
      cells[7].textContent = (payroll.other_deductions || 0).toFixed(2);
      cells[8].textContent = (payroll.total_deductions || 0).toFixed(2);
      const netRow = table.querySelector(".net-pay-row .amount");
      if (netRow) netRow.textContent = (payroll.net_pay || 0).toFixed(2);
    }
  } catch (_) {}
}

document.addEventListener("DOMContentLoaded", () => {
  handleLoginPage();
  handleAdminAddEmployeePage();
  handleAdminEmployeeViewPage();
  handleAdminEmployeeEditSearchPage();
  handleAdminEmployeeEditFoundPage();
  handleAdminEmployeeDeactivateSearchPage();
  handleAdminEmployeeDeactivateStatusPage();
  handleEmployeePayrollPage();
  handleAdminPayrollRecordsPage();
  handleAdminPayrollReportPage();
  handleAdminSalarySearchPage();
  handleAdminSalaryEditPage();
  handleAdminSalaryReportPage();
});
