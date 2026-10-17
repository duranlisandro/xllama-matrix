// Copyright (c) 2024 Gianluca Mazza
// SPDX-License-Identifier: MIT
// Server-Sent Events framing for the streamed chat completion
// (docs/api-endpoint.md). WinRT-free so the host suite can test it; the UWP
// transport only owns the socket and moves the bytes.
#pragma once

#include <string>
#include <string_view>

namespace xllama {

// One SSE event: "data: <payload>\n\n". The OpenAI streaming convention puts one
// JSON object per event and closes the stream with sse_done().
std::string sse_event(std::string_view payload);

// The terminal event of a streamed completion.
std::string sse_done();

// False when the payload holds a byte that would break the framing: an event is
// one line, so CR, LF and NUL are refused instead of emitted. JSON encoding
// escapes them, so this catches a builder that forgot to escape.
bool sse_payload_is_safe(std::string_view payload);

// One HTTP/1.1 chunked-transfer frame: "<hex size>\r\n<payload>\r\n". A
// streamed response carries no Content-Length, so every event travels in a chunk.
std::string chunk_frame(std::string_view payload);

// The last chunk of a chunked response.
std::string chunk_final();

// Response head for a streamed completion.
std::string sse_response_head();

} // namespace xllama
