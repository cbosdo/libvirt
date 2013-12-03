/*
 * domain_event.c: domain event queue processing helpers
 *
 * Copyright (C) 2010-2013 Red Hat, Inc.
 * Copyright (C) 2008 VirtualIron
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
 * Author: Ben Guthro
 */

#include <config.h>

#include "domain_event.h"
#include "object_event.h"
#include "object_event_private.h"
#include "virlog.h"
#include "datatypes.h"
#include "viralloc.h"
#include "virerror.h"
#include "virstring.h"

#define VIR_FROM_THIS VIR_FROM_NONE


static virClassPtr virDomainEventClass;
static virClassPtr virDomainEventLifecycleClass;
static virClassPtr virDomainEventRTCChangeClass;
static virClassPtr virDomainEventWatchdogClass;
static virClassPtr virDomainEventIOErrorClass;
static virClassPtr virDomainEventGraphicsClass;
static virClassPtr virDomainEventBlockJobClass;
static virClassPtr virDomainEventDiskChangeClass;
static virClassPtr virDomainEventTrayChangeClass;
static virClassPtr virDomainEventBalloonChangeClass;
static virClassPtr virDomainEventDeviceRemovedClass;


static void virDomainEventDispose(void *obj);
static void virDomainEventLifecycleDispose(void *obj);
static void virDomainEventRTCChangeDispose(void *obj);
static void virDomainEventWatchdogDispose(void *obj);
static void virDomainEventIOErrorDispose(void *obj);
static void virDomainEventGraphicsDispose(void *obj);
static void virDomainEventBlockJobDispose(void *obj);
static void virDomainEventDiskChangeDispose(void *obj);
static void virDomainEventTrayChangeDispose(void *obj);
static void virDomainEventBalloonChangeDispose(void *obj);
static void virDomainEventDeviceRemovedDispose(void *obj);


struct _virDomainEvent {
    virObjectEvent parent;

    /* Unused attribute to get virDomainEvent class being created */
    bool dummy;
};
typedef struct _virDomainEvent virDomainEvent;
typedef virDomainEvent *virDomainEventPtr;

struct _virDomainEventLifecycle {
    virDomainEvent parent;

    int type;
    int detail;
};
typedef struct _virDomainEventLifecycle virDomainEventLifecycle;
typedef virDomainEventLifecycle *virDomainEventLifecyclePtr;

struct _virDomainEventRTCChange {
    virDomainEvent parent;

    long long offset;
};
typedef struct _virDomainEventRTCChange virDomainEventRTCChange;
typedef virDomainEventRTCChange *virDomainEventRTCChangePtr;

struct _virDomainEventWatchdog {
    virDomainEvent parent;

    int action;
};
typedef struct _virDomainEventWatchdog virDomainEventWatchdog;
typedef virDomainEventWatchdog *virDomainEventWatchdogPtr;

struct _virDomainEventIOError {
    virDomainEvent parent;

    char *srcPath;
    char *devAlias;
    int action;
    char *reason;
};
typedef struct _virDomainEventIOError virDomainEventIOError;
typedef virDomainEventIOError *virDomainEventIOErrorPtr;

struct _virDomainEventBlockJob {
    virDomainEvent parent;

    char *path;
    int type;
    int status;
};
typedef struct _virDomainEventBlockJob virDomainEventBlockJob;
typedef virDomainEventBlockJob *virDomainEventBlockJobPtr;

struct _virDomainEventGraphics {
    virDomainEvent parent;

    int phase;
    virDomainEventGraphicsAddressPtr local;
    virDomainEventGraphicsAddressPtr remote;
    char *authScheme;
    virDomainEventGraphicsSubjectPtr subject;
};
typedef struct _virDomainEventGraphics virDomainEventGraphics;
typedef virDomainEventGraphics *virDomainEventGraphicsPtr;

struct _virDomainEventDiskChange {
    virDomainEvent parent;

    char *oldSrcPath;
    char *newSrcPath;
    char *devAlias;
    int reason;
};
typedef struct _virDomainEventDiskChange virDomainEventDiskChange;
typedef virDomainEventDiskChange *virDomainEventDiskChangePtr;

struct _virDomainEventTrayChange {
    virDomainEvent parent;

    char *devAlias;
    int reason;
};
typedef struct _virDomainEventTrayChange virDomainEventTrayChange;
typedef virDomainEventTrayChange *virDomainEventTrayChangePtr;

struct _virDomainEventBalloonChange {
    virDomainEvent parent;

    /* In unit of 1024 bytes */
    unsigned long long actual;
};
typedef struct _virDomainEventBalloonChange virDomainEventBalloonChange;
typedef virDomainEventBalloonChange *virDomainEventBalloonChangePtr;

struct _virDomainEventDeviceRemoved {
    virDomainEvent parent;

    char *devAlias;
};
typedef struct _virDomainEventDeviceRemoved virDomainEventDeviceRemoved;
typedef virDomainEventDeviceRemoved *virDomainEventDeviceRemovedPtr;


