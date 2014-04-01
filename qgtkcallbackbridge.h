#ifndef QGTKCALLBACKBRIDGE_H
#define QGTKCALLBACKBRIDGE_H

#include <QObject>
#include "glibtypes.h"

class QGtkCallbackBridge : public QObject
{
    Q_OBJECT
public:
    explicit QGtkCallbackBridge(QObject *src, callbackFuncPtr f, void *data, bool swapped=false);

public slots:
    void invoke();

private:
    callbackFuncPtr m_f;
    void *m_data;
    bool m_swapped;
};

#endif // QGTKCALLBACKBRIDGE_H
