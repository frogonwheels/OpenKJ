#include "updatechecker.h"
#include <QApplication>
#include <QDebug>
#include <QNetworkReply>
#include <settings.h>

extern Settings *settings;

QString UpdateChecker::getOS() const
{
    return OS;
}

void UpdateChecker::setOS(const QString &value)
{
    OS = value;
}

QString UpdateChecker::getChannel() const
{
    return channel;
}

void UpdateChecker::setChannel(const QString &value)
{
    channel = value;
}

UpdateChecker::UpdateChecker(QObject *parent) : QObject(parent)
{
    OS = "unknown";
    channel = "stable";
    manager = new QNetworkAccessManager(this);
    currentVer = OKJ_VERSION_STRING;
#ifdef Q_OS_WIN32
    OS = "Win32";
#endif
#ifdef Q_OS_WIN64
    OS = "Win64";
#endif
#ifdef Q_OS_MACOS
    OS = "MacOS";
#endif
#ifdef Q_OS_LINUX
    OS = "Linux";
#endif

if (settings->updatesBranch() == 0)
    channel = "stable";
else
    channel = "unstable";
//    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onNetworkReply(QNetworkReply*)));
}

void UpdateChecker::checkForUpdates()
{
    if (!settings->checkUpdates())
        return;
    qWarning() << "Requesting current version info for branch: " << channel;
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onNetworkReply(QNetworkReply*)));
    QNetworkReply *reply = manager->get(QNetworkRequest(QUrl("http://openkj.org/downloads/" + OS + "-" + channel + "-curversion.txt")));
    while (!reply->isFinished())
        QApplication::processEvents();
    qWarning() << "Request completed";
}

void UpdateChecker::onNetworkReply(QNetworkReply *reply)
{
    qWarning() << "Received network reply";
    if (reply->error() != QNetworkReply::NoError)
    {
        qWarning() << reply->errorString();
        //output some meaningful error msg
        return;
    }
    availVersion = QString(reply->readAll());
    availVersion = availVersion.trimmed();
    QStringList curVersionParts = currentVer.split(".");
    QStringList availVersionParts = availVersion.split(".");
    if (availVersionParts.size() != 3 || curVersionParts.size() != 3)
    {
        qWarning() << "Got invalid version info from server";
        return;
    }
    int availMajor = availVersionParts.at(0).toInt();
    int availMinor = availVersionParts.at(1).toInt();
    int availRevis = availVersionParts.at(2).toInt();
    int curMajor = OKJ_VERSION_MAJOR;
    int curMinor = OKJ_VERSION_MINOR;
    int curRevis = OKJ_VERSION_BUILD;
    if (availMajor > curMajor)
        emit newVersionAvailable(availVersion);
    else if (availMajor == curMajor && availMinor > curMinor)
        emit newVersionAvailable(availVersion);
    else if (availMajor == curMajor && availMinor == curMinor && availRevis > curRevis)
        emit newVersionAvailable(availVersion);
    qWarning() << "Received version: " << availVersion << " Current version: " << currentVer;
    reply->deleteLater();
}

void UpdateChecker::downloadInstaller()
{
    QString url;
    if (channel == "unstable")
    {
        if (OS == "Win64")
            url = "http://openkj.org/downloads/unstable/Windows/OpenKJ-" + availVersion + "-64bit-setup.exe";
        if (OS == "Win32")
            url = "http://openkj.org/downloads/unstable/Windows/OpenKJ-" + availVersion + "-32bit-setup.exe";
        if (OS == "MacOS")
            url = "https://openkj.org/downloads/unstable/MacOS/OpenKJ-" + availVersion + "-unstable-osx-installer.dmg";
    }
}
