/*
 * network_event.c: network event queue processing helpers
 *
 * Copyright (C) 2013 SUSE LINUX Products GmbH, Nuernberg, Germany.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Author: Cedric Bosdonnat
 */

#include <config.h>

#include "network_event.h"
#include "network_event.h"
#include "object_event.h"
#include "object_event_private.h"
#include "datatypes.h"
#include "virlog.h"

struct _virNetworkEventLifecycle {
    virObjectEvent parent;

    int type;
};
typedef struct _virNetworkEventLifecycle virNetworkEventLifecycle;
typedef virNetworkEventLifecycle *virNetworkEventLifecyclePtr;

static virClassPtr virNetworkEventLifecycleClass;
static void virNetworkEventLifecycleDispose(void *obj);

static int virNetworkEventsOnceInit(void)
{
    if (!(virNetworkEventLifecycleClass = virClassNew(
                                             virClassForObjectEvent(),
                                             "virNetworkEventLifecycle",
                                             sizeof(virNetworkEventLifecycle),
                                             virNetworkEventLifecycleDispose)))
        return -1;
    return 0;
}

VIR_ONCE_GLOBAL_INIT(virNetworkEvents)

void virNetworkEventLifecycleDispose(void *obj)
{
    virNetworkEventLifecyclePtr event = obj;
    VIR_DEBUG("obj=%p", event);
}


void
virNetworkEventDispatchDefaultFunc(virConnectPtr conn,
                                   virObjectEventPtr event,
                                   virConnectNetworkEventGenericCallback cb ATTRIBUTE_UNUSED,
                                   void *cbopaque ATTRIBUTE_UNUSED,
                                   void *opaque ATTRIBUTE_UNUSED)
{
    virNetworkPtr net = virGetNetwork(conn, event->meta.name, event->meta.uuid);
    if (!net)
        return;

    switch ((virNetworkEventID) (event->eventID &0xFF) ) {
    case VIR_NETWORK_EVENT_ID_LIFECYCLE:
        {
            virNetworkEventLifecyclePtr networkLifecycleEvent;

            networkLifecycleEvent = (virNetworkEventLifecyclePtr)event;
            ((virConnectNetworkEventLifecycleCallback)cb)(conn, net,
                                                          networkLifecycleEvent->type,
                                                          cbopaque);
            goto cleanup;
        }

#ifdef VIR_ENUM_SENTINELS
    case VIR_NETWORK_EVENT_ID_LAST:
        break;
#endif
    }
    VIR_WARN("Unexpected event ID %d", event->eventID);

cleanup:
    virNetworkFree(net);
}


/**
 * virNetworkEventStateRegisterID:
 * @conn: connection to associate with callback
 * @state: object event state
 * @net: network to filter on or NULL for all networks
 * @eventID: ID of the event type to register for
 * @cb: function to add to event
 * @opaque: data blob to pass to callback
 * @freecb: callback to free @opaque
 * @callbackID: filled with callback ID
 *
 * Register the function @callbackID with connection @conn,
 * from @state, for events of type @eventID.
 *
 * Returns: the number of callbacks now registered, or -1 on error
 */
int
virNetworkEventStateRegisterID(virConnectPtr conn,
                              virObjectEventStatePtr state,
                              virNetworkPtr net,
                              int eventID,
                              virConnectObjectEventGenericCallback cb,
                              void *opaque,
                              virFreeCallback freecb,
                              int *callbackID)
{
    if (net)
        return virObjectEventStateRegisterID(conn, state,
                                             net->uuid, net->name, 0, eventID,
                                             cb, opaque, freecb, callbackID);
    else
        return virObjectEventStateRegisterID(conn, state,
                                             NULL, NULL, 0, eventID,
                                             cb, opaque, freecb, callbackID);
}

void *virNetworkEventLifecycleNew(const char *name,
                                  const unsigned char *uuid,
                                  int type)
{
    virNetworkEventLifecyclePtr event;
    int eventId = (VIR_EVENT_NAMESPACE_NETWORK << 8) + VIR_NETWORK_EVENT_ID_LIFECYCLE;

    if (virNetworkEventsInitialize() < 0)
        return NULL;

    if (!(event = virObjectEventNew(virNetworkEventLifecycleClass,
                                    eventId,
                                    0, name, uuid)))
        return NULL;

    event->type = type;

    return event;
}
