/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QARK_TYPEREGISTRY_H
#define QARK_TYPEREGISTRY_H

#include "parameters.h"

#include "qmt/infrastructure/qmtassert.h"

#include <exception>
#include <typeinfo>
#include <type_traits>

#include <QString>
#include <QHash>


namespace qark {

class unregisteredType :
        public std::exception
{
};

class abstractType :
        public std::exception
{
};


namespace registry {

// we use a template to allow definition of static variables in header
template<int N>
class TypeNameMaps {
public:

    typedef QHash<QString, QString> mapType;

public:

    static mapType &getNameToUidMap() { return *typeidNameToUidMap; }

    static mapType &getUidToNameMap() { return *typeidUidToNameMap; }

#if !defined(QT_NO_DEBUG)
    static bool hasNameToUidMap() { return typeidNameToUidMap != 0; }

    static bool hasUidToNameMap() { return typeidUidToNameMap != 0; }
#endif

protected:

    static void init()
    {
        static bool initialized = false;
        static mapType nameToUidMap;
        static mapType uidToNameMap;

        if (!initialized) {
            typeidNameToUidMap = &nameToUidMap;
            typeidUidToNameMap = &uidToNameMap;
            initialized = true;
        }
    }

private:

    static mapType *typeidNameToUidMap;

    static mapType *typeidUidToNameMap;
};

template<int N>
typename TypeNameMaps<N>::mapType *TypeNameMaps<N>::typeidNameToUidMap;

template<int N>
typename TypeNameMaps<N>::mapType *TypeNameMaps<N>::typeidUidToNameMap;


template<class T>
class TypeNameRegistry :
        public TypeNameMaps<0>
{

    typedef TypeNameMaps<0> base;

private:

    static int __static_init;

private:
    static int __init(const QString &name)
    {
        base::init();
        QMT_CHECK(!base::getNameToUidMap().contains(QLatin1String(typeid(T).name())) || base::getNameToUidMap().value(QLatin1String(typeid(T).name())) == name);
        QMT_CHECK(!base::getUidToNameMap().contains(name) || base::getUidToNameMap().value(name) == QLatin1String(typeid(T).name()));
        base::getNameToUidMap().insert(QLatin1String(typeid(T).name()), name);
        base::getUidToNameMap().insert(name, QLatin1String(typeid(T).name()));
        return 0;
    }
};



template<class Archive, class BASE>
class TypeRegistry {
public:

    struct typeInfo {

        typedef Archive &(*saveFuncType)(Archive &, BASE * const &p);
        typedef Archive &(*loadFuncType)(Archive &, BASE * &p);

        explicit typeInfo()
            : m_saveFunc(0),
              m_loadFunc(0)
        {
        }

        explicit typeInfo(saveFuncType sfunc, loadFuncType lfunc)
            : m_saveFunc(sfunc),
              m_loadFunc(lfunc)
        {
        }

        bool operator==(const typeInfo &rhs) const
        {
            return m_saveFunc == rhs.m_saveFunc && m_loadFunc == rhs.m_loadFunc;
        }

        saveFuncType m_saveFunc;
        loadFuncType m_loadFunc;
    };

    typedef QHash<QString, typeInfo> mapType;

public:

    static mapType &getMap() { return *map; }

#if !defined(QT_NO_DEBUG)
    static bool hasMap() { return map != 0; }
#endif

protected:

    static void init() {
        static bool initialized = false;
        static mapType theMap;

        if (!initialized) {
            map = &theMap;
            initialized = true;
        }
    }

private:

