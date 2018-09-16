#include "qpropertymemory.h"

//---------------------------------------------------------------------
QPropertyMemory::QPropertyMemory(QObject* obj, const QString &name)
{
    Q_ASSERT(obj != NULL);

    m_objectPtr = obj;
    m_propertyName = name;
    m_propertyValue = m_objectPtr->property(m_propertyName.toLatin1());
}

//---------------------------------------------------------------------
QPropertyMemory::~QPropertyMemory()
{
    Q_ASSERT(m_objectPtr != NULL);

    m_objectPtr->setProperty(m_propertyName.toLatin1(), m_propertyValue);
}
