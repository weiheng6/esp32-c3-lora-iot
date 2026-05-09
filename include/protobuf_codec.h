#ifndef PROTOBUF_CODEC_H
#define PROTOBUF_CODEC_H

#include <Arduino.h>
#include <stdint.h>
#include <string.h>

class ProtobufCodec {
private:
    uint8_t* buffer;
    size_t bufferSize;
    size_t encodedSize;

public:
    ProtobufCodec(size_t maxBufferSize = 256);
    ~ProtobufCodec();

    bool encodeSensorData(
        const char* deviceId,
        int64_t timestamp,
        float temperature,
        float humidity,
        int32_t batteryLevel,
        int32_t signalStrength
    );

    bool decodeCommand(
        char* type,
        size_t typeSize,
        char* params,
        size_t paramsSize,
        const uint8_t* data,
        size_t length
    );

    const uint8_t* getBuffer() const { return buffer; }
    size_t getEncodedSize() const { return encodedSize; }
    void reset() { encodedSize = 0; }
};

#endif
