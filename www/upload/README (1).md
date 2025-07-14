# Upload Directory

This directory is used for file uploads via the WebServ 42 server.

## Usage:

Upload files using the form on the main page or via curl:

```bash
curl -X POST -F "file=@/path/to/your/file.txt" http://localhost:8080/upload
```

## Features:

- File upload via POST requests
- Multipart form data support
- File size validation
- Secure file storage

Files uploaded here will be stored securely and can be accessed via the web interface.