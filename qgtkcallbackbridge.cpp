#include "qgtkcallbackbridge.h"

QGtkCallbackBridge::QGtkCallbackBridge(QObject *src, callbackFuncPtr f, void *data, bool swapped) :
    QObject(0), m_f(f), m_data(data), m_swapped(swapped)
{
}

void QGtkCallbackBridge::invoke()
{
    if (!m_swapped)
    {
        m_f(sender(), m_data);
    } else {
        swappedCallbackFuncPtr m_fs = (swappedCallbackFuncPtr)m_f;
        m_fs(m_data, sender());
    }
}
