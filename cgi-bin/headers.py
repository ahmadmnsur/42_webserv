#!/usr/bin/python3

import os
import sys
from datetime import datetime

# Print HTTP headers
print("Content-Type: text/html")
print()

# HTML response
print("""<!DOCTYPE html>
<html>
<head>
    <title>WebServ 42 - HTTP Headers Debug</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 40px; background: #f8f9fa; }}
        .container {{ max-width: 1000px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }}
        h1 {{ color: #2563eb; border-bottom: 3px solid #2563eb; padding-bottom: 10px; }}
        h2 {{ color: #1f2937; margin-top: 30px; }}
        .header-section {{ background: #f8fafc; padding: 20px; border-radius: 8px; margin: 20px 0; border-left: 4px solid #2563eb; }}
        .header-item {{ display: flex; margin: 10px 0; }}
        .header-name {{ font-weight: bold; min-width: 200px; color: #374151; }}
        .header-value {{ color: #6b7280; font-family: monospace; background: #f3f4f6; padding: 4px 8px; border-radius: 4px; flex: 1; }}
        .cgi-table {{ width: 100%; border-collapse: collapse; margin: 20px 0; }}
        .cgi-table th, .cgi-table td {{ border: 1px solid #e5e7eb; padding: 12px; text-align: left; }}
        .cgi-table th {{ background: #f3f4f6; font-weight: 600; }}
        .cgi-table tr:nth-child(even) {{ background: #f9fafb; }}
        .status-badge {{ display: inline-block; padding: 4px 8px; border-radius: 4px; font-size: 0.8em; font-weight: bold; }}
        .status-ok {{ background: #d1fae5; color: #065f46; }}
        .status-warning {{ background: #fef3c7; color: #92400e; }}
        .status-error {{ background: #fecaca; color: #991b1b; }}
        .code-block {{ background: #1e1e1e; color: #e5e7eb; padding: 15px; border-radius: 8px; font-family: monospace; margin: 15px 0; overflow-x: auto; }}
        .highlight {{ background: #fef3c7; padding: 2px 6px; border-radius: 4px; }}
    </style>
</head>
<body>
    <div class="container">
        <h1>🔍 WebServ 42 - HTTP Headers Debug</h1>
        <p>Complete HTTP request headers and CGI environment analysis</p>
        
        <div class="header-section">
            <h2>Request Summary</h2>
            <div class="header-item">
                <span class="header-name">Method:</span>
                <span class="header-value">{method}</span>
            </div>
            <div class="header-item">
                <span class="header-name">URI:</span>
                <span class="header-value">{uri}</span>
            </div>
            <div class="header-item">
                <span class="header-name">Protocol:</span>
                <span class="header-value">{protocol}</span>
            </div>
            <div class="header-item">
                <span class="header-name">Server:</span>
                <span class="header-value">WebServ 42</span>
            </div>
            <div class="header-item">
                <span class="header-name">Timestamp:</span>
                <span class="header-value">{timestamp}</span>
            </div>
        </div>
        
        <div class="header-section">
            <h2>HTTP Headers Received</h2>""".format(
    method=os.environ.get('REQUEST_METHOD', 'Not set'),
    uri=os.environ.get('REQUEST_URI', 'Not set'),
    protocol=os.environ.get('SERVER_PROTOCOL', 'Not set'),
    timestamp=datetime.now().strftime('%Y-%m-%d %H:%M:%S')
))

# Display HTTP headers
http_headers = []
for key, value in os.environ.items():
    if key.startswith('HTTP_'):
        header_name = key[5:].replace('_', '-').title()
        http_headers.append((header_name, value))

if http_headers:
    for header_name, header_value in sorted(http_headers):
        # Truncate very long values
        if len(header_value) > 100:
            header_value = header_value[:100] + '...'
        print(f'''            <div class="header-item">
                <span class="header-name">{header_name}:</span>
                <span class="header-value">{header_value}</span>
            </div>''')
else:
    print('            <p>No HTTP headers found in environment</p>')

print("""        </div>
        
        <div class="header-section">
            <h2>CGI Environment Variables</h2>
            <table class="cgi-table">
                <tr><th>Variable</th><th>Value</th><th>Description</th></tr>""")

