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

// @File    :  jt808_terminal_parameter_config.cc
// @Version :  1.0
// @Time    :  2020/07/16 17:53:18
// @Author  :  Meng Yuming
// @Contact :  mengyuming@hotmail.com
// @Desc    :  None

#include <stdio.h>
#include <math.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "jt808/packager.h"
#include "jt808/parser.h"
#include "jt808/terminal_parameter.h"

using std::cin;
using std::cout;

using libjt808::GetTerminalParameter;
using libjt808::SetTerminalParameter;

namespace {

// 自定义的终端参数.
enum CustomTerminalParameterID {
    //
    // NTRIP CORS differential station.
    //
    // STRING,  Address.
    kNtripCorsIP = 0xF020,
    // WORD,  Port.
    kNtripCorsPort = 0xF021,
    // STRING,  Username.
    kNtripCorsUser = 0xF022,
    // STRING,  Password.
    kNtripCorsPasswd = 0xF023,
    // STRING,  Mount point.
    kNtripCorsMountPoint = 0xF024,
    // BYTE,  GGA report interval.
    kNtripCorsGGAReportInterval = 0xF025,
    // BYTE,  Enable module at startup. 0: Disable; 1: Enable.
    kNtripCorsStartup = 0xF026,
};

//
// Custom terminal parameter item packaging/parsing.
//
// Package Ntrip CORS differential station configuration.
// Args:
//    ip:  Ntrip CORS server IP address.
//    port:  Ntrip CORS server port.
//    user:  Ntrip CORS server username.
//    pwd:  Ntrip CORS server password.
//    mp:  Ntrip CORS server mount point.
//    intv:  Ntrip CORS server GGA statement reporting interval.
//    startup:  Ntrip CORS differential startup enable status, 0 - off, 1 - on.
//    items:  Map container to save the parsed terminal parameter items.
// Returns:
//    Returns 0 on success, -1 on failure.
inline int PackagingTerminalParameterNtripCors(std::string const& ip, uint16_t const& port, std::string const& user,
                                               std::string const& pwd, std::string const& mp, uint8_t const& intv,
                                               uint8_t const& startup, libjt808::TerminalParameters* items) {
    int ret = !SetTerminalParameter(kNtripCorsIP, ip, items) && !SetTerminalParameter(kNtripCorsPort, port, items) &&
              !SetTerminalParameter(kNtripCorsUser, user, items) &&
              !SetTerminalParameter(kNtripCorsPasswd, pwd, items) &&
              !SetTerminalParameter(kNtripCorsMountPoint, mp, items) &&
              !SetTerminalParameter(kNtripCorsGGAReportInterval, intv, items) &&
              !SetTerminalParameter(kNtripCorsStartup, startup, items);
    return (ret > 0 ? 0 : -1);
}

// Parse Ntrip CORS differential station configuration.
// Args:
//    items:  Map container of terminal parameter items.
//    ip:  Pointer to store the parsed Ntrip CORS server IP address.
//    port:  Pointer to store the parsed Ntrip CORS server port.
//    user:  Pointer to store the parsed Ntrip CORS server username.
//    pwd:  Pointer to store the parsed Ntrip CORS server password.
//    mp:  Pointer to store the parsed Ntrip CORS server mount point.
//    intv:  Pointer to store the parsed Ntrip CORS server GGA statement reporting interval.
//    startup: Pointer to store the parsed Ntrip CORS server startup enable status.
// Returns:
//    Returns 0 on success, -1 on failure.
inline int ParseTerminalParameterNtripCors(libjt808::TerminalParameters const& items, std::string* ip, uint16_t* port,
                                           std::string* user, std::string* pwd, std::string* mp, uint8_t* intv,
                                           uint8_t* startup) {
    int ret = !GetTerminalParameter(items, kNtripCorsIP, ip) && !GetTerminalParameter(items, kNtripCorsPort, port) &&
              !GetTerminalParameter(items, kNtripCorsUser, user) &&
              !GetTerminalParameter(items, kNtripCorsPasswd, pwd) &&
              !GetTerminalParameter(items, kNtripCorsMountPoint, mp) &&
              !GetTerminalParameter(items, kNtripCorsGGAReportInterval, intv) &&
              !GetTerminalParameter(items, kNtripCorsStartup, startup);
    return (ret > 0 ? 0 : -1);
}

} // namespace

