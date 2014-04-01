#ifndef QGTKGENERALEVENTFILTER_H
#define QGTKGENERALEVENTFILTER_H

#include "qgtkeventfilter.h"

class QGtkGeneralEventFilter : public QGtkEventFilter
{
Q_OBJECT
public:
    explicit QGtkGeneralEventFilter(eventFuncPtr f, void *data);

protected:
    bool eventFilter(QObject *src, QEvent *e);
};

#endif // QGTKGENERALEVENTFILTER_H
