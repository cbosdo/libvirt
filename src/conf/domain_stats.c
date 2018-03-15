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

#define VIR_FROM_THIS VIR_FROM_DOMAIN

VIR_LOG_INIT("conf.domain_stats");


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
