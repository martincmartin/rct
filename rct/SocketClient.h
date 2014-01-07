#ifndef TCPSOCKET_H
#define TCPSOCKET_H

#include "SignalSlot.h"
#include "Buffer.h"
#include "String.h"
#include <memory>

class SocketClient : public std::enable_shared_from_this<SocketClient>
{
public:
    typedef std::shared_ptr<SocketClient> SharedPtr;
    typedef std::weak_ptr<SocketClient> WeakPtr;

    enum Mode {
        None = 0x0,
        Tcp = 0x1,
        Udp = 0x2,
        Unix = 0x4,
        IPv6 = 0x8
    };

    SocketClient();
    SocketClient(int fd, unsigned int mode);
    ~SocketClient();

    enum State { Disconnected, Connecting, Connected };
    State state() const { return socketState; }
    unsigned int mode() const { return socketMode; }

    bool connect(const String& path); // UNIX
    bool connect(const String& host, uint16_t port); // TCP
    bool bind(uint16_t port); // UDP

    String hostName() const { return (socketMode & Tcp ? address : String()); }
    String path() const { return (socketMode & Unix ? address : String()); }
    uint16_t port() const { return socketPort; }

    bool isConnected() const { return fd != -1; }
    int socket() const { return fd; }

    enum WriteMode {
        Synchronous,
        Asynchronous
    };
    void setWriteMode(WriteMode m) { wMode = m; }
    WriteMode writeMode() const { return wMode; }

    void close();

    // TCP/UNIX
    bool write(const unsigned char* data, unsigned int num);
    bool write(const String& data) { return write(reinterpret_cast<const unsigned char*>(&data[0]), data.size()); }
    bool peer(String* ip, uint16_t* port = 0) const;
    String peer() const
    {
        String ip;
        if (peer(&ip))
            return ip;
        return String();
    }
    String peerName() const
    {
        String ip;
        uint16_t port;
        if (peer(&ip, &port)) {
            return String::format<64>("%s:%u", ip.constData(), port);
        }
        return String();
    }

    // UDP
    bool writeTo(const String& host, uint16_t port, const unsigned char* data, unsigned int num);
    bool writeTo(const String& host, uint16_t port, const String& data)
    {
        return writeTo(host, port, reinterpret_cast<const unsigned char*>(&data[0]), data.size());
    }

    // UDP Multicast
    bool addMembership(const String& ip);
    bool dropMembership(const String& ip);
    void setMulticastLoop(bool loop);
    void setMulticastTTL(unsigned char ttl);

    const Buffer& buffer() const { return readBuffer; }
    Buffer&& takeBuffer() { return std::move(readBuffer); }

    Signal<std::function<void(const SocketClient::SharedPtr&, Buffer&&)> >& readyRead() { return signalReadyRead; }
    Signal<std::function<void(const SocketClient::SharedPtr&, const String&, uint16_t, Buffer&&)> >& readyReadFrom() { return signalReadyReadFrom; }
    Signal<std::function<void(const SocketClient::SharedPtr&)> >& connected() { return signalConnected; }
    Signal<std::function<void(const SocketClient::SharedPtr&)> >& disconnected() { return signalDisconnected; }
    Signal<std::function<void(const SocketClient::SharedPtr&, int)> >& bytesWritten() { return signalBytesWritten; }

    enum Error {
        InitializeError,
        DnsError,
        ConnectError,
        BindError,
        ReadError,
        WriteError,
        EventLoopError
    };
    Signal<std::function<void(const SocketClient::SharedPtr&, Error)> >& error() { return signalError; }

private:
    bool init(unsigned int mode);

    int fd;
    uint16_t socketPort;
    State socketState;
    unsigned int socketMode;
    WriteMode wMode;
    bool writeWait;
    String address;

    Signal<std::function<void(const SocketClient::SharedPtr&, Buffer&&)> > signalReadyRead;
    Signal<std::function<void(const SocketClient::SharedPtr&, const String&, uint16_t, Buffer&&)> > signalReadyReadFrom;
    Signal<std::function<void(const SocketClient::SharedPtr&)> >signalConnected, signalDisconnected;
    Signal<std::function<void(const SocketClient::SharedPtr&, Error)> > signalError;
    Signal<std::function<void(const SocketClient::SharedPtr&, int)> > signalBytesWritten;
    Buffer readBuffer, writeBuffer;

    int writeData(const unsigned char *data, int size);
    void socketCallback(int, int);

    friend class Resolver;
};

#endif
