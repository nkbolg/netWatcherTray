#ifndef Pinger_h__
#define Pinger_h__


#include <chrono>
#include <vector>

using namespace std::chrono_literals;

typedef unsigned int uint;

class Pinger
{
public:
    Pinger();
    ~Pinger();

    std::vector<uint> ping(uint netStart, uint netEnd, std::chrono::milliseconds timeout = 10s);
private:
    unsigned char requestBuf[8];

    static const int replyBufferSize = 64;
};


#endif // Pinger_h__