int main(int argc, char** argv) {
    //
    // 808 Initialization.
    //
    // Protocol parameters
    libjt808::ProtocolParameter svr_para {};
    libjt808::ProtocolParameter cli_para {};
    cli_para.msg_head.phone_num    = std::move(std::string("13523339527"));
    cli_para.msg_head.msg_flow_num = 1;
    svr_para.msg_head.phone_num    = std::move(std::string("13523339527"));
    svr_para.msg_head.msg_flow_num = 1;
    // Command packager initialization.
    libjt808::Packager jt808_packager;
    libjt808::JT808FramePackagerInit(&jt808_packager);
    // Command parser initialization.
    libjt808::Parser jt808_parser;
    libjt808::JT808FrameParserInit(&jt808_parser);
    std::vector<uint8_t> out;
    // Set up some terminal parameters.
    PackagingTerminalParameterNtripCors("192.168.3.111", 8002, "user01", "123456", "RTCM23_GPS", 10, 1,
                                        &svr_para.terminal_parameters);
    std::string ip;
    uint16_t    port;
    std::string usr;
    std::string pwd;
    std::string mp;
    uint8_t     intv     = 0;
    uint8_t     start_up = 0;
    // Output the configured terminal parameters.
    if (ParseTerminalParameterNtripCors(svr_para.terminal_parameters, &ip, &port, &usr, &pwd, &mp, &intv, &start_up) ==
        0) {
        cout << "Set para: " << ip << ", " << port << ", " << usr << ", " << pwd << ", " << mp << ", "
             << std::to_string(intv) << ", " << std::to_string(start_up) << "\n";
    }
    // Platform generates a message to set terminal parameters.
    svr_para.msg_head.msg_id = libjt808::kSetTerminalParameters;
    if (libjt808::JT808FramePackage(jt808_packager, svr_para, out) < 0) {
        printf("Generate message failed\n");
        return -1;
    }
    ++svr_para.msg_head.msg_flow_num;
    // printf("Raw message: ");
    // for (auto const& uch : out) printf("%02X ", uch);
    // printf("\n");
    // Terminal parses the message to set terminal parameters.
    if (libjt808::JT808FrameParse(jt808_parser, out, &cli_para)) {
        printf("Parse message failed\n");
        return -1;
    }
    // Copy terminal parameters.
    for (auto const& it : cli_para.parse.terminal_parameters) {
        cli_para.terminal_parameters.insert(it);
    }
    // Terminal generates a general response.
    cli_para.msg_head.msg_id = libjt808::kTerminalGeneralResponse;
    cli_para.respone_result  = libjt808::kSuccess;
    if (libjt808::JT808FramePackage(jt808_packager, cli_para, out) < 0) {
        printf("Generate message failed\n");
        return -1;
    }
    ++cli_para.msg_head.msg_flow_num;
    // printf("Raw message: ");
    // for (auto const& uch : out) printf("%02X ", uch);
    // printf("\n");
    // Platform generates a message to query terminal parameters.
    svr_para.msg_head.msg_id = libjt808::kGetTerminalParameters;
    if (libjt808::JT808FramePackage(jt808_packager, svr_para, out) < 0) {
        printf("Generate message failed\n");
        return -1;
    }
    ++svr_para.msg_head.msg_flow_num;
    // printf("Raw message: ");
    // for (auto const& uch : out) printf("%02X ", uch);
    // printf("\n");
    // Terminal parses the message to query terminal parameters.
    if (libjt808::JT808FrameParse(jt808_parser, out, &cli_para)) {
        printf("Parse message failed\n");
        return -1;
    }
    // Terminal generates a response message for querying terminal parameters.
    cli_para.msg_head.msg_id = libjt808::kGetTerminalParametersResponse;
    if (libjt808::JT808FramePackage(jt808_packager, cli_para, out) < 0) {
        printf("Generate message failed\n");
        return -1;
    }
    ++cli_para.msg_head.msg_flow_num;
    // printf("Raw message: ");
    // for (auto const& uch : out) printf("%02X ", uch);
    // printf("\n");
    // Platform parses the response for querying terminal parameters.
    if (libjt808::JT808FrameParse(jt808_parser, out, &svr_para)) {
        printf("Parse message failed\n");
        return -1;
    }
    // Output the parsed terminal parameters.
    if (ParseTerminalParameterNtripCors(svr_para.parse.terminal_parameters, &ip, &port, &usr, &pwd, &mp, &intv,
                                        &start_up) == 0) {
        cout << "Get all para: " << ip << ", " << port << ", " << usr << ", " << pwd << ", " << mp << ", "
             << std::to_string(intv) << ", " << std::to_string(start_up) << "\n";
    }
    // Platform generates a message to query specific terminal parameters.
    svr_para.terminal_parameter_ids.clear();
    svr_para.terminal_parameter_ids.push_back(kNtripCorsIP);
    svr_para.terminal_parameter_ids.push_back(kNtripCorsPort);
    svr_para.terminal_parameter_ids.push_back(kNtripCorsUser);
    svr_para.terminal_parameter_ids.push_back(kNtripCorsPasswd);
    svr_para.terminal_parameter_ids.push_back(kNtripCorsMountPoint);
    svr_para.terminal_parameter_ids.push_back(kNtripCorsGGAReportInterval);
    svr_para.msg_head.msg_id = libjt808::kGetSpecificTerminalParameters;
    if (libjt808::JT808FramePackage(jt808_packager, svr_para, out) < 0) {
        printf("Generate message failed\n");
        return -1;
    }
    ++svr_para.msg_head.msg_flow_num;
    // printf("Raw message: ");
    // for (auto const& uch : out) printf("%02X ", uch);
    // printf("\n");
    // Terminal parses the message to query terminal parameters.
    cli_para.parse.terminal_parameter_ids.clear();
    if (libjt808::JT808FrameParse(jt808_parser, out, &cli_para)) {
        printf("Parse message failed\n");
        return -1;
    }
    // Copy the queried terminal parameter IDs.
    cli_para.terminal_parameter_ids.clear();
    for (auto const& it : cli_para.parse.terminal_parameter_ids) {
        cli_para.terminal_parameter_ids.push_back(it);
    }
    // Terminal generates a response message for querying terminal parameters.
    cli_para.msg_head.msg_id = libjt808::kGetTerminalParametersResponse;
    if (libjt808::JT808FramePackage(jt808_packager, cli_para, out) < 0) {
        printf("Generate message failed\n");
        return -1;
    }
    ++cli_para.msg_head.msg_flow_num;
    // printf("Raw message: ");
    // for (auto const& uch : out) printf("%02X ", uch);
    // printf("\n");
    // Platform parses the response for querying terminal parameters.
    svr_para.parse.terminal_parameters.clear();
    if (libjt808::JT808FrameParse(jt808_parser, out, &svr_para)) {
        printf("Parse message failed\n");
        return -1;
    }
    // Output the parsed terminal parameters.
    if (ParseTerminalParameterNtripCors(svr_para.parse.terminal_parameters, &ip, &port, &usr, &pwd, &mp, &intv,
                                        &start_up) == 0) {
        cout << "Get special para: " << ip << ", " << port << ", " << usr << ", " << pwd << ", " << mp << ", "
             << std::to_string(intv) << ", " << std::to_string(start_up) << "\n";
    }
    return 0;
}
