#ifndef IERRORHANDLER
#define IERRORHANDLER

class IErrorHandler
{
public:
    virtual void handleError(unsigned int errCode) const = 0;
    virtual ~IErrorHandler(){}
};

#endif // IERRORHANDLER

