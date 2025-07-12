#!/usr/bin/env python3
"""
Advanced Webserv Test Suite - Comprehensive HTTP Server Testing
Tests HTTP compliance, edge cases, security, performance, and robustness
"""

import subprocess
import socket
import time
import sys
import os
import urllib.parse
import threading
import json
import signal
import random
import string
from datetime import datetime
from concurrent.futures import ThreadPoolExecutor

class AdvancedWebservTester:
    def __init__(self, host='127.0.0.1', port=8080, config_file='test_8080.conf'):
        self.host = host
        self.port = port
        self.config_file = config_file
        self.server_process = None
        self.test_results = []
        self.start_time = None
        self.total_tests = 0
        self.passed_tests = 0
        
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
            time.sleep(2)
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
            
            if isinstance(raw_request, str):
                raw_request = raw_request.encode('latin-1')
            
            sock.send(raw_request)
            response = sock.recv(16384)
            sock.close()
            
            return response.decode('latin-1', errors='ignore')
        except Exception as e:
            return f"ERROR: {e}"
    
    def send_partial_request(self, parts, delays, timeout=10):
        """Send request in parts with delays"""
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(timeout)
            sock.connect((self.host, self.port))
            
            for i, part in enumerate(parts):
                if isinstance(part, str):
                    part = part.encode('latin-1')
                sock.send(part)
                if i < len(delays):
                    time.sleep(delays[i])
            
            response = sock.recv(16384)
            sock.close()
            
            return response.decode('latin-1', errors='ignore')
        except Exception as e:
            return f"ERROR: {e}"
    
    def parse_response(self, response):
        """Parse HTTP response"""
        if response.startswith("ERROR:"):
            return None, None, None, response
        
        try:
            if '\r\n\r\n' in response:
                headers_part, body = response.split('\r\n\r\n', 1)
            else:
                headers_part, body = response, ""
            
            lines = headers_part.split('\r\n')
            status_line = lines[0] if lines else ""
            headers = {}
            
            for line in lines[1:]:
                if ':' in line:
                    key, value = line.split(':', 1)
                    headers[key.strip()] = value.strip()
            
            return status_line, headers, body, None
        except Exception as e:
            return None, None, None, f"Parse error: {e}"
    
    def log_test(self, test_name, expected, actual, passed, notes=""):
        """Log test result"""
        self.total_tests += 1
        if passed:
            self.passed_tests += 1
        
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
    
    def test_basic_http_methods(self):
        """Test basic HTTP methods"""
        print("\n🧪 BASIC HTTP METHODS")
        
        methods = [
            ("GET", "Valid method"),
            ("POST", "Valid method"),
            ("DELETE", "Valid method"),
            ("HEAD", "Valid method"),
            ("PUT", "Should return 405"),
            ("PATCH", "Should return 405"),
            ("OPTIONS", "Should return 405"),
            ("TRACE", "Should return 405"),
            ("CONNECT", "Should return 405")
        ]
        
        for method, description in methods:
            request = f"{method} / HTTP/1.1\r\nHost: localhost\r\n\r\n"
            response = self.send_raw_request(request, description=description)
            status, headers, body, error = self.parse_response(response)
            
            if error:
                passed = False
                self.log_test(f"Method {method}", "Valid response", error, passed)
            else:
                if method in ["GET", "POST", "DELETE", "HEAD"]:
                    passed = any(code in status for code in ["200", "404", "405"])
                else:
                    passed = "405" in status
                self.log_test(f"Method {method}", description, status, passed)
    
    def test_http_versions(self):
        """Test HTTP version handling"""
        print("\n🧪 HTTP VERSION HANDLING")
        
        versions = [
            ("HTTP/1.1", True),
            ("HTTP/1.0", True),
            ("HTTP/0.9", False),
            ("HTTP/2.0", False),
            ("HTTP/1.2", False),
            ("http/1.1", False),  # case sensitive
            ("HTTPS/1.1", False),
            ("HTTP/1", False),
            ("HTTP/", False),
            ("HTTP", False),
            ("", False)
        ]
        
        for version, should_work in versions:
            request = f"GET / {version}\r\nHost: localhost\r\n\r\n"
            response = self.send_raw_request(request, description=f"Version {version}")
            status, headers, body, error = self.parse_response(response)
            
            if error:
                passed = not should_work
                self.log_test(f"Version {version}", "Accept" if should_work else "Reject", error, passed)
            else:
                if should_work:
                    passed = "200" in status or "404" in status
                else:
                    passed = "400" in status
                self.log_test(f"Version {version}", "Accept" if should_work else "400 Bad Request", status, passed)
    
    def test_malformed_requests(self):
        """Test malformed request handling"""
        print("\n🧪 MALFORMED REQUEST HANDLING")
        
        malformed = [
            ("GET\r\nHost: localhost\r\n\r\n", "Missing path and version"),
            ("GET /\r\nHost: localhost\r\n\r\n", "Missing version"),
            ("GET HTTP/1.1\r\nHost: localhost\r\n\r\n", "Missing path"),
            ("/ HTTP/1.1\r\nHost: localhost\r\n\r\n", "Missing method"),
            ("GET  /  HTTP/1.1\r\nHost: localhost\r\n\r\n", "Extra spaces"),
            ("GET / HTTP/1.1 extra\r\nHost: localhost\r\n\r\n", "Extra text"),
            ("GET /\x00path HTTP/1.1\r\nHost: localhost\r\n\r\n", "Null byte in path"),
            ("GET /\tpath HTTP/1.1\r\nHost: localhost\r\n\r\n", "Tab in path"),
            ("GET /path\rHTTP/1.1\r\nHost: localhost\r\n\r\n", "CR in path"),
            ("GET /path\nHTTP/1.1\r\nHost: localhost\r\n\r\n", "LF in path"),
            ("   GET / HTTP/1.1\r\nHost: localhost\r\n\r\n", "Leading spaces"),
            ("GET / HTTP/1.1\nHost: localhost\r\n\r\n", "LF instead of CRLF"),
            ("GET / HTTP/1.1\r\r\nHost: localhost\r\n\r\n", "Extra CR"),
            ("", "Empty request"),
            ("\r\n\r\n", "Only CRLF"),
            ("INVALID LINE\r\n\r\n", "Invalid request line"),
            ("GET / HTTP/1.1\r\nInvalid header\r\n\r\n", "Invalid header"),
            ("GET / HTTP/1.1\r\nHost\r\n\r\n", "Header without colon"),
            ("GET / HTTP/1.1\r\n:NoName\r\n\r\n", "Header without name")
        ]
        
        for request, description in malformed:
            response = self.send_raw_request(request, description=description)
            status, headers, body, error = self.parse_response(response)
            
            if error:
                passed = True  # Connection error acceptable
                self.log_test(f"Malformed: {description}", "400 or connection error", error, passed)
            else:
                passed = "400" in status
                self.log_test(f"Malformed: {description}", "400 Bad Request", status, passed)
    
    def test_uri_variations(self):
        """Test URI handling variations"""
        print("\n🧪 URI VARIATIONS")
        
        uris = [
            ("/", "Root path"),
            ("/index.html", "Simple file"),
            ("/path/to/file.html", "Nested path"),
            ("//", "Double slash"),
            ("/./", "Current directory"),
            ("/../", "Parent directory"),
            ("/path/../", "Path with parent"),
            ("/path/./file", "Path with current"),
            ("/path//file", "Path with double slash"),
            ("/path?query=value", "Query string"),
            ("/path?query=value&other=data", "Multiple query params"),
            ("/path?", "Empty query"),
            ("/path#fragment", "Fragment"),
            ("/path?query#fragment", "Query and fragment"),
            ("/path%20with%20spaces", "Percent encoded spaces"),
            ("/path%2Fwith%2Fslashes", "Percent encoded slashes"),
            ("/path%3Fwith%3Fquestion", "Percent encoded question marks"),
            ("/path%", "Incomplete percent encoding"),
            ("/path%2", "Incomplete percent encoding"),
            ("/path%ZZ", "Invalid percent encoding"),
            ("/" + "a" * 1000, "Very long path"),
            ("/*", "Asterisk in path"),
            ("/path with spaces", "Unencoded spaces"),
            ("/café", "UTF-8 characters"),
            ("/ ", "Path with trailing space"),
            ("/ HTTP/1.1", "Path that looks like version")
        ]
        
        for uri, description in uris:
            request = f"GET {uri} HTTP/1.1\r\nHost: localhost\r\n\r\n"
            response = self.send_raw_request(request, description=description)
            status, headers, body, error = self.parse_response(response)
            
            if error:
                passed = True  # Connection error acceptable for extreme cases
                self.log_test(f"URI: {description}", "Safe handling", error, passed)
            else:
                passed = "HTTP/1.1" in status  # Any valid HTTP response
                self.log_test(f"URI: {description}", "Valid response", status, passed)
    
    def test_header_variations(self):
        """Test header handling variations"""
        print("\n🧪 HEADER VARIATIONS")
        
        headers = [
            ("Host: localhost", "Standard host header"),
            ("Host:localhost", "No space after colon"),
            ("Host:  localhost", "Multiple spaces after colon"),
            ("Host:\tlocalhost", "Tab after colon"),
            ("Host: localhost ", "Trailing space"),
            ("Host : localhost", "Space before colon"),
            ("host: localhost", "Lowercase header name"),
            ("HOST: localhost", "Uppercase header name"),
            ("HoSt: localhost", "Mixed case header name"),
            ("Content-Length: 0", "Content-Length header"),
            ("Content-Type: text/html", "Content-Type header"),
            ("Connection: close", "Connection close"),
            ("Connection: keep-alive", "Connection keep-alive"),
            ("User-Agent: TestAgent/1.0", "User-Agent header"),
            ("Accept: */*", "Accept header"),
            ("Accept-Encoding: gzip, deflate", "Accept-Encoding header"),
            ("X-Custom-Header: custom-value", "Custom header"),
            ("X-Empty:", "Empty header value"),
            ("X-Space: ", "Space-only header value"),
            ("X-Very-Long-Header-Name-That-Goes-On-And-On: value", "Long header name"),
            ("X-Header: " + "a" * 1000, "Long header value"),
            ("X-Binary: \x00\x01\x02", "Binary data in header"),
            ("X-Unicode: café", "Unicode in header"),
            ("X-Tab: value\tmore", "Tab in header value"),
            ("X-Colon: value:more", "Colon in header value"),
            ("X-CRLF: line1\r\nline2", "CRLF in header value"),
            ("X-Newline: line1\nline2", "Newline in header value")
        ]
        
        for header, description in headers:
            request = f"GET / HTTP/1.1\r\n{header}\r\n\r\n"
            response = self.send_raw_request(request, description=description)
            status, headers_resp, body, error = self.parse_response(response)
            
            if error:
                passed = True  # Connection error acceptable for malformed headers
                self.log_test(f"Header: {description}", "Safe handling", error, passed)
            else:
                passed = "HTTP/1.1" in status
                self.log_test(f"Header: {description}", "Valid response", status, passed)
    
    def test_request_body_handling(self):
        """Test request body handling"""
        print("\n🧪 REQUEST BODY HANDLING")
        
        # POST with Content-Length
        body = "name=test&value=123"
        request = f"POST / HTTP/1.1\r\nHost: localhost\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: {len(body)}\r\n\r\n{body}"
        response = self.send_raw_request(request, description="POST with body")
        status, headers, body_resp, error = self.parse_response(response)
        
        if error:
            passed = False
            self.log_test("POST with body", "200 or 405", error, passed)
        else:
            passed = any(code in status for code in ["200", "405", "501"])
            self.log_test("POST with body", "200, 405, or 501", status, passed)
        
        # POST without Content-Length
        body = "name=test&value=123"
        request = f"POST / HTTP/1.1\r\nHost: localhost\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\n{body}"
        response = self.send_raw_request(request, description="POST without Content-Length")
        status, headers, body_resp, error = self.parse_response(response)
        
        if error:
            passed = True
            self.log_test("POST without Content-Length", "400 or 411", error, passed)
        else:
            passed = any(code in status for code in ["400", "411"])
            self.log_test("POST without Content-Length", "400 or 411", status, passed)
        
        # POST with wrong Content-Length
        body = "short"
        request = f"POST / HTTP/1.1\r\nHost: localhost\r\nContent-Type: text/plain\r\nContent-Length: 100\r\n\r\n{body}"
        response = self.send_raw_request(request, description="POST with wrong Content-Length")
        status, headers, body_resp, error = self.parse_response(response)
        
        if error:
            passed = True
            self.log_test("POST wrong Content-Length", "400 or timeout", error, passed)
        else:
            passed = any(code in status for code in ["400", "408"])
            self.log_test("POST wrong Content-Length", "400 or 408", status, passed)
        
        # Large POST body
        large_body = "x" * 10000
        request = f"POST / HTTP/1.1\r\nHost: localhost\r\nContent-Type: text/plain\r\nContent-Length: {len(large_body)}\r\n\r\n{large_body}"
        response = self.send_raw_request(request, timeout=15, description="Large POST body")
        status, headers, body_resp, error = self.parse_response(response)
        
        if error:
            passed = True  # Server may reject large bodies
            self.log_test("Large POST body", "Safe handling", error, passed)
        else:
            passed = "HTTP/1.1" in status
            self.log_test("Large POST body", "Safe handling", status, passed)
    
    def test_connection_handling(self):
        """Test connection handling"""
        print("\n🧪 CONNECTION HANDLING")
        
        # Test Connection: keep-alive
        request = "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: keep-alive\r\n\r\n"
        response = self.send_raw_request(request, description="Connection: keep-alive")
        status, headers, body, error = self.parse_response(response)
        
        if error:
            passed = False
            self.log_test("Keep-alive", "Valid response", error, passed)
        else:
            passed = "HTTP/1.1" in status
            self.log_test("Keep-alive", "Valid response", status, passed)
        
        # Test Connection: close
        request = "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
        response = self.send_raw_request(request, description="Connection: close")
        status, headers, body, error = self.parse_response(response)
        
        if error:
            passed = False
            self.log_test("Connection close", "Valid response", error, passed)
        else:
            passed = "HTTP/1.1" in status
            self.log_test("Connection close", "Valid response", status, passed)
        
        # Test multiple sequential connections
        for i in range(5):
            request = f"GET / HTTP/1.1\r\nHost: localhost\r\nX-Request-ID: {i}\r\n\r\n"
            response = self.send_raw_request(request, description=f"Sequential {i}")
            status, headers, body, error = self.parse_response(response)
            
            if error:
                passed = False
                self.log_test(f"Sequential {i}", "Valid response", error, passed)
            else:
                passed = "200" in status or "404" in status
                self.log_test(f"Sequential {i}", "Valid response", status, passed)
    
    def test_timeout_handling(self):
        """Test timeout and partial request handling"""
        print("\n🧪 TIMEOUT HANDLING")
        
        # Test slow request
        parts = ["GET", " /", " HTTP/1.1\r\n", "Host: localhost\r\n", "\r\n"]
        delays = [0.5, 0.5, 0.5, 0.5]
        
        response = self.send_partial_request(parts, delays, timeout=10)
        status, headers, body, error = self.parse_response(response)
        
        if error:
            passed = True  # Timeout acceptable
            self.log_test("Slow request", "Response or timeout", error, passed)
        else:
            passed = "HTTP/1.1" in status
            self.log_test("Slow request", "Valid response", status, passed)
        
        # Test incomplete request
        parts = ["GET / HTTP/1.1\r\n", "Host: localhost\r\n", "Content-Length: 10\r\n\r\n"]
        delays = [0.5, 0.5, 3.0]  # Don't send body
        
        response = self.send_partial_request(parts, delays, timeout=15)
        status, headers, body, error = self.parse_response(response)
        
        if error:
            passed = "timeout" in error or "Connection" in error
            self.log_test("Incomplete request", "Timeout or close", error, passed)
        else:
            passed = "400" in status or "408" in status
            self.log_test("Incomplete request", "400 or 408", status, passed)
    
    def test_stress_conditions(self):
        """Test stress conditions"""
        print("\n🧪 STRESS CONDITIONS")
        
        # Test rapid connections
        rapid_passed = 0
        rapid_total = 20
        
        for i in range(rapid_total):
            request = f"GET / HTTP/1.1\r\nHost: localhost\r\nX-Rapid: {i}\r\n\r\n"
            response = self.send_raw_request(request, timeout=5)
            status, headers, body, error = self.parse_response(response)
            
            if not error and "HTTP/1.1" in status:
                rapid_passed += 1
        
        passed = rapid_passed >= rapid_total * 0.7  # 70% success rate
        self.log_test("Rapid connections", f"{rapid_total} connections", f"{rapid_passed}/{rapid_total} successful", passed)
        
        # Test concurrent connections
        def make_request(i):
            request = f"GET / HTTP/1.1\r\nHost: localhost\r\nX-Concurrent: {i}\r\n\r\n"
            response = self.send_raw_request(request, timeout=10)
            status, headers, body, error = self.parse_response(response)
            return not error and "HTTP/1.1" in status
        
        concurrent_total = 10
        with ThreadPoolExecutor(max_workers=concurrent_total) as executor:
            futures = [executor.submit(make_request, i) for i in range(concurrent_total)]
            concurrent_passed = sum(1 for f in futures if f.result())
        
        passed = concurrent_passed >= concurrent_total * 0.7  # 70% success rate
        self.log_test("Concurrent connections", f"{concurrent_total} connections", f"{concurrent_passed}/{concurrent_total} successful", passed)
    
    def test_response_validation(self):
        """Test response format validation"""
        print("\n🧪 RESPONSE VALIDATION")
        
        request = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"
        response = self.send_raw_request(request, description="Response validation")
        status, headers, body, error = self.parse_response(response)
        
        if error:
            passed = False
            self.log_test("Response format", "Valid HTTP response", error, passed)
        else:
            # Check status line format
            status_valid = status.startswith("HTTP/1.1 ") and len(status.split()) >= 3
            
            # Check CRLF usage
            crlf_correct = '\r\n' in response
            
            # Check basic headers
            has_content_length = 'Content-Length' in headers or 'Transfer-Encoding' in headers
            
            validation_results = {
                'status_line': status_valid,
                'crlf_usage': crlf_correct,
                'content_length': has_content_length
            }
            
            passed = status_valid and crlf_correct
            self.log_test("Response format", "Valid format", str(validation_results), passed)
    
    def test_security_edge_cases(self):
        """Test security-related edge cases"""
        print("\n🧪 SECURITY EDGE CASES")
        
        security_tests = [
            ("GET /../../../etc/passwd HTTP/1.1\r\nHost: localhost\r\n\r\n", "Directory traversal"),
            ("GET /..\\..\\..\\windows\\system32\\config\\sam HTTP/1.1\r\nHost: localhost\r\n\r\n", "Windows path traversal"),
            ("GET /%2e%2e%2f%2e%2e%2f%2e%2e%2fetc%2fpasswd HTTP/1.1\r\nHost: localhost\r\n\r\n", "Encoded traversal"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nX-Forwarded-For: 127.0.0.1\r\n\r\n", "X-Forwarded-For injection"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nX-Real-IP: 127.0.0.1\r\n\r\n", "X-Real-IP injection"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nReferer: javascript:alert('xss')\r\n\r\n", "XSS in Referer"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nUser-Agent: <script>alert('xss')</script>\r\n\r\n", "XSS in User-Agent"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nCookie: sessionid='; DROP TABLE users; --\r\n\r\n", "SQL injection in Cookie"),
            ("GET /?param=<script>alert('xss')</script> HTTP/1.1\r\nHost: localhost\r\n\r\n", "XSS in URL parameter"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nContent-Length: -1\r\n\r\n", "Negative Content-Length"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 999999999999999999999\r\n\r\n", "Huge Content-Length"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\n\r\n0\r\n\r\n", "Chunked encoding"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 10\r\nTransfer-Encoding: chunked\r\n\r\n", "Conflicting headers"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nExpect: 100-continue\r\n\r\n", "Expect 100-continue"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nRange: bytes=0-1000\r\n\r\n", "Range request"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nIf-None-Match: *\r\n\r\n", "If-None-Match header"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nIf-Modified-Since: Wed, 21 Oct 2015 07:28:00 GMT\r\n\r\n", "If-Modified-Since header"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nUpgrade: websocket\r\n\r\n", "WebSocket upgrade"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nOrigin: http://evil.com\r\n\r\n", "Cross-origin request"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nAccess-Control-Request-Method: POST\r\n\r\n", "CORS preflight")
        ]
        
        for request, description in security_tests:
            response = self.send_raw_request(request, description=description)
            status, headers, body, error = self.parse_response(response)
            
            if error:
                passed = True  # Connection error acceptable
                self.log_test(f"Security: {description}", "Safe handling", error, passed)
            else:
                # Should not expose sensitive information or crash
                passed = "HTTP/1.1" in status and "etc/passwd" not in body.lower()
                self.log_test(f"Security: {description}", "Safe handling", status, passed)
    
    def test_http_compliance(self):
        """Test HTTP/1.1 compliance"""
        print("\n🧪 HTTP/1.1 COMPLIANCE")
        
        # Test Host header requirement
        request = "GET / HTTP/1.1\r\n\r\n"
        response = self.send_raw_request(request, description="Missing Host header")
        status, headers, body, error = self.parse_response(response)
        
        if error:
            passed = True
            self.log_test("Missing Host header", "400 Bad Request", error, passed)
        else:
            passed = "400" in status
            self.log_test("Missing Host header", "400 Bad Request", status, passed)
        
        # Test case-insensitive headers
        request = "GET / HTTP/1.1\r\nhost: localhost\r\ncontent-type: text/html\r\n\r\n"
        response = self.send_raw_request(request, description="Lowercase headers")
        status, headers, body, error = self.parse_response(response)
        
        if error:
            passed = False
            self.log_test("Lowercase headers", "Valid response", error, passed)
        else:
            passed = "200" in status or "404" in status
            self.log_test("Lowercase headers", "Valid response", status, passed)
        
        # Test multiple header values
        request = "GET / HTTP/1.1\r\nHost: localhost\r\nAccept: text/html\r\nAccept: application/json\r\n\r\n"
        response = self.send_raw_request(request, description="Multiple Accept headers")
        status, headers, body, error = self.parse_response(response)
        
        if error:
            passed = False
            self.log_test("Multiple headers", "Valid response", error, passed)
        else:
            passed = "200" in status or "404" in status
            self.log_test("Multiple headers", "Valid response", status, passed)
    
    def test_edge_cases(self):
        """Test various edge cases"""
        print("\n🧪 EDGE CASES")
        
        edge_cases = [
            ("GET\t/\tHTTP/1.1\r\nHost: localhost\r\n\r\n", "Tabs in request line"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 0\r\n\r\n", "Zero Content-Length"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 000\r\n\r\n", "Leading zeros in Content-Length"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nContent-Length: +0\r\n\r\n", "Plus sign in Content-Length"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 0x0\r\n\r\n", "Hex Content-Length"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 1.0\r\n\r\n", "Decimal Content-Length"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nContent-Length: abc\r\n\r\n", "Non-numeric Content-Length"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nContent-Length: \r\n\r\n", "Empty Content-Length"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nContent-Length:  5  \r\n\r\nhello", "Spaces in Content-Length"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: identity\r\n\r\n", "Identity encoding"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: gzip\r\n\r\n", "Gzip encoding"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: \r\n\r\n", "Empty Transfer-Encoding"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nTE: trailers\r\n\r\n", "TE header"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nTrailer: X-Custom\r\n\r\n", "Trailer header"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nVia: 1.1 proxy\r\n\r\n", "Via header"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nMax-Forwards: 10\r\n\r\n", "Max-Forwards header"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nPragma: no-cache\r\n\r\n", "Pragma header"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nCache-Control: no-cache\r\n\r\n", "Cache-Control header"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nWarning: 199 - \"Miscellaneous warning\"\r\n\r\n", "Warning header"),
            ("GET / HTTP/1.1\r\nHost: localhost\r\nDate: Wed, 21 Oct 2015 07:28:00 GMT\r\n\r\n", "Date header from client")
        ]
        
        for request, description in edge_cases:
            response = self.send_raw_request(request, description=description)
            status, headers, body, error = self.parse_response(response)
            
            if error:
                passed = True  # Connection error acceptable
                self.log_test(f"Edge case: {description}", "Safe handling", error, passed)
            else:
                passed = "HTTP/1.1" in status
                self.log_test(f"Edge case: {description}", "Valid response", status, passed)
    
    def generate_report(self):
        """Generate comprehensive test report"""
        print("\n" + "="*80)
        print("📊 ADVANCED WEBSERV TEST REPORT")
        print("="*80)
        
        success_rate = (self.passed_tests / self.total_tests * 100) if self.total_tests > 0 else 0
        
        print(f"Total Tests: {self.total_tests}")
        print(f"Passed: {self.passed_tests}")
        print(f"Failed: {self.total_tests - self.passed_tests}")
        print(f"Success Rate: {success_rate:.1f}%")
        
        # Category breakdown
        categories = {
            'Basic Methods': [r for r in self.test_results if 'method' in r['test'].lower()],
            'HTTP Versions': [r for r in self.test_results if 'version' in r['test'].lower()],
            'Malformed Requests': [r for r in self.test_results if 'malformed' in r['test'].lower()],
            'URI Handling': [r for r in self.test_results if 'uri' in r['test'].lower()],
            'Header Handling': [r for r in self.test_results if 'header' in r['test'].lower()],
            'Request Body': [r for r in self.test_results if 'post' in r['test'].lower() or 'body' in r['test'].lower()],
            'Connection Handling': [r for r in self.test_results if 'connection' in r['test'].lower() or 'sequential' in r['test'].lower()],
            'Timeout Handling': [r for r in self.test_results if 'timeout' in r['test'].lower() or 'slow' in r['test'].lower() or 'incomplete' in r['test'].lower()],
            'Stress Testing': [r for r in self.test_results if 'rapid' in r['test'].lower() or 'concurrent' in r['test'].lower()],
            'Response Validation': [r for r in self.test_results if 'response' in r['test'].lower()],
            'Security': [r for r in self.test_results if 'security' in r['test'].lower()],
            'HTTP Compliance': [r for r in self.test_results if 'compliance' in r['test'].lower() or 'host header' in r['test'].lower()],
            'Edge Cases': [r for r in self.test_results if 'edge case' in r['test'].lower()]
        }
        
        print(f"\n🎯 CATEGORY BREAKDOWN:")
        for category_name, tests in categories.items():
            if tests:
                passed = sum(1 for t in tests if t['passed'])
                total = len(tests)
                rate = passed/total*100
                status = "✅" if rate >= 80 else "⚠️" if rate >= 60 else "❌"
                print(f"{status} {category_name}: {passed}/{total} ({rate:.1f}%)")
        
        # Failed tests
        failed_tests = [r for r in self.test_results if not r['passed']]
        if failed_tests:
            print(f"\n❌ FAILED TESTS ({len(failed_tests)}):")
            for result in failed_tests[:20]:  # Show first 20 failures
                print(f"  - {result['test']}: {result['actual']}")
            if len(failed_tests) > 20:
                print(f"  ... and {len(failed_tests) - 20} more")
        
        # Recommendations
        print(f"\n💡 RECOMMENDATIONS:")
        if success_rate >= 90:
            print("✅ Excellent! Your server handles most HTTP scenarios correctly.")
        elif success_rate >= 70:
            print("⚠️ Good foundation, but some edge cases need attention.")
        else:
            print("❌ Many issues found. Focus on basic HTTP compliance first.")
        
        if any('malformed' in r['test'].lower() and not r['passed'] for r in self.test_results):
            print("- Improve malformed request handling")
        
        if any('security' in r['test'].lower() and not r['passed'] for r in self.test_results):
            print("- Review security-related request handling")
        
        if any('timeout' in r['test'].lower() and not r['passed'] for r in self.test_results):
            print("- Implement proper timeout handling")
        
        elapsed_time = time.time() - self.start_time if self.start_time else 0
        print(f"\n⏱️ Test duration: {elapsed_time:.2f} seconds")
        
        return self.passed_tests, self.total_tests - self.passed_tests
    
    def run_all_tests(self):
        """Run all tests"""
        if not self.start_server():
            print("❌ Cannot start server, aborting tests")
            return False
        
        try:
            self.start_time = time.time()
            
            print("\n🧪 ADVANCED WEBSERV HTTP SERVER TESTING")
            print("Comprehensive testing for HTTP compliance, security, and robustness")
            print("="*80)
            
            # Run all test categories
            self.test_basic_http_methods()
            self.test_http_versions()
            self.test_malformed_requests()
            self.test_uri_variations()
            self.test_header_variations()
            self.test_request_body_handling()
            self.test_connection_handling()
            self.test_timeout_handling()
            self.test_stress_conditions()
            self.test_response_validation()
            self.test_security_edge_cases()
            self.test_http_compliance()
            self.test_edge_cases()
            
            # Generate report
            passed, failed = self.generate_report()
            
            return failed == 0
            
        finally:
            self.stop_server()

def main():
    """Main test runner"""
    if not os.path.exists('./webserv'):
        print("❌ webserv executable not found. Please build the project first.")
        sys.exit(1)
    
    # Check for config file
    config_files = ['test_8080.conf', 'test.conf', 'webserv.conf']
    config_file = None
    for cf in config_files:
        if os.path.exists(cf):
            config_file = cf
            break
    
    if not config_file:
        print("❌ No configuration file found. Please create test_8080.conf or test.conf")
        sys.exit(1)
    
    print("🧪 ADVANCED WEBSERV TEST SUITE")
    print("Comprehensive HTTP server testing with extensive test cases")
    print("="*80)
    
    # Allow custom host/port
    host = sys.argv[1] if len(sys.argv) > 1 else '127.0.0.1'
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 8080
    
    tester = AdvancedWebservTester(host=host, port=port, config_file=config_file)
    success = tester.run_all_tests()
    
    if success:
        print("\n🎉 ALL TESTS PASSED! Your HTTP server is robust and compliant.")
    else:
        print("\n⚠️ Some tests failed. Review the report above for improvement areas.")
    
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()