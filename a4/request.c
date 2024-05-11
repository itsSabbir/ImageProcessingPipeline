#include "request.h"
#include "response.h"
#include <string.h>


/******************************************************************************
 * ClientState-processing functions
 *****************************************************************************/
ClientState *init_clients(int n) {
    ClientState *clients = malloc(sizeof(ClientState) * n);
    for (int i = 0; i < n; i++) {
        clients[i].sock = -1;  // -1 here indicates available entry
    }
    return clients;
}

/* 
 * Remove the client from the client array, free any memory allocated for
 * fields of the ClientState struct, and close the socket.
 */
void remove_client(ClientState *cs) {
    if (cs->reqData != NULL) {
        free(cs->reqData->method);
        free(cs->reqData->path);
        for (int i = 0; i < MAX_QUERY_PARAMS && cs->reqData->params[i].name != NULL; i++) {
            free(cs->reqData->params[i].name);
            free(cs->reqData->params[i].value);
        }
        free(cs->reqData);
        cs->reqData = NULL;
    }
    close(cs->sock);
    cs->sock = -1;
    cs->num_bytes = 0;
}


/*
 * Search the first inbuf characters of buf for a network newline ("\r\n").
 * Return the index *immediately after* the location of the '\n'
 * if the network newline is found, or -1 otherwise.
 * Definitely do not use strchr or any other string function in here. (Why not?)
 * Reason for not using strchr: strchr scans for a single character, not a character sequence. 
 * Moreover, it does not provide the level of control needed here, especially for buffer boundaries.
 */
int find_network_newline(const char *buf, int inbuf) {
    // Loop through each character in the buffer, but stop before the last character
    // This prevents reading beyond the buffer when checking the character after the current one
    for (int index = 0; index < inbuf - 1; index++) {
        // Check if the current character is '\r' and the next character is '\n'
        if (buf[index] == '\r' && buf[index + 1] == '\n') {
            // Network newline found, return the index immediately following '\n'
            return index + 2;
        }
    }
    // If no network newline is found in the specified range, return -1
    return -1;
}

/*
 * Removes the first line (terminated by a network newline \r\n) from the client's buffer.
 * It updates the client->num_bytes to reflect the new buffer size.
 *
 * For example, if `client->buf` contains "hello\r\ngoodbye\r\nblah", after calling 
 * remove_buffered_line on it, `client->buf` should contain "goodbye\r\nblah".
 * Note that the client buffer is not null-terminated automatically.
 */
void remove_buffered_line(ClientState *clientState) {
    int startOfNextLine = 0;

    // Check if the client's buffer is already empty
    if (clientState->num_bytes == 0) {
        startOfNextLine = -1; // Indicate that the buffer is empty
    } else {
        // Search for a network newline in the buffer
        startOfNextLine = find_network_newline(clientState->buf, clientState->num_bytes);
    }

    // Handling based on the presence or absence of a network newline
    if (startOfNextLine == -1) {
        // If the network newline is not found or buffer is empty, clear the buffer
        memset(clientState->buf, '\0', MAXLINE);
        clientState->num_bytes = 0;
    } else {
        // Update the number of bytes in the buffer excluding the removed line
        clientState->num_bytes -= startOfNextLine;

        // Shift the remaining data to the start of the buffer
        memmove(clientState->buf, clientState->buf + startOfNextLine, clientState->num_bytes);

        // Clear any leftover data at the end of the buffer
        memset(clientState->buf + clientState->num_bytes, '\0', MAXLINE - clientState->num_bytes);
    }
}

/*
 * Reads data from a client's socket and appends it to the client's buffer.
 * This function first clears any processed line from the buffer to make space for new data.
 * It then reads data from the socket into the buffer without overwriting existing unprocessed data.
 * 
 * Parameters:
 * - client: A pointer to the ClientState structure representing the client.
 *
 * Returns:
 * - The number of bytes read from the client socket.
 * - Returns -1 if the read operation fails.
 *
 * Notes:
 * - The function is careful not to overwrite existing data in the buffer and to avoid buffer overflow.
 * - The buffer is not automatically null-terminated by this function.
 * - It updates the client's num_bytes field to reflect the new size of data in the buffer.
 */
int read_from_client(ClientState *client) {
    // Clear out any processed lines from the buffer to free up space for new data.
    remove_buffered_line(client);

    // Read data from the socket into the buffer. We leave one character space for a null-terminator, if needed.
    int bytesRead = read(client->sock, client->buf + client->num_bytes, MAXLINE - client->num_bytes - 1);
    if (bytesRead < 0) {
        perror("Unable to read from socket!");
        return -1;  // Read operation failed
    }

    // Update the total number of unprocessed bytes in the buffer
    client->num_bytes += bytesRead;
    return bytesRead;  // Return the number of bytes read in this operation
}


/*****************************************************************************
 * Parsing the start line of an HTTP request.
 ****************************************************************************/
// Helper function declarations.
void parse_query(ReqData *req, const char *str);
void update_fdata(Fdata *f, const char *str);
void fdata_free(Fdata *f);
void log_request(const ReqData *req);


