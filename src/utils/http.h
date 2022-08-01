#pragma once

#include "vec.h"
#include "str.h"
#include "slice.h"
#include "map.h"
#include "optional.h"

// TODO change this
#include <socket.h>

namespace http {
    constexpr int req_buf_len = 1024 * 10;

    enum req_type {
        REQ_GET,
        REQ_POST,    
        REQ_HEAD,    
        REQ_PUT,    
        REQ_DELETE  
    };

    enum status_type {
        // 2xx: success
        STATUS_OK              = 200,
        STATUS_CREATED         = 201,
        STATUS_ACCEPTED        = 202,
        STATUS_NO_CONTENT      = 204,
        STATUS_RESET_CONTENT   = 205,
        STATUS_PARTIAL_CONTENT = 206,

        // 3xx: redirection
        STATUS_MULTIPLE_CHOICES  = 300,
        STATUS_MOVED_PERMANENTLY = 301,
        STATUS_MOVED_TEMPORARILY = 302,
        STATUS_NOT_MODIFIED      = 304,

        // 4xx: client error
        STATUS_BAD_REQUEST           = 400,
        STATUS_UNAUTHORIZED          = 401,
        STATUS_FORBIDDEN             = 403,
        STATUS_NOT_FOUND             = 404,
        STATUS_RANGE_NOT_SATISFIABLE = 407,

        // 5xx: server error
        STATUS_INTERNAL_SERVER_ERROR = 500,
        STATUS_NOT_IMPLEMENTED       = 501,
        STATUS_BAD_GATEWAY           = 502,
        STATUS_SERVICE_NOT_AVAILABLE = 503,
        STATUS_GATEWAY_TIMEOUT       = 504,
        STATUS_VERSION_NOT_SUPPORTED = 505,
    };

    enum req_error {
        REQERR_INIT,
        REQERR_OPEN,
        REQERR_STR,
        REQERR_SOCK,
        REQERR_DATA,
        REQERR_CONNECT,
        REQERR_CLOSE,
        REQERR_CLEANUP,
    };

    const char *req_error_str(req_error error);

    struct version {
        int to_int();
        u8 major, minor;
    };

    struct field {
        str key;
        str value;
    };

    struct req {
        void set_uri(str_view uri);

        str to_string();

        req_type method = REQ_GET;
        version ver = { 1, 1 };
        map fields;
        str uri = "/";
        str body;
    };

    struct res {
        void parse(const vec<u8> &data);

        status_type status = STATUS_OK;
        map fields;
        version ver = { 1, 1 };
        vec<u8> data;
    };

    struct client {
        void set_host(str_view hostname);
        optional<res, req_error> send_req(req &request);

        str host_name;
        u16 port = 80;
        socket_t socket;
    };

    optional<res, req_error> get(str_view host, str_view uri);
} // namespace http