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

// @File    :  server.cc
// @Version :  1.0
// @Time    :  2020/06/24 09:56:45
// @Author  :  Meng Yuming
// @Contact :  mengyuming@hotmail.com
// @Desc    :  None

#include "jt808/server.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <math.h>
#if defined(__linux__)
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#endif

#include <algorithm>
#include <chrono>
#include <fstream>

#include "jt808/socket_util.h"

namespace libjt808 {

namespace {

// Display location report information.
void PrintLocationReportInfo(ProtocolParameter const& para) {
    auto const& basic_info     = para.parse.location_info;
    auto const& extension_info = para.parse.location_extension;
    printf("Location Report:\n");
    printf("  inout area alarm bit: %d\n", basic_info.alarm.bit.in_out_area);
    printf("  position status: %d\n", basic_info.status.bit.positioning);
    printf("  latitude: %.6lf\n", basic_info.latitude * 1e-6);
    printf("  longitude: %.6lf\n", basic_info.longitude * 1e-6);
    printf("  altitude: %d\n", basic_info.altitude);
    printf("  speed: %f\n", basic_info.speed / 10.0f);
    printf("  bearing: %d\n", basic_info.bearing);
    printf("  time: %s\n", basic_info.time.c_str());
    printf("  location extension:\n");
    for (auto const& item : extension_info) {
        printf("    id:%02X, len: %02X, value:", item.first, static_cast<uint8_t>(item.second.size()));
        for (auto const& uch : item.second)
            printf(" %02X", uch);
        printf("\n");
    }
    auto it = extension_info.find(libjt808::kAccessAreaAlarm);
    if (it != extension_info.end()) {
        uint8_t  location_type;
        uint32_t area_route_id;
        uint8_t  direction;
        printf("  in or out area and route information:\n");
        if (libjt808::GetAccessAreaAlarmBody(it->second, &location_type, &area_route_id, &direction) == 0) {
            printf("    location type: %d\n", location_type);
            printf("    id: %04X\n", area_route_id);
            printf("    direction: %d\n", direction);
        }
    }
}

// Display terminal parameters.
void PrintTerminalParameter(ProtocolParameter const& para) {
    std::string str;
    printf("Terminal Parameters:\n");
    if (!para.terminal_parameter_ids.empty()) {
        for (auto const& id : para.terminal_parameter_ids) {
            auto const& it = para.parse.terminal_parameters.find(id);
            if (it != para.parse.terminal_parameters.end()) {
                printf("  ID:%08X, Length:%d, Value:", it->first, static_cast<int>(it->second.size()));
                for (auto const& uch : it->second)
                    printf(" %02X", uch);
                printf("\n");
            }
        }
    }
    else {
        for (auto const& item : para.parse.terminal_parameters) {
            printf("  ID:%08X, Value:", item.first);
            for (auto const& uch : item.second)
                printf(" %02X", uch);
            printf("\n");
        }
    }
}

} // namespace

// Initialize some parameters.
void JT808Server::Init(void) {
    ip_   = std::move("127.0.0.1");
    port_ = 8888;
    // Maximum number of socket connections.
    max_connection_num_ = 10;
    // Initialize the command parser and packager.
    JT808FrameParserInit(&parser_);
    JT808FramePackagerInit(&packager_);
    // Initialize thread running status.
    waiting_is_running_.store(false);
    service_is_running_.store(false);
}

// Create a socket and bind it to the specified IP and port.
int JT808Server::InitServer(void) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(static_cast<uint16_t>(port_));
#if defined(__linux__)
    addr.sin_addr.s_addr = inet_addr(ip_.c_str());
    listen_              = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_ == -1) {
        printf("%s[%d]: Create socket failed!!!\n", __FUNCTION__, __LINE__);
        return -1;
    }
