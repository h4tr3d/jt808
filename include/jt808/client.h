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

// @File    :  client.h
// @Version :  1.0
// @Time    :  2020/07/17 10:05:36
// @Author  :  Meng Yuming
// @Contact :  mengyuming@hotmail.com
// @Desc    :  None

#ifndef JT808_CLIENT_H_
#define JT808_CLIENT_H_

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
#include <list>
#include <mutex>

#include "jt808/packager.h"
#include "jt808/parser.h"
#include "jt808/protocol_parameter.h"
#include "jt808/terminal_parameter.h"

namespace libjt808 {

// JT808 terminal.
// Implemented terminal registration, terminal authentication, heartbeat packet, and location information reporting functions.
//
// Example:
//     JT808Client client;
//     client.Init();
//     client.SetRemoteAccessPoint("127.0.0.1", 8888);
//     if ((client.ConnectRemote() == 0) &&
//         (client.JT808ConnectionAuthentication() == 0)) {
//       client.Run();
//       std::this_thread::sleep_for(std::chrono::seconds(1));
//       while (client.service_is_running()) {
//         std::this_thread::sleep_for(std::chrono::seconds(1));
//       }
//       client.Stop();
//     }
//
//     // 在其它位置调用client.UpdateLocation(args)来更新位置基本信息.
//     // 在其它位置调用client.SetAlarmBit(args)来更新报警位.
//     // 在其它位置调用client.SetStatusBit(args)来更新状态位.

class JT808Client {
public:
    JT808Client();
    ~JT808Client();
    void Init(void);

    //
    // Connection to the server.
    //
    void SetRemoteAccessPoint(std::string const& ip, int const& port) {
        ip_   = ip;
        port_ = port;
    }

    // Set terminal phone number.
    void SetTerminalPhoneNumber(std::string const& phone) {
        parameter_.msg_head.phone_num.clear();
        parameter_.msg_head.phone_num.assign(phone.begin(), phone.end());
    }

    // Connect to the remote server.
    int ConnectRemote(void);
    // JT808 connection authentication.
    int JT808ConnectionAuthentication(void);

    //
    // Terminal registration.
    //
    // Set terminal registration information.
    void SetTerminalRegisterInfo(RegisterInfo const& info) {
        parameter_.register_info = info;
    }

    // Set terminal registration information.
    // Args:
    //     p_id:  Province ID.
    //     c_id:  City/County ID.
    //     m_id:  Manufacturer ID, up to 5 bytes.
    //     t_model:  Terminal model, up to 20 bytes.
    //     t_id:  Terminal ID, up to 7 bytes.
    //     c_color:  License plate color.
    //     c_num:  License plate number.
    // Returns:
    //     None.
    void SetTerminalRegisterInfo(uint16_t const& p_id, uint16_t const& c_id, std::vector<uint8_t> const& m_id,
                                 std::vector<uint8_t> const& t_model, std::vector<uint8_t> const& t_id,
                                 uint8_t const& c_color, std::string const& c_num) {
        parameter_.register_info.province_id = p_id;
        parameter_.register_info.city_id     = c_id;
        parameter_.register_info.manufacturer_id.clear();
        parameter_.register_info.manufacturer_id.assign(m_id.begin(), m_id.end());
        parameter_.register_info.terminal_model.clear();
        parameter_.register_info.terminal_model.assign(t_model.begin(), t_model.end());
        parameter_.register_info.terminal_id.clear();
        parameter_.register_info.terminal_id.assign(t_id.begin(), t_id.end());
        parameter_.register_info.car_plate_color = c_color;
        if (c_color != kVin) {
            parameter_.register_info.car_plate_num.clear();
            parameter_.register_info.car_plate_num.assign(c_num.begin(), c_num.end());
        }
    }

    //
    // Service thread operation and termination.
    //
    // Start the service thread.
    void Run(void);
    // Stop the service thread.
    void Stop(void);
    // Wait for all cached messages to be sent or timeout before stopping the service thread.
    void WattingStop(int const& timeout_msec);

    // Get the current service thread running status.
    bool service_is_running(void) const {
        return service_is_running_;
    }

    //
    // External access to set the current general message body parsing and packaging functions,
    // used for overriding or adding command support.
    // Must be used after calling the Init() member function.
    //
    // Get the general JT808 protocol packager.
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

    // Set the general JT808 protocol packager.
    void set_packager(Packager const& packager) {
        packager_ = packager;
    }

    // Get the general JT808 protocol parser.
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

    // Set the general JT808 protocol parser.
    void set_parser(Parser const& parser) {
        parser_ = parser;
    }

    //
    // Location reporting related.
    //
    // Set alarm bit.
    void SetAlarmBit(uint32_t const& alarm) {
        parameter_.location_info.alarm.value = alarm;
    }

