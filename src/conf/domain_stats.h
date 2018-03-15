/*
 * domain_stats.h: domain stats extraction helpers
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
#ifndef __DOMAIN_STATS_H
# define __DOMAIN_STATS_H

# include "internal.h"
# include "domain_conf.h"


int virDomainStatsAddCountParam(virDomainStatsRecordPtr record,
                                int *maxparams,
                                const char *type,
                                unsigned int count);

int virDomainStatsAddNameParam(virDomainStatsRecordPtr record,
                               int *maxparams,
                               const char *type,
                               const char *subtype,
                               size_t num,
                               const char *name);

int virDomainStatsGetState(virDomainObjPtr dom,
                           virDomainStatsRecordPtr record,
                           int *maxparams);

int virDomainStatsGetInterface(virDomainObjPtr dom,
                               virDomainStatsRecordPtr record,
                               int *maxparams);

#endif /* __DOMAIN_STATS_H */
