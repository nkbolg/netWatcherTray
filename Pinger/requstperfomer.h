#ifndef REQUSTPERFOMER_H
#define REQUSTPERFOMER_H

#include <vector>
#include <string>

class RequstPerfomer
{
public:
    typedef unsigned int IPv4Addr;
    static std::vector<IPv4Addr> performDNSRequest(const std::string& addr);
};

#endif // REQUSTPERFOMER_H