    // Get alarm bit.
    uint32_t const& alarm_bit(void) const {
        return parameter_.location_info.alarm.value;
    }

    // Set the in/out area alarm flag.
    // Args:
    //     in:  In/out area flag, 0 - enter, 1 - leave.
    // Returns:
    //     None.
    void SetInOutAreaAlarmBit(uint8_t const& in) {
        parameter_.location_info.alarm.bit.in_out_area = in;
        location_report_immediately_flag_ |= kAlarmOccurred;
    }

    // Set the in/out area alarm location extension item.
    // Args:
    //     item:  In/out area extension item value, Type(BYTE)+Area ID(DWORD)+Direction(BYTE).
    // Returns:
    //     None.
    void SetInOutAreaAlarmExtension(std::vector<uint8_t> const& item) {
        auto const& it = parameter_.location_extension.find(kAccessAreaAlarm);
        if (it != parameter_.location_extension.end()) {
            it->second.assign(item.begin(), item.end());
        }
        else {
            parameter_.location_extension.insert(std::make_pair(kAccessAreaAlarm, item));
        }
    }

    // Set status bit.
    void SetStatusBit(uint32_t const& status) {
        parameter_.location_info.status.value = status;
        location_report_immediately_flag_ |= kStateChanged;
    }

    // Get status bit.
    uint32_t const& status_bit(void) const {
        return parameter_.location_info.status.value;
    }

    // Update location basic information.
    // Args:
    //     info: Location basic information.
    // Returns:
    //     None.
    void UpdateLocation(LocationBasicInformation const& info) {
        parameter_.location_info = info;
    }

    // Update location basic information.
    // Args:
    //     latitude: Latitude value in degrees (°).
    //     longitude: Longitude value in degrees (°).
    //     altitude: Altitude in meters (m).
    //     speed: Speed in kilometers per hour (km/h).
    //     bearing: Direction.
    //     timestamp: GMT+8 timestamp in YYMMDDhhmmss format.
    // Returns:
    //     None.
    void UpdateLocation(double const& latitude, double const& longitude, float const& altitude, float const& speed,
                        float const& bearing, std::string const& timestamp) {
        parameter_.location_info.latitude  = static_cast<uint32_t>(latitude * 1e6);
        parameter_.location_info.longitude = static_cast<uint32_t>(longitude * 1e6);
        parameter_.location_info.altitude  = static_cast<uint16_t>(altitude);
        parameter_.location_info.speed     = static_cast<uint16_t>(speed * 10);
        parameter_.location_info.bearing   = static_cast<uint16_t>(bearing);
        parameter_.location_info.time.assign(timestamp.begin(), timestamp.end());
    }

    // Get location information extensions.
    int GetLocationExtension(LocationExtensions* items) {
        if (items == nullptr)
            return -1;
        items->clear();
        items->insert(parameter_.location_extension.begin(), parameter_.location_extension.end());
        return 0;
    }

    // Get location information extensions.
    LocationExtensions const& GetLocationExtension(void) const {
        return parameter_.location_extension;
    }

    // Get location information extensions.
    LocationExtensions& GetLocationExtension(void) {
        return parameter_.location_extension;
    }

    // Set the location reporting interval.
    // If you want the reported data to be more accurate each time, set msg_generate_outside to true,
    // and call GenerateLocationReportMsgNow() method after parsing the positioning module data,
    // to immediately generate the location report information at the current moment.
    void set_location_report_inteval(uint8_t const& intv, bool const& msg_generate_outside = false) {
        location_report_inteval_ = intv;
        location_report_msg_generate_outside_.store(msg_generate_outside);
    }

    // Immediate location reporting flag.
    enum ReportImmediatelyFlag {
        kAlarmOccurred = 0x1, // Alarm flag changed.
        kStateChanged  = 0x2, // State flag changed.
    };

    // Immediately generate a location reporting message.
    // Only called when external control of location reporting is enabled.
    void GenerateLocationReportMsgNow(void);

    //
    // Terminal parameter related.
    //
    // Get terminal heartbeat interval.
    int GetTerminalHeartbeatInterval(uint32_t* interval) const {
        return ParseTerminalParameterTerminalHeartBeatInterval(parameter_.terminal_parameters, interval);
    }

    // Set terminal heartbeat interval.
    int SetTerminalHeartbeatInterval(uint32_t const& interval) {
        return PackagingTerminalParameterTerminalHeartBeatInterval(interval, &parameter_.terminal_parameters);
    }

    // Get all terminal parameters.
    int GetTerminalParameters(TerminalParameters* para) const {
        if (para == nullptr)
            return -1;
        para->clear();
        para->insert(parameter_.terminal_parameters.begin(), parameter_.terminal_parameters.end());
        return 0;
    }

