#!/bin/bash

# Start the server in the background
./image_server &
SERVER_PID=$!

# Give the server some time to start up
sleep 2

# Perform a test request for the main page
echo "Testing GET request for the main page"
curl -o /dev/null -s -w "%{http_code}\n" http://localhost:54740/main.html

# Perform more tests as needed
# e.g., Test for a non-existing page
echo "Testing GET request for a non-existing page"
curl -o /dev/null -s -w "%{http_code}\n" http://localhost:54740/non_existing_page.html

# Add more tests for POST requests, image filters, file uploads, etc.

# Shut down the server
kill $SERVER_PID
