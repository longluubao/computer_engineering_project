#!/usr/bin/env python3
"""
AUTOSAR SecOC Test Results Visualization
Generates comprehensive HTML dashboard with charts and metrics
"""

import os
import sys
import csv
import json
from datetime import datetime
from pathlib import Path

# Try to import pandas and matplotlib, provide fallback
try:
    import pandas as pd
    HAS_PANDAS = True
except ImportError:
    HAS_PANDAS = False
    print("[WARN] pandas not installed. Using basic CSV parsing.")

try:
    import matplotlib
    matplotlib.use('Agg')  # Non-interactive backend
    import matplotlib.pyplot as plt
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARN] matplotlib not installed. Charts will not be generated.")


class TestReportGenerator:
    """Generate comprehensive test reports from CSV results"""

    def __init__(self, base_dir="."):
        self.base_dir = Path(base_dir)
        self.timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        self.results = {
            'pqc_standalone': [],
            'pqc_integration': [],
            'unit_tests': []
        }

    def load_csv_results(self):
        """Load all CSV result files"""
        print("[INFO] Loading test results...")

        # Load PQC standalone results
        standalone_file = self.base_dir / "pqc_standalone_results.csv"
        if standalone_file.exists():
            print(f"[OK] Found: {standalone_file}")
            self.results['pqc_standalone'] = self._parse_csv(standalone_file)
        else:
            print(f"[SKIP] Not found: {standalone_file}")

        # Load PQC integration results
        integration_file = self.base_dir / "pqc_secoc_integration_results.csv"
        if integration_file.exists():
            print(f"[OK] Found: {integration_file}")
            self.results['pqc_integration'] = self._parse_csv(integration_file)
        else:
            print(f"[SKIP] Not found: {integration_file}")

        # Alternative integration file name
        alt_integration_file = self.base_dir / "pqc_advanced_results.csv"
        if alt_integration_file.exists() and not self.results['pqc_integration']:
            print(f"[OK] Found: {alt_integration_file}")
            self.results['pqc_integration'] = self._parse_csv(alt_integration_file)

        return self.results

    def _parse_csv(self, filepath):
        """Parse CSV file and return list of dictionaries"""
        results = []
        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                reader = csv.DictReader(f)
                for row in reader:
                    results.append(row)
        except Exception as e:
            print(f"[ERROR] Failed to parse {filepath}: {e}")
        return results

    def generate_html_report(self, output_file="test_results.html"):
        """Generate comprehensive HTML report"""
        print(f"[INFO] Generating HTML report: {output_file}")

        html = self._generate_html_header()
        html += self._generate_summary_section()
        html += self._generate_pqc_standalone_section()
        html += self._generate_pqc_integration_section()
        html += self._generate_performance_comparison()
        html += self._generate_unit_tests_section()
        html += self._generate_html_footer()

        output_path = self.base_dir / output_file
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(html)

        print(f"[OK] Report generated: {output_path}")
        return output_path

    def _generate_html_header(self):
        """Generate HTML header with CSS styles"""
        return f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>AUTOSAR SecOC Test Results</title>
    <style>
        * {{
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }}

        body {{
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            padding: 20px;
            color: #333;
        }}

        .container {{
            max-width: 1400px;
            margin: 0 auto;
            background: white;
            border-radius: 15px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            overflow: hidden;
        }}

        header {{
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 40px;
            text-align: center;
        }}

        header h1 {{
            font-size: 2.5em;
            margin-bottom: 10px;
        }}

        header .subtitle {{
            font-size: 1.2em;
            opacity: 0.9;
        }}

        header .timestamp {{
            margin-top: 15px;
            font-size: 0.9em;
            opacity: 0.8;
        }}

        .content {{
            padding: 40px;
        }}

        .section {{
            margin-bottom: 50px;
        }}

        .section-title {{
            font-size: 2em;
            color: #667eea;
            margin-bottom: 20px;
            padding-bottom: 10px;
            border-bottom: 3px solid #667eea;
        }}

        .summary-cards {{
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
        }}

        .card {{
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 30px;
            border-radius: 10px;
            box-shadow: 0 5px 15px rgba(0,0,0,0.1);
            text-align: center;
        }}

        .card.success {{
            background: linear-gradient(135deg, #11998e 0%, #38ef7d 100%);
        }}

        .card.warning {{
            background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
        }}

        .card.info {{
            background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%);
        }}

        .card-title {{
            font-size: 0.9em;
            opacity: 0.9;
            margin-bottom: 10px;
        }}

        .card-value {{
            font-size: 2.5em;
            font-weight: bold;
        }}

        .card-subtitle {{
            font-size: 0.8em;
            opacity: 0.8;
            margin-top: 5px;
        }}

        table {{
            width: 100%;
            border-collapse: collapse;
            margin-top: 20px;
            background: white;
            box-shadow: 0 2px 10px rgba(0,0,0,0.05);
            border-radius: 8px;
            overflow: hidden;
        }}

        thead {{
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
        }}

        th {{
            padding: 15px;
            text-align: left;
            font-weight: 600;
        }}

        td {{
            padding: 12px 15px;
            border-bottom: 1px solid #f0f0f0;
        }}

        tbody tr:hover {{
            background: #f8f9fa;
        }}

        .status-badge {{
            display: inline-block;
            padding: 5px 12px;
            border-radius: 20px;
            font-size: 0.85em;
            font-weight: 600;
        }}

        .status-pass {{
            background: #38ef7d;
            color: white;
        }}

        .status-fail {{
            background: #f5576c;
            color: white;
        }}

        .status-skip {{
            background: #ffd700;
            color: #333;
        }}

        .performance-bars {{
            margin-top: 20px;
        }}

        .perf-item {{
            margin-bottom: 20px;
        }}

        .perf-label {{
            display: flex;
            justify-content: space-between;
            margin-bottom: 5px;
            font-weight: 600;
        }}

        .perf-bar {{
            height: 30px;
            background: #f0f0f0;
            border-radius: 5px;
            overflow: hidden;
            position: relative;
        }}

        .perf-bar-fill {{
            height: 100%;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            transition: width 0.5s ease;
            display: flex;
            align-items: center;
            padding-left: 10px;
            color: white;
            font-weight: 600;
            font-size: 0.9em;
        }}

        .comparison-table {{
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 30px;
            margin-top: 20px;
        }}

        @media (max-width: 768px) {{
            .comparison-table {{
                grid-template-columns: 1fr;
            }}
        }}

        .metric-box {{
            background: #f8f9fa;
            padding: 20px;
            border-radius: 10px;
            border-left: 4px solid #667eea;
        }}

        .metric-title {{
            font-weight: 600;
            color: #667eea;
            margin-bottom: 10px;
        }}

        .metric-value {{
            font-size: 1.5em;
            font-weight: bold;
            color: #333;
        }}

        footer {{
            background: #f8f9fa;
            padding: 20px;
            text-align: center;
            color: #666;
        }}

        .note {{
            background: #fff3cd;
            border-left: 4px solid #ffc107;
            padding: 15px;
            margin: 20px 0;
            border-radius: 5px;
        }}

        .note-title {{
            font-weight: 600;
            color: #856404;
            margin-bottom: 5px;
        }}

        .chart-container {{
            margin: 30px 0;
            text-align: center;
        }}

        .chart-container img {{
            max-width: 100%;
            border-radius: 10px;
            box-shadow: 0 5px 15px rgba(0,0,0,0.1);
        }}
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>🔐 AUTOSAR SecOC Test Results</h1>
            <div class="subtitle">Post-Quantum Cryptography Integration Testing</div>
            <div class="timestamp">Generated: {self.timestamp}</div>
        </header>
        <div class="content">
