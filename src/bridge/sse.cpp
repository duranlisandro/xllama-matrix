// Copyright (c) 2024 Gianluca Mazza
// SPDX-License-Identifier: MIT
#include "xllama/sse.h"

#include <charconv>
#include <system_error>

namespace xllama {

std::string sse_event(std::string_view payload) {
    std::string out;
    out.reserve(payload.size() + 8);
    out += "data: ";
    out += payload;
    out += "\n\n";
    return out;
}

std::string sse_done() {
    return sse_event("[DONE]");
}

bool sse_payload_is_safe(std::string_view payload) {
    // The literal holds CR, LF and an explicit NUL; count 3 keeps the implicit
    // terminator out of the set.
    return payload.find_first_of("\r\n\0", 0, 3) == std::string_view::npos;
}

std::string chunk_frame(std::string_view payload) {
    char size_hex[32]{};
    const auto converted = std::to_chars(size_hex, size_hex + sizeof(size_hex), payload.size(), 16);
    if (converted.ec != std::errc{})
        return {};
    std::string out(size_hex, converted.ptr);
    out += "\r\n";
    out += payload;
    out += "\r\n";
    return out;
}

std::string chunk_final() {
    return "0\r\n\r\n";
}

std::size_t sse_utf8_complete_prefix(std::string_view text) {
    std::size_t i = 0;
    std::size_t complete = 0;
    while (i < text.size()) {
        const auto lead = static_cast<unsigned char>(text[i]);
        std::size_t continuation = 0;
        if (lead < 0x80)
            continuation = 0;
        else if ((lead & 0xE0) == 0xC0)
            continuation = 1;
        else if ((lead & 0xF0) == 0xE0)
            continuation = 2;
        else if ((lead & 0xF8) == 0xF0)
            continuation = 3;
        else
            return complete; // not a UTF-8 lead byte: keep it buffered

        if (i + continuation >= text.size())
            return complete; // the sequence is split across pieces
        for (std::size_t k = 1; k <= continuation; ++k) {
            if ((static_cast<unsigned char>(text[i + k]) & 0xC0) != 0x80)
                return complete; // malformed sequence: do not emit it as text
        }
        i += continuation + 1;
        complete = i;
    }
    return complete;
}

std::string sse_response_head() {
    return "HTTP/1.1 200 OK\r\nContent-Type: text/event-stream; charset=utf-8\r\n"
           "Cache-Control: no-cache\r\nTransfer-Encoding: chunked\r\nConnection: close\r\n"
           "Access-Control-Allow-Origin: *\r\n\r\n";
}

} // namespace xllama