#elif defined(_WIN32)
    WSADATA ws_data;
    if (WSAStartup(MAKEWORD(2, 2), &ws_data) != 0) {
        return -1;
    }
    listen_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_ == INVALID_SOCKET) {
        printf("%s[%d]: Create socket failed!!!\n", __FUNCTION__, __LINE__);
        WSACleanup();
        return -1;
    }
    addr.sin_addr.S_un.S_addr = inet_addr(ip_.c_str());
#endif
    if (Bind(listen_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == -1) {
        printf("%s[%d]: Connect to remote server failed!!!\n", __FUNCTION__, __LINE__);
        Close(listen_);
#if defined(_WIN32)
        WSACleanup();
#endif
        return -1;
    }
    is_ready_.store(true);
    return 0;
}

// Start threads for waiting for client connections and communicating with clients.
void JT808Server::Run(void) {
    if (!is_ready_)
        return;
    service_thread_ = std::thread(&JT808Server::ServiceHandler, this);
    service_thread_.detach();
    waiting_thread_ = std::thread(&JT808Server::WaitHandler, this);
    waiting_thread_.detach();
}

// Stop the service thread, close connections, and clear sockets.
void JT808Server::Stop(void) {
    if (listen_ > 0) {
        service_is_running_.store(false);
        waiting_is_running_.store(false);
        std::this_thread::sleep_for(std::chrono::seconds(3));
        for (auto& socket : clients_) {
            Close(socket.first);
        }
        clients_.erase(clients_.begin(), clients_.end());
        Close(listen_);
        listen_ = 0;
#if defined(_WIN32)
        WSACleanup();
#endif
        is_ready_.store(false);
    }
}

int JT808Server::UpgradeRequest(decltype(socket(0, 0, 0)) const& socket, int const& upgrade_type,
                                std::vector<uint8_t> const& manufacturer_id, std::string const& version_id,
                                char const* path) {
    std::ifstream ifs;
    ifs.open(path, std::ios::in | std::ios::binary);
    if (!ifs.is_open()) {
        printf("%s[%d]: Updrade file open failed !!!\n", __FUNCTION__, __LINE__);
        return -1;
    }
    ifs.seekg(0, std::ios::end);
    size_t length = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    std::unique_ptr<char[]> buffer(new char[length], std::default_delete<char[]>());
    ifs.read(buffer.get(), length);
    ifs.close();
    is_upgrading_clients_.insert(std::make_pair(socket, 0));
    auto& para = clients_[socket];
    para.upgrade_info.manufacturer_id.assign(manufacturer_id.begin(), manufacturer_id.end());
    para.upgrade_info.upgrade_type = upgrade_type;
    para.upgrade_info.version_id   = version_id;
    uint16_t max_content           = 1023 - 9 - para.upgrade_info.version_id.size();
    if (length > max_content) {                    // Need to handle packet segmentation.
        para.msg_head.msgbody_attr.bit.packet = 1; // Perform packet segmentation.
        para.msg_head.total_packet            = static_cast<uint16_t>(ceil(length * 1.0 / max_content));
        para.msg_head.packet_seq              = 1;
        size_t len                            = 0;
        for (size_t i = 0; i < length; i += max_content) {
            len = length - i;
            if (len > max_content)
                len = max_content;
            para.upgrade_info.upgrade_data.assign(buffer.get() + i, buffer.get() + i + len);
            if (PackagingAndSendMessage(socket, kTerminalUpgrade, &para) < 0) {
                is_upgrading_clients_.erase(socket);
                return -1;
            }
            if (ReceiveAndParseMessage(socket, 5, &para) < 0) {
                is_upgrading_clients_.erase(socket);
                return -1;
            }
            if (para.parse.msg_head.msg_id != kTerminalGeneralResponse ||
                para.parse.respone_msg_id != kTerminalUpgrade || para.parse.respone_result != kSuccess) {
                is_upgrading_clients_.erase(socket);
                return -1;
            }
            ++para.msg_head.packet_seq;
        }
        para.msg_head.msgbody_attr.bit.packet = 0;
        para.msg_head.total_packet            = 1;
    }
    else {
        para.upgrade_info.upgrade_data.assign(buffer.get(), buffer.get() + length);
        if (PackagingAndSendMessage(socket, kTerminalUpgrade, &para) < 0) {
            is_upgrading_clients_.erase(socket);
            return -1;
        }
        if (ReceiveAndParseMessage(socket, 5, &para) < 0) {
            is_upgrading_clients_.erase(socket);
            return -1;
        }
        if (para.parse.respone_msg_id != kTerminalUpgrade || para.parse.respone_result != kSuccess) {
            is_upgrading_clients_.erase(socket);
            return -1;
        }
    }
    is_upgrading_clients_.erase(socket);
    return 0;
}

