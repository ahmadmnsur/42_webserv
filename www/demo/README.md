# WebServ 42 Demo Directory

This directory contains sample files for testing the WebServ 42 HTTP server capabilities.

## Files:

- `sample.txt` - Plain text file demo
- `sample.html` - HTML file with styling
- `sample.json` - JSON API response example
- `README.md` - This file

## Features Demonstrated:

- Static file serving
- MIME type detection
- Directory listing (when accessed via `/demo/`)
- File permissions and access control

## Test Commands:

```bash
# Test static file serving
curl http://localhost:8080/demo/sample.txt
curl http://localhost:8080/demo/sample.html
curl http://localhost:8080/demo/sample.json

# Test directory listing
curl http://localhost:8080/demo/
```

Browse to http://localhost:8080/demo/ to see the directory listing feature in action!