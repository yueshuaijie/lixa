/*
 * Copyright (c) 2009-2017, Christian Ferrari <tiian@users.sourceforge.net>
 * All rights reserved.
 *
 * This file is part of LIXA.
 *
 * LIXA is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * LIXA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LIXA.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef MYSQL_XA_RESOURCE_H
# define MYSQL_XA_RESOURCE_H



#include <config.h>


/* MySQL front-end */
#include <mysql.h>
/* XTA includes */
#include "xta_xa_resource.h"



/* save old LIXA_TRACE_MODULE and set a new value */
#ifdef LIXA_TRACE_MODULE
# define LIXA_TRACE_MODULE_SAVE LIXA_TRACE_MODULE
# undef LIXA_TRACE_MODULE
#else
# undef LIXA_TRACE_MODULE_SAVE
#endif /* LIXA_TRACE_MODULE */
#define LIXA_TRACE_MODULE      LIXA_TRACE_MOD_XTA



/**
 * XTA MySQL XA Resource data type
 */
typedef struct {
    union {
        xta_xa_resource_t  xa_resource;
    };
} xta_mysql_xa_resource_t;



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



    /**
     * Create a new object to represent a MySQL XA resource
     * @param[in] connection to MySQL already opened by the application
     *            program
     * @return a new object or NULL in the event of error
     */
    xta_mysql_xa_resource_t *xta_mysql_xa_resource_new(MYSQL *connection);

    

    /**
     * Delete an object that represent a MySQL XA Resource
     * @param[in] xa_res : MySQL XA Resource
     */
    void xta_mysql_xa_resource_delete(
        xta_mysql_xa_resource_t *xa_res);
    
    

#ifdef __cplusplus
}
#endif /* __cplusplus */



/* restore old value of LIXA_TRACE_MODULE */
#ifdef LIXA_TRACE_MODULE_SAVE
# undef LIXA_TRACE_MODULE
# define LIXA_TRACE_MODULE LIXA_TRACE_MODULE_SAVE
# undef LIXA_TRACE_MODULE_SAVE
#endif /* LIXA_TRACE_MODULE_SAVE */



#endif /* MYSQL_XA_RESOURCE_H */