// Generate the corresponding JT808 format message based on the provided message ID and the parameters set before
// calling this function, and send it to the server through the socket.
int JT808Server::PackagingAndSendMessage(decltype(socket(0, 0, 0)) const& socket, uint32_t const& msg_id,
                                         ProtocolParameter* para) {
    std::vector<uint8_t> msg;
    para->msg_head.msg_id = msg_id; // Set message ID.
    if (JT808FramePackage(packager_, *para, &msg) < 0) {
        printf("%s[%d]: Package message failed !!!\n", __FUNCTION__, __LINE__);
        return -1;
    }
    ++para->msg_head.msg_flow_num; // Increment message flow number for each successfully generated command.
    if (Send(socket, reinterpret_cast<char*>(msg.data()), msg.size(), 0) <= 0) {
        printf("%s[%d]: Send message failed !!!\n", __FUNCTION__, __LINE__);
        return -2;
    }
    return 0;
}

// Blocking receive data from the socket connection once, then parse it according to the JT808 protocol.
int JT808Server::ReceiveAndParseMessage(decltype(socket(0, 0, 0)) const& socket, int const& timeout,
                                        ProtocolParameter* para) {
    std::vector<uint8_t>    msg;
    int                     ret        = -1;
    int                     timeout_ms = timeout * 1000; // Timeout period in milliseconds.
    auto                    tp         = std::chrono::steady_clock::now();
    std::unique_ptr<char[]> buffer(new char[4096], std::default_delete<char[]>());
    while (1) {
        if ((ret = Recv(socket, buffer.get(), 4096, 0)) > 0) {
            msg.assign(buffer.get(), buffer.get() + ret);
            break;
        }
        else if (ret == 0) {
            printf("%s[%d]: Disconnect !!!\n", __FUNCTION__, __LINE__);
            return -2;
        }
        else {
            // TODO: Handle other connection errors.
        }
        // Check for timeout exit.
        if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tp).count() >=
            timeout_ms) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (msg.empty())
        return -2;
    // Parse the message.
    if (JT808FrameParse(parser_, msg, para) == -1) {
        printf("%s[%d]: Parse message failed !!!\n", __FUNCTION__, __LINE__);
        return -1;
    }
    return 0;
}

