#!/bin/bash

# Set the server's port number
PORT=54740

echo "Starting tests for HTTP request parsing..."

# Test a basic GET request to the server
echo "1. Testing basic GET request:"
curl -i "http://localhost:$PORT/main.html"
echo -e "\n"

# Test a GET request with query parameters
echo "2. Testing GET request with query parameters:"
curl -i "http://localhost:$PORT/main.html?name1=value1&name2=value2"
echo -e "\n"

# Test a GET request for a non-existing page
echo "3. Testing GET request for a non-existing page:"
curl -i "http://localhost:$PORT/nonexistentpage"
echo -e "\n"

# Test a basic POST request (assuming handling POST is part of your assignment)
echo "4. Testing basic POST request:"
# Replace '/path/to/local/file' with an actual file path or remove -F option if not needed
curl -i -X POST "http://localhost:$PORT/upload" -F "data=@/path/to/local/file"
echo -e "\n"

# Additional tests can be added here for other scenarios, like:
# - Different types of resources (e.g., images, scripts)
# - Requests with different HTTP methods (if applicable)
# - Requests with malformed URLs or query strings
# - Edge cases like extremely long URLs

echo "Tests completed."