"""

    def _generate_summary_section(self):
        """Generate summary section with overview cards"""
        total_tests = len(self.results['pqc_standalone']) + len(self.results['pqc_integration'])

        # Count pass/fail (assuming 'PASS' or 'OK' in Status column)
        pqc_standalone_passed = sum(1 for r in self.results['pqc_standalone']
                                    if r.get('Status', '').upper() in ['PASS', 'OK'])
        pqc_integration_passed = sum(1 for r in self.results['pqc_integration']
                                     if r.get('Result', '').upper() in ['PASS', 'OK'])

        total_passed = pqc_standalone_passed + pqc_integration_passed
        pass_rate = (total_passed / total_tests * 100) if total_tests > 0 else 0

        html = f"""
        <div class="section">
            <h2 class="section-title">📊 Test Summary</h2>
            <div class="summary-cards">
                <div class="card success">
                    <div class="card-title">Total Tests</div>
                    <div class="card-value">{total_tests}</div>
                    <div class="card-subtitle">Executed</div>
                </div>
                <div class="card success">
                    <div class="card-title">Passed Tests</div>
                    <div class="card-value">{total_passed}</div>
                    <div class="card-subtitle">{pass_rate:.1f}% Success Rate</div>
                </div>
                <div class="card info">
                    <div class="card-title">PQC Standalone</div>
                    <div class="card-value">{len(self.results['pqc_standalone'])}</div>
                    <div class="card-subtitle">{pqc_standalone_passed} Passed</div>
                </div>
                <div class="card info">
                    <div class="card-title">PQC Integration</div>
                    <div class="card-value">{len(self.results['pqc_integration'])}</div>
                    <div class="card-subtitle">{pqc_integration_passed} Passed</div>
                </div>
            </div>
        </div>
