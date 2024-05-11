# Web Server Application

This project is part of the CSC209H1 F course at the University of Toronto, focusing on system-level programming. The assignment involves building a basic web server that allows users to run image filters on custom images using a web browser.

## Table of Contents
- [Learning Objectives](#learning-objectives)
- [Introduction](#introduction)
- [Setup](#setup)
- [Usage](#usage)
- [Functionalities](#functionalities)
  - [Parsing HTTP Requests](#parsing-http-requests)
  - [Handling Responses](#handling-responses)
  - [Image Filtering](#image-filtering)
  - [Uploading Files](#uploading-files)
- [Testing](#testing)
- [Submission](#submission)
- [License](#license)

## Learning Objectives

By the end of this assignment, students will be able to:
- Communicate with other processes across a network using sockets.
- Parse parts of HTTP requests.
- Create new processes and manipulate data both in plaintext and binary forms.
- Enhance understanding of a medium-sized C program.

## Introduction

The web server developed in this assignment listens for HTTP requests using sockets and constructs appropriate HTTP responses. The server is capable of serving HTML documents and other types of responses. This server primarily handles image filter operations which were built in a previous assignment (Assignment 3).

## Setup

1. Clone the repository and navigate to the project directory:
   ```
   git clone <repository-url>
   cd <project-directory>
   ```
2. Perform a git pull to ensure you have the latest starter code:
   ```
   git pull
   ```
3. Compile the code using the provided Makefile:
   ```
   make
   ```

## Usage

To start the server, run:
```
./image_server
```
Navigate to `http://localhost:<port>/main.html` in your web browser to interact with the server. Use Chrome or Firefox for best results.

## Functionalities

### Parsing HTTP Requests

The server analyzes the first line of an HTTP request to determine the request type (GET or POST) and the requested resource.

### Handling Responses

- **GET Requests**: For valid resources, the server sends back the `main.html` or processes image filter requests.
- **Error Handling**: Returns a "404 Not Found" for invalid resources and "400 Bad Request" for improperly formatted requests.

### Image Filtering

Allows users to apply predefined image filters to images stored on the server. Filters must be executable and located in the `filters/` directory.

### Uploading Files

Users can upload bitmap images which are saved in the `images/` directory. The server handles multipart/form-data for uploads.

## Testing

Open the HTML files directly in a browser to see how the server responds to various requests. Test different GET and POST scenarios by manipulating URL query parameters and observing server logs.

## Submission

Commit your changes to your repository. Ensure no executables or object files are included in the final commit:
```
git rm --cached <file-to-remove>
git commit -m "Remove unnecessary files"
```

## License

This project is licensed under the MIT License - see the LICENSE file for details.
