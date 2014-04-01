#ifndef GLIBTYPES_H
#define GLIBTYPES_H

enum EventType { GDK_BUTTON_PRESS,
                 GDK_BUTTON_RELEASE,
                 GDK_SCROLL,
                 GDK_MOTION_NOTIFY,
                 GDK_DELETE,
                 GDK_DESTROY,
                 GDK_EXPOSE,
                 GDK_KEY_PRESS,
                 GDK_KEY_RELEASE,
                 GDK_ENTER_NOTIFY,
                 GDK_LEAVE_NOTIFY,
                 GDK_CONFIGURE,
                 GDK_FOCUS_IN,
                 GDK_FOCUS_OUT,
                 GDK_MAP,
                 GDK_UNMAP,
                 GDK_PROPERTY_NOTIFY,
                 GDK_SELECTION_CLEAR,
                 GDK_SELECTION_REQUEST,
                 GDK_SELECTION_NOTIFY,
                 GDK_PROXIMITY_IN,
                 GDK_PROXIMITY_OUT,
                 GDK_VISIBILITY_NOTIFY,
                 GDK_CLIENT,
                 GDK_NO_EXPOSE,
                 GDK_WINDOW_STATE };

// TODO we need to investigate how this structure looks in detail it is a union
struct GdkEvent {

    EventType type;
};

typedef void (*callbackFuncPtr)(QObject*, void*);
typedef void (*swappedCallbackFuncPtr)(void*, QObject*);
typedef bool (*eventFuncPtr)(QObject*, GdkEvent*, void*);
typedef bool (*swappedEventFuncPtr)(void*, GdkEvent*, QObject*);

#endif // GLIBTYPES_H