    static mapType *map;
};

template<class Archive, class BASE>
typename TypeRegistry<Archive, BASE>::mapType *TypeRegistry<Archive,BASE>::map;


template<class Archive, class BASE, class DERIVED>
class DerivedTypeRegistry :
        public TypeRegistry<Archive, BASE>
{

    typedef TypeRegistry<Archive, BASE> base;

    typedef Archive &(*saveFuncType)(Archive &, BASE * const &);

    typedef Archive &(*loadFuncType)(Archive &, BASE * &);

private:

    static int __static_init;

private:
    static int __init(saveFuncType sfunc, loadFuncType lfunc)
    {
        base::init();
        QMT_CHECK(!base::getMap().contains(QLatin1String(typeid(DERIVED).name())) || base::getMap().value(QLatin1String(typeid(DERIVED).name())) == typename base::typeInfo(sfunc, lfunc));
        base::getMap().insert(QLatin1String(typeid(DERIVED).name()), typename base::typeInfo(sfunc, lfunc));
        return 0;
    }
};

template<class Archive, class BASE, class DERIVED>
Archive &savePointer(Archive &ar, BASE * const &p)
{
    DERIVED &t = dynamic_cast<DERIVED &>(*p);
    save(ar, t, Parameters());
    return ar;
}

template<class Archive, class BASE, class DERIVED>
Archive &loadPointer(Archive &ar, BASE *&p)
{
    DERIVED *t = new DERIVED();
    load(ar, *t, Parameters());
    p = t;
    return ar;
}

template<class Archive, class T>
typename std::enable_if<!std::is_abstract<T>::value, void>::type loadNonVirtualPointer(Archive &archive, T *&p)
{
    registry::loadPointer<Archive, T, T>(archive, p);
}

template<class Archive, class T>
typename std::enable_if<std::is_abstract<T>::value, void>::type loadNonVirtualPointer(Archive &archive, T *&p)
{
    (void) archive;
    (void) p;

    throw abstractType();
}

inline QString demangleTypename(const char *mangledName)
{
    // TODO convert compiler specific mangledName into human readable type name
    return QLatin1String(mangledName);
}

inline QString flattenTypename(const char *typeName)
{
    // convert C++ type name into simple identifier (no extra characters)
    return QString(QLatin1String(typeName)).replace(QChar(QLatin1Char(':')), QLatin1String("-"));
}

}

template<class T>
QString getTypeUid()
{
#if !defined(QT_NO_DEBUG) // avoid warning about unused function ::hasNameToUidMap in Qt >= 5.5
    QMT_CHECK_X((registry::TypeNameRegistry<T>::hasNameToUidMap()), "getTypeUid<T>()", "type maps are not correctly initialized");
    QMT_CHECK_X((registry::TypeNameRegistry<T>::getNameToUidMap().contains(QLatin1String(typeid(T).name()))), "getTypeUid<T>()",
                qPrintable(QString(QLatin1String("type with typeid %1 is not registered. Use QARK_REGISTER_TYPE or QARK_REGISTER_TYPE_NAME.")).arg(registry::demangleTypename(typeid(T).name()))));
#endif
    return registry::TypeNameRegistry<T>::getNameToUidMap().value(QLatin1String(typeid(T).name()));
}

template<class T>
QString getTypeUid(const T &t)
{
    Q_UNUSED(t);
#if !defined(QT_NO_DEBUG) // avoid warning about unused function ::hasNameToUidMap in Qt >= 5.5
    QMT_CHECK_X((registry::TypeNameRegistry<T>::hasNameToUidMap()), "getTypeUid<T>()", "type maps are not correctly initialized");
    QMT_CHECK_X((registry::TypeNameRegistry<T>::getNameToUidMap().contains(QLatin1String(typeid(t).name()))), "getTypeUid<T>()",
                qPrintable(QString(QLatin1String("type with typeid %1 is not registered. Use QARK_REGISTER_TYPE or QARK_REGISTER_TYPE_NAME.")).arg(registry::demangleTypename(typeid(t).name()))));
#endif
    return registry::TypeNameRegistry<T>::getNameToUidMap().value(QLatin1String(typeid(t).name()));
}

template<class Archive, class T>
typename registry::TypeRegistry<Archive, T>::typeInfo getTypeInfo(const T &t)
{
    Q_UNUSED(t);
#if !defined(QT_NO_DEBUG) // avoid warning about unused function ::hasNameToUidMap in Qt >= 5.5
    QMT_CHECK_X((registry::TypeRegistry<Archive,T>::hasMap()),
                qPrintable(QString(QLatin1String("TypeRegistry<Archive, %1>::getTypeInfo(const T&)")).arg(getTypeUid<T>())),
                qPrintable(QString(QLatin1String("%1 is not a registered base class. Declare your derived classes with QARK_REGISTER_DERIVED_CLASS.")).arg(getTypeUid<T>())));
#endif
    return registry::TypeRegistry<Archive,T>::getMap()[QLatin1String(typeid(t).name())];
}

template<class Archive, class T>
typename registry::TypeRegistry<Archive,T>::typeInfo getTypeInfo(const QString &uid)
{
#if !defined(QT_NO_DEBUG) // avoid warning about unused function ::hasNameToUidMap in Qt >= 5.5
    QMT_CHECK_X((registry::TypeNameRegistry<T>::hasUidToNameMap()), "getTypeInfo<T>(const QString &)", "type maps are not correctly initialized");
    QMT_CHECK_X((registry::TypeRegistry<Archive,T>::hasMap()),
                qPrintable(QString(QLatin1String("TypeRegistry<Archive, %1>::getTypeInfo(const QString &)")).arg(getTypeUid<T>())),
                qPrintable(QString(QLatin1String("%1 is not a registered base class. Declare your derived classes with QARK_REGISTER_DERIVED_CLASS.")).arg(getTypeUid<T>())));
#endif
    return registry::TypeRegistry<Archive,T>::getMap().value(registry::TypeNameRegistry<T>::getUidToNameMap().value(uid));
}

}


#define QARK_TYPE_STRING(T) #T

#define QARK_REGISTER_TYPE_NAME(T, NAME) \
    template<> \
    int qark::registry::TypeNameRegistry<T>::__static_init = qark::registry::TypeNameRegistry<T>::__init(QLatin1String(NAME));

#define QARK_REGISTER_TYPE(T) \
    template<> \
    int qark::registry::TypeNameRegistry<T>::__static_init = qark::registry::TypeNameRegistry<T>::__init(qark::registry::flattenTypename(QARK_TYPE_STRING(T)));

#define QARK_REGISTER_DERIVED_CLASS(INARCHIVE, OUTARCHIVE, DERIVED, BASE) \
    template<> \
    int qark::registry::DerivedTypeRegistry<INARCHIVE,BASE,DERIVED>::__static_init = \
            qark::registry::DerivedTypeRegistry<INARCHIVE, BASE, DERIVED>::__init(0, qark::registry::loadPointer<INARCHIVE, BASE, DERIVED>); \
    template<> \
    int qark::registry::DerivedTypeRegistry<OUTARCHIVE, BASE, DERIVED>::__static_init = \
            qark::registry::DerivedTypeRegistry<OUTARCHIVE, BASE, DERIVED>::__init(qark::registry::savePointer<OUTARCHIVE, BASE, DERIVED>, 0); \
    template<> \
    int qark::registry::DerivedTypeRegistry<OUTARCHIVE, typename std::add_const<BASE>::type, typename std::add_const<DERIVED>::type>::__static_init = \
            qark::registry::DerivedTypeRegistry<OUTARCHIVE, typename std::add_const<BASE>::type, typename std::add_const<DERIVED>::type>:: \
                __init(qark::registry::savePointer<OUTARCHIVE, typename std::add_const<BASE>::type, typename std::add_const<DERIVED>::type>, 0);

#endif // QARK_TYPEREGISTRY_H
