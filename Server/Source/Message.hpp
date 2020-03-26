/*
 * Copyright (c) 2020 Andreas Pohl
 * Licensed under MIT (https://github.com/apohl79/audiogridder/blob/master/COPYING)
 *
 * Author: Andreas Pohl
 */

#ifndef Message_hpp
#define Message_hpp

#include <sys/socket.h>
#include <type_traits>

#include "KeyAndMouse.hpp"

namespace e47 {

/*
 * Core I/O functions
 */
struct MessageHelper {
    enum ReadError { E_NONE, E_DATA, E_TIMEOUT, E_STATE, E_SYSCALL };
    static void seterr(ReadError* e, ReadError c) {
        if (nullptr != e) {
            *e = c;
        }
    }
};

static bool send(StreamingSocket* socket, const char* data, int size) {
    if (nullptr != socket && socket->isConnected()) {
        int toWrite = size;
        do {
            int ret = socket->write(static_cast<const char*>(data) + size - toWrite, toWrite);
            data += ret;
            toWrite -= ret;
        } while (toWrite > 0);
        return true;
    } else {
        return false;
    }
}

static bool read(StreamingSocket* socket, void* data, int size, int timeoutMilliseconds = 0,
                 MessageHelper::ReadError* e = nullptr) {
    MessageHelper::seterr(e, MessageHelper::E_NONE);
    if (nullptr != socket && !socket->isConnected()) {
        MessageHelper::seterr(e, MessageHelper::E_STATE);
        return false;
    }
    auto now = Time::getMillisecondCounterHiRes();
    auto until = now;
    if (timeoutMilliseconds > 0) {
        until += timeoutMilliseconds;
    }
    int toRead = size;
    while (toRead > 0 && now <= until) {
        int ret = socket->waitUntilReady(true, 100);
        if (ret < 0) {
            MessageHelper::seterr(e, MessageHelper::E_SYSCALL);
            return false;  // error
        } else if (ret > 0) {
            int len = socket->read(static_cast<char*>(data) + size - toRead, toRead, timeoutMilliseconds == 0);
            if (len < 0) {
                MessageHelper::seterr(e, MessageHelper::E_SYSCALL);
                return false;
            } else if (len == 0) {
                MessageHelper::seterr(e, MessageHelper::E_DATA);
                return false;
            }
            toRead -= len;
        }
        now = Time::getMillisecondCounterHiRes();
    }
    if (toRead == 0) {
        return true;
    } else {
        MessageHelper::seterr(e, MessageHelper::E_TIMEOUT);
        return false;
    }
}

/*
 * Client/Server handshake
 */
struct Handshake {
    int version;
    int clientPort;
    int channels;
    double rate;
    int samplesPerBlock;
    bool doublePrecission;
};

/*
 * Audio streaming
 */
class AudioMessage {
  public:
    struct RequestHeader {
        int channels;
        int samples;
        bool isDouble;
    };

    struct ResponseHeader {
        int latencySamples;
    };

    int getChannels() const { return m_reqHeader.channels; }
    int getSamples() const { return m_reqHeader.samples; }
    bool isDouble() const { return m_reqHeader.isDouble; }

    int getLatencySamples() const { return m_resHeader.latencySamples; }

    template <typename T>
    bool sendToServer(StreamingSocket* socket, AudioBuffer<T>& buffer) {
        m_reqHeader.channels = buffer.getNumChannels();
        m_reqHeader.samples = buffer.getNumSamples();
        m_reqHeader.isDouble = std::is_same<T, double>::value;
        if (socket->isConnected()) {
            if (!send(socket, reinterpret_cast<const char*>(&m_reqHeader), sizeof(m_reqHeader))) {
                return false;
            }
            for (int chan = 0; chan < m_reqHeader.channels; ++chan) {
                if (!send(socket, reinterpret_cast<const char*>(buffer.getReadPointer(chan)),
                          m_reqHeader.samples * sizeof(T))) {
                    return false;
                }
            }
        }
        return true;
    }

    template <typename T>
    bool sendToClient(StreamingSocket* socket, AudioBuffer<T>& buffer, int latencySamples) {
        m_resHeader.latencySamples = latencySamples;
        if (socket->isConnected()) {
            if (!send(socket, reinterpret_cast<const char*>(&m_resHeader), sizeof(m_resHeader))) {
                return false;
            }
            for (int chan = 0; chan < m_reqHeader.channels; ++chan) {
                if (!send(socket, reinterpret_cast<const char*>(buffer.getReadPointer(chan)),
                          m_reqHeader.samples * sizeof(T))) {
                    return false;
                }
            }
        }
        return true;
    }