    // Get all terminal parameters.
    TerminalParameters const& GetTerminalParameters(void) const {
        return parameter_.terminal_parameters;
    }

    // Get all terminal parameters.
    TerminalParameters& GetTerminalParameters(void) {
        return parameter_.terminal_parameters;
    }

    // Set all terminal parameters.
    void SetTerminalParameters(TerminalParameters const& para) {
        parameter_.terminal_parameters.clear();
        parameter_.terminal_parameters.insert(para.begin(), para.end());
    }

    // Terminal parameter callback function.
    using TerminalParameterCallback = std::function<void(void /* Parameters to be determined. */)>;

    // Set the callback function when the platform configuration modifies terminal parameters.
    void OnTerminalParameteUpdated(TerminalParameterCallback const& callback) {
        terminal_parameter_callback_ = callback;
    }

    //
    // Upgrade related.
    //
    void UpgradeResultReport(uint8_t const& result);
    // Callback function for issuing upgrade packages.
    // Args:
    //     type:  Upgrade type.
    //     data:  Upgrade package buffer address.
    //     size:  Upgrade package size.
    // Returns:
    //     None.
    using UpgradeCallback = std::function<void(uint8_t const& type, char const* data, int const& size)>;

    // Set the callback function for issuing upgrade packages.
    void OnUpgraded(UpgradeCallback const& callback) {
        upgrade_callback_ = callback;
    }

    //
    // Area route related.
    //
    // Get the current polygon area information set.
    PolygonAreaSet const& polygon_areas(void) const {
        return polygon_areas_;
    }

    // Get all polygon areas.
    int GetAllPolygonArea(PolygonAreaSet* areas) const {
        if (areas == nullptr || polygon_areas_.empty())
            return -1;
        areas->clear();
        areas->insert(polygon_areas_.begin(), polygon_areas_.end());
        return 0;
    }

    // Get the polygon area by ID.
    int GetPolygonAreaByID(uint32_t const& id, PolygonArea* area) const {
        if (area == nullptr || polygon_areas_.empty())
            return -1;
        auto const& it = polygon_areas_.find(id);
        if (it != polygon_areas_.end())
            return -1;
        *area = it->second;
        return 0;
    }

    // Add a polygon area.
    // Args:
    //     area:  Polygon area.
    // Returns:
    //     Returns -1 if the area ID already exists, otherwise returns 0.
    int AddPolygonArea(PolygonArea const& area) {
        auto const& id = area.area_id;
        return (polygon_areas_.insert(std::make_pair(id, area)).second - 1);
    }

    // Add a new polygon area.
    // Args:
    //     id:  Area ID.
    //     attr:  Area attribute.
    //     begin_time:  Start time, format "YYMMDDhhmmss";
    //     end_time:  End time, format "YYMMDDhhmmss";
    //     max_speed:  Maximum speed, km/h;
    //     overspeed_time:  Overspeed duration, s;
    //     vertices:  Area vertices, stored in clockwise order;
    // Returns:
    //     Returns -1 if the area ID already exists, otherwise returns 0.
    int AddPolygonArea(uint32_t const& id, uint16_t const& attr, std::string const& begin_time,
                       std::string const& end_time, uint16_t const& max_speed, uint8_t const& overspeed_time,
                       std::vector<LocationPoint> const& vertices) {
        PolygonArea area = {id, AreaAttribute {attr}, begin_time, end_time, max_speed, overspeed_time, vertices};
        return (polygon_areas_.insert(std::make_pair(id, area)).second - 1);
    }

    // Update a polygon area information.
    // If the area ID does not exist, insert it directly, otherwise update the existing area information.
    // Args:
    //     id:  Area ID.
    //     attr:  Area attribute.
    //     begin_time:  Start time, format "YYMMDDhhmmss";
    //     end_time:  End time, format "YYMMDDhhmmss";
    //     max_speed:  Maximum speed, km/h;
    //     overspeed_time:  Overspeed duration, s;
    //     vertices:  Area vertices, stored in clockwise order;
    // Returns:
    //     None.
    void UpdatePolygonArea(uint32_t const& id, uint16_t const& attr, std::string const& begin_time,
                           std::string const& end_time, uint16_t const& max_speed, uint8_t const& overspeed_time,
                           std::vector<LocationPoint> const& vertices) {
        PolygonArea area   = {id, AreaAttribute {attr}, begin_time, end_time, max_speed, overspeed_time, vertices};
        polygon_areas_[id] = area;
    }

    // Update the specified polygon area.
    // Args:
    //     area:  Polygon area.
    // Returns:
    //     None.
    void UpdatePolygonAreaByArea(PolygonArea const& area) {
        polygon_areas_[area.area_id] = area;
    }

