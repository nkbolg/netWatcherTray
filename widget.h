#ifndef WIDGET_H
#define WIDGET_H

#include <array>
#include <map>

#include <memory>

#include <QWidget>

#include <QDebug>

typedef void* HANDLE;

class Widget;

struct Request
{
    std::array<uchar,64> arr;
    Widget *widget;
    uint id;

    Request(uint id, Widget *widget) : id(id), widget(widget){ qDebug () << "Cons"; }
    ~Request(){ qDebug () << "Dest"; }
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
    void changed(quint32, bool);
    void newNode(quint32);
private:

    typedef std::map< uint, std::unique_ptr<Request> > RequestMap;
    RequestMap requestMap;

    typedef std::map< quint32, bool > NodeMap;
    NodeMap nodeMap;

    HANDLE hIcmp;
    uint idCounter;
};

#endif // WIDGET_H