    template <typename T>
    bool readFromServer(StreamingSocket* socket, AudioBuffer<T>& buffer, MessageHelper::ReadError* e) {
        if (socket->isConnected()) {
            if (!read(socket, &m_resHeader, sizeof(m_resHeader), 1000, e)) {
                return false;
            }
            for (int chan = 0; chan < buffer.getNumChannels(); ++chan) {
                if (!read(socket, buffer.getWritePointer(chan), buffer.getNumSamples() * sizeof(T), 1000, e)) {
                    return false;
                }
            }
        } else {
            *e = MessageHelper::E_STATE;
            return false;
        }
        *e = MessageHelper::E_NONE;
        return true;
    }

    bool readFromClient(StreamingSocket* socket, AudioBuffer<float>& bufferF, AudioBuffer<double>& bufferD) {
        if (socket->isConnected()) {
            MessageHelper::ReadError e;
            do {
                if (!read(socket, &m_reqHeader, sizeof(m_reqHeader), 1000, &e) && e != MessageHelper::E_TIMEOUT) {
                    return false;
                }
            } while (e == MessageHelper::E_TIMEOUT);
            int size;
            if (m_reqHeader.isDouble) {
                bufferD.setSize(m_reqHeader.channels, m_reqHeader.samples);
                size = m_reqHeader.samples * sizeof(double);
            } else {
                bufferF.setSize(m_reqHeader.channels, m_reqHeader.samples);
                size = m_reqHeader.samples * sizeof(float);
            }
            for (int chan = 0; chan < m_reqHeader.channels; ++chan) {
                char* data = m_reqHeader.isDouble ? reinterpret_cast<char*>(bufferD.getWritePointer(chan))
                                                  : reinterpret_cast<char*>(bufferF.getWritePointer(chan));
                do {
                    if (!read(socket, data, size, 0, &e) && e != MessageHelper::E_TIMEOUT) {
                        return false;
                    }
                } while (e == MessageHelper::E_TIMEOUT);
            }
        } else {
            return false;
        }
        return true;
    }

  private:
    RequestHeader m_reqHeader;
    ResponseHeader m_resHeader;
};

/*
 * Command I/O
 */
class Payload {
  public:
    using Buffer = std::vector<char>;

    Payload() : payloadType(-1) {}
    Payload(int t, size_t s = 0) : payloadType(t), payloadBuffer(s) { memset(getData(), 0, s); }
    virtual ~Payload() {}
    Payload& operator=(const Payload& other) = delete;
    Payload& operator=(Payload&& other) {
        if (this != &other) {
            payloadType = other.payloadType;
            other.payloadType = -1;
            payloadBuffer = std::move(other.payloadBuffer);
        }
        return *this;
    }

    int getType() const { return payloadType; }
    void setType(int t) { payloadType = t; }
    size_t getSize() const { return payloadBuffer.size(); }
    void setSize(size_t size) {
        payloadBuffer.resize(size);
        realign();
    }
    char* getData() { return reinterpret_cast<char*>(payloadBuffer.data()); }
    const char* getData() const { return payloadBuffer.data(); }

    virtual void realign() {}

    int payloadType;
    Buffer payloadBuffer;
};

template <typename T>
class DataPayload : public Payload {
  public:
    T* data;
    DataPayload(int type) : Payload(type, sizeof(T)) { realign(); }
    virtual void realign() override { data = reinterpret_cast<T*>(payloadBuffer.data()); }
};

class NumberPayload : public DataPayload<int> {
  public:
    NumberPayload(int type) : DataPayload<int>(type) {}
    void setNumber(int n) { *data = n; }
    int getNumber() const { return *data; }
};

class StringPayload : public Payload {
  public:
    int* size;
    char* str;

    StringPayload(int type) : Payload(type, sizeof(int)) { realign(); }

    void setString(const String& s) {
        setSize(sizeof(int) + s.length());
        *size = s.length();
        memcpy(str, s.getCharPointer(), s.length());
    }

    String getString() const { return String(str, *size); }

    virtual void realign() override {
        size = reinterpret_cast<int*>(payloadBuffer.data());
        str = getSize() > sizeof(int) ? reinterpret_cast<char*>(payloadBuffer.data()) + sizeof(int) : nullptr;
    }
};

class BinaryPayload : public Payload {
  public:
    int* size;
    char* data;

    BinaryPayload(int type) : Payload(type, sizeof(int)) { realign(); }

    void setData(const char* src, int len) {
        setSize(sizeof(int) + len);
        *size = len;
        memcpy(data, src, len);
    }

