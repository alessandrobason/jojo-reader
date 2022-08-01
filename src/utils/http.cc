#include "http.h"

// TODO change this
#include <strstream.h>

namespace http {
    
    const char *req_error_str(req_error error) {
        switch(error) {
        case REQERR_INIT: return "Couldn't initialize sockets";
        case REQERR_OPEN: return "Couldn't open socket";
        case REQERR_STR: return "Couldn't get string from request";
        case REQERR_SOCK: return "Couldn't send request to socket";
        case REQERR_DATA: return "Couldn't get the data from the serve";
        case REQERR_CONNECT: return "Couldn't connect to host";
        case REQERR_CLOSE: return "Couldn't close socket";
        case REQERR_CLEANUP: return "couldn't clean up sockets";
        }
        return "unrecognised error";
    }

    int version::to_int() {
        return major * 10 + minor;
    }

    void req::set_uri(str_view new_uri) {
        if (new_uri.empty()) return;
        if (false) {
            uri.buf = (char *)realloc(uri.buf, new_uri.len + 1);
            uri[0] = '/';
            memcpy(uri.buf + 1, new_uri.buf, new_uri.len);
            uri[new_uri.len] = '\0';
        }
        else {
            uri = new_uri;
        }
    }

    str req::to_string() {
        auto out = ostrInitLen(1024);

        const char *met = nullptr;
        switch (method) {
        case REQ_GET:    met = "GET";    break;
        case REQ_POST:   met = "POST";   break;
        case REQ_HEAD:   met = "HEAD";   break;
        case REQ_PUT:    met = "PUT";    break;
        case REQ_DELETE: met = "DELETE"; break;
        }

        ostrPrintf(&out, "%s %s HTTP/%hhu.%hhu\r\n",
            met, uri.buf, ver.major, ver.minor
        );

        for (const auto &field : fields) {
            ostrPrintf(&out, "%s: %s\r\n", field.key.buf, field.value.buf);
        }

        ostrAppendview(&out, strvInit("\r\n"));
        if (!body.empty()) {
            ostrAppendview(&out, strvInitLen(body.buf, body.len));
        }

        return { out.buf, out.size };
    }


    void res::parse(const vec<u8> &in_data) {
        auto in = istrInitLen((const char *)in_data.buf, in_data.len);

        char hp[5];
        istrGetstringBuf(&in, hp, 5);
        if (stricmp(hp, "http") != 0) {
            return;
        }
        istrSkip(&in, 1); // skip /
        istrGetu8(&in, &ver.major);
        istrSkip(&in, 1); // skip .
        istrGetu8(&in, &ver.minor);
        istrGeti32(&in, (i32*)&status);

        istrIgnore(&in, '\n');
        istrSkip(&in, 1); // skip \n

        strview_t line;

        do {
            line = istrGetview(&in, '\r');

            size_t pos = strvFind(line, ':', 0);
            if(pos != STRV_NOT_FOUND) {
                strview_t key = strvSubstr(line, 0, pos);
                strview_t value = strvSubstr(line, pos + 2, SIZE_MAX);

                fields.set({ key.buf, key.len }, { value.buf, value.len });
            }

            istrSkip(&in, 2); // skip \r\n
        } while(line.len > 2);

        const char *tran_encoding = fields.get("transfer-encoding");
        if(tran_encoding == NULL || stricmp(tran_encoding, "chunked") != 0) {
            auto data_slice = slice<u8>((u8*)in.cur, in.size - (in.cur - in.start));
            data.append_slice(data_slice);
        }
    }


    void client::set_host(str_view hostname) {
        if (hostname.empty()) return;

        if(hostname.sub(0, 5) == "http:") {
            host_name = hostname.sub(7);
        }
        else if (hostname.sub(0, 6) == "https:") {
            assert(false && "https not supported");
            return;
        }
        else {
            host_name = hostname;
        }
    }

    optional<res, req_error> client::send_req(req &request) {
        optional<res, req_error> out;
        out.success = true;

        assert(!host_name.empty());
        
        if (host_name[host_name.len - 1] == '/') {
            host_name[--host_name.len] = '\0';
        }

        auto &fields = request.fields;

        fields.has_set("Host", host_name.to_slice());

        if (!fields.has("Content-Length")) {
            if (request.body.empty()) {
                fields.set("Content-Length", "0");
            }
            else {
                char buf[20];
                snprintf(buf, sizeof(buf), "%llu", request.body.len);
                fields.set("Content-Length", buf);
            }
        }

        if (request.method == REQ_POST) {
            fields.has_set("Content-Type", "application/x-www-form-urlencoded");
        }

        if (request.ver.to_int() >= 11) {
            fields.has_set("Connection", "close");
        }

        res response;
        vec<u8> received;

        if (!skInit()) {
            out = REQERR_INIT;
            goto skopen_error;
        }

        socket = skOpen(SOCK_TCP);
        if (socket == INVALID_SOCKET) {
            out = REQERR_OPEN;
            goto error;
        }

        if (skConnect(socket, host_name.buf, port)) {
            str req_str = request.to_string();
            if (req_str.empty()) {
                out = REQERR_STR;
                goto error;
            }

            if (skSend(socket, req_str.buf, (int)req_str.len) == SOCKET_ERROR) {
                out = REQERR_SOCK;
                goto error;
            }

            u8 buffer[req_buf_len];
            int read = 0;
            do {
                read = skReceive(socket, buffer, sizeof(buffer));
                if (read == -1) {
                    out = REQERR_DATA;
                    goto error;
                }
                received.append_slice({ buffer, (usize)read });
            } while (read != 0);

            response.parse(received);
        }
        else {
            out = REQERR_CONNECT;
        }

        if (!skClose(socket)) {
            out = REQERR_CLOSE;
        }

        if (out.good()) {
            out = response;
        }

error:
        if (!skCleanup()) {
            out = REQERR_CLEANUP;
        }
skopen_error:
        return out;
    }


    optional<res, req_error> get(str_view host, str_view uri) {
        req request;
        request.set_uri(uri);

        client c;
        c.set_host(host);
        return c.send_req(request);
    }

} // namespace http
