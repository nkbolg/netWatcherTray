#ifndef WIDGET_H
#define WIDGET_H

#include <array>
#include <map>
#include <set>
#include <memory>
#include <thread>

#include <QSystemTrayIcon>
#include <QActionGroup>
#include <QStringList>
#include <QObject>

class QTimer;
class QMenu;
class Pinger;
class QAction;
class QHostAddress;
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
    NetworkState currentState;

    typedef quint32 NodeAddr;
    typedef quint32 ThrustLevel;

    typedef std::map< NodeAddr, ThrustLevel > PresentNodes;
    PresentNodes presentNodes;
    std::unique_ptr<Pinger> pingerPtr;
    std::unique_ptr<QMenu> trayContextMenu;

    QActionGroup persistentActionGroup;
    QList<QAction*> temporaryActionsList;
    std::thread pingerThread;

    QTimer *timer;

    quint32 srcIpv4;
    quint32 netStartIpv4;
    quint32 netEndIpv4;

    static const ThrustLevel maxThrustLevel = 2;
};

#endif // WIDGET_H