    virtual void realign() override {
        size = reinterpret_cast<int*>(payloadBuffer.data());
        data = getSize() > sizeof(int) ? reinterpret_cast<char*>(payloadBuffer.data()) + sizeof(int) : nullptr;
    }
};

class Any : public Payload {
  public:
    static constexpr int Type = 0;
    Any() : Payload(Type) {}
};

class Quit : public Payload {
  public:
    static constexpr int Type = 1;
    Quit() : Payload(Type) {}
};

class Result : public Payload {
  public:
    static constexpr int Type = 2;

    struct hdr_t {
        int rc;
        int size;
    };
    hdr_t* hdr;
    char* str;

    Result() : Payload(Type) { realign(); }

    void setResult(int rc, const String& s) {
        setSize(sizeof(hdr_t) + s.length());
        hdr->rc = rc;
        hdr->size = s.length();
        memcpy(str, s.getCharPointer(), s.length());
    }

    int getReturnCode() const { return hdr->rc; }
    String getString() const { return String(str, hdr->size); }

    virtual void realign() override {
        hdr = reinterpret_cast<hdr_t*>(payloadBuffer.data());
        str = getSize() > sizeof(hdr_t) ? reinterpret_cast<char*>(payloadBuffer.data()) + sizeof(hdr_t) : nullptr;
    }
};

struct preparetoplay_data_t {
    double rate;
    int samples;
};

class PluginList : public StringPayload {
  public:
    static constexpr int Type = 3;
    PluginList() : StringPayload(Type) {}
};

class AddPlugin : public StringPayload {
  public:
    static constexpr int Type = 4;
    AddPlugin() : StringPayload(Type) {}
};

class DelPlugin : public NumberPayload {
  public:
    static constexpr int Type = 5;
    DelPlugin() : NumberPayload(Type) {}
};

class EditPlugin : public NumberPayload {
  public:
    static constexpr int Type = 6;
    EditPlugin() : NumberPayload(Type) {}
};

class HidePlugin : public Payload {
  public:
    static constexpr int Type = 7;
    HidePlugin() : Payload(Type) {}
};

class ScreenCapture : public Payload {
  public:
    static constexpr int Type = 8;

    struct hdr_t {
        int width;
        int height;
        size_t size;
    };
    hdr_t* hdr;
    char* data;

    ScreenCapture() : Payload(Type) { realign(); }

    void setImage(int width, int height, const void* p, size_t size) {
        setSize(sizeof(hdr_t) + size);
        hdr->width = width;
        hdr->height = height;
        hdr->size = size;
        if (nullptr != p) {
            memcpy(data, p, size);
        }
    }

    virtual void realign() override {
        hdr = reinterpret_cast<hdr_t*>(payloadBuffer.data());
        data = getSize() > sizeof(hdr_t) ? reinterpret_cast<char*>(payloadBuffer.data()) + sizeof(hdr_t) : nullptr;
    }
};

struct mouseevent_t {
    MouseEvType type;
    float x;
    float y;
};

class Mouse : public DataPayload<mouseevent_t> {
  public:
    static constexpr int Type = 9;
    Mouse() : DataPayload<mouseevent_t>(Type) {}
};

class GetPluginSettings : public NumberPayload {
  public:
    static constexpr int Type = 10;
    GetPluginSettings() : NumberPayload(Type) {}
};

class PluginSettings : public BinaryPayload {
  public:
    static constexpr int Type = 11;
    PluginSettings() : BinaryPayload(Type) {}
};

class Key : public BinaryPayload {
  public:
    static constexpr int Type = 12;
    Key() : BinaryPayload(Type) {}

    const uint16_t* getKeyCodes() const { return reinterpret_cast<const uint16_t*>(data); }
    size_t getKeyCount() const { return *size / sizeof(uint16_t); }
};

class BypassPlugin : public NumberPayload {
  public:
    static constexpr int Type = 13;
    BypassPlugin() : NumberPayload(Type) {}
};

class UnbypassPlugin : public NumberPayload {
  public:
    static constexpr int Type = 14;
    UnbypassPlugin() : NumberPayload(Type) {}
};

struct exchange_t {
    int idxA;
    int idxB;
};

class ExchangePlugins : public DataPayload<exchange_t> {
  public:
    static constexpr int Type = 15;
    ExchangePlugins() : DataPayload<exchange_t>(Type) {}
};

class RecentsList : public StringPayload {
  public:
    static constexpr int Type = 16;
    RecentsList() : StringPayload(Type) {}
};

struct parameter_t {
    char name[32];
    int index;
    int numSteps;
    bool isDiscrete;
    bool isBoolean;
    bool isMetaParameter;
};

class Parameter : public DataPayload<parameter_t> {
  public:
    static constexpr int Type = 16;
    Parameter() : DataPayload<parameter_t>(Type) {}
};

struct parametervalue_t {
    int index;
    float value;
};

class ParameterValue : public DataPayload<parametervalue_t> {
  public:
    static constexpr int Type = 17;
    ParameterValue() : DataPayload<parametervalue_t>(Type) {}
};

template <typename T>
class Message {
  public:
    static constexpr size_t MAX_SIZE = 200 * 1024;

