/*
 * domain_stats.c: domain stats extraction helpers
 *
 * Copyright (C) 2006-2016 Red Hat, Inc.
 * Copyright (C) 2006-2008 Daniel P. Berrange
 * Copyright (c) 2018 SUSE LINUX Products GmbH, Nuernberg, Germany.
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
 * Author: Daniel P. Berrange <berrange@redhat.com>
 */

#include <config.h>

#include <stdio.h>

#include "virlog.h"
#include "domain_stats.h"
#include "virtypedparam.h"
#include "virnetdevtap.h"
#include "virnetdevopenvswitch.h"

#define VIR_FROM_THIS VIR_FROM_DOMAIN

VIR_LOG_INIT("conf.domain_stats");


int
virDomainStatsAddCountParam(virDomainStatsRecordPtr record,
                            int *maxparams,
                            const char *type,
                            unsigned int count)
{
    char param_name[VIR_TYPED_PARAM_FIELD_LENGTH];
    snprintf(param_name, VIR_TYPED_PARAM_FIELD_LENGTH, "%s.count", type);
    return virTypedParamsAddUInt(&(record)->params,
                                 &(record)->nparams,
                                 maxparams,
                                 param_name,
                                 count);
}


int
virDomainStatsAddNameParam(virDomainStatsRecordPtr record,
                           int *maxparams,
                           const char *type,
                           const char *subtype,
                           size_t num,
                           const char *name)
{
    char param_name[VIR_TYPED_PARAM_FIELD_LENGTH];
    snprintf(param_name, VIR_TYPED_PARAM_FIELD_LENGTH,
             "%s.%zu.%s", type, num, subtype);
    return virTypedParamsAddString(&(record)->params,
                                   &(record)->nparams,
                                   maxparams,
                                   param_name,
                                   name);
}


int
virDomainStatsGetState(virDomainObjPtr dom,
                       virDomainStatsRecordPtr record,
                       int *maxparams)
{
    if (virTypedParamsAddInt(&record->params,
                             &record->nparams,
                             maxparams,
                             "state.state",
                             dom->state.state) < 0)
        return -1;

    if (virTypedParamsAddInt(&record->params,
                             &record->nparams,
                             maxparams,
                             "state.reason",
                             dom->state.reason) < 0)
        return -1;

    return 0;
}

#define STATS_ADD_NET_PARAM(record, maxparams, num, name, value) \
do { \
    char param_name[VIR_TYPED_PARAM_FIELD_LENGTH]; \
    snprintf(param_name, VIR_TYPED_PARAM_FIELD_LENGTH, \
             "net.%zu.%s", num, name); \
    if (value >= 0 && virTypedParamsAddULLong(&(record)->params, \
                                              &(record)->nparams, \
                                              maxparams, \
                                              param_name, \
                                              value) < 0) \
        return -1; \
} while (0)


int
virDomainStatsGetInterface(virDomainObjPtr dom,
                           virDomainStatsRecordPtr record,
                           int *maxparams)
{
    size_t i;
    struct _virDomainInterfaceStats tmp;
    int ret = -1;

    if (!virDomainObjIsActive(dom))
        return 0;

    if (virDomainStatsAddCountParam(record, maxparams, "net", dom->def->nnets) < 0)
        goto cleanup;

    /* Check the path is one of the domain's network interfaces. */
    for (i = 0; i < dom->def->nnets; i++) {
        virDomainNetDefPtr net = dom->def->nets[i];
        virDomainNetType actualType;

        if (!net->ifname)
            continue;

        memset(&tmp, 0, sizeof(tmp));

        actualType = virDomainNetGetActualType(net);

        if (virDomainStatsAddNameParam(record, maxparams,
                                       "net", "name", i, net->ifname) < 0)
            goto cleanup;

        if (actualType == VIR_DOMAIN_NET_TYPE_VHOSTUSER) {
            if (virNetDevOpenvswitchInterfaceStats(net->ifname, &tmp) < 0) {
                virResetLastError();
                continue;
            }
        } else {
            if (virNetDevTapInterfaceStats(net->ifname, &tmp,
                                           !virDomainNetTypeSharesHostView(net)) < 0) {
                virResetLastError();
                continue;
            }
        }

        STATS_ADD_NET_PARAM(record, maxparams, i,
                           "rx.bytes", tmp.rx_bytes);
        STATS_ADD_NET_PARAM(record, maxparams, i,
                           "rx.pkts", tmp.rx_packets);
        STATS_ADD_NET_PARAM(record, maxparams, i,
                           "rx.errs", tmp.rx_errs);
        STATS_ADD_NET_PARAM(record, maxparams, i,
                           "rx.drop", tmp.rx_drop);
        STATS_ADD_NET_PARAM(record, maxparams, i,
                           "tx.bytes", tmp.tx_bytes);
        STATS_ADD_NET_PARAM(record, maxparams, i,
                           "tx.pkts", tmp.tx_packets);
        STATS_ADD_NET_PARAM(record, maxparams, i,
                           "tx.errs", tmp.tx_errs);
        STATS_ADD_NET_PARAM(record, maxparams, i,
                           "tx.drop", tmp.tx_drop);
    }

    ret = 0;
 cleanup:
    return ret;
}

#undef STATS_ADD_NET_PARAM
