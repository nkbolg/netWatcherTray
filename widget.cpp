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
    : currentState(&trayIcon),
      idCounter(0),
      timer( new QTimer(&trayIcon) ),
      srcIpv4(0)
{
    timer->setInterval(1000*60);

    if (!setupNetworkInterface()) {
        qApp->quit();
        return;
    }

    pingerPtr = std::make_unique<Pinger>();
    trayContextMenu = std::make_unique<QMenu>();

//    auto res = pingerPtr->ping(netStartIpv4, netEndIpv4);
//    std::sort(res.begin(),res.end());
//    QStringList lst;
//    lst.reserve((int)res.size());
//    for (auto &&elem : res)
//    {
//        lst << QHostAddress(elem).toString();
//    }

    currentState = NetworkState::Good;

    for (auto &&entry : interfaces)
    {
        QAction *act = trayContextMenu->addAction(entry.toString());
        if (entry.toIPv4Address() == srcIpv4) {
            act->setCheckable(true);
            act->setChecked(true);
        }
    }
    trayContextMenu->addSeparator();
    QAction *act = trayContextMenu->addAction("Exit");
    QObject::connect(act, &QAction::triggered, qApp, &QApplication::quit);
    trayIcon.setContextMenu(trayContextMenu.get());
//    timer->start();
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
    if (suitableAddress == 0xA9FEF8C9)
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
}

void Widget::showIcon()
{
    trayIcon.show();
}

Widget::NetworkState &Widget::NetworkState::operator=(const Widget::NetworkState::State &state)
{
    if (currState == state) {
        return *this;
    }
    currState = state;
    switch (currState) {
    case Good:
        icon->setIcon(QIcon(":/images/ok.png"));
        break;
    case NoInternetAccess:
        icon->setIcon(QIcon(":/images/noint.png"));
        break;
    case NoLocalNetAccess:
        icon->setIcon(QIcon(":/images/noloc.png"));
        break;
    default:
        break;
    }
    return *this;
}
