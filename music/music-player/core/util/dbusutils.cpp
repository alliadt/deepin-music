#include "dbusutils.h"
#include <QDBusConnection>
#include <QDBusReply>
#include <QDBusInterface>
#include <QDebug>
#include <QDBusError>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QMutex>

static QMutex mutex;

DBusUtils::DBusUtils()
{

}

QVariant DBusUtils::readDBusProperty(const QString &service, const QString &path, const QString &interface, const char *propert)
{
    // 创建QDBusInterface接口
    mutex.lock();
    QDBusInterface ainterface(service, path,
                              interface,
                              QDBusConnection::sessionBus());
    if (!ainterface.isValid()) {
        qDebug() << qPrintable(QDBusConnection::sessionBus().lastError().message());
        //cause dead lock if no unlock here,
        mutex.unlock();
        QVariant v(0) ;
        return  v;
    }
    //调用远程的value方法
    QVariant v = ainterface.property(propert);
    mutex.unlock();
    return  v;
}
//QVariant DBusUtils::readDBusMethod(const QString &service, const QString &path, const QString &interface, const char *method)
//{
//    // 创建QDBusInterface接口
//    QDBusInterface ainterface(service, path,
//                              interface,
//                              QDBusConnection::sessionBus());
//    if (!ainterface.isValid()) {
//        qDebug() <<  "error:" << qPrintable(QDBusConnection::sessionBus().lastError().message());
//        QVariant v(0) ;
//        return  v;
//    }
//    //调用远程的value方法
//    QDBusReply<QDBusVariant> reply = ainterface.call(method);
//    if (reply.isValid()) {
////        return reply.value();
//        QVariant v(0) ;
//        return  v;
//    } else {
//        qDebug() << "error1:" << qPrintable(QDBusConnection::sessionBus().lastError().message());
//        QVariant v(0) ;
//        return  v;
//    }
//}