static int virDomainEventsOnceInit(void)
{
    if (!(virDomainEventClass = virClassNew(virClassForObjectEvent(),
                                             "virDomainEvent",
                                             sizeof(virDomainEvent),
                                             virDomainEventDispose)))
        return -1;
    if (!(virDomainEventLifecycleClass = virClassNew(
                                             virDomainEventClass,
                                             "virDomainEventLifecycle",
                                             sizeof(virDomainEventLifecycle),
                                             virDomainEventLifecycleDispose)))
        return -1;
    if (!(virDomainEventRTCChangeClass = virClassNew(
                                             virDomainEventClass,
                                             "virDomainEventRTCChange",
                                             sizeof(virDomainEventRTCChange),
                                             virDomainEventRTCChangeDispose)))
        return -1;
    if (!(virDomainEventWatchdogClass = virClassNew(
                                             virDomainEventClass,
                                             "virDomainEventWatchdog",
                                             sizeof(virDomainEventWatchdog),
                                             virDomainEventWatchdogDispose)))
        return -1;
    if (!(virDomainEventIOErrorClass = virClassNew(
                                             virDomainEventClass,
                                             "virDomainEventIOError",
                                             sizeof(virDomainEventIOError),
                                             virDomainEventIOErrorDispose)))
        return -1;
    if (!(virDomainEventGraphicsClass = virClassNew(
                                             virDomainEventClass,
                                             "virDomainEventGraphics",
                                             sizeof(virDomainEventGraphics),
                                             virDomainEventGraphicsDispose)))
        return -1;
    if (!(virDomainEventBlockJobClass = virClassNew(
                                             virDomainEventClass,
                                             "virDomainEventBlockJob",
                                             sizeof(virDomainEventBlockJob),
                                             virDomainEventBlockJobDispose)))
        return -1;
    if (!(virDomainEventDiskChangeClass = virClassNew(
                                             virDomainEventClass,
                                             "virDomainEventDiskChange",
                                             sizeof(virDomainEventDiskChange),
                                             virDomainEventDiskChangeDispose)))
        return -1;
    if (!(virDomainEventTrayChangeClass = virClassNew(
                                             virDomainEventClass,
                                             "virDomainEventTrayChange",
                                             sizeof(virDomainEventTrayChange),
                                             virDomainEventTrayChangeDispose)))
        return -1;
    if (!(virDomainEventBalloonChangeClass = virClassNew(
                                             virDomainEventClass,
                                             "virDomainEventBalloonChange",
                                             sizeof(virDomainEventBalloonChange),
                                             virDomainEventBalloonChangeDispose)))
        return -1;
    if (!(virDomainEventDeviceRemovedClass = virClassNew(
                                             virDomainEventClass,
                                             "virDomainEventDeviceRemoved",
                                             sizeof(virDomainEventDeviceRemoved),
                                             virDomainEventDeviceRemovedDispose)))
        return -1;
    return 0;
}

VIR_ONCE_GLOBAL_INIT(virDomainEvents)


static void virDomainEventDispose(void *obj)
{
    virDomainEventPtr event = obj;

    VIR_DEBUG("obj=%p", event);
}

static void virDomainEventLifecycleDispose(void *obj)
{
    virDomainEventLifecyclePtr event = obj;
    VIR_DEBUG("obj=%p", event);
}

static void virDomainEventRTCChangeDispose(void *obj)
{
    virDomainEventRTCChangePtr event = obj;
    VIR_DEBUG("obj=%p", event);
}

static void virDomainEventWatchdogDispose(void *obj)
{
    virDomainEventWatchdogPtr event = obj;
    VIR_DEBUG("obj=%p", event);
}

static void virDomainEventIOErrorDispose(void *obj)
{
    virDomainEventIOErrorPtr event = obj;
    VIR_DEBUG("obj=%p", event);

    VIR_FREE(event->srcPath);
    VIR_FREE(event->devAlias);
    VIR_FREE(event->reason);
}

static void virDomainEventGraphicsDispose(void *obj)
{
    virDomainEventGraphicsPtr event = obj;
    VIR_DEBUG("obj=%p", event);

    if (event->local) {
        VIR_FREE(event->local->node);
        VIR_FREE(event->local->service);
        VIR_FREE(event->local);
    }
    if (event->remote) {
        VIR_FREE(event->remote->node);
        VIR_FREE(event->remote->service);
        VIR_FREE(event->remote);
    }
    VIR_FREE(event->authScheme);
    if (event->subject) {
        size_t i;
        for (i = 0; i < event->subject->nidentity; i++) {
            VIR_FREE(event->subject->identities[i].type);
            VIR_FREE(event->subject->identities[i].name);
        }
        VIR_FREE(event->subject);
    }
}

static void virDomainEventBlockJobDispose(void *obj)
{
    virDomainEventBlockJobPtr event = obj;
    VIR_DEBUG("obj=%p", event);

    VIR_FREE(event->path);
}

static void virDomainEventDiskChangeDispose(void *obj)
{
    virDomainEventDiskChangePtr event = obj;
    VIR_DEBUG("obj=%p", event);

    VIR_FREE(event->oldSrcPath);
    VIR_FREE(event->newSrcPath);
    VIR_FREE(event->devAlias);
}

static void virDomainEventTrayChangeDispose(void *obj)
{
    virDomainEventTrayChangePtr event = obj;
    VIR_DEBUG("obj=%p", event);

    VIR_FREE(event->devAlias);
}

static void virDomainEventBalloonChangeDispose(void *obj)
{
    virDomainEventBalloonChangePtr event = obj;
    VIR_DEBUG("obj=%p", event);
}

static void virDomainEventDeviceRemovedDispose(void *obj)
{
    virDomainEventDeviceRemovedPtr event = obj;
    VIR_DEBUG("obj=%p", event);

    VIR_FREE(event->devAlias);
}


/**
 * virDomainEventCallbackListRemove:
 * @conn: pointer to the connection
 * @cbList: the list
 * @callback: the callback to remove
 *
 * Internal function to remove a callback from a virObjectEventCallbackListPtr
 */
