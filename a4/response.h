#ifndef RESPONSE_H_
#define RESPONSE_H_

#include <sys/socket.h>
#include "request.h"


/*
 * Write the main.html response to the given fd.
 * This response dynamically populates the image-filter form with
 * the filenames located in IMAGE_DIR.
 */
void main_html_response(int fd);

/*
 * Write an response for the image-filter route with the given request data.
 */
void image_filter_response(int fd, const ReqData *reqData);


/*
 * Respond to an image-upload request.
 */
void image_upload_response(ClientState *client);


/*
 * The following are generic responses for different HTTP response codes;
 * we have provided these for you to use in various parts of the assignment.
 * Some of them can be customized with a message.
 */
void not_found_response(int fd);
void bad_request_response(int fd, const char *message);
void internal_server_error_response(int fd, const char *message);

// This one takes a resource name instead, and redirects the client
// to that resource.
void see_other_response(int fd, const char *other);

#endif /* RESPONSE_H_*/
