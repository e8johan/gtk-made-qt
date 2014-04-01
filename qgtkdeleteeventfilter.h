#ifndef QGTKDELETEEVENTFILTER_H
#define QGTKDELETEEVENTFILTER_H

#include "qgtkeventfilter.h"

class QGtkDeleteEventFilter : public QGtkEventFilter
{
public:
    explicit QGtkDeleteEventFilter(eventFuncPtr f, void *data);

protected:
    bool eventFilter(QObject *src, QEvent *e);
};

#endif // QGTKDELETEEVENTFILTER_H
