#!/usr/bin/python3

import os
import sys
import cgi
import cgitb
from datetime import datetime

# Enable CGI error reporting
cgitb.enable()

# Print HTTP headers
print("Content-Type: text/html")
print()

# HTML response
print("""<!DOCTYPE html>
<html>
<head>
    <title>Python CGI Form Test</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        .form { background: #f9f9f9; padding: 20px; margin: 20px 0; border: 1px solid #ddd; }
        .result { background: #e8f5e8; padding: 15px; margin: 20px 0; border: 1px solid #4caf50; }
        input, textarea { width: 100%; padding: 8px; margin: 5px 0; }
        button { background: #4caf50; color: white; padding: 10px 20px; border: none; cursor: pointer; }
        button:hover { background: #45a049; }
    </style>
</head>
<body>
    <h1>🐍 Python CGI Form Handler</h1>""")

# Check if this is a POST request with form data
if os.environ.get('REQUEST_METHOD') == 'POST':
    # Read POST data
    content_length = int(os.environ.get('CONTENT_LENGTH', '0'))
    if content_length > 0:
        post_data = sys.stdin.read(content_length)
        
        print(f"""    <div class="result">
        <h2>✅ Form Submitted Successfully!</h2>
        <p><strong>Received POST data:</strong></p>
        <pre>{post_data}</pre>
        <p><strong>Content Length:</strong> {content_length} bytes</p>
        <p><strong>Content Type:</strong> {os.environ.get('CONTENT_TYPE', 'Not set')}</p>
        <p><strong>Timestamp:</strong> {datetime.now()}</p>
    </div>""")
        
        # Parse form data if it's URL-encoded
        if os.environ.get('CONTENT_TYPE', '').startswith('application/x-www-form-urlencoded'):
            import urllib.parse
            parsed_data = urllib.parse.parse_qs(post_data)
            print("""    <div class="result">
        <h3>Parsed Form Fields:</h3>
        <table style="width: 100%; border-collapse: collapse;">
            <tr style="background: #f2f2f2;"><th style="border: 1px solid #ddd; padding: 8px;">Field</th><th style="border: 1px solid #ddd; padding: 8px;">Value</th></tr>""")
            for key, values in parsed_data.items():
                for value in values:
                    print(f'            <tr><td style="border: 1px solid #ddd; padding: 8px;">{key}</td><td style="border: 1px solid #ddd; padding: 8px;">{value}</td></tr>')
            print("""        </table>
    </div>""")

# Display the form
print("""    <div class="form">
        <h2>Test Form</h2>
        <form method="POST" action="/cgi-bin/form.py">
            <label for="name">Name:</label>
            <input type="text" id="name" name="name" placeholder="Enter your name" required>
            
            <label for="email">Email:</label>
            <input type="email" id="email" name="email" placeholder="Enter your email" required>
            
            <label for="message">Message:</label>
            <textarea id="message" name="message" rows="4" placeholder="Enter your message" required></textarea>
            
            <button type="submit">Submit Form</button>
        </form>
    </div>
    
    <div class="form">
        <h2>Test Information</h2>
        <p><strong>Request Method:</strong> {}</p>
        <p><strong>Current Time:</strong> {}</p>
        <p><strong>Python Version:</strong> {}</p>
    </div>
    
    <hr>
    <p><em>Python CGI Form Handler - webserv</em></p>
</body>
</html>""".format(
    os.environ.get('REQUEST_METHOD', 'Unknown'),
    datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
    sys.version.split()[0]
))