#include "launcher.h"
#include "powerManager/PowerManager.h"
#include <QDebug>
#include<QTimer>
#include<QFile>
#include<QUrl>
Launcher::Launcher(QObject *parent) : QObject(parent)
{
}


void Launcher::slot_standby()
{
}

void Launcher::pickWallpaper(QString path)
{
    if (path.isNull())
    {
        qDebug()<<"pickWallpaper path is null!"<<endl;
        return;
    }
    if (QFile::exists("/usr/share/backgrounds/background.jpg"))
    {
        QFile::remove("/usr/share/backgrounds/background.jpg");
    }

    bool result=QFile::copy(QUrl(path).toLocalFile(), "/usr/share/backgrounds/background.jpg");
        qDebug()<<"pickWallpaper path="<<QUrl(path).toLocalFile()<<",result="<<result;
}

void Launcher::powerControl()
{
}

void Launcher::emitNewIntent()
{
    emit newIntentReceived();
}

void Launcher::registerMethods()
{
    registerNativeMethods();
}

void Launcher::minimize()
{
    emit minimized();
}

void Launcher::registerNativeMethods()
{

}
