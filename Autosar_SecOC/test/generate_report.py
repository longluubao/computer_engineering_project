#!/usr/bin/env python3
"""
generate_report.py — Cantata-Style Test & Coverage Report Generator

Generates a single-page HTML report that mirrors the structure of a
QA-Systems Cantata report:
  1. Project Header (project, date, build info, compiler)
  2. Test Execution Summary (pass/fail/skip counts, overall verdict)
  3. Test Case Details (per-suite tables with verdict icons)
  4. Code Coverage Summary (statement %, branch %, function %)
  5. Per-Function Coverage Breakdown (from gcovr JSON)
  6. Link to annotated source (if lcov/gcovr HTML available)

Usage:
    python3 generate_report.py \
        --test-results-dir  build/test_results/ \
        --coverage-json     build/coverage.json \
        --output            build/report/SecOC_Test_Report.html
"""

import argparse
import datetime
import json
import os
import platform
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

# ── Jinja2 is optional for initial setup; fall back to string.Template ──
try:
    from jinja2 import Template
    HAVE_JINJA2 = True
except ImportError:
    HAVE_JINJA2 = False


# ═══════════════════════════════════════════════════════════════════════════
#  GTest XML Parsing
# ═══════════════════════════════════════════════════════════════════════════

def parse_gtest_xml(xml_path):
    """Parse a single GTest XML result file."""
    tree = ET.parse(xml_path)
    root = tree.getroot()
    suites = []

    for ts in root.iter("testsuite"):
        suite = {
            "name": ts.get("name", "Unknown"),
            "tests": int(ts.get("tests", 0)),
            "failures": int(ts.get("failures", 0)),
            "errors": int(ts.get("errors", 0)),
            "disabled": int(ts.get("disabled", 0)),
            "time": float(ts.get("time", 0)),
            "cases": [],
        }
        for tc in ts.iter("testcase"):
            failure = tc.find("failure")
            case = {
                "name": tc.get("name", "Unknown"),
                "classname": tc.get("classname", ""),
                "time": float(tc.get("time", 0)),
                "status": tc.get("status", "run"),
                "passed": failure is None,
                "failure_message": failure.get("message", "") if failure is not None else "",
            }
            suite["cases"].append(case)
        suites.append(suite)
    return suites


def collect_test_results(results_dir):
    """Collect all GTest XML results from a directory."""
    all_suites = []
    xml_files = sorted(Path(results_dir).glob("*.xml"))
    for xml_file in xml_files:
        try:
            suites = parse_gtest_xml(str(xml_file))
            all_suites.extend(suites)
        except ET.ParseError as e:
            print(f"WARNING: Failed to parse {xml_file}: {e}", file=sys.stderr)
    return all_suites


# ═══════════════════════════════════════════════════════════════════════════
#  Coverage JSON Parsing (gcovr format)
# ═══════════════════════════════════════════════════════════════════════════

def parse_coverage_json(json_path):
    """Parse gcovr JSON output for coverage data."""
    with open(json_path, "r") as f:
        data = json.load(f)

    # Extract summary
    root = data.get("root", "")
    summary = {
        "line_covered": 0,
        "line_total": 0,
        "branch_covered": 0,
        "branch_total": 0,
        "function_covered": 0,
        "function_total": 0,
    }

    files = []
    functions = []

    for fdata in data.get("files", []):
        filename = fdata.get("filename", "")
        # Skip test files and external libraries
        if "/test/" in filename or "/external/" in filename or "gtest" in filename:
            continue

        # Shorten path
        short_name = filename
        if "source/" in short_name:
            short_name = short_name[short_name.index("source/"):]

        lines = fdata.get("lines", [])
        line_total = len(lines)
        line_covered = sum(1 for l in lines if l.get("count", 0) > 0)

        branches = fdata.get("branches", [])
        branch_total = len(branches)
        branch_covered = sum(1 for b in branches if b.get("count", 0) > 0)

        summary["line_covered"] += line_covered
        summary["line_total"] += line_total
        summary["branch_covered"] += branch_covered
        summary["branch_total"] += branch_total

        file_info = {
            "filename": short_name,
            "line_covered": line_covered,
            "line_total": line_total,
            "line_pct": (line_covered * 100.0 / line_total) if line_total > 0 else 0.0,
            "branch_covered": branch_covered,
            "branch_total": branch_total,
            "branch_pct": (branch_covered * 100.0 / branch_total) if branch_total > 0 else 0.0,
        }
        files.append(file_info)

        # Extract per-function coverage
        for func in fdata.get("functions", []):
            func_lines = func.get("execution_count", 0)
            functions.append({
                "name": func.get("name", "unknown"),
                "filename": short_name,
                "execution_count": func_lines,
                "line_number": func.get("lineno", 0),
                "covered": func_lines > 0,
            })

    summary["function_total"] = len(functions)
    summary["function_covered"] = sum(1 for f in functions if f["covered"])
    summary["line_pct"] = (summary["line_covered"] * 100.0 / summary["line_total"]) if summary["line_total"] > 0 else 0.0
    summary["branch_pct"] = (summary["branch_covered"] * 100.0 / summary["branch_total"]) if summary["branch_total"] > 0 else 0.0
    summary["function_pct"] = (summary["function_covered"] * 100.0 / summary["function_total"]) if summary["function_total"] > 0 else 0.0

    return summary, files, functions


