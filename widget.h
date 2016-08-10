#ifndef WIDGET_H
#define WIDGET_H

#include <map>
#include <thread>
#include <atomic>

#include <QSystemTrayIcon>
#include <QActionGroup>
#include <QObject>
#include <QMenu>
#include <QList>

#include "Pinger/Pinger.h"

class QTimer;
class QAction;
class QNetworkAddressEntry;

class Widget : public QObject
{
    Q_OBJECT

public:
    Widget();
    ~Widget();

    void showIcon();

private slots:
    void onTimerEvent();
    void onUpdateTrayMenu();
    void onSetInterfaceActive(QAction *sender);

signals:
    void updateTrayMenu();

private:
    bool setupNetworkInterface(std::vector<QNetworkAddressEntry> &interfaces);
    void setupPersistentMenu(const std::vector<QNetworkAddressEntry> &interfaces);
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
    QMenu trayContextMenu;
    NetworkState currentState;
    Pinger pinger;

    typedef quint32 NodeAddr;
    typedef quint32 ThrustLevel;

    typedef std::map< NodeAddr, ThrustLevel > PresentNodes;
    PresentNodes presentNodes;

    QActionGroup persistentActionGroup;
    QList<QAction*> temporaryActionsList;
    std::thread pingerThread;
    std::atomic_bool threadWorking;

    QTimer *timer;

    quint32 srcIpv4;
    quint32 netStartIpv4;
    quint32 netEndIpv4;

    static const ThrustLevel maxThrustLevel = 2;
};

#endif // WIDGET_H
