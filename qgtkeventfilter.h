#ifndef QGTKEVENTFILTER_H
#define QGTKEVENTFILTER_H

#include <QObject>

#include "glibtypes.h"

class QGtkEventFilter : public QObject
{
    Q_OBJECT
public:
    static QObject *createFilter(const QString &eventName, eventFuncPtr f, void *data, bool swapped=false);

protected:
    explicit QGtkEventFilter(eventFuncPtr f, void *data);
    bool invoke(QObject *src, GdkEvent *ge);

private:
    eventFuncPtr m_f;
    void *m_data;
    bool m_swapped;
};

#endif // QGTKEVENTFILTER_H