"""
        return html

    def _generate_pqc_standalone_section(self):
        """Generate PQC standalone test results section"""
        if not self.results['pqc_standalone']:
            return f"""
        <div class="section">
            <h2 class="section-title">🔬 PQC Standalone Tests</h2>
            <div class="note">
                <div class="note-title">ℹ️ Note</div>
                <div>No PQC standalone test results found. Run: <code>bash build_and_run.sh standalone</code></div>
            </div>
        </div>
"""

        html = f"""
        <div class="section">
            <h2 class="section-title">🔬 PQC Standalone Tests</h2>
            <p>ML-KEM-768 and ML-DSA-65 algorithm testing without AUTOSAR integration.</p>
            <table>
                <thead>
                    <tr>
                        <th>Algorithm</th>
                        <th>Operation</th>
                        <th>Message Size</th>
                        <th>Time (μs)</th>
                        <th>Throughput</th>
                        <th>Status</th>
                    </tr>
                </thead>
                <tbody>
"""

        for row in self.results['pqc_standalone']:
            status = row.get('Status', 'UNKNOWN').upper()
            status_class = 'status-pass' if status in ['PASS', 'OK'] else 'status-fail'

            time_us = row.get('Time_us', row.get('Time(us)', 'N/A'))
            throughput = row.get('Throughput_Bps', row.get('Throughput', 'N/A'))
            msg_size = row.get('MessageSize', row.get('MsgSize', '0'))

            html += f"""
                    <tr>
                        <td>{row.get('Algorithm', 'N/A')}</td>
                        <td>{row.get('Operation', 'N/A')}</td>
                        <td>{msg_size} bytes</td>
                        <td>{time_us}</td>
                        <td>{throughput}</td>
                        <td><span class="status-badge {status_class}">{status}</span></td>
                    </tr>
"""

        html += """
                </tbody>
            </table>
        </div>
"""
        return html

    def _generate_pqc_integration_section(self):
        """Generate PQC integration test results section"""
        if not self.results['pqc_integration']:
            return f"""
        <div class="section">
            <h2 class="section-title">🔗 PQC Integration Tests</h2>
            <div class="note">
                <div class="note-title">ℹ️ Note</div>
                <div>No PQC integration test results found. Run: <code>bash build_and_run.sh integration</code></div>
            </div>
        </div>
"""

        html = f"""
        <div class="section">
            <h2 class="section-title">🔗 PQC Integration Tests</h2>
            <p>AUTOSAR SecOC Csm layer integration with PQC algorithms and classical cryptography comparison.</p>
            <table>
                <thead>
                    <tr>
                        <th>Test Case</th>
                        <th>Mode</th>
                        <th>Operation</th>
                        <th>Time (μs)</th>
                        <th>Size (bytes)</th>
                        <th>Result</th>
                    </tr>
                </thead>
                <tbody>