// Client connection waiting thread handler function.
// If a client connects, perform registration and authentication operations first.
// After successful authentication, data exchange will be transferred to the main service thread.
void JT808Server::WaitHandler(void) {
    waiting_is_running_.store(true);
    if (Listen(listen_, max_connection_num_) < 0) {
        waiting_is_running_.store(false);
        Stop();
        return;
    }
    struct sockaddr_in addr;
    int                len = sizeof(addr);
    while (waiting_is_running_) {
        auto socket = Accept(listen_, reinterpret_cast<struct sockaddr*>(&addr), &len);
        if (socket <= 0) {
            printf("%s[%d]: Invalid socket!!!\n", __FUNCTION__, __LINE__);
            break;
        }
        ProtocolParameter para {};
        if (ReceiveAndParseMessage(socket, 3, &para) < 0 || para.parse.msg_head.msg_id != kTerminalRegister) {
            Close(socket);
            continue;
        }
        // Generate authentication code.
        srand(time(NULL));
        std::string tmp(std::to_string(rand()));
        para.authentication_code.assign(tmp.begin(), tmp.end());
        para.respone_result = kRegisterSuccess;
        if (PackagingAndSendMessage(socket, kTerminalRegisterResponse, &para) < 0) {
            Close(socket);
            continue;
        }
        // Wait for the authentication code to be returned.
        if (ReceiveAndParseMessage(socket, 3, &para) < 0) {
            Close(socket);
            continue;
        }
        // Parse the returned message and compare the authentication code.
        if (para.parse.msg_head.msg_id != kTerminalAuthentication ||
            para.authentication_code != para.parse.authentication_code) {
            Close(socket);
            continue;
        }
        para.respone_result = kSuccess;
        if (PackagingAndSendMessage(socket, kPlatformGeneralResponse, &para) < 0) {
            Close(socket);
            continue;
        }
        // printf("Connected\n");
        // Set non-blocking mode.
#if defined(__linux__)
        int flags = fcntl(socket, F_GETFL, 0);
        fcntl(socket, F_SETFL, flags | O_NONBLOCK);
#elif defined(_WIN32)
        unsigned long ul = 1;
        if (ioctlsocket(socket, FIONBIO, (unsigned long*)&ul) == SOCKET_ERROR) {
            printf("%s[%d]: Set socket nonblock failed!!!\n", __FUNCTION__, __LINE__);
            Close(socket);
            continue;
        }
#endif
        clients_.insert(std::make_pair(socket, para));
    }
    waiting_is_running_.store(false);
    Stop();
}

