// Copyright (c) 2024 Gianluca Mazza
// SPDX-License-Identifier: MIT

#include <doctest/doctest.h>

#include "xllama/sse.h"

#include <string>

using xllama::chunk_final;
using xllama::chunk_frame;
using xllama::sse_done;
using xllama::sse_event;
using xllama::sse_payload_is_safe;
using xllama::sse_response_head;

namespace {
// Reads one chunked frame back: the size line in hex, then the payload.
bool read_chunk(const std::string& data, std::string& payload, size_t& consumed) {
    const size_t crlf = data.find("\r\n");
    if (crlf == std::string::npos)
        return false;
    size_t size = 0;
    for (const char c : data.substr(0, crlf)) {
        int digit = -1;
        if (c >= '0' && c <= '9')
            digit = c - '0';
        else if (c >= 'a' && c <= 'f')
            digit = c - 'a' + 10;
        if (digit < 0)
            return false;
        size = size * 16 + static_cast<size_t>(digit);
    }
    if (data.size() < crlf + 2 + size + 2)
        return false;
    payload = data.substr(crlf + 2, size);
    consumed = crlf + 2 + size + 2;
    return data.compare(consumed - 2, 2, "\r\n") == 0;
}
} // namespace

TEST_CASE("sse: an event is one data line and a blank line") {
    CHECK(sse_event("{\"a\":1}") == "data: {\"a\":1}\n\n");
    CHECK(sse_event("") == "data: \n\n");
    CHECK(sse_done() == "data: [DONE]\n\n");
}

TEST_CASE("sse: a payload that would break the framing is refused") {
    CHECK(sse_payload_is_safe("{\"content\":\"hi\"}"));
    CHECK(sse_payload_is_safe(""));
    CHECK_FALSE(sse_payload_is_safe("two\nlines"));
    CHECK_FALSE(sse_payload_is_safe("two\rlines"));
    CHECK_FALSE(sse_payload_is_safe(std::string("nul\0here", 8)));
}

TEST_CASE("sse: chunked framing matches HTTP/1.1 hex sizes") {
    CHECK(chunk_frame("hello") == "5\r\nhello\r\n");
    CHECK(chunk_frame("") == "0\r\n\r\n");
    // 26 bytes -> 1a in hex: the size line is lower-case hex, no 0x, no padding.
    const std::string twenty_six(26, 'x');
    CHECK(chunk_frame(twenty_six) == "1a\r\n" + twenty_six + "\r\n");
    CHECK(chunk_final() == "0\r\n\r\n");
}

TEST_CASE("sse: an event inside a chunk reads back byte for byte") {
    const std::string json = "{\"content\":\"hola\"}";
    const std::string framed = chunk_frame(sse_event(json));
    std::string payload;
    size_t consumed = 0;
    REQUIRE(read_chunk(framed, payload, consumed));
    CHECK(consumed == framed.size());
    CHECK(payload == "data: " + json + "\n\n");
}

TEST_CASE("sse: the response head is an event stream without Content-Length") {
    const std::string head = sse_response_head();
    CHECK(head.find("Content-Type: text/event-stream") != std::string::npos);
    CHECK(head.find("Transfer-Encoding: chunked") != std::string::npos);
    CHECK(head.find("Content-Length") == std::string::npos);
    CHECK(head.find("Access-Control-Allow-Origin: *") != std::string::npos);
    CHECK(head.size() >= 4);
    CHECK(head.compare(head.size() - 4, 4, "\r\n\r\n") == 0);
}
