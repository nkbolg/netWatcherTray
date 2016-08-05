#ifndef Pinger_h__
#define Pinger_h__

#include <memory>
#include <vector>
#include <chrono>

using namespace std::chrono_literals;

#include "boost/asio/ip/icmp.hpp"

namespace boost
{
    namespace asio
    {
        class io_service;
    }
}

typedef unsigned int uint;
typedef std::shared_ptr<boost::asio::io_service> ServicePtr;

ServicePtr createService();

class Pinger
{
public:
    Pinger();
    ~Pinger();

    std::vector<uint> ping(uint netStart, uint netEnd, std::chrono::milliseconds timeout = 10s);
private:
    ServicePtr servicePtr;

//    unsigned int netStart;
//    unsigned int netEnd;

    boost::asio::ip::icmp::socket icmpSocket;

    unsigned char requestBuf[8];

    static const int replyBufferSize = 64;
};


#endif // Pinger_h__
