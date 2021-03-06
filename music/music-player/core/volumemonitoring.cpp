/*
 * Copyright (C) 2017 ~ 2018 Wuhan Deepin Technology Co., Ltd.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "volumemonitoring.h"
#include <QtMath>
#include <QTimer>
#include <QThread>
#include <QDBusObjectPath>
#include <QDBusInterface>
#include <QDBusReply>
#include <QtDebug>
#include "util/global.h"
#include "util/dbusutils.h"
#include "musicsettings.h"
static const int timerInterval = 100;//轮巡Debus音量的周期

class VolumeMonitoringPrivate
{
public:
    explicit VolumeMonitoringPrivate(VolumeMonitoring *parent) : q_ptr(parent) {}

    QTimer            timer;
    int oldVolume = 0;
    bool oldMute  = false;
    bool bMuteUpdate = true;
    bool bVolUpdate = true;
    VolumeMonitoring  *q_ptr;
    Q_DECLARE_PUBLIC(VolumeMonitoring)
};

VolumeMonitoring::VolumeMonitoring(QObject *parent)
    : QObject(parent), d_ptr(new VolumeMonitoringPrivate(this))
{
    Q_D(VolumeMonitoring);
    d->oldMute = MusicSettings::value("base.play.mute").toBool();
    d->oldVolume = MusicSettings::value("base.play.volume").toInt();
    connect(&d->timer, SIGNAL(timeout()), this, SLOT(timeoutSlot()));
}

VolumeMonitoring::~VolumeMonitoring()
{
    stop();
}

void VolumeMonitoring::start()
{
    Q_D(VolumeMonitoring);
    d->timer.start(timerInterval);
}

void VolumeMonitoring::stop()
{
    Q_D(VolumeMonitoring);
    d->timer.stop();
}

bool VolumeMonitoring::needSyncLocalFlag(int type)
{
    Q_D(VolumeMonitoring);
    return type ? d->bVolUpdate : d->bMuteUpdate;
}

int VolumeMonitoring::readSinkInputValid()
{
    QVariant v = DBusUtils::readDBusProperty("com.deepin.daemon.Audio", "/com/deepin/daemon/Audio",
                                             "com.deepin.daemon.Audio", "SinkInputs");

    if (!v.isValid())
        return -1;

    QList<QDBusObjectPath> allSinkInputsList = v.value<QList<QDBusObjectPath> >();

    QString sinkInputPath;
    for (auto curPath : allSinkInputsList) {
        QVariant nameV = DBusUtils::readDBusProperty("com.deepin.daemon.Audio", curPath.path(),
                                                     "com.deepin.daemon.Audio.SinkInput", "Name");

        if (!nameV.isValid() || nameV != Global::getAppName())
            continue;

        sinkInputPath = curPath.path();
        break;
    }
    if (sinkInputPath.isEmpty())
        return -1;

    QDBusInterface ainterface("com.deepin.daemon.Audio", sinkInputPath,
                              "com.deepin.daemon.Audio.SinkInput",
                              QDBusConnection::sessionBus());
    if (!ainterface.isValid()) {
        return -1;
    }

    return 0;
}

void VolumeMonitoring::syncLocalFlag(int type)
{
    Q_D(VolumeMonitoring);
    if (type)
        d->bVolUpdate = true;
    else
        d->bMuteUpdate = true;
}

void VolumeMonitoring::timeoutSlot()
{
    Q_D(VolumeMonitoring);
    QVariant v = DBusUtils::readDBusProperty("com.deepin.daemon.Audio", "/com/deepin/daemon/Audio",
                                             "com.deepin.daemon.Audio", "SinkInputs");

    if (!v.isValid())
        return;

    QList<QDBusObjectPath> allSinkInputsList = v.value<QList<QDBusObjectPath> >();

    QString sinkInputPath;
    for (auto curPath : allSinkInputsList) {
        QVariant nameV = DBusUtils::readDBusProperty("com.deepin.daemon.Audio", curPath.path(),
                                                     "com.deepin.daemon.Audio.SinkInput", "Name");

        if (!nameV.isValid() || nameV != Global::getAppName())
            continue;

        sinkInputPath = curPath.path();
        break;
    }
    if (sinkInputPath.isEmpty())
        return;

    QDBusInterface ainterface("com.deepin.daemon.Audio", sinkInputPath,
                              "com.deepin.daemon.Audio.SinkInput",
                              QDBusConnection::sessionBus());
    if (!ainterface.isValid()) {
        return ;
    }

    if (d->bMuteUpdate) {
        /*************************************
         * sync local mute to dbus
         * ***********************************/
        //调用设置音量
        QVariant muteV = DBusUtils::readDBusProperty("com.deepin.daemon.Audio", sinkInputPath,
                                                     "com.deepin.daemon.Audio.SinkInput", "Mute");
        if (muteV.toBool() != MusicSettings::value("base.play.mute").toBool())
            ainterface.call(QLatin1String("SetMute"), MusicSettings::value("base.play.mute").toBool());
        d->bMuteUpdate = false;
    }

    if (d->bVolUpdate) {
        /*************************************
         * sync local  volume to dbus
         * ***********************************/
        int volm = MusicSettings::value("base.play.volume").toInt();
        double vol = (volm + 0.1) / 100.0;
        if (vol > 1.0)
            vol = 1.000;
        ainterface.call(QLatin1String("SetVolume"), vol, false);
        if (qFuzzyCompare(volm, 0.0))
            ainterface.call(QLatin1String("SetMute"), true);
        d->bVolUpdate =  false;
    }


    //获取音量
    QVariant volumeV = DBusUtils::readDBusProperty("com.deepin.daemon.Audio", sinkInputPath,
                                                   "com.deepin.daemon.Audio.SinkInput", "Volume");

    //获取音量
    QVariant muteV = DBusUtils::readDBusProperty("com.deepin.daemon.Audio", sinkInputPath,
                                                 "com.deepin.daemon.Audio.SinkInput", "Mute");
    //取最小正整数
    int volume = qFloor(volumeV.toDouble() * 100);

    bool mute = muteV.toBool();

    if (volume != d->oldVolume) {
        d->oldVolume = volume;
        Q_EMIT volumeChanged(volume);
    }
    if (mute != d->oldMute) {
        d->oldMute = mute;
        Q_EMIT muteChanged(mute);
    }
}
