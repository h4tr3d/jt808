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

// @File    :  packager.h
// @Version :  1.0
// @Time    :  2020/07/08 16:52:33
// @Author  :  Meng Yuming
// @Contact :  mengyuming@hotmail.com
// @Desc    :  None

#ifndef JT808_PACKAGER_H_
#define JT808_PACKAGER_H_

#include <stdint.h>

#include <functional>
#include <map>
#include <utility>
#include <vector>

#include "jt808/protocol_parameter.h"

namespace libjt808 {

// Message body packaging function definition.
// Returns the total length of the message body (in bytes) on success, -1 on failure.
using PackageHandler = std::function<int(ProtocolParameter const& para, std::vector<uint8_t>* out)>;

// Packager definition, map<key, value>, key: message ID, value: packaging handler function.
using Packager = std::map<uint16_t, PackageHandler>;

// Packager initialization command, provides packaging functionality for some commands.
int JT808FramePackagerInit(Packager* packager);

// Additional commands to support the packager.
bool JT808FramePackagerAppend(Packager* packager, std::pair<uint16_t, PackageHandler> const& pair);
bool JT808FramePackagerAppend(Packager* packager, uint16_t const& msg_id, PackageHandler const& handler);

// Overwrite commands to support the packager.
bool JT808FramePackagerOverride(Packager* packager, std::pair<uint16_t, PackageHandler> const& pair);
bool JT808FramePackagerOverride(Packager* packager, uint16_t const& msg_id, PackageHandler const& handler);

// Packaging command.
int JT808FramePackage(Packager const& packager, ProtocolParameter const& para, std::vector<uint8_t>* out);

} // namespace libjt808

#endif // JT808_PACKAGER_H_
