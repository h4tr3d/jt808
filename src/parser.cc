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

// @File    :  parser.cc
// @Version :  1.0
// @Time    :  2020/07/08 17:09:29
// @Author  :  Meng Yuming
// @Contact :  mengyuming@hotmail.com
// @Desc    :  None

#include "jt808/parser.h"

#include <string.h>

#include "jt808/bcd.h"
#include "jt808/util.h"

namespace libjt808 {

namespace {

// Parse message header.
int JT808FrameHeadParse(InputBuffer in, MsgHead* msg_head) {
    if (msg_head == nullptr || in.size() < 15)
        return -1;
    // Message ID.
    msg_head->msg_id = in[1] * 256 + in[2];
    // Message body attributes.
    msg_head->msgbody_attr.u16val = in[3] * 256 + in[4];
    // Terminal phone number.
    std::vector<uint8_t> phone_num_bcd;
    phone_num_bcd.assign(in.begin() + 5, in.begin() + 11);
    if (BcdToString(phone_num_bcd, &(msg_head->phone_num)) != 0)
        return -1;
    // Message flow number.
    msg_head->msg_flow_num = in[11] * 256 + in[12];
    // Packet occurrence.
    if ((msg_head->msgbody_attr.bit.packet == 1) && ((in.size() - 15 - msg_head->msgbody_attr.bit.msglen) == 4)) {
        msg_head->total_packet = in[13] * 256 + in[14];
        msg_head->packet_seq   = in[15] * 256 + in[16];
    }
    else {
        msg_head->total_packet = 0;
        msg_head->packet_seq   = 0;
    }
    return 0;
}

} // namespace

// Command parser initialization.
int JT808FrameParserInit(Parser* parser) {
    // 0x0001, Terminal general response.
    parser->insert(std::pair<uint16_t, ParseHandler>(
        kTerminalGeneralResponse, [](InputBuffer in, ProtocolParameter* para) -> int {
            if (para == nullptr)
                return -1;
            uint16_t pos = MSGBODY_NOPACKET_POS;
            // if (para->msg_head.msgbody_attr.bit.package == 1)
            //   pos = MSGBODY_PACKET_POS;
            // Response flow number.
            para->parse.respone_flow_num = in[pos] * 256 + in[pos + 1];
            // Response message ID.
            para->parse.respone_msg_id = in[pos + 2] * 256 + in[pos + 3];
            // Response result.
            para->parse.respone_result = in[pos + 4];
            return 0;
        }));

    // 0x8001, Platform general response.
    parser->insert(std::pair<uint16_t, ParseHandler>(
        kPlatformGeneralResponse, [](InputBuffer in, ProtocolParameter* para) -> int {
            if (para == nullptr)
                return -1;
            uint16_t pos = MSGBODY_NOPACKET_POS;
            // if (para->msg_head.msgbody_attr.bit.package == 1)
            //   pos = MSGBODY_PACKET_POS;
            // Response flow number.
            para->parse.respone_flow_num = in[pos] * 256 + in[pos + 1];
            // Response message ID.
            para->parse.respone_msg_id = in[pos + 2] * 256 + in[pos + 3];
            // Response result.
            para->parse.respone_result = in[pos + 4];
            return 0;
        }));

    // 0x0002, Terminal heartbeat.
    parser->insert(std::pair<uint16_t, ParseHandler>(
        kTerminalHeartBeat, [](InputBuffer in, ProtocolParameter* para) -> int {
            if (para == nullptr)
                return -1;
            // Empty message body.
            return 0;
        }));

    // 0x8003, Fill packet request.
    parser->insert(std::pair<uint16_t, ParseHandler>(
        kFillPacketRequest, [](InputBuffer in, ProtocolParameter* para) -> int {
            if (para == nullptr)
                return -1;
            auto const& msg_len = para->parse.msg_head.msgbody_attr.bit.msglen;
            uint16_t    pos     = MSGBODY_NOPACKET_POS;
            if (para->parse.msg_head.msgbody_attr.bit.packet == 1)
                pos = MSGBODY_PACKET_POS;
            auto& fill_packet = para->parse.fill_packet;
            // First packet flow number.
            fill_packet.first_packet_msg_flow_num = in[pos] * 256 + in[pos + 1];
            pos += 2;
            // Total number of retransmission packets.
            uint16_t cnt = in[pos];
            ++pos;
            // Retransmission packet IDs.
            if (msg_len - 3 != cnt * 2)
                return -1;
            fill_packet.packet_id.clear();
            uint16_t id = 0;
            for (uint8_t i = 0; i < cnt; ++i) {
                id = in[pos + i * 2] + in[pos + 1 + i * 2];
                fill_packet.packet_id.push_back(id);
            }
            return 0;
        }));

    // 0x0100, Terminal registration.
    parser->insert(std::pair<uint16_t, ParseHandler>(
        kTerminalRegister, [](InputBuffer in, ProtocolParameter* para) -> int {
            if (para == nullptr)
                return -1;
            uint16_t pos           = MSGBODY_NOPACKET_POS;
            auto&    register_info = para->parse.register_info;
            // Province ID.
            U16ToU8Array u16converter;
            for (int i = 0; i < 2; ++i)
                u16converter.u8array[i] = in[pos++];
            register_info.province_id = EndianSwap16(u16converter.u16val);
            // City/County ID.
            for (int i = 0; i < 2; ++i)
                u16converter.u8array[i] = in[pos++];
            register_info.city_id = EndianSwap16(u16converter.u16val);
            // Manufacturer ID.
            register_info.manufacturer_id.clear();
            for (size_t i = 0; i < 5; ++i)
                register_info.manufacturer_id.push_back(in[pos + i]);
            pos += 5;
            // Terminal model.
            for (size_t i = 0; i < 20; ++i) {
                if (in[pos + i] == 0x0)
                    break;
                register_info.terminal_model.push_back(in[pos + i]);
            }
            pos += 20;
            // Terminal ID.
            for (size_t i = 0; i < 7; ++i) {
                if (in[pos + i] == 0x0)
                    break;
                register_info.terminal_id.push_back(in[pos + i]);
            }
            pos += 7;
            // Vehicle plate color and identifier.
            register_info.car_plate_color = in[pos++];
            if (register_info.car_plate_color != VehiclePlateColor::kVin) {
                register_info.car_plate_num.clear();
                size_t len = para->parse.msg_head.msgbody_attr.bit.msglen - 37;
                register_info.car_plate_num.assign(in.begin() + pos, in.begin() + pos + len);
            }
            return 0;
        }));

    // 0x8100, Terminal registration response.
    parser->insert(std::pair<uint16_t, ParseHandler>(
        kTerminalRegisterResponse, [](InputBuffer in, ProtocolParameter* para) -> int {
            if (para == nullptr)
                return -1;
            uint16_t pos = MSGBODY_NOPACKET_POS;
            // Response flow number.
            para->parse.respone_flow_num = in[pos] * 256 + in[pos + 1];
            // Response result.
            para->parse.respone_result = in[pos + 2];
            // Parse the additional authentication code if the response result is 0 (success).
            if (para->parse.respone_result == 0) {
                auto begin = in.begin() + pos + 3;
                auto end   = begin + para->parse.msg_head.msgbody_attr.bit.msglen - 3;
                para->parse.authentication_code.assign(begin, end);
            }
            return 0;
        }));

    // 0x0003, Terminal logout.
    parser->insert(std::pair<uint16_t, ParseHandler>(
        kTerminalLogOut, [](InputBuffer in, ProtocolParameter* para) -> int {
            if (para == nullptr)
                return -1;
            // Empty message body.
            return 0;
        }));

    // 0x0102, Terminal authentication.
    parser->insert(std::pair<uint16_t, ParseHandler>(
        kTerminalAuthentication, [](InputBuffer in, ProtocolParameter* para) -> int {
            if (para == nullptr)
                return -1;
            uint16_t pos = MSGBODY_NOPACKET_POS;
            if (para->parse.msg_head.msgbody_attr.bit.packet == 1)
                pos = MSGBODY_PACKET_POS;
            // Extract authentication code.
            auto begin = in.begin() + pos;
            auto end   = begin + para->parse.msg_head.msgbody_attr.bit.msglen;
            para->parse.authentication_code.assign(begin, end);
            return 0;
        }));

    // 0x8103, Set terminal parameters.
    parser->insert(std::pair<uint16_t, ParseHandler>(
        kSetTerminalParameters, [](InputBuffer in, ProtocolParameter* para) -> int {
            if (para == nullptr)
                return -1;
            uint16_t pos = MSGBODY_NOPACKET_POS;
            if (para->parse.msg_head.msgbody_attr.bit.packet == 1)
                pos = MSGBODY_PACKET_POS;
            auto const& msg_len = para->parse.msg_head.msgbody_attr.bit.msglen;
            if (msg_len < 1)
                return -1;
            // Total number of parameters set.
            uint8_t cnt = in[pos];
            ++pos;
            U32ToU8Array u32converter;
            // Parameter items set.
            uint32_t             id = 0;
            std::vector<uint8_t> value;
            auto&                paras = para->parse.terminal_parameters;
            paras.clear();
            for (int i = 0; i < cnt; ++i) {
                // Parameter ID.
                memcpy(u32converter.u8array, &(in[pos]), 4);
                id = EndianSwap32(u32converter.u32val);
                pos += 4;
                // Parameter value.
                value.assign(in.begin() + pos + 1, in.begin() + pos + 1 + in[pos]);
                paras.insert({id, value});
                pos += 1 + in[pos];
            }
            return 0;
        }));

    // 0x8104, Query terminal parameters.
    parser->insert(std::pair<uint16_t, ParseHandler>(
        kGetTerminalParameters, [](InputBuffer in, ProtocolParameter* para) -> int {
            if (para == nullptr)
                return -1;
            // Used to distinguish whether it is a query for special terminal parameters.
            para->parse.terminal_parameter_ids.clear();
            // Empty message body.
            return 0;
        }));

    // 0x8106, Query specific terminal parameters.
    parser->insert(std::pair<uint16_t, ParseHandler>(
        kGetSpecificTerminalParameters, [](InputBuffer in, ProtocolParameter* para) -> int {
            if (para == nullptr)
                return -1;
            uint16_t pos = MSGBODY_NOPACKET_POS;
            if (para->parse.msg_head.msgbody_attr.bit.packet == 1)
                pos = MSGBODY_PACKET_POS;
            auto const& msg_len = para->parse.msg_head.msgbody_attr.bit.msglen;
            if (msg_len < 1)
                return -1;
            // Total number of parameter IDs.
            uint8_t cnt = in[pos++];
            if (msg_len != cnt * 4 + 1)
                return -1;
            // Parameter ID parsing.
            uint32_t     id = 0;
            U32ToU8Array u32converter;
            para->parse.terminal_parameter_ids.clear();
            for (uint8_t i = 0; i < cnt; ++i) {
                memcpy(u32converter.u8array, &(in[pos]), 4);
                id = EndianSwap32(u32converter.u32val);
                para->parse.terminal_parameter_ids.push_back(id);
                pos += 4;
            }
            return 0;
        }));

    // 0x0104, Query terminal parameters response.
    parser->insert(std::pair<uint16_t, ParseHandler>(
        kGetTerminalParametersResponse, [](InputBuffer in, ProtocolParameter* para) -> int {
            if (para == nullptr)
                return -1;
            uint16_t pos = MSGBODY_NOPACKET_POS;
            if (para->parse.msg_head.msgbody_attr.bit.packet == 1)
                pos = MSGBODY_PACKET_POS;
            auto const& msg_len = para->parse.msg_head.msgbody_attr.bit.msglen;
            if (msg_len < 3)
                return -1;
            // Response flow number.
            U16ToU8Array u16converter;
            memcpy(u16converter.u8array, &(in[pos]), 2);
            para->parse.respone_flow_num = EndianSwap16(u16converter.u16val);
            pos += 2;
            // The following content is consistent with the parsing of setting terminal parameters.
            // Total number of parameters set.
            uint8_t cnt = in[pos];
            ++pos;
            U32ToU8Array u32converter;
            // Parameter items set.
            uint32_t             id = 0;
            std::vector<uint8_t> value;
            auto&                paras = para->parse.terminal_parameters;
            paras.clear();
            for (int i = 0; i < cnt; ++i) {
                // Parameter ID.
                memcpy(u32converter.u8array, &(in[pos]), 4);
                id = EndianSwap32(u32converter.u32val);
                pos += 4;
                // Parameter value.
                value.assign(in.begin() + pos + 1, in.begin() + pos + 1 + in[pos]);
                paras.insert({id, value});
                pos += 1 + in[pos];
            }
            return 0;
        }));

    // 0x8108, Issue terminal upgrade package.
    parser->insert(std::pair<uint16_t, ParseHandler>(
        kTerminalUpgrade, [](InputBuffer in, ProtocolParameter* para) -> int {
            if (para == nullptr)
                return -1;
            auto const& msg_len = para->parse.msg_head.msgbody_attr.bit.msglen;
            uint16_t    pos     = MSGBODY_NOPACKET_POS;
            if (para->parse.msg_head.msgbody_attr.bit.packet == 1)
                pos = MSGBODY_PACKET_POS;
            uint16_t beg          = pos;
            auto&    upgrade_info = para->parse.upgrade_info;
            // Upgrade type.
            upgrade_info.upgrade_type = in[pos++];
            // Manufacturer ID.
            upgrade_info.manufacturer_id.clear();
            for (int i = 0; i < 5; ++i) {
                upgrade_info.manufacturer_id.push_back(in[pos++]);
            }
            // Upgrade version number.
            upgrade_info.version_id.clear();
            for (int i = 0; i < in[pos]; ++i) {
                upgrade_info.version_id.push_back(static_cast<char>(in[pos + i + 1]));
            }
            pos += in[pos] + 1;
            // Total length of the upgrade package.
            upgrade_info.upgrade_data_total_len =
                in[pos] * 65536 * 256 + in[pos + 1] * 65536 + in[pos + 2] * 256 + in[pos + 3];
            // Upgrade data package content.
            pos += 4;
            uint16_t content_len = msg_len - (pos - beg);
            if (content_len + 9 + upgrade_info.version_id.size() > msg_len)
                return -1;
            upgrade_info.upgrade_data.assign(in.begin() + pos, in.begin() + pos + content_len);
            return 0;
        }));

    // 0x0108, Terminal upgrade result notification.
    parser->insert(std::pair<uint16_t, ParseHandler>(
        kTerminalUpgradeResultReport, [](InputBuffer in, ProtocolParameter* para) -> int {
            if (para == nullptr)
                return -1;
            uint16_t pos = MSGBODY_NOPACKET_POS;
            if (para->parse.msg_head.msgbody_attr.bit.packet == 1)
                pos = MSGBODY_PACKET_POS;
            auto& upgrade_info = para->parse.upgrade_info;
            // Upgrade type.
            upgrade_info.upgrade_type = in[pos++];
            // Upgrade result.
            upgrade_info.upgrade_result = in[pos++];
            return 0;
        }));

    // 0x0200, Location information report.
    parser->insert(std::pair<uint16_t, ParseHandler>(
        kLocationReport, [](InputBuffer in, ProtocolParameter* para) -> int {
            if (para == nullptr)
                return -1;
            auto const& msg_len = para->parse.msg_head.msgbody_attr.bit.msglen;
            if (msg_len < 28)
                return -1;
            uint16_t pos = MSGBODY_NOPACKET_POS;
            if (para->parse.msg_head.msgbody_attr.bit.packet == 1)
                pos = MSGBODY_PACKET_POS;
            auto&        basic_info     = para->parse.location_info;
            auto&        extension_info = para->parse.location_extension;
            U32ToU8Array u32converter;
            // Alarm flag.
            memcpy(u32converter.u8array, &(in[pos]), 4);
            basic_info.alarm.value = EndianSwap32(u32converter.u32val);
            // Status.
            memcpy(u32converter.u8array, &(in[pos + 4]), 4);
            basic_info.status.value = EndianSwap32(u32converter.u32val);
            // Latitude.
            memcpy(u32converter.u8array, &(in[pos + 8]), 4);
            basic_info.latitude = EndianSwap32(u32converter.u32val);
            // Longitude.
            memcpy(u32converter.u8array, &(in[pos + 12]), 4);
            basic_info.longitude = EndianSwap32(u32converter.u32val);
            U16ToU8Array u16converter;
            // Altitude.
            memcpy(u16converter.u8array, &(in[pos + 16]), 2);
            basic_info.altitude = EndianSwap16(u16converter.u16val);
            // Speed.
            memcpy(u16converter.u8array, &(in[pos + 18]), 2);
            basic_info.speed = EndianSwap16(u16converter.u16val);
            // Bearing.
            memcpy(u16converter.u8array, &(in[pos + 20]), 2);
            basic_info.bearing = EndianSwap16(u16converter.u16val);
            // UTC time (BCD-8421 code).
            std::vector<uint8_t> bcd;
            bcd.assign(in.begin() + pos + 22, in.begin() + pos + 28);
            BcdToStringFillZero(bcd, &basic_info.time);
            if (msg_len > 28) { // Location additional information items.
                uint8_t end = msg_len + pos;
                pos += 28;
                std::vector<uint8_t> item_content;
                while (pos <= end - 2) { // Additional information length is at least 1.
                    if (pos + 1 + in[pos + 1] > end)
                        return -1; // Additional information length exceeds the range.
                    item_content.assign(in.begin() + pos + 2, in.begin() + pos + 2 + in[pos + 1]);
                    extension_info[in[pos]] = item_content;
                    pos += 2 + in[pos + 1];
                }
            }
            return 0;
        }));

    // 0x8201, Location information query.
    parser->insert(std::pair<uint16_t, ParseHandler>(
        kGetLocationInformation, [](InputBuffer in, ProtocolParameter* para) -> int {
            if (para == nullptr)
                return -1;
            // Empty message body.
            return 0;
        }));

    // 0x0201, Location information query response.
    parser->insert(std::pair<uint16_t, ParseHandler>(
        kGetLocationInformationResponse, [](InputBuffer in, ProtocolParameter* para) -> int {
            if (para == nullptr)
                return -1;
            auto const& msg_len = para->parse.msg_head.msgbody_attr.bit.msglen;
            if (msg_len < 30)
                return -1;
            uint16_t pos = MSGBODY_NOPACKET_POS;
            if (para->parse.msg_head.msgbody_attr.bit.packet == 1)
                pos = MSGBODY_PACKET_POS;
            // Response flow number.
            para->parse.respone_flow_num = in[pos] * 256 + in[pos];
            pos += 2;
            // The following is the location information report content.
            auto&        basic_info     = para->parse.location_info;
            auto&        extension_info = para->parse.location_extension;
            U32ToU8Array u32converter;
            // Alarm flag.
            memcpy(u32converter.u8array, &(in[pos]), 4);
            basic_info.alarm.value = EndianSwap32(u32converter.u32val);
            // Status.
            memcpy(u32converter.u8array, &(in[pos + 4]), 4);
            basic_info.status.value = EndianSwap32(u32converter.u32val);
            // Latitude.
            memcpy(u32converter.u8array, &(in[pos + 8]), 4);
            basic_info.latitude = EndianSwap32(u32converter.u32val);
            // Longitude.
            memcpy(u32converter.u8array, &(in[pos + 12]), 4);
            basic_info.longitude = EndianSwap32(u32converter.u32val);
            U16ToU8Array u16converter;
            // Altitude.
            memcpy(u16converter.u8array, &(in[pos + 16]), 2);
            basic_info.altitude = EndianSwap16(u16converter.u16val);
            // Speed.
            memcpy(u16converter.u8array, &(in[pos + 18]), 2);
            basic_info.speed = EndianSwap16(u16converter.u16val);
            // Bearing.
            memcpy(u16converter.u8array, &(in[pos + 20]), 2);
            basic_info.bearing = EndianSwap16(u16converter.u16val);
            // UTC time (BCD-8421 code).
            std::vector<uint8_t> bcd;
            bcd.assign(in.begin() + pos + 22, in.begin() + pos + 28);
            BcdToStringFillZero(bcd, &basic_info.time);
            if (msg_len > 28) { // Location additional information items.
                uint8_t end = msg_len + pos;
                pos += 28;
                std::vector<uint8_t> item_content;
                while (pos <= end - 2) { // Additional information length is at least 1.
                    if (pos + 1 + in[pos + 1] > end)
                        return -1; // Additional information length exceeds the range.
                    item_content.assign(in.begin() + pos + 2, in.begin() + pos + 2 + in[pos + 1]);
                    extension_info[in[pos]] = item_content;
                    pos += 2 + in[pos + 1];
                }
            }
            return 0;
        }));

    // 0x8202, Temporary location tracking control.
    parser->insert(std::pair<uint16_t, ParseHandler>(
        kLocationTrackingControl, [](InputBuffer in, ProtocolParameter* para) -> int {
            if (para == nullptr)
                return -1;
            auto const& msg_len = para->parse.msg_head.msgbody_attr.bit.msglen;
            if (msg_len != 6)
                return -1;
            uint16_t pos = MSGBODY_NOPACKET_POS;
            if (para->parse.msg_head.msgbody_attr.bit.packet == 1)
                pos = MSGBODY_PACKET_POS;
            auto& ctrl = para->parse.location_tracking_control;
            // Location information reporting interval during tracking.
            ctrl.interval = in[pos] * 256 + in[pos + 1];
            pos += 2;
            // Tracking valid time.
            U32ToU8Array u32converter;
            memcpy(u32converter.u8array, &(in[pos]), 4);
            ctrl.tracking_time = EndianSwap32(u32converter.u32val);
            return 0;
        }));

    // 0x08604, Set polygon area.
    parser->insert(std::pair<uint16_t, ParseHandler>(
        kSetPolygonArea, [](InputBuffer in, ProtocolParameter* para) -> int {
            if (para == nullptr)
                return -1;
            auto const& msg_len = para->parse.msg_head.msgbody_attr.bit.msglen;
            if (msg_len < 28)
                return -1;
            uint16_t pos = MSGBODY_NOPACKET_POS;
            if (para->parse.msg_head.msgbody_attr.bit.packet == 1)
                pos = MSGBODY_PACKET_POS;
            uint16_t     end          = pos + msg_len;
            auto&        polygon_area = para->parse.polygon_area;
            U16ToU8Array u16converter;
            U32ToU8Array u32converter;
            // Area ID.
            memcpy(u32converter.u8array, &(in[pos]), 4);
            polygon_area.area_id = EndianSwap32(u32converter.u32val);
            pos += 4;
            // Area attributes.
            memcpy(u16converter.u8array, &(in[pos]), 2);
            polygon_area.area_attribute.value = EndianSwap16(u16converter.u16val);
            pos += 2;
            // Start time, enabled only if the relevant flag in area attributes is set to 1.
            if (polygon_area.area_attribute.bit.by_time) {
                std::vector<uint8_t> bcd;
                bcd.assign(in.begin() + pos, in.begin() + pos);
                BcdToStringFillZero(bcd, &polygon_area.start_time);
                pos += 6;
                bcd.assign(in.begin() + pos, in.begin() + pos);
                BcdToStringFillZero(bcd, &polygon_area.stop_time);
                pos += 6;
            }
            // Speed limit, enabled only if the relevant flag in area attributes is set to 1.
            if (polygon_area.area_attribute.bit.speed_limit) {
                memcpy(u16converter.u8array, &(in[pos]), 2);
                polygon_area.max_speed = EndianSwap16(u16converter.u16val);
                pos += 2;
                polygon_area.overspeed_time = in[pos];
                ++pos;
            }
            // Number of vertices.
            memcpy(u16converter.u8array, &(in[pos]), 2);
            uint16_t cnt = EndianSwap16(u16converter.u16val);
            pos += 2;
            // Check the length of the subsequent content.
            if (end - pos != cnt * 8)
                return -1;
            LocationPoint location_point {};
            polygon_area.vertices.clear();
            // All vertex latitudes and longitudes.
            while (pos < end) {
                memcpy(u32converter.u8array, &(in[pos]), 4);
                location_point.latitude = EndianSwap32(u32converter.u32val) * 1e-6;
                pos += 4;
                memcpy(u32converter.u8array, &(in[pos]), 4);
                location_point.longitude = EndianSwap32(u32converter.u32val) * 1e-6;
                pos += 4;
                polygon_area.vertices.push_back(location_point);
            }
            return 0;
        }));

    // 0x08605, Delete polygon area.
    parser->insert(std::pair<uint16_t, ParseHandler>(
        kDeletePolygonArea, [](InputBuffer in, ProtocolParameter* para) -> int {
            if (para == nullptr)
                return -1;
            auto const& msg_len = para->parse.msg_head.msgbody_attr.bit.msglen;
            uint16_t    pos     = MSGBODY_NOPACKET_POS;
            if (para->parse.msg_head.msgbody_attr.bit.packet == 1)
                pos = MSGBODY_PACKET_POS;
            // Number of areas to delete.
            uint8_t cnt = in[pos];
            if (cnt * 4 + 1 != msg_len)
                return -1;
            auto& polygon_area_id = para->parse.polygon_area_id;
            polygon_area_id.clear();
            U32ToU8Array u32converter;
            // All area IDs to delete.
            for (uint8_t i = 0; i < cnt; ++i) {
                memcpy(u32converter.u8array, &(in[pos + 1 + i * 4]), 4);
                polygon_area_id.push_back(EndianSwap32(u32converter.u32val));
            }
            return 0;
        }));

    // 0x0801, Multimedia data upload.
    parser->insert(std::pair<uint16_t, ParseHandler>(
        kMultimediaDataUpload, [](InputBuffer in, ProtocolParameter* para) -> int {
            if (para == nullptr)
                return -1;
            auto const& msg_len = para->parse.msg_head.msgbody_attr.bit.msglen;
            uint16_t    pos     = MSGBODY_NOPACKET_POS;
            if (para->parse.msg_head.msgbody_attr.bit.packet == 1)
                pos = MSGBODY_PACKET_POS;
            U32ToU8Array u32converter;
            // Multimedia ID.
            memcpy(u32converter.u8array, in.data() + pos, 4);
            para->parse.multimedia_upload.media_id = EndianSwap32(u32converter.u32val);
            // Multimedia type.
            para->parse.multimedia_upload.media_type = in[pos + 4];
            // Multimedia format.
            para->parse.multimedia_upload.media_format = in[pos + 5];
            // Event item.
            para->parse.multimedia_upload.media_event = in[pos + 6];
            // Channel ID.
            para->parse.multimedia_upload.channel_id = in[pos + 7];
            para->parse.multimedia_upload.loaction_report_body.assign(in.begin() + pos + 8, in.begin() + pos + 36);
            para->parse.multimedia_upload.media_data.assign(in.begin() + pos + 36, in.begin() + pos + msg_len);
            return 0;
        }));

    // 0x8800, Multimedia data upload response.
    parser->insert(std::pair<uint16_t, ParseHandler>(
        kMultimediaDataUploadResponse, [](InputBuffer in, ProtocolParameter* para) -> int {
            if (para == nullptr)
                return -1;
            auto const& msg_len = para->parse.msg_head.msgbody_attr.bit.msglen;
            uint16_t    pos     = MSGBODY_NOPACKET_POS;
            if (para->parse.msg_head.msgbody_attr.bit.packet == 1)
                pos = MSGBODY_PACKET_POS;
            U32ToU8Array u32converter;
            // Multimedia ID.
            memcpy(u32converter.u8array, in.data() + pos, 4);
            para->parse.multimedia_upload_response.media_id = EndianSwap32(u32converter.u32val);
            // Check if retransmission is needed.
            if (msg_len > 4) {
                U16ToU8Array u16converter;
                para->parse.multimedia_upload_response.reload_packet_ids.clear();
                int   cnt = in[pos + 4];
                auto& ids = para->parse.multimedia_upload_response.reload_packet_ids;
                for (int i = 0; i < cnt; ++i) {
                    memcpy(u16converter.u8array, &(in[pos + 5 + 2 * i]), 2);
                    ids.push_back(EndianSwap16(u16converter.u16val));
                }
            }
            return 0;
        }));

    return 0;
}

// Additional parser support commands.
bool JT808FrameParserAppend(Parser* parser, std::pair<uint16_t, ParseHandler> const& pair) {
    if (parser == nullptr)
        return false;
    return parser->insert(pair).second;
}

// Additional parser support commands.
bool JT808FrameParserAppend(Parser* parser, uint16_t const& msg_id, ParseHandler const& handler) {
    return JT808FrameParserAppend(parser, {msg_id, handler});
}

// Override parser support commands.
bool JT808FrameParserOverride(Parser* parser, std::pair<uint16_t, ParseHandler> const& pair) {
    if (parser == nullptr)
        return false;
    for (auto const& item : *parser) {
        if (item.first == pair.first) {
            parser->erase(item.first);
            break;
        }
    }
    return parser->insert(pair).second;
}

// Override parser support commands.
bool JT808FrameParserOverride(Parser* parser, uint16_t const& msg_id, ParseHandler const& handler) {
    return JT808FrameParserOverride(parser, {msg_id, handler});
}

/**
 * @brief Parses a JT808 frame.
 *
 * @param parser The parser containing message ID to function mappings.
 * @param in The input vector of bytes to be parsed.
 * @param para The protocol parameter structure pointer to store parsed data.
 * @return int Returns 0 on success, -1 on failure.
 */
std::error_code JT808FrameParse(Parser const& parser, InputBuffer in, ProtocolParameter* para) {
    if (para == nullptr)
        return make_error_code(ParserError::ParametersNull);
    std::vector<uint8_t> out;
    out.reserve(in.size());
    // Reverse escape.
    if (ReverseEscape(in, out) < 0)
        return make_error_code(ParserError::UnesapingError);
    // XOR checksum check.
    if (BccCheckSum(&(out[1]), out.size() - 3) != *(out.end() - 2))
        return make_error_code(ParserError::ChecksumError);
    // Parse message header.
    if (JT808FrameHeadParse(out, &para->parse.msg_head) != 0)
        return make_error_code(ParserError::HeaderParseError);
    para->msg_head.phone_num = para->parse.msg_head.phone_num;
    // Parse message content.
    auto it = parser.find(para->parse.msg_head.msg_id);
    if (it == parser.end())
        return make_error_code(ParserError::UnregisteredMessageParser);
    return std::error_code(it->second(out, para), parser_category());
}

} // namespace libjt808
