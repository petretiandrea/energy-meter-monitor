#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#ifndef _DATA_MESSAGE_H_
#define _DATA_MESSAGE_H_

#define MAX_DATA_BUFFER 50
#define MAX_DEVICE_NAME_BUFFER 20

// Common model with gateway
namespace protocol
{
    namespace edge
    {
        enum DeviceType
        {
            POWER_METER_PULSES,
            UNKNOWN
        };

        struct DataMessage
        {
            char data[MAX_DATA_BUFFER];
        };

        struct AnnounceMessage
        {
            const char device_name[MAX_DEVICE_NAME_BUFFER];
        };

        // serialized espnow message
        struct ProtocolMessage
        {
            bool is_announce;
            DeviceType type;
            union
            {
                DataMessage data;
                AnnounceMessage announce;
            };
        };
    }
}

#endif

#endif