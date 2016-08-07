#include "Pinger.h"

#include <cstdlib>
#include <thread>

#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/icmp.hpp>

using namespace boost::asio;

#include "icmp_header.hpp"
#include "ipv4_header.hpp"

struct charbuf : std::streambuf
{
    charbuf(char* begin, char* end) {
        this->setg(begin, begin, end);
    }
};

unsigned short get_identifier()
{
#if defined(BOOST_ASIO_WINDOWS)
    return static_cast<unsigned short>(::GetCurrentProcessId());
#else
    return static_cast<unsigned short>(::getpid());
#endif
}

Pinger::Pinger()
{
    icmp_header echo_request(requestBuf);
    echo_request.type(icmp_header::echo_request);
    echo_request.code(0);
    echo_request.identifier(get_identifier());
    echo_request.sequence_number(10500);
    compute_checksum(echo_request, nullptr, nullptr);
}

Pinger::~Pinger()
{
}

std::vector<uint> Pinger::ping(uint netStart, uint netEnd, std::chrono::milliseconds timeout)
{
    io_service ioService;
    ip::icmp::socket icmpSocket(ioService, ip::icmp::v4());

    std::vector<uint> hosts;
    //TODO: split to chunks of max available requests
    std::size_t numOfHosts = netEnd - netStart;
    if (numOfHosts > 0xFFFF) {
        return hosts;
    }
    std::size_t replyBufSize = numOfHosts * replyBufferSize;
    
    std::unique_ptr<unsigned char, void(*)(unsigned char*)> replyBuf((unsigned char*)std::malloc(replyBufSize),
        [](unsigned char *ptr) {free(ptr); });

    unsigned char *data = replyBuf.get();
    if (data == 0)
    {
        return hosts;
    }

    icmp_header req(requestBuf);
    static int seq = 10;
    req.sequence_number(seq++);
    compute_checksum(req, nullptr, nullptr);

    std::thread worker;
    std::size_t counter = 0;

    basic_waitable_timer<std::chrono::high_resolution_clock> timer(ioService);

    for (uint addr = netStart; addr < netEnd; addr++)
    {
        icmpSocket.async_send_to(buffer(requestBuf),ip::icmp::endpoint(ip::address_v4(addr),0),[addr]
        (const boost::system::error_code& error, std::size_t /*bytes_transferred*/)
        {
            if (error)
            {
//                std::cout << addr << " send err: " << error << std::endl;
            }
        });

        icmpSocket.async_receive(buffer(data, replyBufferSize),
            [data, replyBufferSize = this->replyBufferSize, &hosts, &counter, &numOfHosts, &timer]
        (const boost::system::error_code& /*error*/, std::size_t /*bytes_transferred*/) {
            counter++;
            if (counter == numOfHosts)
            {
                timer.cancel();
            }
            charbuf cbuf((char*)data, (char*)data + replyBufferSize);
            std::istream strm(&cbuf);
            ipv4_header ipHeader;
            unsigned char icmpHeaderBuf[8];
            icmp_header icmpHeader(icmpHeaderBuf);
            strm >> ipHeader >> icmpHeader;

            if (icmpHeader.type() == 0 &&
                icmpHeader.code() == 0)
            {
                hosts.push_back(ipHeader.source_address().to_ulong());
            }
        });

        data += replyBufferSize;

        std::this_thread::sleep_for(5ms);

        if (addr % 64 == 0 && !worker.joinable())
        {
            worker = std::thread([service = &ioService]{ service->run(); });
        }
    }

    timer.expires_from_now(timeout);
    timer.async_wait([service = &ioService]
    (const boost::system::error_code& error)
    {
        if (!error)
        {
            service->stop();
        }
    });

    if (worker.joinable())
    {
        worker.join();
    }
    ioService.run();
    ioService.reset();

    return hosts;
}
