/*
 * Copyright (c) 2009-2016, Christian Ferrari <tiian@users.sourceforge.net>
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
#include <config.h>

#ifdef HAVE_SYS_TIME_H

#include <sys/time.h>

#endif
#ifdef HAVE_TIME_H

#include <time.h>

#endif
#ifdef HAVE_SYSLOG_H

#include <syslog.h>

#endif

#include <lixa_errors.h>
#include <lixa_crash.h>
#include <lixa_trace.h>
#include <lixa_syslog.h>
#include <lixa_xid.h>
#include <src/common/lixa_xml_msg.h>
#include "server_tpm.h"

/* set module trace flag */
#ifdef LIXA_TRACE_MODULE
#undef LIXA_TRACE_MODULE
#endif /* LIXA_TRACE_MODULE */
#define LIXA_TRACE_MODULE LIXA_TRACE_MOD_SERVER_TPM

int server_trans(struct thread_status_s *ts, size_t slot_id,
                 const struct lixa_msg_s *lmi, struct lixa_msg_s *lmo,
                 uint32_t block_id, struct lixa_msg_verb_step_s *last_verb_step)
{
    enum Exception
    {
        SERVER_TRANS_8_ERROR, INVALID_STEP, NONE
    } excp;
    int ret_cod = LIXA_RC_INTERNAL_ERROR;

    LIXA_TRACE(("server_trans\n"));
    TRY {
        switch (lmi->header.pvs.step) {
            case 8:
                if (LIXA_RC_OK != (
                        ret_cod = server_trans_8(
                            ts, slot_id, lmi, lmo, block_id,
                            last_verb_step))) THROW(SERVER_TRANS_8_ERROR);
                break;
            default: THROW(INVALID_STEP);
        }

        THROW(NONE);
    }
    CATCH
        {
            switch (excp) {
                case SERVER_TRANS_8_ERROR:
                    break;
                case INVALID_STEP:
                    ret_cod = LIXA_RC_INVALID_STATUS;
                    break;
                case NONE:
                    ret_cod = LIXA_RC_OK;
                    break;
                default:
                    ret_cod = LIXA_RC_INTERNAL_ERROR;
            } /* switch (excp) */
        } /* TRY-CATCH */
    LIXA_TRACE(("server_trans/excp=%d/"
                "ret_cod=%d/errno=%d\n", excp, ret_cod, errno));
    return ret_cod;
}

int server_trans_8(struct thread_status_s *ts, size_t slot_id,
                   const struct lixa_msg_s *lmi, struct lixa_msg_s *lmo,
                   uint32_t block_id,
                   struct lixa_msg_verb_step_s *last_verb_step)
{
    enum Exception
    {
        PROTOCOL_ERROR,
        JOB_SET_RAW_ERROR,
        SERVER_TRANS_RESULT_ERROR,
        TRANS_RESULT_EMPTY_ERROR,
        GET_XID_ERROR,
        NONE
    } excp;
    int ret_cod = LIXA_RC_INTERNAL_ERROR;

    LIXA_TRACE(("server_trans_8\n"));
    TRY {
        if (LIXA_MSG_VERB_TRANS != lmi->header.pvs.verb) THROW(PROTOCOL_ERROR);

        /* query all transactions matching gtrid */
        struct server_trans_tbl_rec_s query;
        struct server_trans_tbl_rec_s result[12] = {{0}};
        guint result_array_size = 0;

        query.gtrid = lixa_xid_get_gtrid_ascii(
            &(ts->curr_status[block_id].sr.data.pld.ph.state.xid));
        query.tsid = ts->id;

        ret_cod = server_trans_tbl_query_xid(ts->trans_table, &query, result,
                                             &result_array_size,
                                             lmi->body.trans_8.client.maint);

        switch (ret_cod) {
            case LIXA_RC_OK:
                if (LIXA_RC_OK != (ret_cod = server_trans_result(
                                       ts, result, result_array_size, lmi, lmo, block_id))) THROW(
                                           SERVER_TRANS_RESULT_ERROR);
                break;
            case LIXA_RC_OBJ_NOT_FOUND:
                if (LIXA_RC_OK != (ret_cod = server_trans_empty_result(
                                       ts, lmi, lmo))) THROW(TRANS_RESULT_EMPTY_ERROR);
                break;
            default: THROW(GET_XID_ERROR);
        } /* switch (rc) */

        /* prepare next protocol step */
        last_verb_step->verb = lmo->header.pvs.verb;
        last_verb_step->step = lmo->header.pvs.step;

        THROW(NONE);
    }
    CATCH
        {
            switch (excp) {
                case PROTOCOL_ERROR:
                    ret_cod = LIXA_RC_PROTOCOL_ERROR;
                    break;
                case JOB_SET_RAW_ERROR:
                    break;
                case SERVER_TRANS_RESULT_ERROR:
                    break;
                case TRANS_RESULT_EMPTY_ERROR:
                case GET_XID_ERROR:
                    break;
                case NONE:
                    ret_cod = LIXA_RC_OK;
                    break;
                default:
                    ret_cod = LIXA_RC_INTERNAL_ERROR;
            } /* switch (excp) */
        }
    LIXA_TRACE(("server_trans_8/excp=%d/"
                "ret_cod=%d/errno=%d\n", excp, ret_cod, errno));
    return ret_cod;
}

