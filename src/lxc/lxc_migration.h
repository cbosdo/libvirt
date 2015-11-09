/*
 * lxc_migration.h: LXC migration
 *
 * Copyright (c) 2015 SUSE LINUX Products GmbH, Nuernberg, Germany.
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
 * Author: Cedric Bosdonnat <cbosdonnat@suse.com>
 */

#ifndef __LXC_MIGRATION_H__
# define __LXC_MIGRATION_H__

/* All supported lxc migration flags */
# define LXC_MIGRATION_FLAGS              \
    (VIR_MIGRATE_LIVE |                   \
     VIR_MIGRATE_UNDEFINE_SOURCE)

char *
lxcMigrationBegin(virConnectPtr conn,
                  virDomainObjPtr vm,
                  const char *xmlin,
                  char **cookitout,
                  iont *cookieoutlen,
                  unsigned long flags,
                  const char *dname);

virDomainDefPtr
lxcMigrationPrepareDef(virLXCDriverPtr driver,
                       const char *dom_xml,
                       const char *dname,
                       char **origname);

int
lxcMigrationPrepare(virLXCDriverPtr driver,
                    virConnectPtr dconn,
                    const char *cookiein,
                    int cookieinlen,
                    char **cookieout,
                    int *cookieoutlen,
                    const char *uri_in,
                    char **uri_out,
                    virDomainDefPtr *def,
                    const char *origname,
                    unsigned long flags);


#endif /* __LXC_MIGRATION_H__ */