static int
virDomainEventCallbackListRemove(virConnectPtr conn,
                                 virObjectEventCallbackListPtr cbList,
                                 virConnectDomainEventCallback callback)
{
    int ret = 0;
    size_t i;
    for (i = 0; i < cbList->count; i++) {
        if (cbList->callbacks[i]->cb == VIR_OBJECT_EVENT_CALLBACK(callback) &&
            cbList->callbacks[i]->eventID == VIR_DOMAIN_EVENT_ID_LIFECYCLE &&
            cbList->callbacks[i]->conn == conn) {
            virFreeCallback freecb = cbList->callbacks[i]->freecb;
            if (freecb)
                (*freecb)(cbList->callbacks[i]->opaque);
            virObjectUnref(cbList->callbacks[i]->conn);
            VIR_FREE(cbList->callbacks[i]);

            if (i < (cbList->count - 1))
                memmove(cbList->callbacks + i,
                        cbList->callbacks + i + 1,
                        sizeof(*(cbList->callbacks)) *
                                (cbList->count - (i + 1)));

            if (VIR_REALLOC_N(cbList->callbacks,
                              cbList->count - 1) < 0) {
                ; /* Failure to reduce memory allocation isn't fatal */
            }
            cbList->count--;

            for (i = 0; i < cbList->count; i++) {
                if (!cbList->callbacks[i]->deleted)
                    ret++;
            }
            return ret;
        }
    }

    virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                   _("could not find event callback for removal"));
    return -1;
}


static int
virDomainEventCallbackListMarkDelete(virConnectPtr conn,
                                     virObjectEventCallbackListPtr cbList,
                                     virConnectDomainEventCallback callback)
{
    int ret = 0;
    size_t i;
    for (i = 0; i < cbList->count; i++) {
        if (cbList->callbacks[i]->cb == VIR_OBJECT_EVENT_CALLBACK(callback) &&
            cbList->callbacks[i]->eventID == VIR_DOMAIN_EVENT_ID_LIFECYCLE &&
            cbList->callbacks[i]->conn == conn) {
            cbList->callbacks[i]->deleted = 1;
            for (i = 0; i < cbList->count; i++) {
                if (!cbList->callbacks[i]->deleted)
                    ret++;
            }
            return ret;
        }
    }

    virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                   _("could not find event callback for deletion"));
    return -1;
}


/**
 * virDomainEventCallbackListAdd:
 * @conn: pointer to the connection
 * @cbList: the list
 * @callback: the callback to add
 * @opaque: opaque data tio pass to callback
 *
 * Internal function to add a callback from a virObjectEventCallbackListPtr
 */
static int
virDomainEventCallbackListAdd(virConnectPtr conn,
                              virObjectEventCallbackListPtr cbList,
                              virConnectDomainEventCallback callback,
                              void *opaque,
                              virFreeCallback freecb)
{
    return virObjectEventCallbackListAddID(conn, cbList, NULL, NULL, 0,
                                           VIR_DOMAIN_EVENT_ID_LIFECYCLE,
                                           VIR_OBJECT_EVENT_CALLBACK(callback),
                                           opaque, freecb, NULL);
}


static void *virDomainEventNew(virClassPtr klass,
                               int eventID,
                               int id,
                               const char *name,
                               const unsigned char *uuid)
{
    virDomainEventPtr event;

    if (virDomainEventsInitialize() < 0)
        return NULL;

    if (!virClassIsDerivedFrom(klass, virDomainEventClass)) {
        virReportInvalidArg(klass,
                            _("Class %s must derive from virDomainEvent"),
                            virClassName(klass));
        return NULL;
    }

    if (!(event = virObjectEventNew(klass, eventID,
                                    id, name, uuid)))
        return NULL;

    return (virObjectEventPtr)event;
}

virObjectEventPtr virDomainEventLifecycleNew(int id, const char *name,
                                 const unsigned char *uuid,
                                 int type, int detail)
{
    virDomainEventLifecyclePtr event;

    if (virDomainEventsInitialize() < 0)
        return NULL;

    if (!(event = virDomainEventNew(virDomainEventLifecycleClass,
                                    VIR_DOMAIN_EVENT_ID_LIFECYCLE,
                                    id, name, uuid)))
        return NULL;

    event->type = type;
    event->detail = detail;

    return (virObjectEventPtr)event;
}

virObjectEventPtr virDomainEventLifecycleNewFromDom(virDomainPtr dom, int type, int detail)
{
    return virDomainEventLifecycleNew(dom->id, dom->name, dom->uuid,
                                      type, detail);
}

virObjectEventPtr virDomainEventLifecycleNewFromObj(virDomainObjPtr obj, int type, int detail)
{
    return virDomainEventLifecycleNewFromDef(obj->def, type, detail);
}

virObjectEventPtr virDomainEventLifecycleNewFromDef(virDomainDefPtr def, int type, int detail)
{
    return virDomainEventLifecycleNew(def->id, def->name, def->uuid,
                                      type, detail);
}

virObjectEventPtr virDomainEventRebootNew(int id, const char *name,
                                          const unsigned char *uuid)
{
    if (virDomainEventsInitialize() < 0)
        return NULL;

    return virDomainEventNew(virDomainEventClass,
                             VIR_DOMAIN_EVENT_ID_REBOOT,
                             id, name, uuid);
}

virObjectEventPtr virDomainEventRebootNewFromDom(virDomainPtr dom)
{
    if (virDomainEventsInitialize() < 0)
        return NULL;

    return virDomainEventNew(virDomainEventClass,
                             VIR_DOMAIN_EVENT_ID_REBOOT,
                             dom->id, dom->name, dom->uuid);
}

virObjectEventPtr virDomainEventRebootNewFromObj(virDomainObjPtr obj)
{
    if (virDomainEventsInitialize() < 0)
        return NULL;

    return virDomainEventNew(virDomainEventClass,
                             VIR_DOMAIN_EVENT_ID_REBOOT,
                             obj->def->id, obj->def->name, obj->def->uuid);
}

