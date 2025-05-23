// MIT License
//
// Copyright (c) 2020 Yuming Meng
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// @File    :  parser.h
// @Version :  1.0
// @Time    :  2020/07/08 17:09:41
// @Author  :  Meng Yuming
// @Contact :  mengyuming@hotmail.com
// @Desc    :  None

#ifndef JT808_PARSER_H_
#define JT808_PARSER_H_

#include <stdint.h>

#include <functional>
#include <map>
#include <system_error>
#include <utility>
#include <vector>

#include "jt808/protocol_parameter.h"

namespace libjt808 {

// auto f = std::errc::address_family_not_supported;

enum class ParserError
{
    Ok,
    MiscError = -1,
    ParametersNull = -2,
    UnesapingError = -3,
    ChecksumError = -4,
    HeaderParseError = -5,
    UnregisteredMessageParser = -6,
};

class ParserCategory : public std::error_category
{
public:

    virtual const char *name() const noexcept override
    {
        return "JT808-Parser";
    }

    virtual std::string message(int ev) const override
    {
        auto code = static_cast<ParserError>(ev);
        switch (code) {
            case ParserError::Ok: return "Success";
            case ParserError::MiscError: return "Misc error";
            case ParserError::ParametersNull: return "ProtocolParameters null";
            case ParserError::UnesapingError: return "UnEscaping process fail";
            case ParserError::ChecksumError: return "Checksum verification error";
            case ParserError::HeaderParseError: return "Header Parser Error";
            case ParserError::UnregisteredMessageParser: return "Message-specific parser is not registered";
        }
        return "Unknown Parser Error";
    }
};

inline const ParserCategory& parser_category() noexcept
{
    static ParserCategory cat;
    return cat;
}

inline std::error_code make_error_code(ParserError errc) noexcept
{
    return std::error_code(static_cast<int>(errc), parser_category());
}

inline std::error_condition make_error_condition(ParserError errc) noexcept
{
    return std::error_condition(static_cast<int>(errc), parser_category());
}


// Message body parsing function definition.
// Returns 0 on success, -1 on failure.
using ParseHandler = std::function<int(InputBuffer in, ProtocolParameter* para)>;

// Parser definition, map<key, value>, key: message ID, value: message body parsing handler.
using Parser = std::map<uint16_t, ParseHandler>;

// Parser initialization command, provides parsing functionality for some commands.
int JT808FrameParserInit(Parser* parser);

// Additional parser support commands.
bool JT808FrameParserAppend(Parser* parser, std::pair<uint16_t, ParseHandler> const& pair);
bool JT808FrameParserAppend(Parser* parser, uint16_t const& msg_id, ParseHandler const& handler);

// Override parser support commands.
bool JT808FrameParserOverride(Parser* parser, std::pair<uint16_t, ParseHandler> const& pair);
bool JT808FrameParserOverride(Parser* parser, uint16_t const& msg_id, ParseHandler const& handler);

// Parse command.
/**
 * @brief Parses a JT808 frame.
 *
 * This function takes a parser, an input vector of bytes, and a protocol parameter
 * structure pointer. It performs the following steps:
 * 1. Checks if the protocol parameter pointer is null.
 * 2. Performs reverse escape on the input vector.
 * 3. Checks the XOR checksum of the resulting vector.
 * 4. Parses the message header from the resulting vector.
 * 5. Sets the phone number in the protocol parameter structure.
 * 6. Finds the message ID in the parser and calls the corresponding function.
 *
 * @param parser The parser containing message ID to function mappings.
 * @param in The input vector of bytes to be parsed.
 * @param para The protocol parameter structure pointer to store parsed data.
 * @return int Returns 0 on success, -1 on failure.
 */
std::error_code JT808FrameParse(Parser const& parser, InputBuffer in, ProtocolParameter* para);

} // namespace libjt808

#endif // JT808_PARSER_H_