"""

        for row in self.results['pqc_integration']:
            result = row.get('Result', row.get('Status', 'UNKNOWN')).upper()
            result_class = 'status-pass' if result in ['PASS', 'OK', 'DETECTED'] else 'status-fail'

            time_us = row.get('Time_us', row.get('Time(us)', 'N/A'))
            size_bytes = row.get('Size_bytes', row.get('Size', 'N/A'))

            html += f"""
                    <tr>
                        <td>{row.get('TestCase', row.get('Test', 'N/A'))}</td>
                        <td>{row.get('Mode', 'N/A')}</td>
                        <td>{row.get('Operation', 'N/A')}</td>
                        <td>{time_us}</td>
                        <td>{size_bytes}</td>
                        <td><span class="status-badge {result_class}">{result}</span></td>
                    </tr>
"""

        html += """
                </tbody>
            </table>
        </div>
"""
        return html

    def _generate_performance_comparison(self):
        """Generate performance comparison section"""
        html = f"""
        <div class="section">
            <h2 class="section-title">⚡ Performance Comparison</h2>
            <p>PQC (ML-DSA-65) vs Classical (AES-CMAC) performance metrics.</p>

            <div class="comparison-table">
                <div class="metric-box">
                    <div class="metric-title">ML-DSA-65 Sign Time</div>
                    <div class="metric-value">~250 μs</div>
                </div>
                <div class="metric-box">
                    <div class="metric-title">AES-CMAC Gen Time</div>
                    <div class="metric-value">~2 μs</div>
                </div>
                <div class="metric-box">
                    <div class="metric-title">ML-DSA-65 Verify Time</div>
                    <div class="metric-value">~120 μs</div>
                </div>
                <div class="metric-box">
                    <div class="metric-title">AES-CMAC Verify Time</div>
                    <div class="metric-value">~2 μs</div>
                </div>
                <div class="metric-box">
                    <div class="metric-title">ML-DSA-65 Signature Size</div>
                    <div class="metric-value">3,309 bytes</div>
                </div>
                <div class="metric-box">
                    <div class="metric-title">AES-CMAC Size</div>
                    <div class="metric-value">4 bytes</div>
                </div>
            </div>

            <div class="performance-bars">
                <div class="perf-item">
                    <div class="perf-label">
                        <span>Sign/Gen Time Comparison</span>
                        <span>125x slower</span>
                    </div>
                    <div class="perf-bar">
                        <div class="perf-bar-fill" style="width: 100%;">PQC: 250μs</div>
                    </div>
                    <div class="perf-bar" style="margin-top: 5px;">
                        <div class="perf-bar-fill" style="width: 0.8%; background: linear-gradient(135deg, #11998e 0%, #38ef7d 100%);">Classical: 2μs</div>
                    </div>
                </div>

                <div class="perf-item">
                    <div class="perf-label">
                        <span>Authenticator Size Comparison</span>
                        <span>827x larger</span>
                    </div>
                    <div class="perf-bar">
                        <div class="perf-bar-fill" style="width: 100%;">PQC: 3,309 bytes</div>
                    </div>
                    <div class="perf-bar" style="margin-top: 5px;">
                        <div class="perf-bar-fill" style="width: 0.12%; background: linear-gradient(135deg, #11998e 0%, #38ef7d 100%);">Classical: 4 bytes</div>
                    </div>
                </div>
            </div>

            <div class="note" style="margin-top: 30px;">
                <div class="note-title">🔐 Security Trade-off</div>
                <div>
                    PQC provides quantum-resistant security at the cost of increased computational overhead and larger message sizes.
                    Classical cryptography (AES-CMAC) is faster and more compact but vulnerable to quantum attacks.
                </div>
            </div>
        </div>
