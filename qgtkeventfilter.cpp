#include "qgtkeventfilter.h"

#include "qgtkdeleteeventfilter.h"
#include "qgtkgeneraleventfilter.h"

QGtkEventFilter::QGtkEventFilter(eventFuncPtr f, void *data) :
    QObject(0), m_f(f), m_data(data)
{
}

bool QGtkEventFilter::invoke(QObject *src, GdkEvent *ge)
{
    if(!m_swapped) {
        return m_f(src, ge, m_data);
    } else {
        swappedEventFuncPtr m_fs = (swappedEventFuncPtr)m_f;
        return m_fs(m_data, ge, src);
    }
}

QObject *QGtkEventFilter::createFilter(const QString &eventName, eventFuncPtr f, void *data, bool swapped)
{
    QGtkEventFilter *res = 0;

    if (eventName == "event")
        res = new QGtkGeneralEventFilter(f, data);
    else if (eventName == "delete_event")
        res = new QGtkDeleteEventFilter(f, data);

    if (res)
        res->m_swapped = swapped;

    return res;
}

//event
//button_press_event
//button_release_event
//scroll_event
//motion_notify_event
//delete_event - OK
//destroy_event
//expose_event
//key_press_event
//key_release_event
//enter_notify_event
//leave_notify_event
//configure_event
//focus_in_event
//focus_out_event
//map_event
//unmap_event
//property_notify_event
//selection_clear_event
//selection_request_event
//selection_notify_event
//proximity_in_event
//proximity_out_event
//visibility_notify_event
//client_event
//no_expose_event
//window_state_event
