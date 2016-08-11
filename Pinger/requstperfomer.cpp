#include "requstperfomer.h"

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>

using namespace boost::asio;

std::vector<RequstPerfomer::IPv4Addr> RequstPerfomer::performDNSRequest(const std::string &addr)
{
    std::vector<RequstPerfomer::IPv4Addr> resultVector;
    io_service ioService;
    ip::tcp::resolver resolver(ioService);
    ip::tcp::resolver::query query(addr,"");
    resolver.async_resolve(query,[&resultVector]
             (const boost::system::error_code& error, ip::tcp::resolver::iterator iterator )
    {
        if (error) {
            return;
        }
        ip::tcp::resolver::iterator end;
        for (; iterator != end; iterator++)
        {
            resultVector.push_back(iterator->endpoint().address().to_v4().to_ulong());
        }
    });
    ioService.run();
    return resultVector;
}
