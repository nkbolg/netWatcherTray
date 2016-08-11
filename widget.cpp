#include "widget.h"

#include <QNetworkInterface>
#include <QHostAddress>
#include <QTimer>
#include <QDebug>
#include <QApplication>
#include <QMessageBox>
#include <QString>

#include "requstperfomer.h"

Q_DECLARE_METATYPE(QNetworkAddressEntry)

Widget::Widget()
    : QObject(),
      currentState(&trayIcon),
      persistentActionGroup(this),
      timer(new QTimer(this)),
      srcIpv4(0)
{
    timer->setInterval(1000*10);
    connect(timer, &QTimer::timeout, this, &Widget::onTimerEvent);

    connect(this, &Widget::updateTrayMenu, this, &Widget::onUpdateTrayMenu);

    std::vector<QNetworkAddressEntry> interfaces;
    if (!setupNetworkInterface(interfaces)) {
        qApp->quit();
        return;
    }

    setupPersistentMenu(interfaces);

    timer->start();
}

std::vector<QNetworkAddressEntry> Widget::getFilteredAddressEntries()
{
    std::vector<QNetworkAddressEntry> resultVector;
    QList<QNetworkInterface> interfaceList = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &netInterface : interfaceList)
    {
        for (QNetworkAddressEntry &addressEntry : netInterface.addressEntries())
        {
            QHostAddress hostAddress = addressEntry.ip();
            if (hostAddress.protocol() != QAbstractSocket::IPv4Protocol ||
                hostAddress.isLoopback())
            {
                continue;
            }
            resultVector.push_back(addressEntry);
        }
    }
    return resultVector;
}

void Widget::setupPersistentMenu(const std::vector<QNetworkAddressEntry> &interfaces)
{
    currentState = NetworkState::Good;

    persistentActionGroup.addAction(QStringLiteral("Interfaces"));
    persistentActionGroup.addAction(QString())->setSeparator(true);

    for (auto &&entry : interfaces)
    {
        QHostAddress hostAddr = entry.ip();
        auto act = persistentActionGroup.addAction(hostAddr.toString());
        act->setCheckable(true);
        QVariant var;
        var.setValue<QNetworkAddressEntry>(entry);
        act->setData(var);
        if (hostAddr.toIPv4Address() == srcIpv4) {
            act->setChecked(true);
        }
    }

    persistentActionGroup.addAction(QString())->setSeparator(true);

    auto actExit = persistentActionGroup.addAction(QStringLiteral("Exit"));
    connect(actExit, &QAction::triggered, qApp, &QApplication::quit);

    connect(&persistentActionGroup, &QActionGroup::triggered, this, &Widget::onSetInterfaceActive);

    trayContextMenu.addActions(persistentActionGroup.actions());

    trayIcon.setContextMenu(&trayContextMenu);
}

bool Widget::setupNetworkInterface(std::vector<QNetworkAddressEntry> &interfaces)
{
    if (srcIpv4 != 0) {
        return true;
    }
    std::vector<QNetworkAddressEntry> addrEntries = getFilteredAddressEntries();
    interfaces.reserve((int)addrEntries.size());
    quint32 suitableAddress = 0;
    for(auto &&addressEntry : addrEntries)
    {
        QHostAddress hostAddress = addressEntry.ip();

        bool ok = false;
        quint32 convertedAddress = hostAddress.toIPv4Address(&ok);
        if (!ok) {
            continue;
        }

        interfaces.push_back(addressEntry);

        if (srcIpv4 == 0) {
            suitableAddress = convertedAddress;
            int netmask = 32 - addressEntry.prefixLength();
            netStartIpv4 = ((suitableAddress >> netmask) << netmask) + 1;
            netEndIpv4 = netStartIpv4 + (1 << netmask) - 2;

            if (hostAddress.isInSubnet(QHostAddress(QStringLiteral("192.168.0.0")), 16) ||
                    hostAddress.isInSubnet(QHostAddress(QStringLiteral("10.0.0.0")), 8))
            {
                srcIpv4 = suitableAddress;
            }
        }
    }
    if (suitableAddress == 0)
    {
        QMessageBox::warning(0, QStringLiteral("No suitable Ipv4 interface found"), QStringLiteral("No suitable Ipv4 interface found.\nApplication is shutting down."));
        return false;
    }
    //autoconf ipv4 ( 169.254.248.201 ) - invalid
    if (suitableAddress == 0xA9FEF8C9 && srcIpv4 == 0)
    {
        QMessageBox::warning(0, QStringLiteral("Ipv4 autoconfigurated interface found"), QStringLiteral("Ipv4 autoconfigurated interface found.\nApplication is shutting down."));
        return false;
    }
    else if (srcIpv4 == 0)
    {
        srcIpv4 = suitableAddress;
    }
    return true;
}