    struct Header {
        int type;
        int size;
    };

    virtual ~Message() {}

    bool read(StreamingSocket* socket, MessageHelper::ReadError* e = nullptr, int timeoutMilliseconds = 1000) {
        bool success = false;
        MessageHelper::seterr(e, MessageHelper::E_NONE);
        if (nullptr != socket && socket->isConnected()) {
            Header hdr;
            success = true;
            int ret = socket->waitUntilReady(true, timeoutMilliseconds);
            if (ret > 0) {
                if (e47::read(socket, &hdr, sizeof(hdr))) {
                    if (T::Type > 0 && hdr.type != T::Type) {
                        std::cerr << "invalid message type " << hdr.type << " (" << T::Type << " expected)"
                                  << std::endl;
                        success = false;
                        MessageHelper::seterr(e, MessageHelper::E_DATA);
                    } else {
                        payload.setType(hdr.type);
                        if (hdr.size > 0) {
                            if (hdr.size > MAX_SIZE) {
                                std::cerr << "max size of " << MAX_SIZE << " bytes exceeded (" << hdr.size << " bytes)"
                                          << std::endl;
                                success = false;
                                MessageHelper::seterr(e, MessageHelper::E_DATA);
                            } else {
                                if (payload.getSize() != hdr.size) {
                                    payload.setSize(hdr.size);
                                }
                                if (!e47::read(socket, payload.getData(), hdr.size)) {
                                    std::cerr << "failed to read message body" << std::endl;
                                    success = false;
                                    MessageHelper::seterr(e, MessageHelper::E_DATA);
                                }
                            }
                        }
                    }
                } else {
                    std::cerr << "failed to read message header" << std::endl;
                    success = false;
                    MessageHelper::seterr(e, MessageHelper::E_DATA);
                }
            } else if (ret < 0) {
                std::cerr << "failed to wait for message header" << std::endl;
                success = false;
                MessageHelper::seterr(e, MessageHelper::E_SYSCALL);
            } else {
                success = false;
                MessageHelper::seterr(e, MessageHelper::E_TIMEOUT);
            }
        } else {
            MessageHelper::seterr(e, MessageHelper::E_STATE);
        }
        return success;
    }

    bool send(StreamingSocket* socket) {
        Header hdr = {payload.getType(), (int)payload.getSize()};
        if (!e47::send(socket, reinterpret_cast<const char*>(&hdr), sizeof(hdr))) {
            return false;
        }
        if (payload.getSize() > 0 && !e47::send(socket, payload.getData(), (int)payload.getSize())) {
            return false;
        }
        return true;
    }

    int getType() const { return payload.getType(); }
    size_t getSize() const { return payload.getSize(); }
    const char* getData() const { return payload.getData(); }

    template <typename T2>
    static std::shared_ptr<Message<T2>> convert(std::shared_ptr<Message<T>> in) {
        auto out = std::make_shared<Message<T2>>();
        out->payload.payloadBuffer = std::move(in->payload.payloadBuffer);
        out->payload.realign();
        return out;
    }

    T payload;
};

class MessageFactory {
  public:
    static std::shared_ptr<Message<Any>> getNextMessage(StreamingSocket* socket) {
        if (nullptr != socket) {
            auto msg = std::make_shared<Message<Any>>();
            MessageHelper::ReadError e;
            if (msg->read(socket, &e)) {
                return msg;
            } else if (e != MessageHelper::E_TIMEOUT) {
                socket->close();
            }
        }
        return nullptr;
    }

    static std::shared_ptr<Result> getResult(StreamingSocket* socket) {
        if (nullptr != socket) {
            auto msg = std::make_shared<Message<Result>>();
            MessageHelper::ReadError e;
            int retry = 5;
            do {
                if (msg->read(socket, &e)) {
                    auto res = std::make_shared<Result>();
                    *res = std::move(msg->payload);
                    return res;
                }
            } while (retry-- > 0 && e == MessageHelper::E_TIMEOUT);
        }
        return nullptr;
    }

    static bool sendResult(StreamingSocket* socket, int rc) { return sendResult(socket, rc, ""); }

    static bool sendResult(StreamingSocket* socket, int rc, const String& str) {
        Message<Result> msg;
        msg.payload.setResult(rc, str);
        return msg.send(socket);
    }
};

}  // namespace e47

#endif /* Message_hpp */
