#include "qgtkgeneraleventfilter.h"

#include <QEvent>

QGtkGeneralEventFilter::QGtkGeneralEventFilter(eventFuncPtr f, void *data) :
    QGtkEventFilter(f, data)
{
}

bool QGtkGeneralEventFilter::eventFilter(QObject *src, QEvent *e)
{
    GdkEvent ge;

    // TODO ignores the return value of invoke...
    switch (e->type())
    {
    case QEvent::Close:
        ge.type = GDK_DELETE;
        invoke(src, &ge);
        break;
    case QEvent::MouseButtonPress:
        ge.type = GDK_BUTTON_PRESS;
        invoke(src, &ge);
        break;

    default:
// TODO (noicy!)        qDebug("QGtkGeneralEventFilter ignored event of type %d", e->type());
        break;
    }

    // Let everyone else see the event too
    return false;
}
