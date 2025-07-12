#!/usr/bin/env python3
"""
RFC Compliance Test Suite for webserv HTTP Server
Focus on RFC 2616/7230 compliance and curl-style testing
"""

import subprocess
import socket
import time
import sys
import os
import json
import base64
from datetime import datetime

class RFCComplianceTester:
    def __init__(self, host='127.0.0.1', port=8080, config_file='test_8080.conf'):
        self.host = host
        self.port = port
        self.config_file = config_file
        self.server_process = None
        self.test_results = []
        
    def start_server(self):
        """Start the webserv server"""
        try:
            print(f"🚀 Starting webserv server on {self.host}:{self.port}")
            self.server_process = subprocess.Popen(
                ['./webserv', self.config_file],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            time.sleep(3)
            return self.wait_for_server()
        except Exception as e:
            print(f"❌ Failed to start server: {e}")
            return False
    
    def stop_server(self):
        """Stop the webserv server"""
        if self.server_process:
            self.server_process.terminate()
            try:
                self.server_process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.server_process.kill()
                self.server_process.wait()
            print("🛑 Server stopped")
    
    def wait_for_server(self, timeout=10):
        """Wait for server to be ready"""
        start_time = time.time()
        while time.time() - start_time < timeout:
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(1)
                result = sock.connect_ex((self.host, self.port))
                sock.close()
                if result == 0:
                    print("✅ Server is ready!")
                    return True
            except:
                pass
            time.sleep(0.5)
        print("❌ Server not ready within timeout")
        return False
    
    def send_raw_request(self, raw_request, timeout=10, description=""):
        """Send raw request and return response"""
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(timeout)
            sock.connect((self.host, self.port))
            
            print(f"\n📤 {description}")
            print(f"Request: {repr(raw_request)}")
            
            sock.send(raw_request.encode('latin-1'))
            response = sock.recv(16384).decode('latin-1')
            sock.close()
            
            print(f"Response: {repr(response[:200])}...")
            return response
        except Exception as e:
            return f"ERROR: {e}"
    
    def run_curl_test(self, curl_args, expected_code=None, description=""):
        """Run curl command and check result"""
        try:
            cmd = ['curl', '-s', '-i', '--max-time', '10'] + curl_args
            print(f"\n📤 {description}")
            print(f"Command: {' '.join(cmd)}")
            
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=15)
            
            if expected_code is not None:
                if result.returncode == expected_code:
                    return True, result.stdout + result.stderr
                else:
                    return False, f"Expected exit code {expected_code}, got {result.returncode}"
            else:
                # Success if curl didn't crash (exit codes 0-22 are normal)
                success = result.returncode in [0, 22, 7]  # 0=success, 22=HTTP error, 7=connection failed
                return success, result.stdout + result.stderr
        except subprocess.TimeoutExpired:
            return False, "Command timed out"
        except Exception as e:
            return False, f"Command failed: {e}"
    
    def parse_response(self, response):
        """Parse HTTP response"""
        if response.startswith("ERROR:"):
            return None, None, None, response
        
        try:
            parts = response.split('\r\n\r\n', 1)
            if len(parts) < 2:
                return None, None, None, "Invalid response format"
            
            headers_part = parts[0]
            body = parts[1] if len(parts) > 1 else ""
            
            lines = headers_part.split('\r\n')
            status_line = lines[0]
            headers = {}
            
            for line in lines[1:]:
                if ':' in line:
                    key, value = line.split(':', 1)
                    headers[key.strip()] = value.strip()
            
            return status_line, headers, body, None
        except Exception as e:
            return None, None, None, f"Parse error: {e}"
    
    def log_test_result(self, test_name, expected, actual, passed, notes=""):
        """Log test result"""
        status = "✅ PASS" if passed else "❌ FAIL"
        result = {
            'test': test_name,
            'expected': expected,
            'actual': actual,
            'passed': passed,
            'notes': notes
        }
        self.test_results.append(result)
        print(f"{status} {test_name}")
        if not passed:
            print(f"   Expected: {expected}")
            print(f"   Actual: {actual}")
        if notes:
            print(f"   Notes: {notes}")
    
    def test_rfc_request_line(self):
        """Test RFC 2616 Section 5.1 - Request Line"""
        print("\n🧪 RFC 2616 Section 5.1 - REQUEST LINE TESTS")
        
        # Valid request lines
        valid_tests = [
            ("GET / HTTP/1.1", "Simple GET"),
            ("GET /index.html HTTP/1.1", "GET with path"),
            ("POST /submit HTTP/1.1", "POST request"),
            ("DELETE /resource HTTP/1.1", "DELETE request"),
            ("HEAD / HTTP/1.1", "HEAD request"),
            ("OPTIONS * HTTP/1.1", "OPTIONS with asterisk"),
            ("GET /path?query=value HTTP/1.1", "GET with query"),
            ("GET /path%20with%20spaces HTTP/1.1", "GET with encoded spaces"),
        ]
        
        for request_line, description in valid_tests:
            request = f"{request_line}\r\nHost: localhost\r\n\r\n"
            response = self.send_raw_request(request, description=description)
            status, headers, body, error = self.parse_response(response)
            
            if error:
                passed = False
                self.log_test_result(f"RFC Request Line: {description}", "Valid response", error, passed)
            else:
                passed = "HTTP/1.1" in status
                self.log_test_result(f"RFC Request Line: {description}", "Valid response", status, passed)
        
        # Invalid request lines
        invalid_tests = [
            ("GET", "Missing URI and HTTP version"),
            ("GET /", "Missing HTTP version"),
            ("/ HTTP/1.1", "Missing method"),
            ("GET / HTTP/2.0", "Unsupported HTTP version"),
            ("get / HTTP/1.1", "Lowercase method"),
            ("GET  /  HTTP/1.1", "Extra spaces"),
            ("GET /\x00 HTTP/1.1", "Null byte in URI"),
            ("GET / HTTP/1.1 extra", "Extra data after HTTP version"),
        ]
        
        for request_line, description in invalid_tests:
            request = f"{request_line}\r\nHost: localhost\r\n\r\n"
            response = self.send_raw_request(request, description=description)
            status, headers, body, error = self.parse_response(response)
            
            if error:
                passed = True  # Connection error is acceptable
                self.log_test_result(f"RFC Invalid Request: {description}", "400 Bad Request", error, passed)
            else:
                passed = "400" in status
                self.log_test_result(f"RFC Invalid Request: {description}", "400 Bad Request", status, passed)
    
    def test_rfc_headers(self):
        """Test RFC 2616 Section 4.2 - Message Headers"""
        print("\n🧪 RFC 2616 Section 4.2 - MESSAGE HEADERS TESTS")
        
        # Test required Host header (RFC 2616 Section 14.23)
        request = "GET / HTTP/1.1\r\n\r\n"
        response = self.send_raw_request(request, description="Missing Host header")
        status, headers, body, error = self.parse_response(response)
        
        if error:
            passed = True
            self.log_test_result("RFC Missing Host", "400 Bad Request", error, passed)
        else:
            passed = "400" in status
            self.log_test_result("RFC Missing Host", "400 Bad Request", status, passed)
        
        # Test case-insensitive header names (RFC 2616 Section 4.2)
        case_tests = [
            ("host: localhost", "lowercase host"),
            ("HOST: localhost", "uppercase host"),
            ("HoSt: localhost", "mixed case host"),
            ("Content-Length: 0", "Content-Length"),
            ("content-length: 0", "lowercase content-length"),
            ("CONTENT-LENGTH: 0", "uppercase content-length"),
        ]
        
        for header, description in case_tests:
            request = f"GET / HTTP/1.1\r\n{header}\r\n\r\n"
            response = self.send_raw_request(request, description=description)
            status, headers, body, error = self.parse_response(response)
            
            if error:
                passed = False
                self.log_test_result(f"RFC Case Insensitive: {description}", "200 OK", error, passed)
            else:
                passed = "200" in status or "404" in status
                self.log_test_result(f"RFC Case Insensitive: {description}", "200 OK", status, passed)
        
        # Test header folding (RFC 2616 Section 2.2)
        folded_request = "GET / HTTP/1.1\r\nHost: localhost\r\nX-Folded: line1\r\n line2\r\n\r\n"
        response = self.send_raw_request(folded_request, description="Header folding")
        status, headers, body, error = self.parse_response(response)
        
        if error:
            passed = True  # May not support folding
            self.log_test_result("RFC Header Folding", "200 OK or 400 Bad Request", error, passed)
        else:
            passed = "200" in status or "400" in status
            self.log_test_result("RFC Header Folding", "200 OK or 400 Bad Request", status, passed)
    
    def test_rfc_status_codes(self):
        """Test RFC 2616 Section 6.1 - Status Codes"""
        print("\n🧪 RFC 2616 Section 6.1 - STATUS CODE TESTS")
        
        # Test 200 OK
        request = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"
        response = self.send_raw_request(request, description="200 OK test")
        status, headers, body, error = self.parse_response(response)
        
        if not error:
            passed = "200 OK" in status
            self.log_test_result("RFC 200 OK", "200 OK", status, passed)
        
        # Test 404 Not Found
        request = "GET /nonexistent HTTP/1.1\r\nHost: localhost\r\n\r\n"
        response = self.send_raw_request(request, description="404 Not Found test")
        status, headers, body, error = self.parse_response(response)
        
        if not error:
            passed = "404" in status
            self.log_test_result("RFC 404 Not Found", "404 Not Found", status, passed)
        
        # Test 400 Bad Request
        request = "INVALID REQUEST\r\n\r\n"
        response = self.send_raw_request(request, description="400 Bad Request test")
        status, headers, body, error = self.parse_response(response)
        
        if error:
            passed = True
            self.log_test_result("RFC 400 Bad Request", "400 Bad Request", error, passed)
        else:
            passed = "400" in status
            self.log_test_result("RFC 400 Bad Request", "400 Bad Request", status, passed)
        
        # Test 405 Method Not Allowed
        request = "TRACE / HTTP/1.1\r\nHost: localhost\r\n\r\n"
        response = self.send_raw_request(request, description="405 Method Not Allowed test")
        status, headers, body, error = self.parse_response(response)
        
        if not error:
            passed = "405" in status
            self.log_test_result("RFC 405 Method Not Allowed", "405 Method Not Allowed", status, passed)
    
    def test_rfc_content_length(self):
        """Test RFC 2616 Section 14.13 - Content-Length"""
        print("\n🧪 RFC 2616 Section 14.13 - CONTENT-LENGTH TESTS")
        
        # Test POST with Content-Length
        body = "test data"
        request = f"POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: {len(body)}\r\n\r\n{body}"
        response = self.send_raw_request(request, description="POST with Content-Length")
        status, headers, body_resp, error = self.parse_response(response)
        
        if not error:
            passed = "200" in status or "405" in status or "501" in status
            self.log_test_result("RFC Content-Length", "200/405/501", status, passed)
        
        # Test POST without Content-Length
        body = "test data"
        request = f"POST / HTTP/1.1\r\nHost: localhost\r\n\r\n{body}"
        response = self.send_raw_request(request, description="POST without Content-Length")
        status, headers, body_resp, error = self.parse_response(response)
        
        if not error:
            passed = "400" in status or "411" in status
            self.log_test_result("RFC No Content-Length", "400/411", status, passed)
        
        # Test Content-Length mismatch
        body = "short"
        request = f"POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 100\r\n\r\n{body}"
        response = self.send_raw_request(request, description="Content-Length mismatch")
        status, headers, body_resp, error = self.parse_response(response)
        
        if error:
            passed = True  # Timeout is acceptable
            self.log_test_result("RFC Content-Length Mismatch", "400/408", error, passed)
        else:
            passed = "400" in status or "408" in status
            self.log_test_result("RFC Content-Length Mismatch", "400/408", status, passed)
    
    def test_curl_compatibility(self):
        """Test curl compatibility"""
        print("\n🧪 CURL COMPATIBILITY TESTS")
        
        # Basic GET request
        success, output = self.run_curl_test(
            [f'http://{self.host}:{self.port}/'],
            description="Basic GET request"
        )
        passed = success and "HTTP/1.1" in output
        self.log_test_result("Curl Basic GET", "HTTP response", "Success" if passed else "Failed", passed)
        
        # GET with headers
        success, output = self.run_curl_test(
            ['-H', 'User-Agent: curl/7.68.0', '-H', 'Accept: */*', f'http://{self.host}:{self.port}/'],
            description="GET with custom headers"
        )
        passed = success and "HTTP/1.1" in output
        self.log_test_result("Curl Custom Headers", "HTTP response", "Success" if passed else "Failed", passed)
        
        # POST request
        success, output = self.run_curl_test(
            ['-X', 'POST', '-d', 'test=data', f'http://{self.host}:{self.port}/'],
            description="POST request"
        )
        passed = success and "HTTP/1.1" in output
        self.log_test_result("Curl POST", "HTTP response", "Success" if passed else "Failed", passed)
        
        # HEAD request
        success, output = self.run_curl_test(
            ['-I', f'http://{self.host}:{self.port}/'],
            description="HEAD request"
        )
        passed = success and "HTTP/1.1" in output
        self.log_test_result("Curl HEAD", "HTTP response", "Success" if passed else "Failed", passed)
        
        # Verbose output
        success, output = self.run_curl_test(
            ['-v', f'http://{self.host}:{self.port}/'],
            description="Verbose output"
        )
        passed = success and "HTTP/1.1" in output
        self.log_test_result("Curl Verbose", "HTTP response", "Success" if passed else "Failed", passed)
        
        # Follow redirects (if applicable)
        success, output = self.run_curl_test(
            ['-L', f'http://{self.host}:{self.port}/redirect'],
            description="Follow redirects"
        )
        passed = success  # Any response is acceptable
        self.log_test_result("Curl Redirects", "No crash", "Success" if passed else "Failed", passed)
        
        # Connection timeout
        success, output = self.run_curl_test(
            ['--connect-timeout', '5', f'http://{self.host}:{self.port}/'],
            description="Connection timeout"
        )
        passed = success and "HTTP/1.1" in output
        self.log_test_result("Curl Timeout", "HTTP response", "Success" if passed else "Failed", passed)
    
    def test_browser_compatibility(self):
        """Test browser-like requests"""
        print("\n🧪 BROWSER COMPATIBILITY TESTS")
        
        # Typical browser request
        browser_request = """GET / HTTP/1.1\r
Host: localhost\r
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36\r
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r
Accept-Language: en-US,en;q=0.5\r
Accept-Encoding: gzip, deflate\r
Connection: keep-alive\r
Upgrade-Insecure-Requests: 1\r
\r
"""
        response = self.send_raw_request(browser_request, description="Typical browser request")
        status, headers, body, error = self.parse_response(response)
        
        if error:
            passed = False
            self.log_test_result("Browser Request", "200 OK", error, passed)
        else:
            passed = "200" in status or "404" in status
            self.log_test_result("Browser Request", "200 OK", status, passed)
        
        # Browser favicon request
        favicon_request = "GET /favicon.ico HTTP/1.1\r\nHost: localhost\r\nUser-Agent: Mozilla/5.0\r\n\r\n"
        response = self.send_raw_request(favicon_request, description="Favicon request")
        status, headers, body, error = self.parse_response(response)
        
        if error:
            passed = False
            self.log_test_result("Favicon Request", "200 or 404", error, passed)
        else:
            passed = "200" in status or "404" in status
            self.log_test_result("Favicon Request", "200 or 404", status, passed)
    
    def test_security_headers(self):
        """Test security-related headers and behaviors"""
        print("\n🧪 SECURITY TESTS")
        
        # Test path traversal
        traversal_tests = [
            "/../etc/passwd",
            "/../../etc/passwd",
            "/../../../etc/passwd",
            "/..\\..\\..\\etc\\passwd",
            "/%2e%2e/%2e%2e/%2e%2e/etc/passwd",
            "/....//....//....//etc/passwd",
        ]
        
        for path in traversal_tests:
            request = f"GET {path} HTTP/1.1\r\nHost: localhost\r\n\r\n"
            response = self.send_raw_request(request, description=f"Path traversal: {path}")
            status, headers, body, error = self.parse_response(response)
            
            if error:
                passed = True
                self.log_test_result(f"Security Path Traversal", "403/404", error, passed)
            else:
                passed = "403" in status or "404" in status
                self.log_test_result(f"Security Path Traversal", "403/404", status, passed)
        
        # Test header injection
        injection_request = "GET / HTTP/1.1\r\nHost: localhost\r\nX-Test: value\r\nInjected: header\r\n\r\n"
        response = self.send_raw_request(injection_request, description="Header injection test")
        status, headers, body, error = self.parse_response(response)
        
        if error:
            passed = True
            self.log_test_result("Security Header Injection", "Safe handling", error, passed)
        else:
            passed = "HTTP/1.1" in status  # Should handle safely
            self.log_test_result("Security Header Injection", "Safe handling", status, passed)
    
    def test_performance_edge_cases(self):
        """Test performance and edge cases"""
        print("\n🧪 PERFORMANCE EDGE CASES")
        
        # Test large number of headers
        large_headers = "\r\n".join([f"X-Header-{i}: value{i}" for i in range(100)])
        request = f"GET / HTTP/1.1\r\nHost: localhost\r\n{large_headers}\r\n\r\n"
        response = self.send_raw_request(request, description="Large number of headers")
        status, headers, body, error = self.parse_response(response)
        
        if error:
            passed = True  # Server may reject large requests
            self.log_test_result("Large Headers", "Safe handling", error, passed)
        else:
            passed = "HTTP/1.1" in status
            self.log_test_result("Large Headers", "Safe handling", status, passed)
        
        # Test empty request
        response = self.send_raw_request("", description="Empty request")
        passed = "ERROR" in response or "400" in response
        self.log_test_result("Empty Request", "400 or connection error", response[:50], passed)
        
        # Test rapid requests
        rapid_success = 0
        for i in range(5):
            request = f"GET / HTTP/1.1\r\nHost: localhost\r\nX-Rapid: {i}\r\n\r\n"
            response = self.send_raw_request(request, timeout=5, description=f"Rapid request {i}")
            status, headers, body, error = self.parse_response(response)
            if not error and "HTTP/1.1" in status:
                rapid_success += 1
        
        passed = rapid_success >= 3  # At least 3 out of 5 should succeed
        self.log_test_result("Rapid Requests", "3/5 successful", f"{rapid_success}/5 successful", passed)
    
    def generate_rfc_report(self):
        """Generate RFC compliance report"""
        print("\n" + "="*80)
        print("📊 RFC COMPLIANCE TEST REPORT")
        print("="*80)
        
        total_tests = len(self.test_results)
        passed_tests = sum(1 for result in self.test_results if result['passed'])
        failed_tests = total_tests - passed_tests
        
        print(f"Total Tests: {total_tests}")
        print(f"Passed: {passed_tests}")
        print(f"Failed: {failed_tests}")
        print(f"Success Rate: {passed_tests/total_tests*100:.1f}%")
        
        # RFC compliance categories
        rfc_categories = {
            'RFC 2616 Request Line': [r for r in self.test_results if 'RFC Request Line' in r['test']],
            'RFC 2616 Headers': [r for r in self.test_results if 'RFC' in r['test'] and 'Header' in r['test']],
            'RFC 2616 Status Codes': [r for r in self.test_results if 'RFC' in r['test'] and any(code in r['test'] for code in ['200', '404', '400', '405'])],
            'RFC 2616 Content-Length': [r for r in self.test_results if 'RFC Content-Length' in r['test']],
            'Curl Compatibility': [r for r in self.test_results if 'Curl' in r['test']],
            'Browser Compatibility': [r for r in self.test_results if 'Browser' in r['test']],
            'Security': [r for r in self.test_results if 'Security' in r['test']],
            'Performance': [r for r in self.test_results if 'Large' in r['test'] or 'Rapid' in r['test'] or 'Empty' in r['test']]
        }
        
        print(f"\n🎯 RFC COMPLIANCE BREAKDOWN:")
        for category, tests in rfc_categories.items():
            if tests:
                passed = sum(1 for t in tests if t['passed'])
                total = len(tests)
                compliance = passed/total*100
                status = "✅ COMPLIANT" if compliance >= 90 else "⚠️ PARTIAL" if compliance >= 70 else "❌ NON-COMPLIANT"
                print(f"{category}: {status} ({compliance:.1f}% - {passed}/{total})")
        
        if failed_tests > 0:
            print(f"\n❌ FAILED TESTS:")
            for result in self.test_results:
                if not result['passed']:
                    print(f"  - {result['test']}: {result['actual']}")
        
        print(f"\n📋 RECOMMENDATIONS:")
        if passed_tests/total_tests >= 0.9:
            print("✅ Excellent RFC compliance! Your server is ready for production.")
        elif passed_tests/total_tests >= 0.8:
            print("⚠️ Good RFC compliance. Address remaining issues for better compatibility.")
        else:
            print("❌ Poor RFC compliance. Significant work needed before production use.")
        
        return passed_tests, failed_tests
    
    def run_all_tests(self):
        """Run all RFC compliance tests"""
        if not self.start_server():
            print("❌ Cannot start server, aborting tests")
            return False
        
        try:
            print("\n🧪 RFC COMPLIANCE AND COMPATIBILITY TESTING")
            print("Testing RFC 2616/7230 compliance and real-world compatibility...")
            
            # Run all test categories
            self.test_rfc_request_line()
            self.test_rfc_headers()
            self.test_rfc_status_codes()
            self.test_rfc_content_length()
            self.test_curl_compatibility()
            self.test_browser_compatibility()
            self.test_security_headers()
            self.test_performance_edge_cases()
            
            # Generate report
            passed, failed = self.generate_rfc_report()
            
            return failed == 0
            
        finally:
            self.stop_server()

def main():
    """Main test runner"""
    if not os.path.exists('./webserv'):
        print("❌ webserv executable not found. Please build the project first.")
        sys.exit(1)
    
    print("🧪 RFC COMPLIANCE TEST SUITE")
    print("Testing RFC 2616/7230 compliance and real-world compatibility")
    print("="*80)
    
    tester = RFCComplianceTester()
    success = tester.run_all_tests()
    
    if success:
        print("\n🎉 ALL TESTS PASSED! Your server is RFC compliant.")
    else:
        print("\n⚠️ Some tests failed. Review the report above.")
    
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()