# ═══════════════════════════════════════════════════════════════════════════
#  HTML Report Template (Cantata-style)
# ═══════════════════════════════════════════════════════════════════════════

HTML_TEMPLATE = r"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>{{ project_name }} — Unit Test &amp; Coverage Report</title>
<style>
  :root {
    --pass: #27ae60; --fail: #e74c3c; --warn: #f39c12; --bg: #f8f9fa;
    --card-bg: #ffffff; --border: #dee2e6; --text: #2c3e50; --text-muted: #6c757d;
    --header-bg: #1a237e; --header-fg: #ffffff;
  }
  * { margin: 0; padding: 0; box-sizing: border-box; }
  body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
         background: var(--bg); color: var(--text); line-height: 1.6; }
  .container { max-width: 1200px; margin: 0 auto; padding: 20px; }

  /* Header bar — Cantata-style project banner */
  .header { background: var(--header-bg); color: var(--header-fg);
            padding: 24px 32px; border-radius: 8px 8px 0 0; }
  .header h1 { font-size: 1.6em; margin-bottom: 4px; }
  .header .subtitle { opacity: 0.85; font-size: 0.95em; }
  .header-meta { display: flex; gap: 32px; margin-top: 12px; font-size: 0.85em; opacity: 0.8; flex-wrap: wrap; }

  /* Overall verdict banner */
  .verdict-banner { padding: 16px 32px; font-size: 1.3em; font-weight: 700;
                    text-align: center; border-radius: 0 0 8px 8px; }
  .verdict-pass { background: var(--pass); color: #fff; }
  .verdict-fail { background: var(--fail); color: #fff; }

  /* Section cards */
  .section { background: var(--card-bg); border-radius: 8px;
             box-shadow: 0 1px 3px rgba(0,0,0,0.08); margin: 24px 0; overflow: hidden; }
  .section-title { background: #eceff1; padding: 12px 20px; font-size: 1.1em;
                   font-weight: 600; border-bottom: 1px solid var(--border); }

  /* Summary cards row */
  .summary-row { display: flex; gap: 16px; padding: 20px; flex-wrap: wrap; }
  .summary-card { flex: 1; min-width: 150px; background: var(--bg); border-radius: 8px;
                  padding: 16px; text-align: center; border: 1px solid var(--border); }
  .summary-card .number { font-size: 2em; font-weight: 700; }
  .summary-card .label { font-size: 0.85em; color: var(--text-muted); margin-top: 4px; }

  /* Coverage bar */
  .cov-bar-wrap { background: #e9ecef; border-radius: 4px; height: 22px; overflow: hidden; position: relative; }
  .cov-bar { height: 100%; border-radius: 4px; transition: width 0.4s; }
  .cov-bar-text { position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%);
                  font-size: 0.8em; font-weight: 600; color: #333; }
  .cov-good { background: var(--pass); }
  .cov-medium { background: var(--warn); }
  .cov-low { background: var(--fail); }

  /* Tables */
  table { width: 100%; border-collapse: collapse; }
  th { background: #eceff1; padding: 10px 12px; text-align: left; font-size: 0.85em;
       text-transform: uppercase; letter-spacing: 0.5px; color: var(--text-muted); }
  td { padding: 8px 12px; border-bottom: 1px solid #f0f0f0; font-size: 0.9em; }
  tr:hover td { background: #f5f7fa; }

  /* Verdict icons */
  .pass { color: var(--pass); font-weight: 700; }
  .fail { color: var(--fail); font-weight: 700; }
  .icon-pass::before { content: "PASS"; }
  .icon-fail::before { content: "FAIL"; }

  /* Footer */
  .footer { text-align: center; padding: 20px; color: var(--text-muted); font-size: 0.8em; }
  .footer a { color: var(--header-bg); }

  /* Collapsible */
  details summary { cursor: pointer; padding: 4px 0; font-weight: 600; }
  details summary:hover { color: var(--header-bg); }

  @media print { .container { max-width: 100%; } body { background: #fff; } }
</style>
</head>
<body>
<div class="container">

<!-- ═══════════════ 1. PROJECT HEADER ═══════════════ -->
<div class="header">
  <h1>{{ project_name }}</h1>
  <div class="subtitle">Unit Test &amp; Code Coverage Report</div>
  <div class="header-meta">
    <span>Date: {{ report_date }}</span>
    <span>Platform: {{ platform_info }}</span>
    <span>Compiler: {{ compiler_info }}</span>
    <span>Build: {{ build_type }}</span>
  </div>
</div>

<!-- ═══════════════ 2. OVERALL VERDICT ═══════════════ -->
<div class="verdict-banner {{ 'verdict-pass' if overall_passed else 'verdict-fail' }}">
  {{ 'OVERALL VERDICT: PASS' if overall_passed else 'OVERALL VERDICT: FAIL' }}
  — {{ total_passed }} / {{ total_tests }} test cases passed
</div>

<!-- ═══════════════ 3. TEST EXECUTION SUMMARY ═══════════════ -->
<div class="section">
  <div class="section-title">Test Execution Summary</div>
  <div class="summary-row">
    <div class="summary-card">
      <div class="number" style="color: var(--pass)">{{ total_passed }}</div>
      <div class="label">Passed</div>
    </div>
    <div class="summary-card">
      <div class="number" style="color: var(--fail)">{{ total_failed }}</div>
      <div class="label">Failed</div>
    </div>
    <div class="summary-card">
      <div class="number">{{ total_tests }}</div>
      <div class="label">Total Tests</div>
    </div>
    <div class="summary-card">
      <div class="number">{{ total_suites }}</div>
      <div class="label">Test Suites</div>
    </div>
    <div class="summary-card">
      <div class="number">{{ '%.3f' % total_time }}s</div>
      <div class="label">Total Time</div>
    </div>
  </div>
</div>

<!-- ═══════════════ 4. CODE COVERAGE SUMMARY ═══════════════ -->
{% if has_coverage %}
<div class="section">
  <div class="section-title">Code Coverage Summary</div>
  <div class="summary-row">
    <div class="summary-card">
      <div class="number">{{ '%.1f' % cov_summary.line_pct }}%</div>
      <div class="label">Statement Coverage</div>
      <div style="margin-top:8px">
        <div class="cov-bar-wrap">
          <div class="cov-bar {{ cov_class(cov_summary.line_pct) }}" style="width:{{ cov_summary.line_pct }}%"></div>
          <div class="cov-bar-text">{{ cov_summary.line_covered }}/{{ cov_summary.line_total }}</div>
        </div>
      </div>
    </div>
    <div class="summary-card">
      <div class="number">{{ '%.1f' % cov_summary.branch_pct }}%</div>
      <div class="label">Branch Coverage</div>
      <div style="margin-top:8px">
        <div class="cov-bar-wrap">
          <div class="cov-bar {{ cov_class(cov_summary.branch_pct) }}" style="width:{{ cov_summary.branch_pct }}%"></div>
          <div class="cov-bar-text">{{ cov_summary.branch_covered }}/{{ cov_summary.branch_total }}</div>
        </div>
      </div>
    </div>
    <div class="summary-card">
      <div class="number">{{ '%.1f' % cov_summary.function_pct }}%</div>
      <div class="label">Function Coverage</div>
      <div style="margin-top:8px">
        <div class="cov-bar-wrap">
          <div class="cov-bar {{ cov_class(cov_summary.function_pct) }}" style="width:{{ cov_summary.function_pct }}%"></div>
          <div class="cov-bar-text">{{ cov_summary.function_covered }}/{{ cov_summary.function_total }}</div>
        </div>
      </div>
    </div>
  </div>
</div>
{% endif %}

<!-- ═══════════════ 5. TEST CASE DETAILS (per suite) ═══════════════ -->
<div class="section">
  <div class="section-title">Test Case Details</div>
  {% for suite in test_suites %}
  <details {{ 'open' if suite.failures > 0 else '' }}>
    <summary style="padding: 12px 20px; border-bottom: 1px solid var(--border);">
      {{ suite.name }}
      — <span class="{{ 'fail' if suite.failures > 0 else 'pass' }}">
        {{ suite.tests - suite.failures }} / {{ suite.tests }} passed
      </span>
      ({{ '%.3f' % suite.time }}s)
    </summary>
    <table>
      <thead><tr><th>Test Case</th><th>Verdict</th><th>Time</th><th>Message</th></tr></thead>
      <tbody>
      {% for tc in suite.cases %}
      <tr>
        <td>{{ tc.name }}</td>
        <td><span class="{{ 'pass icon-pass' if tc.passed else 'fail icon-fail' }}"></span></td>
        <td>{{ '%.4f' % tc.time }}s</td>
        <td style="font-size:0.82em;color:var(--text-muted)">{{ tc.failure_message[:200] }}</td>
      </tr>
      {% endfor %}
      </tbody>
    </table>
  </details>
  {% endfor %}
</div>

<!-- ═══════════════ 6. PER-FILE COVERAGE ═══════════════ -->
{% if has_coverage %}
<div class="section">
  <div class="section-title">Per-File Coverage Breakdown</div>
  <table>
    <thead><tr><th>Source File</th><th>Statement %</th><th>Stmts (covered/total)</th>
               <th>Branch %</th><th>Branches (covered/total)</th></tr></thead>
    <tbody>
    {% for f in cov_files %}
    <tr>
      <td>{{ f.filename }}</td>
      <td>
        <div class="cov-bar-wrap" style="width:120px;display:inline-block;vertical-align:middle">
          <div class="cov-bar {{ cov_class(f.line_pct) }}" style="width:{{ f.line_pct }}%"></div>
          <div class="cov-bar-text">{{ '%.0f' % f.line_pct }}%</div>
        </div>
      </td>
      <td>{{ f.line_covered }}/{{ f.line_total }}</td>
      <td>
        <div class="cov-bar-wrap" style="width:120px;display:inline-block;vertical-align:middle">
          <div class="cov-bar {{ cov_class(f.branch_pct) }}" style="width:{{ f.branch_pct }}%"></div>
          <div class="cov-bar-text">{{ '%.0f' % f.branch_pct }}%</div>
        </div>
      </td>
      <td>{{ f.branch_covered }}/{{ f.branch_total }}</td>
    </tr>
    {% endfor %}
    </tbody>
  </table>
</div>

<!-- ═══════════════ 7. PER-FUNCTION COVERAGE ═══════════════ -->
<div class="section">
  <div class="section-title">Per-Function Coverage ({{ cov_summary.function_covered }}/{{ cov_summary.function_total }} functions covered)</div>
  <table>
    <thead><tr><th>Function</th><th>File</th><th>Line</th><th>Covered</th><th>Call Count</th></tr></thead>
    <tbody>
    {% for func in cov_functions %}
    <tr>
      <td><code>{{ func.name }}</code></td>
      <td>{{ func.filename }}</td>
      <td>{{ func.line_number }}</td>
      <td><span class="{{ 'pass icon-pass' if func.covered else 'fail icon-fail' }}"></span></td>
      <td>{{ func.execution_count }}</td>
    </tr>
    {% endfor %}
    </tbody>
  </table>
</div>
{% endif %}

<!-- ═══════════════ FOOTER ═══════════════ -->
<div class="footer">
  Generated by <strong>SecOC Test Report Generator</strong> (Cantata-style)
  — <a href="https://github.com/longluubao/computer_engineering_project">AUTOSAR SecOC + PQC</a>
  <br>Report format modeled after QA-Systems Cantata. Tool chain: Google Test + gcov + gcovr + Jinja2.
</div>

</div>
</body>
</html>"""


# ═══════════════════════════════════════════════════════════════════════════
#  Report Generation
# ═══════════════════════════════════════════════════════════════════════════

def cov_class(pct):
    """Return CSS class for coverage bar color."""
    if pct >= 80:
        return "cov-good"
    elif pct >= 50:
        return "cov-medium"
    return "cov-low"


def generate_report(test_results_dir, coverage_json, output_path):
    """Generate the Cantata-style HTML report."""

    # Collect test results
    test_suites = collect_test_results(test_results_dir) if os.path.isdir(test_results_dir) else []

    total_tests = sum(s["tests"] for s in test_suites)
    total_failures = sum(s["failures"] for s in test_suites)
    total_passed = total_tests - total_failures
    total_time = sum(s["time"] for s in test_suites)

    # Collect coverage data
    has_coverage = coverage_json and os.path.isfile(coverage_json)
    cov_summary = {}
    cov_files = []
    cov_functions = []
    if has_coverage:
        cov_summary, cov_files, cov_functions = parse_coverage_json(coverage_json)
        # Sort files by coverage percentage (lowest first)
        cov_files.sort(key=lambda f: f["line_pct"])
        # Sort functions: uncovered first, then by name
        cov_functions.sort(key=lambda f: (f["covered"], f["name"]))

    # Determine compiler info
    compiler_info = "GCC (Linux)"
    try:
        import subprocess
        result = subprocess.run(["gcc", "--version"], capture_output=True, text=True)
        first_line = result.stdout.split("\n")[0]
        compiler_info = first_line
    except Exception:
        pass

    # Build template context
    context = {
        "project_name": "AUTOSAR SecOC + Post-Quantum Cryptography",
        "report_date": datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        "platform_info": f"{platform.system()} {platform.machine()} ({platform.node()})",
        "compiler_info": compiler_info,
        "build_type": "Debug + Coverage",
        "overall_passed": total_failures == 0 and total_tests > 0,
        "total_tests": total_tests,
        "total_passed": total_passed,
        "total_failed": total_failures,
        "total_suites": len(test_suites),
        "total_time": total_time,
        "test_suites": test_suites,
        "has_coverage": has_coverage,
        "cov_summary": cov_summary,
        "cov_files": cov_files,
        "cov_functions": cov_functions,
        "cov_class": cov_class,
    }

    # Render HTML
    if HAVE_JINJA2:
        template = Template(HTML_TEMPLATE)
        html = template.render(**context)
    else:
        print("ERROR: Jinja2 is required. Install with: pip3 install jinja2", file=sys.stderr)
        sys.exit(1)

    # Write output
    os.makedirs(os.path.dirname(output_path) or ".", exist_ok=True)
    with open(output_path, "w", encoding="utf-8") as f:
        f.write(html)

    print(f"\n{'='*60}")
    print(f"  CANTATA-STYLE REPORT GENERATED")
    print(f"{'='*60}")
    print(f"  Output: {output_path}")
    print(f"  Tests:  {total_passed}/{total_tests} passed")
    if has_coverage:
        print(f"  Statement Coverage:  {cov_summary['line_pct']:.1f}%")
        print(f"  Branch Coverage:     {cov_summary['branch_pct']:.1f}%")
        print(f"  Function Coverage:   {cov_summary['function_pct']:.1f}%")
    print(f"  Verdict: {'PASS' if total_failures == 0 and total_tests > 0 else 'FAIL'}")
    print(f"{'='*60}\n")


# ═══════════════════════════════════════════════════════════════════════════
#  CLI Entry Point
# ═══════════════════════════════════════════════════════════════════════════

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Generate Cantata-style test & coverage report"
    )
    parser.add_argument(
        "--test-results-dir", default="build/test_results",
        help="Directory containing GTest XML result files"
    )
    parser.add_argument(
        "--coverage-json", default="build/coverage.json",
        help="gcovr JSON coverage output file"
    )
    parser.add_argument(
        "--output", default="build/report/SecOC_Test_Report.html",
        help="Output HTML report path"
    )
    args = parser.parse_args()

    generate_report(args.test_results_dir, args.coverage_json, args.output)
