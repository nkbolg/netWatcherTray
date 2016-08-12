#ifndef Pinger_h__
#define Pinger_h__


#include <chrono>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>

using namespace std::chrono_literals;


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


    typedef unsigned int uint;
    std::vector<uint> ping(uint srcIPv4, uint netStart, uint netEnd, std::chrono::milliseconds timeout = 10s);
    void stop();
private:
    unsigned char requestBuf[8];
    static const int replyBufferSize = 64;

    std::unique_ptr<boost::asio::io_service> ioService;
    std::atomic_bool stopped;
    std::mutex mutex;
};


#endif // Pinger_h__
