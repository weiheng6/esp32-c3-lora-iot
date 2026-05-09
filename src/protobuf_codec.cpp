#include "protobuf_codec.h"

ProtobufCodec::ProtobufCodec(size_t maxBufferSize) 
    : bufferSize(maxBufferSize), encodedSize(0) {
    buffer = new uint8_t[bufferSize];
    memset(buffer, 0, bufferSize);
}

ProtobufCodec::~ProtobufCodec() {
    if (buffer) {
        delete[] buffer;
        buffer = nullptr;
    }
}

bool ProtobufCodec::encodeSensorData(
    const char* deviceId,
    int64_t timestamp,
    float temperature,
    float humidity,
    int32_t batteryLevel,
    int32_t signalStrength
) {
    if (!buffer || encodedSize >= bufferSize) {
        return false;
    }

    memset(buffer, 0, bufferSize);
    encodedSize = 0;

    buffer[encodedSize++] = 0x01;

    size_t devIdLen = strlen(deviceId);
    if (devIdLen > 30) devIdLen = 30;
    buffer[encodedSize++] = devIdLen;
    memcpy(&buffer[encodedSize], deviceId, devIdLen);
    encodedSize += devIdLen;

    buffer[encodedSize++] = 0x02;
    uint8_t tsBytes[8];
    for (int i = 0; i < 8; i++) {
        tsBytes[i] = (timestamp >> (i * 8)) & 0xFF;
    }
    memcpy(&buffer[encodedSize], tsBytes, 8);
    encodedSize += 8;

    buffer[encodedSize++] = 0x03;
    uint8_t tempBytes[4];
    memcpy(tempBytes, &temperature, 4);
    buffer[encodedSize++] = tempBytes[0];
    buffer[encodedSize++] = tempBytes[1];
    buffer[encodedSize++] = tempBytes[2];
    buffer[encodedSize++] = tempBytes[3];

    buffer[encodedSize++] = 0x04;
    uint8_t humBytes[4];
    memcpy(humBytes, &humidity, 4);
    buffer[encodedSize++] = humBytes[0];
    buffer[encodedSize++] = humBytes[1];
    buffer[encodedSize++] = humBytes[2];
    buffer[encodedSize++] = humBytes[3];

    buffer[encodedSize++] = 0x05;
    uint8_t batBytes[4];
    memcpy(batBytes, &batteryLevel, 4);
    buffer[encodedSize++] = batBytes[0];
    buffer[encodedSize++] = batBytes[1];
    buffer[encodedSize++] = batBytes[2];
    buffer[encodedSize++] = batBytes[3];

    buffer[encodedSize++] = 0x06;
    uint8_t rssiBytes[4];
    memcpy(rssiBytes, &signalStrength, 4);
    buffer[encodedSize++] = rssiBytes[0];
    buffer[encodedSize++] = rssiBytes[1];
    buffer[encodedSize++] = rssiBytes[2];
    buffer[encodedSize++] = rssiBytes[3];

    buffer[encodedSize++] = 0xFF;

    return true;
}

bool ProtobufCodec::decodeCommand(
    char* type,
    size_t typeSize,
    char* params,
    size_t paramsSize,
    const uint8_t* data,
    size_t length
) {
    if (!data || length == 0 || !type || !params) {
        return false;
    }

    memset(type, 0, typeSize);
    memset(params, 0, paramsSize);

    if (length < 3) {
        return false;
    }

    size_t pos = 0;
    while (pos < length - 1) {
        uint8_t tag = data[pos++];
        if (tag == 0xFF) break;

        switch (tag) {
            case 0x01: {
                uint8_t len = data[pos++];
                if (len < typeSize && pos + len <= length) {
                    memcpy(type, &data[pos], len);
                    pos += len;
                }
                break;
            }
            case 0x02: {
                uint8_t len = data[pos++];
                if (len < paramsSize && pos + len <= length) {
                    memcpy(params, &data[pos], len);
                    pos += len;
                }
                break;
            }
            default:
                if (pos < length) pos++;
                break;
        }
    }

    return strlen(type) > 0;
}
