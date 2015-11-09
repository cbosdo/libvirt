/*
 * lxc_migration.c: LXC migration
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

#include <config.h>
#include <stdio.h>

#include "lxc_migration.h"
#include "util/virlog.h"

#define VIR_FROM_THIS VIR_FROM_LXC

VIR_LOG_INIT("lxc.lxc_migration");

char *
lxcMigrationBegin(virConnectPtr conn,
                  virDomainObjPtr vm,
                  const char *xmlin,
                  char **cookitout,
                  iont *cookieoutlen,
                  unsigned long flags,
                  const char *dname)
{
    /* TODO Implement me:
     * lock the VM,
     * return the XML definition of the VM */
    return NULL;
}

/* TODO This function could be factorized with QEMU driver */
virDomainDefPtr
lxcMigrationPrepareDef(virLXCDriverPtr driver,
                       const char *dom_xml,
                       const char *dname,
                       char **origname)
{
    virCapsPtr caps = NULL;
    virDomainDefPtr def;
    char *name = NULL;

    if (!dom_xml) {
        virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                       _("no domain XML passed"));
        return NULL;
    }

    if (!(caps = virLXCDriverGetCapabilities(driver, false)))
        return NULL;

    if (!(def = virDomainDefParseString(dom_xml, caps, driver->xmlopt,
                                        VIR_DOMAIN_DEF_PARSE_INACTIVE)))
        goto cleanup;

    if (dname) {
        name = def->name;
        if (VIR_STRDUP(def->name, dname) < 0) {
            virDomainDefFree(def);
            def = NULL;
        }
    }

 cleanup:
    virObjectUnref(caps);
    if (def && origname)
        *origname = name;
    else
        VIR_FREE(name);
    return def;
}

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
                    unsigned long flags)
{
    int ret = -1;

    VIR_DEBUG("driver=%p, dconn=%p, cookiein=%s, cookieinlen=%d, "
              "cookieout=%p, cookieoutlen=%p, uri_in=%s, uri_out=%p, "
              "def=%p, origname=%s, flags=%lx",
              driver, dconn, NULLSTR(cookiein), cookieinlen,
              cookieout, cookieoutlen, NULLSTR(uri_in), uri_out,
              *def, origname, flags);

    /* TODO Eat in cookie (Optional) */

    /* TODO start criu page-server */

    /* TODO Prepare storage for disk if needed (Later) */

    /* TODO Create out cookie (Optional) */

    return ret;
}