/*
 * Parses the start line of an HTTP request and initializes the reqData structure in the client state.
 *
 * Parameters:
 * - client: A pointer to ClientState, representing the state of the client, including its buffer and request data.
 *
 * Returns:
 * - 1 if a complete line (terminated by a network newline) is successfully parsed from the client's buffer.
 * - 0 if a complete line has not yet been read (i.e., no network newline found in the buffer).
 *
 * The function first checks for a network newline in the buffer. If found, it proceeds to parse the HTTP
 * method, path, and optionally a query string from the start line of the request. It also initializes
 * additional fields in the reqData structure.
 */
int parse_req_start_line(ClientState *client) {
    int crlf_exists = find_network_newline(client->buf, client->num_bytes);
    if (crlf_exists == -1) {
        return 0; // No complete line yet
    }

    // Allocate memory for reqData
    client->reqData = malloc(sizeof(ReqData));
    if (client->reqData == NULL) {
        perror("Memory allocation failed for reqData");
        exit(1);
    }

    // Extract the method
    char *method = strtok(client->buf, " ");
    if (method == NULL) {
        perror("Failed to parse HTTP method");
        free(client->reqData);
        exit(1);
    }
    client->reqData->method = strdup(method);

    // Extract the path and optionally the query string
    char *path = strtok(NULL, " ?");
    if (path == NULL) {
        perror("Failed to parse HTTP path");
        free(client->reqData->method);
        free(client->reqData);
        exit(1);
    }
    client->reqData->path = strdup(path);

    // Check if there's a query string
    char *query_string = strtok(NULL, " ");
    if (query_string != NULL) {
        parse_query(client->reqData, query_string);
    }

    // Initialize remaining query parameters to NULL
    for (int i = 0; i < MAX_QUERY_PARAMS; i++) {
        if (client->reqData->params[i].name == NULL) {
            client->reqData->params[i].name = NULL;
            client->reqData->params[i].value = NULL;
        }
    }

    // Debugging: Log the request
    log_request(client->reqData);

    return 1;
}


/*
 * Parses the query string of an HTTP request and initializes the query parameters in the reqData structure.
 *
 * Parameters:
 * - req: A pointer to ReqData, representing the request data structure where parsed query parameters are stored.
 * - str: The query string part of the HTTP request URL, typically following the '?' character.
 *
 * This function extracts key-value pairs from the query string and stores them in the reqData structure.
 * It handles up to MAX_QUERY_PARAMS key-value pairs.
 * 
 * The function assumes the query string format as "key1=value1&key2=value2&...".
 * It dynamically allocates memory for each key and value parsed from the query string.
 */
void parse_query(ReqData *req, const char *str) {
    // Check for an empty or NULL query string, or absence of '=' character
    if (!str || strlen(str) == 0 || strchr(str, '=') == NULL) {
        return; // No query parameters to parse
    }

    const char *current = str;
    int index = 0; // Index for storing parsed parameters in req->params

    // Loop through the query string to parse key-value pairs
    while (*current != '\0' && index < MAX_QUERY_PARAMS) {
        // Find the end of the key (marked by '=' character)
        const char *key_end = strchr(current, '=');
        if (!key_end) break; // Break the loop if no '=' found

        // Allocate and store the key
        size_t key_length = key_end - current;
        char *key = malloc(key_length + 1); // Allocate memory for key
        strncpy(key, current, key_length); // Copy key from query string
        key[key_length] = '\0'; // Null-terminate the key

        // Move past '=' to start parsing the value
        current = key_end + 1;
        // Find the end of the value (marked by '&' or end of the string)
        const char *value_end = strchr(current, '&');
        if (!value_end) value_end = current + strlen(current);

        // Allocate and store the value
        size_t value_length = value_end - current;
        char *value = malloc(value_length + 1); // Allocate memory for value
        strncpy(value, current, value_length); // Copy value from query string
        value[value_length] = '\0'; // Null-terminate the value

        // Store the key and value in the reqData structure
        req->params[index].name = key;
        req->params[index].value = value;

        // Move to the next key-value pair
        index++;
        current = value_end;
        if (*current == '&') current++; // Skip '&' for next pair
    }

    // Initialize remaining query parameters in the array to NULL
    for (int i = index; i < MAX_QUERY_PARAMS; i++) {
        req->params[i].name = NULL;
        req->params[i].value = NULL;
    }
}

/*
 * Print information stored in the given request data to stderr.
 */
void log_request(const ReqData *req) {
    fprintf(stderr, "Request parsed: [%s] [%s]\n", req->method, req->path);
    for (int i = 0; i < MAX_QUERY_PARAMS && req->params[i].name != NULL; i++) {
        fprintf(stderr, "  %s -> %s\n", 
                req->params[i].name, req->params[i].value);
    }
}


/******************************************************************************
 * Parsing multipart form data (image-upload)
 *****************************************************************************/