    // Update the specified polygon areas.
    // Args:
    //     areas:  Polygon area information set.
    // Returns:
    //     None.
    void UpdatePolygonAreaByAreas(PolygonAreaSet const& areas) {
        for (auto const& item : areas) {
            polygon_areas_[item.first] = item.second;
        }
    }

    // Delete the specified polygon area by ID.
    // Args:
    //     id:  Area ID.
    // Returns:
    //     None.
    void DeletePolygonAreaByID(uint32_t const& id) {
        auto const& it = polygon_areas_.find(id);
        if (it != polygon_areas_.end())
            polygon_areas_.erase(it);
    }

    // Delete the specified polygon area by ID.
    // When the area ID set is empty, delete all polygon area information.
    // Args:
    //     ids:  Area ID set.
    // Returns:
    //     None.
    void DeletePolygonAreaByIDs(std::vector<uint32_t> const& ids) {
        if (ids.empty()) {
            DeleteAllPolygonArea();
            return;
        }
        for (auto const& id : ids) {
            DeletePolygonAreaByID(id);
        }
    }

    // Delete all polygon areas.
    // Args:
    //     None.
    // Returns:
    //     None.
    void DeleteAllPolygonArea(void) {
        polygon_areas_.clear();
    }

    // Polygon area callback function.
    using PolygonAreaCallback = std::function<void(void /* Parameters to be determined. */)>;

    // Set the callback function when the platform configuration modifies polygon area information.
    void OnPolygonAreaUpdated(PolygonAreaCallback const& callback) {
        polygon_area_callback_ = callback;
    }

    //
    // Multimedia data upload.
    //
    // Args:
    //     path: Path to upload JPEG image.
    //     location_basic: Encapsulation of basic location information.
    // Returns:
    //     None.
    int MultimediaUpload(char const* path, std::vector<uint8_t> const& location_basic);

    // General message packaging and sending function.
    // Args:
    //     msg_id:  Message ID.
    // Returns:
    //     Returns 0 on success, -1 on failure.
    int PackagingAndSendMessage(uint32_t const& msg_id);

    // General message receiving and parsing function.
    // Blocking function.
    // Do not call after the service thread is enabled.
    // Args:
    //     timeout:  Timeout period in seconds (s).
    // Returns:
    //     Returns 0 on success, -1 on failure.
    int ReceiveAndParseMessage(int const& timeout);

private:
    // Generate a message.
    int PackagingMessage(uint32_t const& msg_id, std::vector<uint8_t>* out);
    // Generate a message and store it in the general message list.
    int PackagingGeneralMessage(uint32_t const& msg_id);
    // Send a message.
    int SendMessage(std::vector<uint8_t> const& msg);
    // Main thread handler function.
    void ThreadHandler(void);
    // Thread handler function for sending messages to the server.
    void SendHandler(std::atomic_bool* const running);
    // Thread handler function for receiving messages from the server.
    void ReceiveHandler(std::atomic_bool* const running);

    std::atomic_bool          manual_deal_;        // Manual processing flag.
    std::mutex                msg_generate_mutex_; // Message generation mutex to ensure unique message serial numbers.
    decltype(socket(0, 0, 0)) client_;             // General TCP connection socket.
    std::atomic_bool          is_connected_;       // TCP connection status with the server.
    std::atomic_bool          is_authenticated_;   // Authentication status.
    std::string               ip_;                 // Server IP address.
    int                       port_;               // Server port.
    uint8_t                   location_report_inteval_;          // Location information reporting interval.
    uint16_t                  location_report_immediately_flag_; // Immediate location reporting flag.
    std::atomic_bool
                location_report_msg_generate_outside_; // External control to generate location reporting information.
    std::thread service_thread_;                       // Service thread.
    std::atomic_bool service_is_running_;              // Service thread running flag.
    std::atomic_bool tcp_connection_handling_;         // Flag indicating TCP connection is being established.
    std::atomic_bool jt808_connection_handling_; // Flag indicating JT808 connection authentication is in progress.
    TerminalParameterCallback terminal_parameter_callback_; // Callback function for modifying terminal parameters.
    UpgradeCallback           upgrade_callback_;            // Callback function for issuing terminal upgrade packages.
    PolygonAreaCallback       polygon_area_callback_;       // Callback function for modifying polygon area information.
    Packager                  packager_;                    // General JT808 protocol packager.
    Parser                    parser_;                      // General JT808 protocol parser.
    std::list<std::vector<uint8_t>> location_report_msg_;   // Location reporting message list.
    std::list<std::vector<uint8_t>> general_msg_;           // Message list excluding location reporting messages.
    PolygonAreaSet                  polygon_areas_;         // Polygon area information set.
    ProtocolParameter               parameter_;             // JT808 protocol parameters.
};

} // namespace libjt808

#endif // JT808_CLIENT_H_
