#include "qgtkdeleteeventfilter.h"

#include <QEvent>

QGtkDeleteEventFilter::QGtkDeleteEventFilter(eventFuncPtr f, void *data) :
    QGtkEventFilter(f, data)
{
}

bool QGtkDeleteEventFilter::eventFilter(QObject *src, QEvent *e)
{
    GdkEvent ge;

    if( e->type() == QEvent::Close)
    {
        ge.type = GDK_DELETE;

        // TODO check the sign of the return value, and the interpretation
        if(invoke(src, &ge))
            e->accept();
        else
            e->ignore();
    }

    // Let everyone else see the event too
    return false;
}
