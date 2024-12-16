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

// @File    :  protocol_parameter.h
// @Version :  1.0
// @Time    :  2020/06/24 10:00:51
// @Author  :  Meng Yuming
// @Contact :  mengyuming@hotmail.com
// @Desc    :  None

#ifndef JT808_PROTOCOL_PARAMETER_H_
#define JT808_PROTOCOL_PARAMETER_H_

#include <string>
#include <vector>

#include "jt808/area_route.h"
#include "jt808/location_report.h"
#include "jt808/terminal_parameter.h"
#include "jt808/multimedia_upload.h"

namespace libjt808 {

// Supported protocol commands.
enum SupportedCommands {
    kTerminalGeneralResponse        = 0x0001, // Terminal general response.
    kPlatformGeneralResponse        = 0x8001, // Platform general response.
    kTerminalHeartBeat              = 0x0002, // Terminal heartbeat.
    kFillPacketRequest              = 0x8003, // Fill packet request.
    kTerminalRegister               = 0x0100, // Terminal register.
    kTerminalRegisterResponse       = 0x8100, // Terminal register response.
    kTerminalLogOut                 = 0x0003, // Terminal logout.
    kTerminalAuthentication         = 0x0102, // Terminal authentication.
    kSetTerminalParameters          = 0x8103, // Set terminal parameters.
    kGetTerminalParameters          = 0x8104, // Get terminal parameters.
    kGetSpecificTerminalParameters  = 0x8106, // Get specific terminal parameters.
    kGetTerminalParametersResponse  = 0x0104, // Get terminal parameters response.
    kTerminalUpgrade                = 0x8108, // Terminal upgrade.
    kTerminalUpgradeResultReport    = 0x0108, // Terminal upgrade result report.
    kLocationReport                 = 0x0200, // Location report.
    kGetLocationInformation         = 0x8201, // Get location information.
    kGetLocationInformationResponse = 0x0201, // Get location information response.
    kLocationTrackingControl        = 0x8202, // Location tracking control.
    kSetPolygonArea                 = 0x8604, // Set polygon area.
    kDeletePolygonArea              = 0x8605, // Delete polygon area.
    kMultimediaDataUpload           = 0x0801, // Multimedia data upload.
    kMultimediaDataUploadResponse   = 0x8800, // Multimedia data upload response.

    //* Additional supported commands.
    kVersionInformation  = 0x0205, ///< Version information.
    kDrivingLicenseData  = 0x0252, ///< Driving license data.
    kBatchLocationReport = 0x0704, ///< Batch location report.
    kCANBroadcastData    = 0x0705, ///< CAN broadcast data.
};

// All response commands.
constexpr uint16_t kResponseCommand[] = {
    kTerminalGeneralResponse,       kPlatformGeneralResponse,        kTerminalRegisterResponse,
    kGetTerminalParametersResponse, kGetLocationInformationResponse,
};

// Vehicle plate color.
enum VehiclePlateColor {
    kVin = 0x0, // Vehicle not registered.
    kBlue,
    kYellow,
    kBlack,
    kWhite,
    kOther
};

// General response result.
enum GeneralResponseResult {
    kSuccess = 0x0,             // Success/Confirmation.
    kFailure,                   // Failure.
    kMessageHasWrong,           // Message has error.
    kNotSupport,                // Not supported.
    kAlarmHandlingConfirmation, // Alarm handling confirmation, used only by platform response.
};

// Register response result.
enum RegisterResponseResult {
    kRegisterSuccess = 0x0,       // Success.
    kVehiclesHaveBeenRegistered,  // Vehicle has been registered.
    kNoSuchVehicleInTheDatabase,  // No such vehicle in the database.
    kTerminalHaveBeenRegistered,  // Terminal has been registered.
    kNoSuchTerminalInTheDatabase, // No such terminal in the database.
};

// Message body attributes.
union MsgBodyAttribute {
    struct {
        // Message body length, occupies 10 bits.
        uint16_t msglen  : 10;
        // Data encryption method, when these three bits are all 0, it means the message body is not encrypted,
        // when the 10th bit is 1, it means the message body is encrypted with RSA algorithm.
        uint16_t encrypt : 3;
        // Packet flag.
        uint16_t packet  : 1;
        // Reserved 2 bits.
        uint16_t retain  : 2;
    } bit;

