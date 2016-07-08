#include "widget.h"


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
//    DWORD bytesRecieved = IoStatusBlock->Information;
    PICMP_ECHO_REPLY32 replies = (PICMP_ECHO_REPLY32)req->arr.data();
//    DWORD count = IcmpParseReplies(replies, bytesRecieved);
//    Q_UNUSED(count);
//    for (int i = 0; i < count; ++i) {
    bool nodePresent = true;
    quint32 node = replies[0].Address;
    if (replies[0].Status != IP_SUCCESS) {
        nodePresent = false;
//            emit error();
    }
    req->widget->setNodeInfo(node, nodePresent);
    req->widget->removeRequest(req->id);
//    }

    return;
}

Widget::Widget(QWidget *parent)
    : QWidget(parent),
      idCounter(0)
{
    in_addr dst = { 192, 168, 1, 1 };
    in_addr src = { 192, 168, 1, 101 };

    std::unique_ptr<Request> * reqPtr = &(requestMap.insert(std::make_pair(idCounter, std::make_unique<Request>(idCounter,this))).first->second);


    hIcmp = IcmpCreateFile();
    DWORD res = IcmpSendEcho2Ex(hIcmp, 0, icmpProc, reqPtr, src.S_un.S_addr, dst.S_un.S_addr, 0, 0, 0, (*reqPtr)->arr.data(), (DWORD)(*reqPtr)->arr.size(), 100);
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

Widget::~Widget()
{
    IcmpCloseHandle(hIcmp);
}

void Widget::setNodeInfo(quint32 node, bool present)
{
    if (nodeMap.count(node)) {
        if (nodeMap[node] != present) {
            nodeMap[node] = present;
            emit changed(node, present);
        }
    }
    else {
        nodeMap.insert(std::make_pair(node,present));
        emit newNode(node);
    }
}

void Widget::removeRequest(uint id)
{
    requestMap.erase(id);
}