virObjectEventPtr virDomainEventRTCChangeNewFromDom(virDomainPtr dom,
                                                    long long offset)
{
    virDomainEventRTCChangePtr ev;

    if (virDomainEventsInitialize() < 0)
        return NULL;

    if (!(ev = virDomainEventNew(virDomainEventRTCChangeClass,
                                 VIR_DOMAIN_EVENT_ID_RTC_CHANGE,
                                 dom->id, dom->name, dom->uuid)))
        return NULL;

    ev->offset = offset;

    return (virObjectEventPtr)ev;
}
virObjectEventPtr virDomainEventRTCChangeNewFromObj(virDomainObjPtr obj,
                                                    long long offset)
{
    virDomainEventRTCChangePtr ev;

    if (virDomainEventsInitialize() < 0)
        return NULL;

    if (!(ev = virDomainEventNew(virDomainEventRTCChangeClass,
                                 VIR_DOMAIN_EVENT_ID_RTC_CHANGE,
                                 obj->def->id, obj->def->name,
                                 obj->def->uuid)))
        return NULL;

    ev->offset = offset;

    return (virObjectEventPtr)ev;
}

virObjectEventPtr virDomainEventWatchdogNewFromDom(virDomainPtr dom, int action)
{
    virDomainEventWatchdogPtr ev;

    if (virDomainEventsInitialize() < 0)
        return NULL;

    if (!(ev = virDomainEventNew(virDomainEventWatchdogClass,
                                 VIR_DOMAIN_EVENT_ID_WATCHDOG,
                                 dom->id, dom->name, dom->uuid)))
        return NULL;

    ev->action = action;

    return (virObjectEventPtr)ev;
}
virObjectEventPtr virDomainEventWatchdogNewFromObj(virDomainObjPtr obj, int action)
{
    virDomainEventWatchdogPtr ev;

    if (virDomainEventsInitialize() < 0)
        return NULL;

    if (!(ev = virDomainEventNew(virDomainEventWatchdogClass,
                                 VIR_DOMAIN_EVENT_ID_WATCHDOG,
                                 obj->def->id, obj->def->name,
                                 obj->def->uuid)))
        return NULL;

    ev->action = action;

    return (virObjectEventPtr)ev;
}

static virObjectEventPtr virDomainEventIOErrorNewFromDomImpl(int event,
                                                             virDomainPtr dom,
                                                             const char *srcPath,
                                                             const char *devAlias,
                                                             int action,
                                                             const char *reason)
{
    virDomainEventIOErrorPtr ev;

    if (virDomainEventsInitialize() < 0)
        return NULL;

    if (!(ev = virDomainEventNew(virDomainEventIOErrorClass, event,
                                 dom->id, dom->name, dom->uuid)))
        return NULL;

    ev->action = action;
    if (VIR_STRDUP(ev->srcPath, srcPath) < 0 ||
        VIR_STRDUP(ev->devAlias, devAlias) < 0 ||
        VIR_STRDUP(ev->reason, reason) < 0) {
        virObjectUnref(ev);
        ev = NULL;
    }

    return (virObjectEventPtr)ev;
}

static virObjectEventPtr virDomainEventIOErrorNewFromObjImpl(int event,
                                                             virDomainObjPtr obj,
                                                             const char *srcPath,
                                                             const char *devAlias,
                                                             int action,
                                                             const char *reason)
{
    virDomainEventIOErrorPtr ev;

    if (virDomainEventsInitialize() < 0)
        return NULL;

    if (!(ev = virDomainEventNew(virDomainEventIOErrorClass, event,
                                 obj->def->id, obj->def->name,
                                 obj->def->uuid)))
        return NULL;

    ev->action = action;
    if (VIR_STRDUP(ev->srcPath, srcPath) < 0 ||
        VIR_STRDUP(ev->devAlias, devAlias) < 0 ||
        VIR_STRDUP(ev->reason, reason) < 0) {
        virObjectUnref(ev);
        ev = NULL;
    }

    return (virObjectEventPtr)ev;
}

virObjectEventPtr virDomainEventIOErrorNewFromDom(virDomainPtr dom,
                                                  const char *srcPath,
                                                  const char *devAlias,
                                                  int action)
{
    return virDomainEventIOErrorNewFromDomImpl(VIR_DOMAIN_EVENT_ID_IO_ERROR,
                                               dom, srcPath, devAlias,
                                               action, NULL);
}

virObjectEventPtr virDomainEventIOErrorNewFromObj(virDomainObjPtr obj,
                                                  const char *srcPath,
                                                  const char *devAlias,
                                                  int action)
{
    return virDomainEventIOErrorNewFromObjImpl(VIR_DOMAIN_EVENT_ID_IO_ERROR,
                                               obj, srcPath, devAlias,
                                               action, NULL);
}

virObjectEventPtr virDomainEventIOErrorReasonNewFromDom(virDomainPtr dom,
                                                        const char *srcPath,
                                                        const char *devAlias,
                                                        int action,
                                                        const char *reason)
{
    return virDomainEventIOErrorNewFromDomImpl(VIR_DOMAIN_EVENT_ID_IO_ERROR_REASON,
                                               dom, srcPath, devAlias,
                                               action, reason);
}

virObjectEventPtr virDomainEventIOErrorReasonNewFromObj(virDomainObjPtr obj,
                                                        const char *srcPath,
                                                        const char *devAlias,
                                                        int action,
                                                        const char *reason)
{
    return virDomainEventIOErrorNewFromObjImpl(VIR_DOMAIN_EVENT_ID_IO_ERROR_REASON,
                                               obj, srcPath, devAlias,
                                               action, reason);
}


