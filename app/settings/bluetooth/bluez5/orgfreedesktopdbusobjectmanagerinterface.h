#ifndef OBJECTMANAGER_H_1396883000
#define OBJECTMANAGER_H_1396883000

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>

typedef QMap<QString, QVariantMap> InterfaceList;
typedef QMap<QDBusObjectPath, InterfaceList> ManagedObjectList;

Q_DECLARE_METATYPE(InterfaceList)
Q_DECLARE_METATYPE(ManagedObjectList)

/*
 * Proxy class for interface org.freedesktop.DBus.ObjectManager
 */
class OrgFreedesktopDBusObjectManagerInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "org.freedesktop.DBus.ObjectManager"; }

public:
    OrgFreedesktopDBusObjectManagerInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~OrgFreedesktopDBusObjectManagerInterface();

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<ManagedObjectList> GetManagedObjects()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("GetManagedObjects"), argumentList);
    }

Q_SIGNALS: // SIGNALS
    void InterfacesAdded(const QDBusObjectPath &object_path, InterfaceList interfaces_and_properties);
    void InterfacesRemoved(const QDBusObjectPath &object_path, const QStringList &interfaces);
};

namespace org {
  namespace freedesktop {
    namespace DBus {
      typedef ::OrgFreedesktopDBusObjectManagerInterface ObjectManager;
    }
  }
}
#endif