char *get_boundary(ClientState *client) {
    int len_header = strlen(POST_BOUNDARY_HEADER);

    while (1) {
        int where = find_network_newline(client->buf, client->num_bytes);
        if (where > 0) {
            if (where < len_header || strncmp(POST_BOUNDARY_HEADER, client->buf, len_header) != 0) {
                remove_buffered_line(client);
            } else {
                // We've found the boundary string!
                // We are going to add "--" to the beginning to make it easier
                // to match the boundary line later
                char *boundary = malloc(where - len_header + 1);
                strncpy(boundary, "--", where - len_header + 1);
                strncat(boundary, client->buf + len_header, where - len_header - 1);
                boundary[where - len_header] = '\0';
                return boundary;
            }
        } else {
            // Need to read more bytes
            if (read_from_client(client) <= 0) {
                // Couldn't read; this is a bad request, so give up.
                return NULL;
            }
        }
    }
    return NULL;
}


char *get_bitmap_filename(ClientState *client, const char *boundary) {
    int len_boundary = strlen(boundary);

    // Read until finding the boundary string.
    while (1) {
        int where = find_network_newline(client->buf, client->num_bytes);
        if (where > 0) {
            if (where < len_boundary + 2 ||
                    strncmp(boundary, client->buf, len_boundary) != 0) {
                remove_buffered_line(client);
            } else {
                // We've found the line with the boundary!
                remove_buffered_line(client);
                break;
            }
        } else {
            // Need to read more bytes
            if (read_from_client(client) <= 0) {
                // Couldn't read; this is a bad request, so give up.
                return NULL;
            }
        }
    }

    int where = find_network_newline(client->buf, client->num_bytes);

    client->buf[where-1] = '\0';  // Used for strrchr to work on just the single line.
    char *raw_filename = strrchr(client->buf, '=') + 2;
    int len_filename = client->buf + where - 3 - raw_filename;
    char *filename = malloc(len_filename + 1);
    strncpy(filename, raw_filename, len_filename);
    filename[len_filename] = '\0';

    // Restore client->buf
    client->buf[where - 1] = '\n';
    remove_buffered_line(client);
    return filename;
}

/*
 * Read the file data from the socket and write it to the file descriptor
 * file_fd.
 * You know when you have reached the end of the file in one of two ways:
 *    - search for the boundary string in each chunk of data read 
 * (Remember the "\r\n" that comes before the boundary string, and the 
 * "--\r\n" that comes after.)
 *    - extract the file size from the bitmap data, and use that to determine
 * how many bytes to read from the socket and write to the file
 */
int save_file_upload(ClientState *client, const char *boundary, int file_fd) {
    char end_boundary[MAXLINE];
    snprintf(end_boundary, sizeof(end_boundary), "\r\n%s--\r\n", boundary);

    // Buffer to accumulate file data if it doesn't fit in client->buf entirely
    char *file_data_buffer = NULL;
    int file_data_buffer_size = 0;

    // Process data in the initial buffer
    char *start_of_data = strstr(client->buf, "\r\n\r\n");
    if (start_of_data == NULL) {
        perror("Start of data not found");
        fprintf(stderr, "Buffer content: %s\n", client->buf); // Debugging output
        return -1;
    }
    start_of_data += 4; // Move past the "\r\n\r\n"

    // Check and write initial chunk of data
    int initial_data_length = client->num_bytes - (start_of_data - client->buf);
    if (initial_data_length > 0) {
        if (write(file_fd, start_of_data, initial_data_length) != initial_data_length) {
            perror("Failed to write initial data to file");
            return -1;
        }
    }

    while (1) {
        // Read next chunk of data
        int bytes_read = read(client->sock, client->buf, MAXLINE);
        if (bytes_read <= 0) {
            perror("Failed to read from socket");
            free(file_data_buffer);
            return -1;
        }

        // Look for end boundary in the buffer
        char *boundary_loc = strstr(client->buf, end_boundary);
        if (boundary_loc != NULL) {
            // Calculate the size to write, excluding the boundary
            int write_size = boundary_loc - client->buf;

            // Check if there's accumulated data in file_data_buffer
            if (file_data_buffer_size > 0) {
                if (write(file_fd, file_data_buffer, file_data_buffer_size) != file_data_buffer_size ||
                    write(file_fd, client->buf, write_size) != write_size) {
                    perror("Failed to write final data to file");
                    free(file_data_buffer);
                    return -1;
                }
            } else {
                if (write(file_fd, client->buf, write_size) != write_size) {
                    perror("Failed to write final data to file");
                    return -1;
                }
            }
            break; // End boundary found, stop reading
        }

        // Append new data to file_data_buffer
        char *new_buffer = realloc(file_data_buffer, file_data_buffer_size + bytes_read);
        if (new_buffer == NULL) {
            perror("Failed to allocate memory for file data");
            free(file_data_buffer);
            return -1;
        }
        file_data_buffer = new_buffer;
        memcpy(file_data_buffer + file_data_buffer_size, client->buf, bytes_read);
        file_data_buffer_size += bytes_read;
    }

    free(file_data_buffer);
    return 0;
}