virObjectEventPtr virDomainEventGraphicsNewFromDom(virDomainPtr dom,
                                       int phase,
                                       virDomainEventGraphicsAddressPtr local,
                                       virDomainEventGraphicsAddressPtr remote,
                                       const char *authScheme,
                                       virDomainEventGraphicsSubjectPtr subject)
{
    virDomainEventGraphicsPtr ev;

    if (virDomainEventsInitialize() < 0)
        return NULL;

    if (!(ev = virDomainEventNew(virDomainEventGraphicsClass,
                                 VIR_DOMAIN_EVENT_ID_GRAPHICS,
                                 dom->id, dom->name, dom->uuid)))
        return NULL;

    ev->phase = phase;
    if (VIR_STRDUP(ev->authScheme, authScheme) < 0) {
        virObjectUnref(ev);
        return NULL;
    }
    ev->local = local;
    ev->remote = remote;
    ev->subject = subject;

    return (virObjectEventPtr)ev;
}

virObjectEventPtr virDomainEventGraphicsNewFromObj(virDomainObjPtr obj,
                                       int phase,
                                       virDomainEventGraphicsAddressPtr local,
                                       virDomainEventGraphicsAddressPtr remote,
                                       const char *authScheme,
                                       virDomainEventGraphicsSubjectPtr subject)
{
    virDomainEventGraphicsPtr ev;

    if (virDomainEventsInitialize() < 0)
        return NULL;

    if (!(ev = virDomainEventNew(virDomainEventGraphicsClass,
                                 VIR_DOMAIN_EVENT_ID_GRAPHICS,
                                 obj->def->id, obj->def->name,
                                 obj->def->uuid)))
        return NULL;

    ev->phase = phase;
    if (VIR_STRDUP(ev->authScheme, authScheme) < 0) {
        virObjectUnref(ev);
        return NULL;
    }
    ev->local = local;
    ev->remote = remote;
    ev->subject = subject;

    return (virObjectEventPtr)ev;
}

static
virObjectEventPtr  virDomainEventBlockJobNew(int id,
                                             const char *name,
                                             unsigned char *uuid,
                                             const char *path,
                                             int type,
                                             int status)
{
    virDomainEventBlockJobPtr ev;

    if (virDomainEventsInitialize() < 0)
        return NULL;

    if (!(ev = virDomainEventNew(virDomainEventBlockJobClass,
                                 VIR_DOMAIN_EVENT_ID_BLOCK_JOB,
                                 id, name, uuid)))
        return NULL;

    if (VIR_STRDUP(ev->path, path) < 0) {
        virObjectUnref(ev);
        return NULL;
    }
    ev->type = type;
    ev->status = status;

    return (virObjectEventPtr)ev;
}

virObjectEventPtr virDomainEventBlockJobNewFromObj(virDomainObjPtr obj,
                                       const char *path,
                                       int type,
                                       int status)
{
    return virDomainEventBlockJobNew(obj->def->id, obj->def->name,
                                     obj->def->uuid, path, type, status);
}

virObjectEventPtr virDomainEventBlockJobNewFromDom(virDomainPtr dom,
                                       const char *path,
                                       int type,
                                       int status)
{
    return virDomainEventBlockJobNew(dom->id, dom->name, dom->uuid,
                                     path, type, status);
}

virObjectEventPtr virDomainEventControlErrorNewFromDom(virDomainPtr dom)
{
    virObjectEventPtr ev;

    if (virDomainEventsInitialize() < 0)
        return NULL;

    if (!(ev = virDomainEventNew(virDomainEventClass,
                                 VIR_DOMAIN_EVENT_ID_CONTROL_ERROR,
                                 dom->id, dom->name, dom->uuid)))
        return NULL;
    return ev;
}


virObjectEventPtr virDomainEventControlErrorNewFromObj(virDomainObjPtr obj)
{
    virObjectEventPtr ev;

    if (virDomainEventsInitialize() < 0)
        return NULL;

    if (!(ev = virDomainEventNew(virDomainEventClass,
                                 VIR_DOMAIN_EVENT_ID_CONTROL_ERROR,
                                 obj->def->id, obj->def->name,
                                 obj->def->uuid)))
        return NULL;
    return ev;
}

static
virObjectEventPtr virDomainEventDiskChangeNew(int id, const char *name,
                                              unsigned char *uuid,
                                              const char *oldSrcPath,
                                              const char *newSrcPath,
                                              const char *devAlias, int reason)
{
    virDomainEventDiskChangePtr ev;

    if (virDomainEventsInitialize() < 0)
        return NULL;

    if (!(ev = virDomainEventNew(virDomainEventDiskChangeClass,
                                 VIR_DOMAIN_EVENT_ID_DISK_CHANGE,
                                 id, name, uuid)))
        return NULL;

    if (VIR_STRDUP(ev->devAlias, devAlias) < 0)
        goto error;

    if (VIR_STRDUP(ev->oldSrcPath, oldSrcPath) < 0)
        goto error;

    if (VIR_STRDUP(ev->newSrcPath, newSrcPath) < 0)
        goto error;

    ev->reason = reason;

    return (virObjectEventPtr)ev;

error:
    virObjectUnref(ev);
    return NULL;
}

virObjectEventPtr virDomainEventDiskChangeNewFromObj(virDomainObjPtr obj,
                                                     const char *oldSrcPath,
                                                     const char *newSrcPath,
                                                     const char *devAlias,
                                                     int reason)
{
    return virDomainEventDiskChangeNew(obj->def->id, obj->def->name,
                                       obj->def->uuid, oldSrcPath,
                                       newSrcPath, devAlias, reason);
}

