#include "widget.h"

#include <QtEndian>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QTimer>
#include <QDebug>

#define PIO_APC_ROUTINE_DEFINED


#include <WinSock2.h>
#include <winternl.h>
#include <iphlpapi.h>
#include <Icmpapi.h>

#include <utility>

#pragma comment(lib, "IPHLPAPI.lib")

VOID NTAPI icmpProc(
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG Reserved
)
{
    Q_UNUSED(IoStatusBlock);
    Q_UNUSED(Reserved);
    std::unique_ptr<Request> *reqPtr = (std::unique_ptr<Request> *)ApcContext;
    Request *req = reqPtr->get();
    PICMP_ECHO_REPLY32 replies = (PICMP_ECHO_REPLY32)req->arr.data();
    bool nodePresent = true;
    quint32 node = qFromBigEndian<quint32>(replies[0].Address);
    if (replies[0].Status != IP_SUCCESS) {
        nodePresent = false;
        if (replies[0].Status != IP_REQ_TIMED_OUT) {
            //            emit error();
        }
    }
    req->widget->setNodeInfo(node, nodePresent);
    req->widget->removeRequest(req->id);
}

Widget::Widget(QWidget *parent)
    : QWidget(parent),
      hIcmp(IcmpCreateFile()),
      idCounter(0),
      timer( new QTimer(this) ),
      srcIpv4(0)
{
    timer->setInterval(1000*10);

    setupNetworkInterface();

    connect(timer, &QTimer::timeout, this, &Widget::startScan);
    connect(this, &Widget::sendRequest, this, &Widget::onSendRequest);

    timer->start();
}

void Widget::setupNetworkInterface()
{
    quint32 suitableAddress = 0;
    QList<QNetworkInterface> interfaceList = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &netInterface : interfaceList)
    {
        if (srcIpv4 != 0) {
            break;
        }
        qDebug () << netInterface;
        for (QNetworkAddressEntry &addressEntry : netInterface.addressEntries())
        {
            quint32 convertedAddress;
            QHostAddress hostAddress = addressEntry.ip();
//            qDebug () << addressEntry;
            qDebug () << hostAddress;
            if (hostAddress.protocol() != QAbstractSocket::IPv4Protocol ||
                hostAddress.isLoopback())
            {
                continue;
            }
            bool ok = false;
            convertedAddress = hostAddress.toIPv4Address(&ok);
            if (!ok) {
                continue;
            }
            suitableAddress = convertedAddress;

            int netmask = 32 - addressEntry.prefixLength();
            netStartIpv4 = ((suitableAddress >> netmask) << netmask) + 1;
            netEndIpv4 = netStartIpv4 + (1 << netmask) - 2;

            if (hostAddress.isInSubnet(QHostAddress("192.168.0.0"), 16) ||
                    hostAddress.isInSubnet(QHostAddress("10.0.0.0"), 8)) {
                srcIpv4 = suitableAddress;
                break;
            }
        }
    }
    if (suitableAddress == 0)
    {
//        error()
    }
    else if (srcIpv4 == 0)
    {
        srcIpv4 = suitableAddress;
    }
}

void Widget::onSendRequest(quint32 dstIpv4)
{

    std::unique_ptr<Request> * reqPtr = &(requestMap.insert(std::make_pair(idCounter, std::make_unique<Request>(idCounter,this))).first->second);

    DWORD res = IcmpSendEcho2Ex(hIcmp, 0, icmpProc, reqPtr, qToBigEndian(srcIpv4), qToBigEndian(dstIpv4), 0, 0, 0, (*reqPtr)->arr.data(), (DWORD)(*reqPtr)->arr.size(), 6000);
    if (res != ERROR_IO_PENDING)
    {
        DWORD err = GetLastError();
        if (err != ERROR_IO_PENDING)
        {
//            emit error()
            return;
        }
    }
    idCounter++;
}

void Widget::startScan()
{
    for (quint32 dst = netStartIpv4; dst < netEndIpv4; ++dst) {
        sendRequest(dst);
    }
}

Widget::~Widget()
{
    IcmpCloseHandle(hIcmp);
}

void Widget::setNodeInfo(quint32 node, bool present)
{
    int count = (int)nodeMap.count(node);
    if (count == 1 && !present) {
        nodeMap.erase(node);
        emit delNode(node);
    }
    else if (count == 0 && present) {
        nodeMap.insert(std::make_pair(node,present));
        emit newNode(node);
    }
}

void Widget::removeRequest(uint id)
{
    requestMap.erase(id);
}