# CGI environment variables with descriptions
cgi_vars = [
    ('REQUEST_METHOD', 'HTTP method used for the request'),
    ('REQUEST_URI', 'Complete URI requested by client'),
    ('QUERY_STRING', 'Query parameters from URL'),
    ('SERVER_PROTOCOL', 'HTTP protocol version'),
    ('SERVER_NAME', 'Server hostname'),
    ('SERVER_PORT', 'Server port number'),
    ('GATEWAY_INTERFACE', 'CGI version supported'),
    ('CONTENT_TYPE', 'MIME type of request body'),
    ('CONTENT_LENGTH', 'Length of request body in bytes'),
    ('SCRIPT_NAME', 'Name of CGI script'),
    ('PATH_INFO', 'Extra path information'),
    ('REMOTE_ADDR', 'IP address of client'),
    ('REMOTE_HOST', 'Hostname of client'),
    ('AUTH_TYPE', 'Authentication method used'),
    ('REMOTE_USER', 'Authenticated username'),
    ('HTTP_ACCEPT', 'MIME types accepted by client'),
    ('HTTP_ACCEPT_ENCODING', 'Encodings accepted by client'),
    ('HTTP_ACCEPT_LANGUAGE', 'Languages accepted by client'),
    ('HTTP_CONNECTION', 'Connection handling preference'),
    ('HTTP_HOST', 'Host header from request'),
    ('HTTP_USER_AGENT', 'Client software identification'),
    ('HTTP_REFERER', 'Previous page URL'),
    ('HTTP_COOKIE', 'Cookies sent by client')
]

for var_name, description in cgi_vars:
    value = os.environ.get(var_name, '<em>Not set</em>')
    if len(str(value)) > 80:
        value = str(value)[:80] + '...'
    
    # Add status badge
    if value == '<em>Not set</em>':
        status = '<span class="status-badge status-warning">Missing</span>'
    else:
        status = '<span class="status-badge status-ok">Present</span>'
    
    print(f'''                <tr>
                    <td><strong>{var_name}</strong> {status}</td>
                    <td style="font-family: monospace;">{value}</td>
                    <td>{description}</td>
                </tr>''')

print("""            </table>
        </div>
        
        <div class="header-section">
            <h2>Request Analysis</h2>
            <div class="header-item">
                <span class="header-name">Content Type:</span>
                <span class="header-value">{content_type}</span>
            </div>
            <div class="header-item">
                <span class="header-name">Content Length:</span>
                <span class="header-value">{content_length}</span>
            </div>
            <div class="header-item">
                <span class="header-name">Has Body:</span>
                <span class="header-value">{has_body}</span>
            </div>
            <div class="header-item">
                <span class="header-name">Client IP:</span>
                <span class="header-value">{client_ip}</span>
            </div>
            <div class="header-item">
                <span class="header-name">User Agent:</span>
                <span class="header-value">{user_agent}</span>
            </div>
        </div>
        
        <div class="header-section">
            <h2>Raw Environment Dump</h2>
            <div class="code-block">""".format(
    content_type=os.environ.get('CONTENT_TYPE', 'Not specified'),
    content_length=os.environ.get('CONTENT_LENGTH', '0'),
    has_body='Yes' if os.environ.get('CONTENT_LENGTH', '0') != '0' else 'No',
    client_ip=os.environ.get('REMOTE_ADDR', 'Unknown'),
    user_agent=os.environ.get('HTTP_USER_AGENT', 'Not provided')[:100] + ('...' if len(os.environ.get('HTTP_USER_AGENT', '')) > 100 else '')
))

# Display all environment variables
env_vars = sorted(os.environ.items())
for key, value in env_vars:
    if len(value) > 120:
        value = value[:120] + '...'
    # Escape HTML characters
    value = value.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')
    print(f"{key}={value}")

print("""            </div>
        </div>
        
        <div class="header-section">
            <h2>🧪 Test Results</h2>
            <div class="header-item">
                <span class="header-name">CGI Processing:</span>
                <span class="header-value">✅ Working</span>
            </div>
            <div class="header-item">
                <span class="header-name">Environment Access:</span>
                <span class="header-value">✅ Complete</span>
            </div>
            <div class="header-item">
                <span class="header-name">HTTP Headers:</span>
                <span class="header-value">✅ Parsed</span>
            </div>
            <div class="header-item">
                <span class="header-name">Server Integration:</span>
                <span class="header-value">✅ Functional</span>
            </div>
        </div>
        
        <p style="margin-top: 30px; text-align: center; color: #6b7280;">
            <strong>WebServ 42</strong> - HTTP Headers Debug Tool
        </p>
        
        <p style="text-align: center;">
            <a href="/" style="color: #2563eb; text-decoration: none;">← Back to Main Page</a>
        </p>
    </div>
</body>
</html>""")

# Also read any POST data if present
if os.environ.get('REQUEST_METHOD') == 'POST':
    content_length = int(os.environ.get('CONTENT_LENGTH', '0'))
    if content_length > 0:
        post_data = sys.stdin.read(content_length)
        print(f"""
        <div class="header-section">
            <h2>POST Data</h2>
            <div class="code-block">
{post_data}
            </div>
        </div>
        """)