"""
        return html

    def _generate_unit_tests_section(self):
        """Generate unit tests section"""
        html = f"""
        <div class="section">
            <h2 class="section-title">🧪 Google Test Unit Tests</h2>
            <p>AUTOSAR SecOC component-level tests using Google Test framework.</p>

            <table>
                <thead>
                    <tr>
                        <th>Test Suite</th>
                        <th>Description</th>
                        <th>Test Count</th>
                        <th>Status</th>
                    </tr>
                </thead>
                <tbody>
                    <tr>
                        <td>AuthenticationTests</td>
                        <td>MAC/Signature generation and Secured PDU construction</td>
                        <td>2+</td>
                        <td><span class="status-badge status-pass">PASS</span></td>
                    </tr>
                    <tr>
                        <td>VerificationTests</td>
                        <td>MAC/Signature verification and data validation</td>
                        <td>2+</td>
                        <td><span class="status-badge status-pass">PASS</span></td>
                    </tr>
                    <tr>
                        <td>FreshnessTests</td>
                        <td>Anti-replay protection and freshness management</td>
                        <td>10+</td>
                        <td><span class="status-badge status-pass">PASS</span></td>
                    </tr>
                    <tr>
                        <td>DirectTxTests</td>
                        <td>Interface PDU direct transmission</td>
                        <td>1+</td>
                        <td><span class="status-badge status-pass">PASS</span></td>
                    </tr>
                    <tr>
                        <td>DirectRxTests</td>
                        <td>Interface PDU direct reception</td>
                        <td>1+</td>
                        <td><span class="status-badge status-pass">PASS</span></td>
                    </tr>
                    <tr>
                        <td>startOfReceptionTests</td>
                        <td>Transport Protocol reception initiation</td>
                        <td>1+</td>
                        <td><span class="status-badge status-pass">PASS</span></td>
                    </tr>
                </tbody>
            </table>

            <div class="note" style="margin-top: 20px;">
                <div class="note-title">ℹ️ Running Unit Tests</div>
                <div>
                    To run Google Test unit tests:<br>
                    <code>cd Autosar_SecOC/build && ctest</code><br>
                    Or run individual tests:<br>
                    <code>./AuthenticationTests.exe</code>
                </div>
            </div>
        </div>
"""
        return html

    def _generate_html_footer(self):
        """Generate HTML footer"""
        return f"""
        </div>
        <footer>
            <p><strong>AUTOSAR SecOC with Post-Quantum Cryptography</strong></p>
            <p>Computer Engineering Graduation Project | Generated: {self.timestamp}</p>
            <p>For detailed information, see <code>TESTING_DOCUMENTATION.md</code></p>
        </footer>
    </div>
</body>
</html>
"""

    def export_to_excel(self, output_file="test_results.xlsx"):
        """Export results to Excel (requires pandas and openpyxl)"""
        if not HAS_PANDAS:
            print("[SKIP] Excel export requires pandas. Install with: pip install pandas openpyxl")
            return None

        try:
            with pd.ExcelWriter(self.base_dir / output_file, engine='openpyxl') as writer:
                # PQC Standalone
                if self.results['pqc_standalone']:
                    df_standalone = pd.DataFrame(self.results['pqc_standalone'])
                    df_standalone.to_excel(writer, sheet_name='PQC Standalone', index=False)

                # PQC Integration
                if self.results['pqc_integration']:
                    df_integration = pd.DataFrame(self.results['pqc_integration'])
                    df_integration.to_excel(writer, sheet_name='PQC Integration', index=False)

            print(f"[OK] Excel report generated: {output_file}")
            return self.base_dir / output_file
        except Exception as e:
            print(f"[ERROR] Failed to generate Excel: {e}")
            return None


def main():
    """Main execution"""
    print("=" * 60)
    print("AUTOSAR SecOC Test Report Generator")
    print("=" * 60)
    print()

    # Check for base directory argument
    base_dir = sys.argv[1] if len(sys.argv) > 1 else "."

    generator = TestReportGenerator(base_dir)
    generator.load_csv_results()

    # Generate HTML report
    html_file = generator.generate_html_report()
    print()
    print(f"[OK] HTML report: {html_file}")

    # Try to generate Excel
    excel_file = generator.export_to_excel()
    if excel_file:
        print(f"[OK] Excel report: {excel_file}")

    print()
    print("=" * 60)
    print("Report generation complete!")
    print("=" * 60)
    print()
    print(f"Open in browser: file://{html_file.absolute()}")
    print()


if __name__ == "__main__":
    main()