int server_trans_result(struct thread_status_s *ts,
                        const struct server_trans_tbl_rec_s *record,
                        const guint record_array_size,
                        const struct lixa_msg_s *lmi, struct lixa_msg_s *lmo,
                        uint32_t block_id)
{
    enum Exception
    {
        XML_CHAR_STRDUP_ERROR, NONE
    } excp;
    int ret_cod = LIXA_RC_INTERNAL_ERROR;

    LIXA_TRACE(("server_trans_result\n"));
    TRY {
        struct status_record_data_payload_s *pld =
            &(ts->curr_status[block_id].sr.data.pld);

        /* set basic answer information */
        lmo->header.pvs.verb = LIXA_MSG_VERB_TRANS;
        lmo->body.trans_16.answer.rc = LIXA_RC_OK;

        /* this duplication is not necessary, but it helps in keeping a clean code: no special behaviour in @ref lixa_msg_free */
        if (NULL == (lmo->body.trans_16.client.job =
                     xmlCharStrdup(lixa_job_get_raw(&pld->ph.job)))) THROW(
                         XML_CHAR_STRDUP_ERROR);
        memcpy(&lmo->body.trans_16.client.config_digest,
               &pld->ph.config_digest, sizeof(md5_digest_hex_t));

        /* check config digest: current and past */
        if (memcmp(lmi->body.trans_8.client.config_digest,
                   lmo->body.trans_16.client.config_digest,
                   sizeof(md5_digest_hex_t))) {
            lixa_ser_xid_t ser_xid = "";
            lixa_xid_serialize(&pld->ph.state.xid, ser_xid);

            lmo->body.trans_16.answer.rc = LIXA_RC_LIXAC_CONF_CHANGED;
            syslog(LOG_WARNING, LIXA_SYSLOG_LXD011W,
                   lmo->body.trans_16.client.job, ser_xid);
            LIXA_TRACE(("server_trans_result: job is '%s', past config "
                        "digest is '%s', current config digest is '%s'\n",
                        lmo->body.trans_16.client.job,
                        lmo->body.trans_16.client.config_digest,
                        lmi->body.trans_8.client.config_digest));
        }

        lmo->body.trans_16.client.last_verb_step.verb =
            pld->ph.last_verb_step[0].verb;
        lmo->body.trans_16.client.last_verb_step.step =
            pld->ph.last_verb_step[0].step;

        lmo->body.trans_16.transactions = g_array_sized_new(
            FALSE, FALSE,
            sizeof(struct lixa_msg_body_trans_16_transaction_s),
            record_array_size);
        guint i;
        for (i = 0; i < record_array_size; ++i) {
            struct lixa_msg_body_trans_16_transaction_s trans;
            memcpy(trans.xid, record[i].xid, LIXA_XID_SERIALIZE_LENGTH);
            g_array_append_val(lmo->body.trans_16.transactions, trans);
        }

        THROW(NONE);
    }
    CATCH
        {
            switch (excp) {
                case XML_CHAR_STRDUP_ERROR:
                    ret_cod = LIXA_RC_XML_CHAR_STRDUP_ERROR;
                    break;
                case NONE:
                    ret_cod = LIXA_RC_OK;
                    break;
                default:
                    ret_cod = LIXA_RC_INTERNAL_ERROR;
            } /* switch (excp) */
        } /* TRY-CATCH */
    LIXA_TRACE(("server_trans_result/excp=%d/"
                "ret_cod=%d/errno=%d\n", excp, ret_cod, errno));
    return ret_cod;
}

int server_trans_empty_result(struct thread_status_s *ts,
                              const struct lixa_msg_s *lmi,
                              struct lixa_msg_s *lmo)
{
    enum Exception
    {
        NONE
    } excp;
    int ret_cod = LIXA_RC_INTERNAL_ERROR;

    LIXA_TRACE(("server_trans_empty_result\n"));
    TRY {
        lmo->header.pvs.verb = LIXA_MSG_VERB_TRANS;
        lmo->body.trans_16.answer.rc = LIXA_RC_OBJ_NOT_FOUND;

        THROW(NONE);
    }
    CATCH
        {
            switch (excp) {
                case NONE:
                    ret_cod = LIXA_RC_OK;
                    break;
                default:
                    ret_cod = LIXA_RC_INTERNAL_ERROR;
            } /* switch (excp) */
        } /* TRY-CATCH */
    LIXA_TRACE(("server_trans_empty_result/excp=%d/"
                "ret_cod=%d/errno=%d\n", excp, ret_cod, errno));
    return ret_cod;
}
