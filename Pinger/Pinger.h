#ifndef Pinger_h__
#define Pinger_h__


#include <chrono>
#include <vector>
#include <memory>
#include <atomic>

using namespace std::chrono_literals;

typedef unsigned int uint;

namespace boost {
namespace asio {
class io_service;
}
}

class Pinger
{
public:
    Pinger();
    ~Pinger();

    std::vector<uint> ping(uint netStart, uint netEnd, std::chrono::milliseconds timeout = 10s);
    void stop();
private:
    unsigned char requestBuf[8];

    static const int replyBufferSize = 64;

    std::unique_ptr<boost::asio::io_service> ioService;
    std::atomic_bool stopped;
};


#endif // Pinger_h__
