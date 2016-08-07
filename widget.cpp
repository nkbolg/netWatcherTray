#include "widget.h"

#include <QtEndian>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QTimer>
#include <QDebug>

#include <QApplication>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QList>
#include <QString>
#include <QMenu>

#include "Pinger/Pinger.h"


Widget::Widget()
    : QObject(),
      currentState(&trayIcon),
      pingerPtr (std::make_unique<Pinger>()),
      trayContextMenu (std::make_unique<QMenu>()),
      timer( new QTimer(this) ),
      srcIpv4(0)
{
    timer->setInterval(1000*10);
    connect(timer, &QTimer::timeout, this, &Widget::onTimerEvent);

    connect(this, &Widget::updateTrayMenu, this, &Widget::onUpdateTrayMenu);

    if (!setupNetworkInterface()) {
        qApp->quit();
        return;
    }

    setupPersistentMenu();

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

void Widget::setupPersistentMenu()
{
    currentState = NetworkState::Good;

    persistentActionsList << new QAction(QStringLiteral("Interfaces"), this);

    auto actionSep = new QAction(this);
    actionSep->setSeparator(true);
    persistentActionsList << actionSep;

    for (auto &&entry : interfaces)
    {
        auto act = new QAction(entry.toString(), this);
        if (entry.toIPv4Address() == srcIpv4) {
            act->setCheckable(true);
            act->setChecked(true);
        }
        persistentActionsList << act;
    }

    actionSep = new QAction(this);
    actionSep->setSeparator(true);
    persistentActionsList << actionSep;

    auto actExit = new QAction(QStringLiteral("Exit"), this);
    connect(actExit, &QAction::triggered, qApp, &QApplication::quit);

    persistentActionsList << actExit;

    trayContextMenu->addActions(persistentActionsList);

    trayIcon.setContextMenu(trayContextMenu.get());
}

bool Widget::setupNetworkInterface()
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

        interfaces.push_back(hostAddress);

        if (srcIpv4 == 0) {
            suitableAddress = convertedAddress;
            int netmask = 32 - addressEntry.prefixLength();
            netStartIpv4 = ((suitableAddress >> netmask) << netmask) + 1;
            netEndIpv4 = netStartIpv4 + (1 << netmask) - 2;

            if (hostAddress.isInSubnet(QHostAddress("192.168.0.0"), 16) ||
                    hostAddress.isInSubnet(QHostAddress("10.0.0.0"), 8))
            {
                srcIpv4 = suitableAddress;
            }
        }
    }
    if (suitableAddress == 0)
    {
        QMessageBox::warning(0, "No suitable Ipv4 interface found", "No suitable Ipv4 interface found.\nApplication is shutting down.");
        return false;
    }
    //autoconf ipv4 ( 169.254.248.201 ) - invalid
    if (suitableAddress == 0xA9FEF8C9 && srcIpv4 == 0)
    {
        QMessageBox::warning(0, "Ipv4 autoconfigurated interface found", "Ipv4 autoconfigurated interface found.\nApplication is shutting down.");
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
//    trayIcon.showMessage("Timer event", "Timer event");
    if (pingerThread.joinable()) {
        pingerThread.join();
    }
    pingerThread = std::thread([this](){
       auto res = pingerPtr->ping(netStartIpv4, netEndIpv4, 7s);

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

       NetworkState::State tmpState;
       //ping google dns to check internet access
       quint32 googlePublicDNS = 0x08080808;
       res = pingerPtr->ping(googlePublicDNS, googlePublicDNS+1, 1s);

       qDebug () << res.size();
       for (auto&& elem : res)
       {
           qDebug () << QHostAddress(elem).toString();
       }

       if (res.size() == 1 && res[0] == googlePublicDNS) {
           tmpState = NetworkState::Good;
       }
       else
       {
           tmpState = NetworkState::NoInternetAccess;
       }

       if (presentNodes.size() == 0 ||
           presentNodes.size() == 1 && presentNodes.count(srcIpv4)) {
           tmpState = NetworkState::NoLocalNetAccess;
       }
       currentState = tmpState;
       emit updateTrayMenu();
    });
}

void Widget::onUpdateTrayMenu()
{
//    trayIcon.showMessage("Menu update", "Update requsted");
    trayContextMenu->clear();

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

    trayContextMenu->addActions(temporaryActionsList);
    trayContextMenu->addActions(persistentActionsList);
}

Widget::NetworkState &Widget::NetworkState::operator=(const Widget::NetworkState::State &state)
{
    if (currState == state) {
        return *this;
    }
    currState = state;
    switch (currState) {
    case Good:
        icon->setToolTip("Ok");
        icon->setIcon(QIcon(":/images/ok.png"));
        break;
    case NoInternetAccess:
        icon->setToolTip("No internet access");
        icon->setIcon(QIcon(":/images/noint.png"));
        break;
    case NoLocalNetAccess:
        icon->setToolTip("No local network access");
        icon->setIcon(QIcon(":/images/noloc.png"));
        break;
    default:
        icon->setIcon(QIcon());
        icon->setToolTip("Err");
        break;
    }
    return *this;
}