virObjectEventPtr virDomainEventDiskChangeNewFromDom(virDomainPtr dom,
                                                     const char *oldSrcPath,
                                                     const char *newSrcPath,
                                                     const char *devAlias,
                                                     int reason)
{
    return virDomainEventDiskChangeNew(dom->id, dom->name, dom->uuid,
                                       oldSrcPath, newSrcPath,
                                       devAlias, reason);
}

static virObjectEventPtr
virDomainEventTrayChangeNew(int id, const char *name,
                            unsigned char *uuid,
                            const char *devAlias,
                            int reason)
{
    virDomainEventTrayChangePtr ev;

    if (virDomainEventsInitialize() < 0)
        return NULL;

    if (!(ev = virDomainEventNew(virDomainEventTrayChangeClass,
                                 VIR_DOMAIN_EVENT_ID_TRAY_CHANGE,
                                 id, name, uuid)))
        return NULL;

    if (VIR_STRDUP(ev->devAlias, devAlias) < 0)
        goto error;

    ev->reason = reason;

    return (virObjectEventPtr)ev;

error:
    virObjectUnref(ev);
    return NULL;
}

virObjectEventPtr virDomainEventTrayChangeNewFromObj(virDomainObjPtr obj,
                                                     const char *devAlias,
                                                     int reason)
{
    return virDomainEventTrayChangeNew(obj->def->id,
                                       obj->def->name,
                                       obj->def->uuid,
                                       devAlias,
                                       reason);
}

virObjectEventPtr virDomainEventTrayChangeNewFromDom(virDomainPtr dom,
                                                     const char *devAlias,
                                                     int reason)
{
    return virDomainEventTrayChangeNew(dom->id, dom->name, dom->uuid,
                                       devAlias, reason);
}

static virObjectEventPtr
virDomainEventPMWakeupNew(int id, const char *name,
                          unsigned char *uuid)
{
    virObjectEventPtr ev;

    if (virDomainEventsInitialize() < 0)
        return NULL;

    if (!(ev = virDomainEventNew(virDomainEventClass,
                                 VIR_DOMAIN_EVENT_ID_PMWAKEUP,
                                 id, name, uuid)))
        return NULL;

    return ev;
}

virObjectEventPtr
virDomainEventPMWakeupNewFromObj(virDomainObjPtr obj)
{
    return virDomainEventPMWakeupNew(obj->def->id,
                                     obj->def->name,
                                     obj->def->uuid);
}

virObjectEventPtr
virDomainEventPMWakeupNewFromDom(virDomainPtr dom)
{
    return virDomainEventPMWakeupNew(dom->id, dom->name, dom->uuid);
}

static virObjectEventPtr
virDomainEventPMSuspendNew(int id, const char *name,
                           unsigned char *uuid)
{
    virObjectEventPtr ev;

    if (virDomainEventsInitialize() < 0)
        return NULL;

    if (!(ev = virDomainEventNew(virDomainEventClass,
                                 VIR_DOMAIN_EVENT_ID_PMSUSPEND,
                                 id, name, uuid)))
        return NULL;

    return ev;
}

virObjectEventPtr
virDomainEventPMSuspendNewFromObj(virDomainObjPtr obj)
{
    return virDomainEventPMSuspendNew(obj->def->id,
                                      obj->def->name,
                                      obj->def->uuid);
}

virObjectEventPtr
virDomainEventPMSuspendNewFromDom(virDomainPtr dom)
{
    return virDomainEventPMSuspendNew(dom->id, dom->name, dom->uuid);
}

static virObjectEventPtr
virDomainEventPMSuspendDiskNew(int id, const char *name,
                               unsigned char *uuid)
{
    virObjectEventPtr ev;

    if (virDomainEventsInitialize() < 0)
        return NULL;

    if (!(ev = virDomainEventNew(virDomainEventClass,
                                 VIR_DOMAIN_EVENT_ID_PMSUSPEND_DISK,
                                 id, name, uuid)))
        return NULL;
    return ev;
}

virObjectEventPtr
virDomainEventPMSuspendDiskNewFromObj(virDomainObjPtr obj)
{
    return virDomainEventPMSuspendDiskNew(obj->def->id,
                                          obj->def->name,
                                          obj->def->uuid);
}

virObjectEventPtr
virDomainEventPMSuspendDiskNewFromDom(virDomainPtr dom)
{
    return virDomainEventPMSuspendDiskNew(dom->id, dom->name, dom->uuid);
}

virObjectEventPtr virDomainEventBalloonChangeNewFromDom(virDomainPtr dom,
                                                        unsigned long long actual)
{
    virDomainEventBalloonChangePtr ev;

    if (virDomainEventsInitialize() < 0)
        return NULL;

    if (!(ev = virDomainEventNew(virDomainEventBalloonChangeClass,
                                 VIR_DOMAIN_EVENT_ID_BALLOON_CHANGE,
                                 dom->id, dom->name, dom->uuid)))
        return NULL;

    ev->actual = actual;

    return (virObjectEventPtr)ev;
}
virObjectEventPtr virDomainEventBalloonChangeNewFromObj(virDomainObjPtr obj,
                                                        unsigned long long actual)
{
    virDomainEventBalloonChangePtr ev;

    if (virDomainEventsInitialize() < 0)
        return NULL;

    if (!(ev = virDomainEventNew(virDomainEventBalloonChangeClass,
                                 VIR_DOMAIN_EVENT_ID_BALLOON_CHANGE,
                                 obj->def->id, obj->def->name, obj->def->uuid)))
        return NULL;

    ev->actual = actual;

    return (virObjectEventPtr)ev;
}

