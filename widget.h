#ifndef WIDGET_H
#define WIDGET_H

#include <array>
#include <map>
#include <memory>

#include <QWidget>

typedef void* HANDLE;

class Widget;
class QTimer;

struct Request
{
    std::array<uchar,64> arr;
    Widget *widget;
    uint id;

    Request(uint id, Widget *widget) : id(id), widget(widget){}
};

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = 0);
    ~Widget();

    void setNodeInfo(quint32 node, bool present);
    void removeRequest(uint id);

signals:
    void delNode(quint32);
    void newNode(quint32);
    void sendRequest(quint32 dstIpv4);
private slots:
    void onSendRequest(quint32 dstIpv4);
    void startScan();
private:
    void setupNetworkInterface();

    typedef std::map< uint, std::unique_ptr<Request> > RequestMap;
    RequestMap requestMap;

    typedef std::map< quint32, bool > NodeMap;
    NodeMap nodeMap;

    HANDLE hIcmp;
    uint idCounter;

    QTimer *timer;

    quint32 srcIpv4;
    quint32 netStartIpv4;
    quint32 netEndIpv4;
};

#endif // WIDGET_H