    uint16_t u16val;
};

// Message content starting position.
enum MsgBodyPos {
    MSGBODY_NOPACKET_POS = 13, // Starting position of short message body content.
    MSGBODY_PACKET_POS   = 17, // Starting position of long message body content.
};

// Escape related flags.
enum ProtocolEscapeFlag {
    PROTOCOL_SIGN          = 0x7E, // Flag bit.
    PROTOCOL_ESCAPE        = 0x7D, // Escape flag.
    PROTOCOL_ESCAPE_SIGN   = 0x02, // 0x7E<-->0x7D followed by 0x02.
    PROTOCOL_ESCAPE_ESCAPE = 0x01, // 0x7D<-->0x7D followed by 0x01.
};

// Message header.
struct MsgHead {
    // Message ID.
    uint16_t msg_id;
    // Message body attributes.
    MsgBodyAttribute msgbody_attr;
    // Terminal phone number.
    std::string phone_num;
    // Message flow number.
    uint16_t msg_flow_num;
    // Total number of packets, used in case of packet segmentation.
    uint16_t total_packet;
    // Current packet sequence number, used in case of packet segmentation.
    uint16_t packet_seq;
};

// Register information.
struct RegisterInfo {
    // Province ID.
    uint16_t province_id;
    // City/County ID.
    uint16_t city_id;
    // Manufacturer ID, fixed 5 bytes.
    std::vector<uint8_t> manufacturer_id;
    // Terminal model, fixed 20 bytes, padded with 0x00 if insufficient.
    std::vector<uint8_t> terminal_model;
    // Terminal ID, fixed 7 bytes, padded with 0x00 if insufficient.
    std::vector<uint8_t> terminal_id;
    // Vehicle plate color, 0 means not registered.
    uint8_t car_plate_color;
    // Vehicle identification, used only when registered.
    std::string car_plate_num;

    // Overload copy assignment operator.
    void operator=(RegisterInfo const& info) {
        province_id = info.province_id;
        city_id     = info.city_id;
        manufacturer_id.clear();
        manufacturer_id.assign(info.manufacturer_id.begin(), info.manufacturer_id.end());
        terminal_model.clear();
        terminal_model.assign(info.terminal_model.begin(), info.terminal_model.end());
        terminal_id.clear();
        terminal_id.assign(info.terminal_id.begin(), info.terminal_id.end());
        car_plate_num.clear();
        car_plate_color = info.car_plate_color;
        if (car_plate_color != kVin) {
            car_plate_num.assign(info.car_plate_num.begin(), info.car_plate_num.end());
        }
    }
};

// Upgrade type.
enum kTerminalUpgradeType {
    // Terminal.
    kTerminal = 0x0,
    // Road transport certificate IC card reader.
    kICCardReader = 0xc,
    // Beidou satellite positioning module.
    kGNSS = 0x34,
};

// Upgrade result.
enum kTerminalUpgradeResultType {
    // Terminal upgrade success.
    kTerminalUpgradeSuccess = 0x0,
    // Terminal upgrade failure.
    kTerminalUpgradeFailed,
    // Terminal upgrade canceled.
    kTerminalUpgradeCancel
};

// Upgrade information.
struct UpgradeInfo {
    // Upgrade type.
    uint8_t upgrade_type;
    // Upgrade result.
    uint8_t upgrade_result;
    // Manufacturer ID, fixed 5 bytes.
    std::vector<uint8_t> manufacturer_id;
    // Upgrade version number.
    std::string version_id;
    // Total length of upgrade package.
    uint32_t upgrade_data_total_len;
    // Upgrade data package.
    std::vector<uint8_t> upgrade_data;
};

// Fill packet information.
struct FillPacket {
    // Message flow number of the first packet of the sub-packet data.
    uint16_t first_packet_msg_flow_num;
    // IDs of packets that need to be retransmitted.
    std::vector<uint16_t> packet_id;
};

// ---------- (00) ADDITIONAL SUPPORTED PACKAGE --------------------------------------------------------------------- //
/**
 * @brief Holds version information and related details for a device.
 *
 * This structure contains various fields that provide detailed information
 * about the version, release date, and other identifiers for a device.
 */
struct VersionInformation {
    std::string          version;     ///< Version number. e.g., "HBT530CVMFF2D1"
    std::string          rel_date;    ///< Release date. e.g., "2020-06-24"
    std::vector<uint8_t> cpu_id;      ///< CPU ID number. e.g. "FDFF0200FF7F008050110136" in byte array
    std::string          model;       ///< Model number. e.g., "EC200U"
    std::string          imei;        ///< IMEI number. e.g., "864714067557109"
    std::string          imsi;        ///< IMSI number. e.g., "520031008795627"
    std::string          iccid;       ///< ICCID number. e.g., "8966032421096431741F"
    uint16_t             car_model;   ///< Car model number. e.g., 61526
    std::string          vin;         ///< Vehicle identification number
    uint32_t             tot_mileage; ///< Total mileage
    uint32_t             tot_fuel;    ///< Total fuel consumption
};

/**
 * @brief Structure to hold driver's card information.
 *
 * This structure contains various fields related to the driver's card information,
 * including the driver's name, country code, citizen ID, expiration date, date of birth,
 * license type, gender, license ID, issuing branch, and track information.
 */
struct CardInfo {
    std::string name;           ///< Driver's name
    std::string country;        ///< Country code
    std::string citizen_id;     ///< Driver's citizen ID
    std::string expire_date;    ///< Expiration date yymm
    std::string dob;            ///< Date of birth yyyymmdd
    std::string license_type;   ///< Driving License type
    std::string gender;         ///< Driver's gender
    std::string license_id;     ///< Driver's license ID
    std::string issuing_branch; ///< Issuing branch
    std::string track;          ///< License Track 1-3 raw data
    //* Example of track data (track 1-3):
    //* "%  ^CHATURAPHATSIRIKUN$IDSARAWAT$MR.^^?;6007643959900137864=270319800301=?+"
    //* "             2400          1            65007168  00100                     ?"
};

/**
 * @brief Structure to hold driving license data.
 *
 * This structure contains various fields related to the driving license data,
 * including card information, login status, and driving license data upload permission flag.
 */
struct DrivingLicenseData {
    CardInfo card_info;     ///< Card information.
    uint8_t  login_sts;     ///< Login status. 0: logout, 1: login
    uint8_t  dlt_allow_flg; ///< Driving license data upload permission flag. 0: not allowed, 1: allowed
};

/**
 * @brief Structure to hold CAN information.
 *
 * This structure contains the CAN ID and the associated CAN data.
 */
struct CANInfo {
    uint32_t             id;   ///< CAN ID (DWORD)
    std::vector<uint8_t> data; ///< CAN data
};

/**
 * @brief Structure to hold CAN broadcast data.
 *
 * This structure contains the number of data entries, the receiving time,
 * and the CAN information including the CAN ID and CAN data.
 */
struct CANBroadcastData {
    uint16_t    nbr_of_data; ///< Number of data
    std::string recv_time;   ///< Receiving time, hh mm ss msms
    CANInfo     can_info;    ///< CAN information
};

// ---------- (00) MAIN PACKAGE (PROTOCOL PARAMETER) -------------------------------------------- //
// All protocol parameters.
struct ProtocolParameter {
    uint8_t  respone_result;
    uint16_t respone_msg_id;
    uint16_t respone_flow_num;
    // Message header.
    MsgHead msg_head;
    // Register information to be filled in when the terminal is registered.
    RegisterInfo register_info;
    // Authentication code randomly generated by the platform.
    std::vector<uint8_t> authentication_code;
    // Set terminal parameter items.
    TerminalParameters terminal_parameters;
    // List of terminal parameter IDs to query.
    std::vector<uint32_t> terminal_parameter_ids;
    // Basic location information to be filled in when reporting location, mandatory.
    LocationBasicInformation location_info;
    // Additional location information to be filled in when reporting location, optional.
    LocationExtensions location_extension;
    // Temporary location tracking control information.
    LocationTrackingControl location_tracking_control;
    // Polygon area set.
    // PolygonAreaSet polygon_area_set;
    // Polygon area.
    PolygonArea polygon_area;
    // Set of polygon area IDs to be deleted.
    std::vector<uint32_t> polygon_area_id;
    // Upgrade information.
    UpgradeInfo upgrade_info;
    // Fill packet information.
    struct FillPacket fill_packet;
    // Multimedia data upload.
    MultiMediaDataUpload multimedia_upload;
    // Multimedia data upload response.
    MultiMediaDataUploadResponse multimedia_upload_response;
    // Reserved fields.
    std::vector<uint8_t> retain;

