#ifndef WIDGET_H
#define WIDGET_H

#include <array>
#include <map>
#include <set>
#include <memory>

#include <QSystemTrayIcon>
#include <QStringList>

class QTimer;
class QMenu;
class Pinger;
class QHostAddress;
class QNetworkAddressEntry;

class Widget //: public QObject
{
    //Q_OBJECT

public:
    Widget();
    ~Widget();

    void showIcon();

private:
    bool setupNetworkInterface();
    std::vector<QNetworkAddressEntry> getFilteredAddressEntries();

    class NetworkState
    {
    public:
        enum State
        {
        Good,
        NoInternetAccess,
        NoLocalNetAccess,
        Invalid
        };
        NetworkState(QSystemTrayIcon *icon) : currState(Invalid), icon(icon) {}
        NetworkState& operator=(const State &state);
    private:
        State currState;
        QSystemTrayIcon *icon;
    };

    QSystemTrayIcon trayIcon;
    NetworkState currentState;

    typedef std::set< quint32 > PresentNodes;
    PresentNodes presentNodes;
    std::unique_ptr<Pinger> pingerPtr;
    std::unique_ptr<QMenu> trayContextMenu;

    std::vector<QHostAddress> interfaces;


    uint idCounter;


    QTimer *timer;

    quint32 srcIpv4;
    quint32 netStartIpv4;
    quint32 netEndIpv4;
};

#endif // WIDGET_H
