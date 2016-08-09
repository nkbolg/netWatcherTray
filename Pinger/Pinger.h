#ifndef Pinger_h__
#define Pinger_h__


#include <chrono>
#include <vector>

using namespace std::chrono_literals;


class Pinger
{
public:
    Pinger();
    ~Pinger();

    typedef unsigned int uint;
    std::vector<uint> ping(uint netStart, uint netEnd, std::chrono::milliseconds timeout = 10s);
private:
    unsigned char requestBuf[8];

    static const int replyBufferSize = 64;
};


#endif // Pinger_h__