static virObjectEventPtr virDomainEventDeviceRemovedNew(int id,
                                            const char *name,
                                            unsigned char *uuid,
                                            const char *devAlias)
{
    virDomainEventDeviceRemovedPtr ev;

    if (virDomainEventsInitialize() < 0)
        return NULL;

    if (!(ev = virDomainEventNew(virDomainEventDeviceRemovedClass,
                                 VIR_DOMAIN_EVENT_ID_DEVICE_REMOVED,
                                 id, name, uuid)))
        return NULL;

    if (VIR_STRDUP(ev->devAlias, devAlias) < 0)
        goto error;

    return (virObjectEventPtr)ev;

error:
    virObjectUnref(ev);
    return NULL;
}

virObjectEventPtr virDomainEventDeviceRemovedNewFromObj(virDomainObjPtr obj,
                                                        const char *devAlias)
{
    return virDomainEventDeviceRemovedNew(obj->def->id, obj->def->name,
                                          obj->def->uuid, devAlias);
}

virObjectEventPtr virDomainEventDeviceRemovedNewFromDom(virDomainPtr dom,
                                                        const char *devAlias)
{
    return virDomainEventDeviceRemovedNew(dom->id, dom->name, dom->uuid,
                                          devAlias);
}


void
virDomainEventDispatchDefaultFunc(virConnectPtr conn,
                                  virObjectEventPtr event,
                                  virConnectDomainEventGenericCallback cb,
                                  void *cbopaque,
                                  void *opaque ATTRIBUTE_UNUSED)
{
    virDomainPtr dom = virGetDomain(conn, event->meta.name, event->meta.uuid);
    int eventID = virObjectEventGetEventID(event);
    if (!dom)
        return;
    dom->id = event->meta.id;

    switch ((virDomainEventID) eventID) {
    case VIR_DOMAIN_EVENT_ID_LIFECYCLE:
        {
            virDomainEventLifecyclePtr lifecycleEvent;

            lifecycleEvent = (virDomainEventLifecyclePtr)event;
            ((virConnectDomainEventCallback)cb)(conn, dom,
                                                lifecycleEvent->type,
                                                lifecycleEvent->detail,
                                                cbopaque);
            goto cleanup;
        }

    case VIR_DOMAIN_EVENT_ID_REBOOT:
        (cb)(conn, dom,
             cbopaque);
        goto cleanup;

    case VIR_DOMAIN_EVENT_ID_RTC_CHANGE:
        {
            virDomainEventRTCChangePtr rtcChangeEvent;

            rtcChangeEvent = (virDomainEventRTCChangePtr)event;
            ((virConnectDomainEventRTCChangeCallback)cb)(conn, dom,
                                                         rtcChangeEvent->offset,
                                                         cbopaque);
            goto cleanup;
        }

    case VIR_DOMAIN_EVENT_ID_WATCHDOG:
        {
            virDomainEventWatchdogPtr watchdogEvent;

            watchdogEvent = (virDomainEventWatchdogPtr)event;
            ((virConnectDomainEventWatchdogCallback)cb)(conn, dom,
                                                        watchdogEvent->action,
                                                        cbopaque);
            goto cleanup;
        }

    case VIR_DOMAIN_EVENT_ID_IO_ERROR:
        {
            virDomainEventIOErrorPtr ioErrorEvent;

            ioErrorEvent = (virDomainEventIOErrorPtr)event;
            ((virConnectDomainEventIOErrorCallback)cb)(conn, dom,
                                                       ioErrorEvent->srcPath,
                                                       ioErrorEvent->devAlias,
                                                       ioErrorEvent->action,
                                                       cbopaque);
            goto cleanup;
        }

    case VIR_DOMAIN_EVENT_ID_IO_ERROR_REASON:
        {
            virDomainEventIOErrorPtr ioErrorEvent;

            ioErrorEvent = (virDomainEventIOErrorPtr)event;
            ((virConnectDomainEventIOErrorReasonCallback)cb)(conn, dom,
                                                             ioErrorEvent->srcPath,
                                                             ioErrorEvent->devAlias,
                                                             ioErrorEvent->action,
                                                             ioErrorEvent->reason,
                                                             cbopaque);
            goto cleanup;
        }

    case VIR_DOMAIN_EVENT_ID_GRAPHICS:
        {
            virDomainEventGraphicsPtr graphicsEvent;

            graphicsEvent = (virDomainEventGraphicsPtr)event;
            ((virConnectDomainEventGraphicsCallback)cb)(conn, dom,
                                                        graphicsEvent->phase,
                                                        graphicsEvent->local,
                                                        graphicsEvent->remote,
                                                        graphicsEvent->authScheme,
                                                        graphicsEvent->subject,
                                                        cbopaque);
            goto cleanup;
        }

    case VIR_DOMAIN_EVENT_ID_CONTROL_ERROR:
        (cb)(conn, dom,
             cbopaque);
        goto cleanup;

    case VIR_DOMAIN_EVENT_ID_BLOCK_JOB:
        {
            virDomainEventBlockJobPtr blockJobEvent;

            blockJobEvent = (virDomainEventBlockJobPtr)event;
            ((virConnectDomainEventBlockJobCallback)cb)(conn, dom,
                                                        blockJobEvent->path,
                                                        blockJobEvent->type,
                                                        blockJobEvent->status,
                                                        cbopaque);
            goto cleanup;
        }

    case VIR_DOMAIN_EVENT_ID_DISK_CHANGE:
        {
            virDomainEventDiskChangePtr diskChangeEvent;

            diskChangeEvent = (virDomainEventDiskChangePtr)event;
            ((virConnectDomainEventDiskChangeCallback)cb)(conn, dom,
                                                          diskChangeEvent->oldSrcPath,
                                                          diskChangeEvent->newSrcPath,
                                                          diskChangeEvent->devAlias,
                                                          diskChangeEvent->reason,
                                                          cbopaque);
            goto cleanup;
        }

    case VIR_DOMAIN_EVENT_ID_TRAY_CHANGE:
        {
            virDomainEventTrayChangePtr trayChangeEvent;

            trayChangeEvent = (virDomainEventTrayChangePtr)event;
            ((virConnectDomainEventTrayChangeCallback)cb)(conn, dom,
                                                          trayChangeEvent->devAlias,
                                                          trayChangeEvent->reason,
                                                          cbopaque);
            goto cleanup;
        }

    case VIR_DOMAIN_EVENT_ID_PMWAKEUP:
        ((virConnectDomainEventPMWakeupCallback)cb)(conn, dom, 0, cbopaque);
        goto cleanup;

    case VIR_DOMAIN_EVENT_ID_PMSUSPEND:
        ((virConnectDomainEventPMSuspendCallback)cb)(conn, dom, 0, cbopaque);
        goto cleanup;

    case VIR_DOMAIN_EVENT_ID_BALLOON_CHANGE:
        {
            virDomainEventBalloonChangePtr balloonChangeEvent;

            balloonChangeEvent = (virDomainEventBalloonChangePtr)event;
            ((virConnectDomainEventBalloonChangeCallback)cb)(conn, dom,
                                                             balloonChangeEvent->actual,
                                                             cbopaque);
            goto cleanup;
        }

    case VIR_DOMAIN_EVENT_ID_PMSUSPEND_DISK:
        ((virConnectDomainEventPMSuspendDiskCallback)cb)(conn, dom, 0, cbopaque);
        goto cleanup;

    case VIR_DOMAIN_EVENT_ID_DEVICE_REMOVED:
        {
            virDomainEventDeviceRemovedPtr deviceRemovedEvent;

            deviceRemovedEvent = (virDomainEventDeviceRemovedPtr)event;
            ((virConnectDomainEventDeviceRemovedCallback)cb)(conn, dom,
                                                             deviceRemovedEvent->devAlias,
                                                             cbopaque);
            goto cleanup;
        }

    case VIR_DOMAIN_EVENT_ID_LAST:
        break;
    }

    VIR_WARN("Unexpected event ID %d", eventID);

cleanup:
    virDomainFree(dom);
}


