#ifndef QPROPERTYMEMORY_H
#define QPROPERTYMEMORY_H

#include <QObject>
#include <QString>
#include <QVariant>

class QPropertyMemory
{
public:
    explicit QPropertyMemory(QObject* obj, const QString &name);
    virtual ~QPropertyMemory();

protected:
    QObject*    m_objectPtr;
    QVariant    m_propertyValue;
    QString     m_propertyName;
};

#endif // QPROPERTYMEMORY_H