    //* Additional fields.
    VersionInformation version_info; ///< Version information.
    DrivingLicenseData license_data; ///< Driving license data.
    CANBroadcastData   can_data;     ///< CAN broadcast data.

    // Used to parse messages.
    struct {
        uint8_t  respone_result;
        uint16_t respone_msg_id;
        uint16_t respone_flow_num;
        // Parsed message header.
        MsgHead msg_head;
        // Parsed register information.
        RegisterInfo register_info;
        // Parsed authentication code.
        std::vector<uint8_t> authentication_code;
        // Parsed set terminal parameter items.
        TerminalParameters terminal_parameters;
        // Parsed list of terminal parameter IDs to query.
        std::vector<uint32_t> terminal_parameter_ids;
        // Parsed basic location information.
        LocationBasicInformation location_info;
        // Parsed additional location information.
        LocationExtensions location_extension;
        // Parsed temporary location tracking control information.
        LocationTrackingControl location_tracking_control;
        // Parsed polygon area set.
        // PolygonAreaSet polygon_area_set;
        // Parsed polygon area.
        PolygonArea polygon_area;
        // Parsed set of polygon area IDs to be deleted.
        std::vector<uint32_t> polygon_area_id;
        // Parsed upgrade information.
        UpgradeInfo upgrade_info;
        // Parsed fill packet information.
        struct FillPacket fill_packet;
        // Parsed multimedia data upload.
        MultiMediaDataUpload multimedia_upload;
        // Parsed multimedia data upload response.
        MultiMediaDataUploadResponse multimedia_upload_response;
        // Parsed reserved fields.
        std::vector<uint8_t> retain;

        //* Additional fields.
        VersionInformation version_info; ///< Version information.
        DrivingLicenseData license_data; ///< Driving license data.
        CANBroadcastData   can_data;     ///< CAN broadcast data.
    } parse;
};

} // namespace libjt808

#endif // JT808_PROTOCOL_PARAMETER_H_