Widget::~Widget()
{
    if (pingerThread.joinable())
    {
        pingerThread.join();
    }
}

void Widget::showIcon()
{
    trayIcon.show();
}

void Widget::onTimerEvent()
{
    if (pingerThread.joinable()) {
        pingerThread.join();
    }
    pingerThread = std::thread([this](){
       auto res = pinger.ping(srcIpv4, netStartIpv4, netEndIpv4, 7s);

       qDebug () << res.size();
       for (auto&& elem : res)
       {
           qDebug () << QHostAddress(elem).toString();
       }

       for (auto &&elem : res)
       {
           presentNodes[elem] = maxThrustLevel;
       }
       for (auto it = presentNodes.begin(); it != presentNodes.end();)
       {
           it->second--;
           if (!it->second) {
               it = presentNodes.erase(it);
           }
           else {
               it++;
           }
       }

       //ping google dns to check internet access
       quint32 googlePublicDNS = 0x08080808;
       res = pinger.ping(srcIpv4, googlePublicDNS, googlePublicDNS+1, 1s);

       qDebug () << res.size();
       for (auto&& elem : res)
       {
           qDebug () << QHostAddress(elem).toString();
       }

       bool dnsOk = !(RequstPerfomer::performDNSRequest("ya.ru").empty());

       NetworkState::State tmpState = NetworkState::Good;

       if (presentNodes.size() == 0 ||
           presentNodes.size() == 1 && presentNodes.count(srcIpv4)) {
           tmpState = NetworkState::NoLocalNetAccess;
       }

       else if (res.size() != 1 || res[0] != googlePublicDNS) {
           tmpState = NetworkState::NoInternetAccess;
       }

       else if (!dnsOk) {
           tmpState = NetworkState::NoDNS;
       }

       currentState = tmpState;
       emit updateTrayMenu();
    });
}

void Widget::onUpdateTrayMenu()
{
    trayContextMenu.clear();

    qDeleteAll(temporaryActionsList);
    temporaryActionsList.clear();

    temporaryActionsList << new QAction(QStringLiteral("Present hosts"), this);
    auto sep = new QAction(this);
    sep->setSeparator(true);
    temporaryActionsList << sep;

    for (const auto &elem : presentNodes)
    {
        temporaryActionsList << new QAction(QHostAddress(elem.first).toString(),this);
    }

    sep = new QAction(this);
    sep->setSeparator(true);
    temporaryActionsList << sep;

    trayContextMenu.addActions(temporaryActionsList);
    trayContextMenu.addActions(persistentActionGroup.actions());
}

void Widget::onSetInterfaceActive(QAction *sender)
{
    bool ok = false;
    quint32 tempIPv4 = QHostAddress(sender->text()).toIPv4Address(&ok);
    if (!ok) {
        return;
    }

    QNetworkAddressEntry addr = sender->data().value<QNetworkAddressEntry>();
    srcIpv4 = tempIPv4;
    int netmask = 32 - addr.prefixLength();
    netStartIpv4 = ((srcIpv4 >> netmask) << netmask) + 1;
    netEndIpv4 = netStartIpv4 + (1 << netmask) - 2;
    //TODO: добавить прерывание активного сканирования
}

Widget::NetworkState &Widget::NetworkState::operator=(const Widget::NetworkState::State &state)
{
    if (currState == state) {
        return *this;
    }
    currState = state;
    switch (currState) {
    case Good:
        icon->setToolTip(QStringLiteral("Ok"));
        icon->setIcon(QIcon(QStringLiteral(":/images/ok.png")));
        break;
    case NoDNS:
        icon->setToolTip(QStringLiteral("No dns access"));
        icon->setIcon(QIcon(QStringLiteral(":/images/nodns.png")));
        break;
    case NoInternetAccess:
        icon->setToolTip(QStringLiteral("No internet access"));
        icon->setIcon(QIcon(QStringLiteral(":/images/noint.png")));
        break;
    case NoLocalNetAccess:
        icon->setToolTip(QStringLiteral("No local network access"));
        icon->setIcon(QIcon(QStringLiteral(":/images/noloc.png")));
        break;
    default:
        icon->setIcon(QIcon());
        icon->setToolTip(QStringLiteral("Err"));
        break;
    }
    return *this;
}