/**
 * virDomainEventStateRegister:
 * @conn: connection to associate with callback
 * @state: object event state
 * @callback: function to remove from event
 * @opaque: data blob to pass to callback
 * @freecb: callback to free @opaque
 *
 * Register the function @callback with connection @conn,
 * from @state, for lifecycle events.
 *
 * Returns: the number of lifecycle callbacks now registered, or -1 on error
 */
int
virDomainEventStateRegister(virConnectPtr conn,
                            virObjectEventStatePtr state,
                            virConnectDomainEventCallback callback,
                            void *opaque,
                            virFreeCallback freecb)
{
    int ret = -1;

    virObjectEventStateLock(state);

    if ((state->callbacks->count == 0) &&
        (state->timer == -1) &&
        (state->timer = virEventAddTimeout(-1,
                                           virObjectEventTimer,
                                           state,
                                           NULL)) < 0) {
        virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                       _("could not initialize domain event timer"));
        goto cleanup;
    }

    ret = virDomainEventCallbackListAdd(conn, state->callbacks,
                                        callback, opaque, freecb);

    if (ret == -1 &&
        state->callbacks->count == 0 &&
        state->timer != -1) {
        virEventRemoveTimeout(state->timer);
        state->timer = -1;
    }

cleanup:
    virObjectEventStateUnlock(state);
    return ret;
}


/**
 * virDomainEventStateRegisterID:
 * @conn: connection to associate with callback
 * @state: object event state
 * @eventID: ID of the event type to register for
 * @cb: function to remove from event
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
virDomainEventStateRegisterID(virConnectPtr conn,
                              virObjectEventStatePtr state,
                              virDomainPtr dom,
                              int eventID,
                              virConnectDomainEventGenericCallback cb,
                              void *opaque,
                              virFreeCallback freecb,
                              int *callbackID)
{
    if (dom)
        return virObjectEventStateRegisterID(conn, state, dom->uuid, dom->name,
                                             dom->id, eventID,
                                             VIR_OBJECT_EVENT_CALLBACK(cb),
                                             opaque, freecb, callbackID);
     else
        return virObjectEventStateRegisterID(conn, state, NULL, NULL, 0,
                                             eventID,
                                             VIR_OBJECT_EVENT_CALLBACK(cb),
                                             opaque, freecb, callbackID);
}


/**
 * virDomainEventStateDeregister:
 * @conn: connection to associate with callback
 * @state: object event state
 * @callback: function to remove from event
 *
 * Unregister the function @callback with connection @conn,
 * from @state, for lifecycle events.
 *
 * Returns: the number of lifecycle callbacks still registered, or -1 on error
 */
int
virDomainEventStateDeregister(virConnectPtr conn,
                              virObjectEventStatePtr state,
                              virConnectDomainEventCallback callback)
{
    int ret;

    virObjectEventStateLock(state);
    if (state->isDispatching)
        ret = virDomainEventCallbackListMarkDelete(conn,
                                                   state->callbacks, callback);
    else
        ret = virDomainEventCallbackListRemove(conn, state->callbacks, callback);

    if (state->callbacks->count == 0 &&
        state->timer != -1) {
        virEventRemoveTimeout(state->timer);
        state->timer = -1;
        virObjectEventQueueClear(state->queue);
    }

    virObjectEventStateUnlock(state);
    return ret;
}
