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

// @File    :  server.h
// @Version :  1.0
// @Time    :  2020/06/24 09:59:07
// @Author  :  Meng Yuming
// @Contact :  mengyuming@hotmail.com
// @Desc    :  None

#ifndef JT808_SERVER_H_
#define JT808_SERVER_H_

#if defined(__linux__)
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <winsock2.h>
#include <windows.h>
#endif

#include <atomic>
#include <functional>
#include <string>
#include <thread>
#include <vector>
#include <map>

#include "packager.h"
#include "parser.h"
#include "protocol_parameter.h"
#include "terminal_parameter.h"

namespace libjt808 {

/**
 * @brief JT808 platform.
 *
 * Implemented terminal registration, terminal authentication, heartbeat packet, and location information reporting
 * functions. Temporarily responds to all commands with a general platform response after successful authentication.
 *
 * @example:
 *
 * JT808Server server;
 * server.Init();
 * server.SetServerAccessPoint("127.0.0.1", 8888);
 * if ((server.InitServer() == 0)) {
 *     server.Run();
 *     std::this_thread::sleep_for(std::chrono::seconds(1));
 *     while (server.service_is_running()) {
 *         std::this_thread::sleep_for(std::chrono::seconds(1));
 *     }
 *     server.Stop();
 * }
 *
 */

class JT808Server {
public:
    JT808Server() {
    }

    ~JT808Server() {
    }

    // Parameter initialization.
    void Init(void);

    // Set server address.
    void SetServerAccessPoint(std::string const& ip, int const& port) {
        ip_   = ip;
        port_ = port;
    }

    // Initialize server.
    int InitServer(void);

    //
    // Service thread run and stop.
    //
    // Start service thread.
    void Run(void);
    // Stop service thread.
    void Stop(void);

    // Get current service thread running status.
    bool service_is_running(void) const {
        return service_is_running_;
    }

    //
    // External access and setting of the current general message body parsing and packaging functions,
    // used for overriding or adding command support.
    // Must be used after calling the Init() member function.
    //
    // Get general JT808 protocol packager.
    Packager& packager(void) {
        return packager_;
    }

    Packager const& packager(void) const {
        return packager_;
    }

    void packager(Packager* packager) const {
        if (packager == nullptr)
            return;
        *packager = packager_;
    }

    // Set general JT808 protocol packager.
    void set_packager(Packager const& packager) {
        packager_ = packager;
    }

    // Get general JT808 protocol parser.
    Parser& parser(void) {
        return parser_;
    }

    Parser const& parser(void) const {
        return parser_;
    }

    void parser(Parser* parser) const {
        if (parser == nullptr)
            return;
        *parser = parser_;
    }

    // Set general JT808 protocol parser.
    void set_parser(Parser const& parser) {
        parser_ = parser;
    }

    /**
     * @brief Sends an upgrade request to the client.
     *
     * This method sends an upgrade request to the client using the specified socket, upgrade type, manufacturer ID,
     * version ID, and upgrade file path.
     *
     * @param socket The client's socket.
     * @param upgrade_type The type of upgrade.
     * @param manufacturer_id A vector of bytes representing the manufacturer ID.
     * @param version_id A string representing the version ID.
     * @param path The file path of the upgrade file.
     * @return Returns 0 on success, -1 on failure.
     */
    int UpgradeRequest(decltype(socket(0, 0, 0)) const& socket, int const& upgrade_type,
                       std::vector<uint8_t> const& manufacturer_id, std::string const& version_id, char const* path);

    /**
     * @brief Sends an upgrade request to the client identified by phone number.
     *
     * This method sends an upgrade request to the client identified by the specified phone number, using the provided
     * upgrade type, manufacturer ID, version ID, and upgrade file path.
     *
     * @param phone The client's terminal phone number.
     * @param upgrade_type The type of upgrade.
     * @param manufacturer_id A vector of bytes representing the manufacturer ID.
     * @param version_id A string representing the version ID.
     * @param path The file path of the upgrade file.
     * @return Returns 0 on success, -1 on failure.
     */
    int UpgradeRequestByPhoneNumber(std::string const& phone, int const& upgrade_type,
                                    std::vector<uint8_t> const& manufacturer_id, std::string const& version_id,
                                    char const* path) {
        for (auto const& item : clients_) {
            if (item.second.msg_head.phone_num == phone) {
                return UpgradeRequest(item.first, upgrade_type, manufacturer_id, version_id, path);
            }
        }
        return -1;
    }

    //
    // Multimedia data upload.
    //
    using MultimediaDataUploadCallback = std::function<void(MultiMediaDataUpload const&)>;

    void OnMultimediaDataUploaded(MultimediaDataUploadCallback const& callback) {
        multimedia_data_upload_callback_ = callback;
    }

    // General message packaging and sending function.
    // Args:
    //     socket:  Client's socket.
    //     msg_id:  Message ID.
    //     para: Protocol parameters.
    // Returns:
    //     Returns 0 on success, -1 on failure.
    int PackagingAndSendMessage(decltype(socket(0, 0, 0)) const& socket, uint32_t const& msg_id,
                                ProtocolParameter* para);

    // General message receiving and parsing function.
    // Blocking function.
    // Clients that have passed authentication are prohibited from calling.
    // Args:
    //     client:  Client's socket.
    //     timeout:  Timeout period, in seconds (s).
    //     para: Protocol parameters.
    // Returns:
    //     Returns 0 on success, -1 on failure.
    int ReceiveAndParseMessage(decltype(socket(0, 0, 0)) const& socket, int const& timeout, ProtocolParameter* para);

private:
    // Wait for client connection thread handler.
    void WaitHandler(void);
    // Main service thread handler.
    void ServiceHandler(void);

    decltype(socket(0, 0, 0))    listen_;   // Listening socket.
    std::atomic_bool             is_ready_; // Server socket status.
    std::string                  ip_;       // Server IP address.
    int                          port_;     // Server port.
    int                          max_connection_num_;
    MultimediaDataUploadCallback multimedia_data_upload_callback_;
    std::thread                  waiting_thread_;     // Wait for client connection thread.
    std::atomic_bool             waiting_is_running_; // Wait for client connection thread running flag.
    std::thread                  service_thread_;     // Main service thread.
    std::atomic_bool             service_is_running_; // Main service thread running flag.
    Packager                     packager_;           // General JT808 protocol packager.
    Parser                       parser_;             // General JT808 protocol parser.

    // Client's socket (key) - Client's protocol parameters (value).
    std::map<decltype(socket(0, 0, 0)), ProtocolParameter> clients_;
    // Clients in upgrade status.
    std::map<decltype(socket(0, 0, 0)), int> is_upgrading_clients_;

    friend class JT808CustomServer; // Allow the custom server to access private members.
};

} // namespace libjt808

#endif // JT808_SERVER_H_