// Main service thread, handles connected client threads.
// Currently supports displaying location report information and terminal parameter query responses.
// For all non-response commands, it temporarily responds with a platform general response, with a response result of 0.
// When a client connection is disconnected, the related socket and terminal parameters are removed.
void JT808Server::ServiceHandler(void) {
    service_is_running_.store(true);
    int                     ret   = -1;
    bool                    alive = false;
    std::unique_ptr<char[]> buffer(new char[4096], std::default_delete<char[]>());
    std::vector<uint8_t>    msg;
    std::vector<uint16_t>   response_cmd = {kResponseCommand,
                                            kResponseCommand + sizeof(kResponseCommand) / sizeof(kResponseCommand[0])};
    std::unique_ptr<char[]> data_buffer;
    int                     total_size      = 0;
    int                     packet_max_size = 0;
    while (service_is_running_) {
        for (auto& socket : clients_) {
            // Upgrade requests are not handled here.
            if (is_upgrading_clients_.find(socket.first) != is_upgrading_clients_.end()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            if ((ret = Recv(socket.first, buffer.get(), 4096, 0)) > 0) {
                if (!alive)
                    alive = true;
                msg.assign(buffer.get(), buffer.get() + ret);
                // printf("Recv[%d]: ", ret);
                // for (auto const& ch : msg) printf("%02X ", ch);
                // printf("\n");
                if (JT808FrameParse(parser_, msg, &socket.second) == 0) {
                    socket.second.respone_result = kSuccess;
                    auto const& msg_id           = socket.second.parse.msg_head.msg_id;
                    if (msg_id == kLocationReport) {
                        PrintLocationReportInfo(socket.second);
                    }
                    else if (msg_id == kGetTerminalParametersResponse) {
                        PrintTerminalParameter(socket.second);
                    }
                    else if (msg_id == kMultimediaDataUpload) { // Multimedia data upload.
                        // TODO: No packet integrity check is performed.
                        auto&       media       = socket.second.parse.multimedia_upload;
                        auto const& msg_head    = socket.second.parse.msg_head;
                        auto const& packet_size = media.media_data.size();
                        // Check for packet segmentation.
                        if (msg_head.msgbody_attr.bit.packet == 1) { // Segmented packet.
                            // Allocate space.
                            if (msg_head.packet_seq == 1) { // First packet.
                                int max_len = (1023 - 36) * msg_head.total_packet;
                                data_buffer = std::move(
                                    std::unique_ptr<char[]>(new char[max_len], std::default_delete<char[]>()));
                                // Maximum data length of sub-packet.
                                packet_max_size = packet_size;
                                total_size      = 0;
                            }
                            memcpy(&(data_buffer[packet_max_size * (msg_head.packet_seq - 1)]), media.media_data.data(),
                                   packet_size);
                            total_size += packet_size;
                            socket.second.respone_result = kSuccess;
                            if (PackagingAndSendMessage(socket.first, kPlatformGeneralResponse, &socket.second) < 0) {
                                printf("%s[%d]: Disconnect !!!\n", __FUNCTION__, __LINE__);
                                data_buffer.reset();
                                Close(socket.first);
                                clients_.erase(socket.first);
                                break; // When deleting a connection, do not continue traversing, but restart
                                       // traversing.
                            }
                            // Wait for all data to be transmitted.
                            if (msg_head.packet_seq == msg_head.total_packet) {
                                media.media_data.clear();
                                media.media_data.assign(data_buffer.get(), data_buffer.get() + total_size);
                                multimedia_data_upload_callback_(media);
                                media.media_data.clear();
                                media.loaction_report_body.clear();
                                data_buffer.reset();
                                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                                // Temporarily return success directly.
                                auto& resp    = socket.second.multimedia_upload_response;
                                resp.media_id = media.media_id;
                                resp.reload_packet_ids.clear();
                                if (PackagingAndSendMessage(socket.first, kMultimediaDataUploadResponse,
                                                            &socket.second) < 0) {
                                    printf("%s[%d]: Disconnect !!!\n", __FUNCTION__, __LINE__);
                                    Close(socket.first);
                                    clients_.erase(socket.first);
                                    break; // When deleting a connection, do not continue traversing, but restart
                                           // traversing.
                                }
                            }
                        }
                        else { // Not segmented.
                            multimedia_data_upload_callback_(media);
                            media.media_data.clear();
                            media.loaction_report_body.clear();
                            socket.second.multimedia_upload_response.media_id = media.media_id;
                            if (PackagingAndSendMessage(socket.first, kMultimediaDataUploadResponse, &socket.second) <
                                0) {
                                printf("%s[%d]: Disconnect !!!\n", __FUNCTION__, __LINE__);
                                Close(socket.first);
                                clients_.erase(socket.first);
                                break; // When deleting a connection, do not continue traversing, but restart
                                       // traversing.
                            }
                        }
                    }
                    // For non-response commands, the default is to use the platform general response.
                    if (find(response_cmd.begin(), response_cmd.end(), msg_id) == response_cmd.end()) {
                        if (PackagingAndSendMessage(socket.first, kPlatformGeneralResponse, &socket.second) < 0) {
                            printf("%s[%d]: Disconnect !!!\n", __FUNCTION__, __LINE__);
                            Close(socket.first);
                            clients_.erase(socket.first);
                            break; // When deleting a connection, do not continue traversing, but restart traversing.
                        }
                    }
                }
                continue;
            }
            else if (ret <= 0) {
                if (ret < 0) {
#if defined(__linux__)
                    if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                        continue;
                    }
#elif defined(_WIN32)
                    auto wsa_errno = WSAGetLastError();
                    if (wsa_errno == WSAEINTR || wsa_errno == WSAEWOULDBLOCK)
                        continue;
#endif
                }
                printf("%s[%d]: Disconnect !!!\n", __FUNCTION__, __LINE__);
                Close(socket.first);
                clients_.erase(socket.first);
                if (!alive)
                    alive = true;
                break; // When deleting a connection, do not continue traversing, but restart traversing.
            }
        }
        if (!alive) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        alive = false;
    }
    service_is_running_.store(false);
    Stop();
}

} // namespace libjt808
