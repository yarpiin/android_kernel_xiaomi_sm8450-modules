/*
 * Copyright (c) 2011-2021 The Linux Foundation. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * DOC: csr_util.c
 *
 * Implementation supporting routines for CSR.
 */

#include "ani_global.h"

#include "csr_support.h"
#include "csr_inside_api.h"
#include "sme_qos_internal.h"
#include "wma_types.h"
#include "cds_utils.h"
#include "wlan_policy_mgr_api.h"
#include "wlan_serialization_legacy_api.h"
#include "wlan_reg_services_api.h"
#include "wlan_crypto_global_api.h"
#include "wlan_cm_roam_api.h"
#include <../../core/src/wlan_cm_vdev_api.h>

uint8_t csr_wpa_oui[][CSR_WPA_OUI_SIZE] = {
	{0x00, 0x50, 0xf2, 0x00}
	,
	{0x00, 0x50, 0xf2, 0x01}
	,
	{0x00, 0x50, 0xf2, 0x02}
	,
	{0x00, 0x50, 0xf2, 0x03}
	,
	{0x00, 0x50, 0xf2, 0x04}
	,
	{0x00, 0x50, 0xf2, 0x05}
	,
#ifdef FEATURE_WLAN_ESE
	{0x00, 0x40, 0x96, 0x00}
	,                       /* CCKM */
#endif /* FEATURE_WLAN_ESE */
};

#define FT_PSK_IDX   4
#define FT_8021X_IDX 3

/*
 * PLEASE DO NOT ADD THE #IFDEF IN BELOW TABLE,
 * IF STILL REQUIRE THEN PLEASE ADD NULL ENTRIES
 * OTHERWISE IT WILL BREAK OTHER LOWER
 * SECUIRTY MODES.
 */

uint8_t csr_rsn_oui[][CSR_RSN_OUI_SIZE] = {
	{0x00, 0x0F, 0xAC, 0x00}
	,                       /* group cipher */
	{0x00, 0x0F, 0xAC, 0x01}
	,                       /* WEP-40 or RSN */
	{0x00, 0x0F, 0xAC, 0x02}
	,                       /* TKIP or RSN-PSK */
	{0x00, 0x0F, 0xAC, 0x03}
	,                       /* Reserved */
	{0x00, 0x0F, 0xAC, 0x04}
	,                       /* AES-CCMP */
	{0x00, 0x0F, 0xAC, 0x05}
	,                       /* WEP-104 */
	{0x00, 0x40, 0x96, 0x00}
	,                       /* CCKM */
	{0x00, 0x0F, 0xAC, 0x06}
	,                       /* BIP (encryption type) or
				 * RSN-PSK-SHA256 (authentication type)
				 */
	/* RSN-8021X-SHA256 (authentication type) */
	{0x00, 0x0F, 0xAC, 0x05},
#ifdef WLAN_FEATURE_FILS_SK
#define ENUM_FILS_SHA256 9
	/* FILS SHA256 */
	{0x00, 0x0F, 0xAC, 0x0E},
#define ENUM_FILS_SHA384 10
	/* FILS SHA384 */
	{0x00, 0x0F, 0xAC, 0x0F},
#define ENUM_FT_FILS_SHA256 11
	/* FILS FT SHA256 */
	{0x00, 0x0F, 0xAC, 0x10},
#define ENUM_FT_FILS_SHA384 12
	/* FILS FT SHA384 */
	{0x00, 0x0F, 0xAC, 0x11},
#else
	{0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00},
#endif
	/* AES GCMP */
	{0x00, 0x0F, 0xAC, 0x08},
	/* AES GCMP-256 */
	{0x00, 0x0F, 0xAC, 0x09},
#define ENUM_DPP_RSN 15
	/* DPP RSN */
	{0x50, 0x6F, 0x9A, 0x02},
#define ENUM_OWE 16
	/* OWE https://tools.ietf.org/html/rfc8110 */
	{0x00, 0x0F, 0xAC, 0x12},
#define ENUM_SUITEB_EAP256 17
	{0x00, 0x0F, 0xAC, 0x0B},
#define ENUM_SUITEB_EAP384 18
	{0x00, 0x0F, 0xAC, 0x0C},

#ifdef WLAN_FEATURE_SAE
#define ENUM_SAE 19
	/* SAE */
	{0x00, 0x0F, 0xAC, 0x08},
#define ENUM_FT_SAE 20
	/* FT SAE */
	{0x00, 0x0F, 0xAC, 0x09},
#else
	{0x00, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00},
#endif
#define ENUM_OSEN 21
	/* OSEN RSN */
	{0x50, 0x6F, 0x9A, 0x01},
#define ENUM_FT_SUITEB_SHA384 22
	/* FT Suite-B SHA384 */
	{0x00, 0x0F, 0xAC, 0x0D},

	/* define new oui here, update #define CSR_OUI_***_INDEX  */
};

#ifdef FEATURE_WLAN_WAPI
uint8_t csr_wapi_oui[][CSR_WAPI_OUI_SIZE] = {
	{0x00, 0x14, 0x72, 0x00}
	,                       /* Reserved */
	{0x00, 0x14, 0x72, 0x01}
	,                       /* WAI certificate or SMS4 */
	{0x00, 0x14, 0x72, 0x02} /* WAI PSK */
};
#endif /* FEATURE_WLAN_WAPI */

uint8_t csr_group_mgmt_oui[][CSR_RSN_OUI_SIZE] = {
#define ENUM_CMAC 0
	{0x00, 0x0F, 0xAC, 0x06},
#define ENUM_GMAC_128 1
	{0x00, 0x0F, 0xAC, 0x0B},
#define ENUM_GMAC_256 2
	{0x00, 0x0F, 0xAC, 0x0C},
};

#define CASE_RETURN_STR(n) {\
	case (n): return (# n);\
}

const char *get_e_roam_cmd_status_str(eRoamCmdStatus val)
{
	switch (val) {
#ifndef FEATURE_CM_ENABLE
		CASE_RETURN_STR(eCSR_ROAM_CANCELLED);
		CASE_RETURN_STR(eCSR_ROAM_ROAMING_START);
		CASE_RETURN_STR(eCSR_ROAM_ROAMING_COMPLETION);
		CASE_RETURN_STR(eCSR_ROAM_CONNECT_COMPLETION);
		CASE_RETURN_STR(eCSR_ROAM_ASSOCIATION_COMPLETION);
		CASE_RETURN_STR(eCSR_ROAM_DISASSOCIATED);
		CASE_RETURN_STR(eCSR_ROAM_ASSOCIATION_FAILURE);
		CASE_RETURN_STR(eCSR_ROAM_SHOULD_ROAM);
#endif
		CASE_RETURN_STR(eCSR_ROAM_LOSTLINK);
		CASE_RETURN_STR(eCSR_ROAM_MIC_ERROR_IND);
		CASE_RETURN_STR(eCSR_ROAM_SET_KEY_COMPLETE);
		CASE_RETURN_STR(eCSR_ROAM_INFRA_IND);
		CASE_RETURN_STR(eCSR_ROAM_WPS_PBC_PROBE_REQ_IND);
#ifndef FEATURE_CM_ENABLE
		CASE_RETURN_STR(eCSR_ROAM_FT_RESPONSE);
		CASE_RETURN_STR(eCSR_ROAM_FT_START);
		CASE_RETURN_STR(eCSR_ROAM_FT_REASSOC_FAILED);
		CASE_RETURN_STR(eCSR_ROAM_PMK_NOTIFY);
#ifdef FEATURE_WLAN_LFR_METRICS
		CASE_RETURN_STR(eCSR_ROAM_PREAUTH_INIT_NOTIFY);
		CASE_RETURN_STR(eCSR_ROAM_PREAUTH_STATUS_SUCCESS);
		CASE_RETURN_STR(eCSR_ROAM_PREAUTH_STATUS_FAILURE);
		CASE_RETURN_STR(eCSR_ROAM_HANDOVER_SUCCESS);
#endif
#endif
		CASE_RETURN_STR(eCSR_ROAM_DISCONNECT_ALL_P2P_CLIENTS);
		CASE_RETURN_STR(eCSR_ROAM_SEND_P2P_STOP_BSS);
		CASE_RETURN_STR(eCSR_ROAM_UNPROT_MGMT_FRAME_IND);
#ifdef FEATURE_WLAN_ESE
		CASE_RETURN_STR(eCSR_ROAM_TSM_IE_IND);
#ifndef FEATURE_CM_ENABLE
		CASE_RETURN_STR(eCSR_ROAM_CCKM_PREAUTH_NOTIFY);
#endif
		CASE_RETURN_STR(eCSR_ROAM_ESE_ADJ_AP_REPORT_IND);
		CASE_RETURN_STR(eCSR_ROAM_ESE_BCN_REPORT_IND);
#endif /* FEATURE_WLAN_ESE */
		CASE_RETURN_STR(eCSR_ROAM_DFS_RADAR_IND);
		CASE_RETURN_STR(eCSR_ROAM_SET_CHANNEL_RSP);
		CASE_RETURN_STR(eCSR_ROAM_DFS_CHAN_SW_NOTIFY);
		CASE_RETURN_STR(eCSR_ROAM_EXT_CHG_CHNL_IND);
		CASE_RETURN_STR(eCSR_ROAM_STA_CHANNEL_SWITCH);
		CASE_RETURN_STR(eCSR_ROAM_NDP_STATUS_UPDATE);
#ifndef FEATURE_CM_ENABLE
		CASE_RETURN_STR(eCSR_ROAM_START);
		CASE_RETURN_STR(eCSR_ROAM_ABORT);
		CASE_RETURN_STR(eCSR_ROAM_NAPI_OFF);
#endif
		CASE_RETURN_STR(eCSR_ROAM_CHANNEL_COMPLETE_IND);
		CASE_RETURN_STR(eCSR_ROAM_SAE_COMPUTE);
	default:
		return "unknown";
	}
}

const char *get_e_csr_roam_result_str(eCsrRoamResult val)
{
	switch (val) {
		CASE_RETURN_STR(eCSR_ROAM_RESULT_NONE);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_FAILURE);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_ASSOCIATED);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_NOT_ASSOCIATED);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_MIC_FAILURE);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_FORCED);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_DISASSOC_IND);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_DEAUTH_IND);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_CAP_CHANGED);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_LOSTLINK);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_MIC_ERROR_UNICAST);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_MIC_ERROR_GROUP);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_AUTHENTICATED);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_NEW_RSN_BSS);
 #ifdef FEATURE_WLAN_WAPI
		CASE_RETURN_STR(eCSR_ROAM_RESULT_NEW_WAPI_BSS);
 #endif /* FEATURE_WLAN_WAPI */
		CASE_RETURN_STR(eCSR_ROAM_RESULT_INFRA_STARTED);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_INFRA_START_FAILED);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_INFRA_STOPPED);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_INFRA_ASSOCIATION_IND);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_INFRA_ASSOCIATION_CNF);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_INFRA_DISASSOCIATED);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_WPS_PBC_PROBE_REQ_IND);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_SEND_ACTION_FAIL);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_MAX_ASSOC_EXCEEDED);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_ASSOC_FAIL_CON_CHANNEL);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_ADD_TDLS_PEER);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_UPDATE_TDLS_PEER);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_DELETE_TDLS_PEER);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_TEARDOWN_TDLS_PEER_IND);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_DELETE_ALL_TDLS_PEER_IND);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_LINK_ESTABLISH_REQ_RSP);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_TDLS_SHOULD_DISCOVER);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_TDLS_SHOULD_TEARDOWN);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_TDLS_SHOULD_PEER_DISCONNECTED);
		CASE_RETURN_STR
			(eCSR_ROAM_RESULT_TDLS_CONNECTION_TRACKER_NOTIFICATION);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_DFS_RADAR_FOUND_IND);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_CHANNEL_CHANGE_SUCCESS);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_CHANNEL_CHANGE_FAILURE);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_CSA_RESTART_RSP);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_DFS_CHANSW_UPDATE_SUCCESS);
		CASE_RETURN_STR(eCSR_ROAM_EXT_CHG_CHNL_UPDATE_IND);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_NDI_CREATE_RSP);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_NDI_DELETE_RSP);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_NDP_INITIATOR_RSP);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_NDP_NEW_PEER_IND);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_NDP_CONFIRM_IND);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_NDP_INDICATION);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_NDP_SCHED_UPDATE_RSP);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_NDP_RESPONDER_RSP);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_NDP_END_RSP);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_NDP_PEER_DEPARTED_IND);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_NDP_END_IND);
		CASE_RETURN_STR(eCSR_ROAM_RESULT_SCAN_FOR_SSID_FAILURE);
	default:
		return "unknown";
	}
}

const char *csr_phy_mode_str(eCsrPhyMode phy_mode)
{
	switch (phy_mode) {
	case eCSR_DOT11_MODE_abg:
		return "abg";
	case eCSR_DOT11_MODE_11a:
		return "11a";
	case eCSR_DOT11_MODE_11b:
		return "11b";
	case eCSR_DOT11_MODE_11g:
		return "11g";
	case eCSR_DOT11_MODE_11n:
		return "11n";
	case eCSR_DOT11_MODE_11g_ONLY:
		return "11g_only";
	case eCSR_DOT11_MODE_11n_ONLY:
		return "11n_only";
	case eCSR_DOT11_MODE_11b_ONLY:
		return "11b_only";
	case eCSR_DOT11_MODE_11ac:
		return "11ac";
	case eCSR_DOT11_MODE_11ac_ONLY:
		return "11ac_only";
	case eCSR_DOT11_MODE_AUTO:
		return "auto";
	case eCSR_DOT11_MODE_11ax:
		return "11ax";
	case eCSR_DOT11_MODE_11ax_ONLY:
		return "11ax_only";
	case eCSR_DOT11_MODE_11be:
		return "11be";
	case eCSR_DOT11_MODE_11be_ONLY:
		return "11be_only";
	default:
		return "unknown";
	}
}

void csr_purge_vdev_pending_ser_cmd_list(struct mac_context *mac_ctx,
					 uint32_t vdev_id)
{
	wlan_serialization_purge_all_pending_cmd_by_vdev_id(mac_ctx->pdev,
							    vdev_id);
}

void csr_purge_vdev_all_scan_ser_cmd_list(struct mac_context *mac_ctx,
					  uint32_t vdev_id)
{
	wlan_serialization_purge_all_scan_cmd_by_vdev_id(mac_ctx->pdev,
							 vdev_id);
}

void csr_purge_pdev_all_ser_cmd_list(struct mac_context *mac_ctx)
{
	wlan_serialization_purge_all_pdev_cmd(mac_ctx->pdev);
}

tListElem *csr_nonscan_active_ll_peek_head(struct mac_context *mac_ctx,
					   bool inter_locked)
{
	struct wlan_serialization_command *cmd;
	tSmeCmd *sme_cmd;

	cmd = wlan_serialization_peek_head_active_cmd_using_psoc(mac_ctx->psoc,
								 false);
	if (!cmd || cmd->source != WLAN_UMAC_COMP_MLME)
		return NULL;

	sme_cmd = cmd->umac_cmd;

	return &sme_cmd->Link;
}

tListElem *csr_nonscan_pending_ll_peek_head(struct mac_context *mac_ctx,
					    bool inter_locked)
{
	struct wlan_serialization_command *cmd;
	tSmeCmd *sme_cmd;

	cmd = wlan_serialization_peek_head_pending_cmd_using_psoc(mac_ctx->psoc,
								  false);
	while (cmd) {
		if (cmd->source == WLAN_UMAC_COMP_MLME) {
			sme_cmd = cmd->umac_cmd;
			return &sme_cmd->Link;
		}
		cmd = wlan_serialization_get_pending_list_next_node_using_psoc(
						mac_ctx->psoc, cmd, false);
	}

	return NULL;
}

bool csr_nonscan_active_ll_remove_entry(struct mac_context *mac_ctx,
					tListElem *entry, bool inter_locked)
{
	tListElem *head;

	head = csr_nonscan_active_ll_peek_head(mac_ctx, inter_locked);
	if (head == entry)
	return true;

	return false;
}

tListElem *csr_nonscan_pending_ll_next(struct mac_context *mac_ctx,
				       tListElem *entry, bool inter_locked)
{
	tSmeCmd *sme_cmd;
	struct wlan_serialization_command cmd, *tcmd;

	if (!entry)
		return NULL;
	sme_cmd = GET_BASE_ADDR(entry, tSmeCmd, Link);
	cmd.cmd_id = sme_cmd->cmd_id;
	cmd.cmd_type = csr_get_cmd_type(sme_cmd);
	cmd.vdev = wlan_objmgr_get_vdev_by_id_from_psoc_no_state(
				mac_ctx->psoc,
				sme_cmd->vdev_id, WLAN_LEGACY_SME_ID);
	tcmd = wlan_serialization_get_pending_list_next_node_using_psoc(
				mac_ctx->psoc, &cmd, false);
	if (cmd.vdev)
		wlan_objmgr_vdev_release_ref(cmd.vdev, WLAN_LEGACY_SME_ID);
	while (tcmd) {
		if (tcmd->source == WLAN_UMAC_COMP_MLME) {
			sme_cmd = tcmd->umac_cmd;
			return &sme_cmd->Link;
		}
		tcmd = wlan_serialization_get_pending_list_next_node_using_psoc(
						mac_ctx->psoc, tcmd, false);
	}

	return NULL;
}

bool csr_get_bss_id_bss_desc(struct bss_description *pSirBssDesc,
			     struct qdf_mac_addr *pBssId)
{
	qdf_mem_copy(pBssId, &pSirBssDesc->bssId[0],
			sizeof(struct qdf_mac_addr));
	return true;
}

bool csr_is_bss_id_equal(struct bss_description *pSirBssDesc1,
			 struct bss_description *pSirBssDesc2)
{
	bool fEqual = false;
	struct qdf_mac_addr bssId1;
	struct qdf_mac_addr bssId2;

	do {
		if (!pSirBssDesc1)
			break;
		if (!pSirBssDesc2)
			break;

		if (!csr_get_bss_id_bss_desc(pSirBssDesc1, &bssId1))
			break;
		if (!csr_get_bss_id_bss_desc(pSirBssDesc2, &bssId2))
			break;

		fEqual = qdf_is_macaddr_equal(&bssId1, &bssId2);
	} while (0);

	return fEqual;
}

static bool csr_is_conn_state(struct mac_context *mac_ctx, uint32_t session_id,
			      eCsrConnectState state)
{
	QDF_BUG(session_id < WLAN_MAX_VDEVS);
	if (session_id >= WLAN_MAX_VDEVS)
		return false;

	return mac_ctx->roam.roamSession[session_id].connectState == state;
}

bool csr_is_conn_state_connected(struct mac_context *mac, uint32_t sessionId)
{
	/* This is temp ifdef will be removed in near future */
#ifdef FEATURE_CM_ENABLE
	return cm_is_vdevid_connected(mac->pdev, sessionId) ||
	       csr_is_conn_state_connected_wds(mac, sessionId);
#else
	return csr_is_conn_state_connected_infra(mac, sessionId) ||
	       csr_is_conn_state_connected_wds(mac, sessionId);
#endif
}

#ifndef FEATURE_CM_ENABLE
bool csr_is_conn_state_connected_infra(struct mac_context *mac_ctx,
				       uint32_t session_id)
{
	return csr_is_conn_state(mac_ctx, session_id,
				 eCSR_ASSOC_STATE_TYPE_INFRA_ASSOCIATED);
}

bool csr_is_conn_state_infra(struct mac_context *mac, uint32_t sessionId)
{
	return csr_is_conn_state_connected_infra(mac, sessionId);
}
#endif

static tSirMacCapabilityInfo csr_get_bss_capabilities(struct bss_description *
						      pSirBssDesc)
{
	tSirMacCapabilityInfo dot11Caps;

	/* tSirMacCapabilityInfo is 16-bit */
	qdf_get_u16((uint8_t *) &pSirBssDesc->capabilityInfo,
		    (uint16_t *) &dot11Caps);

	return dot11Caps;
}

bool csr_is_conn_state_connected_wds(struct mac_context *mac_ctx,
				     uint32_t session_id)
{
	return csr_is_conn_state(mac_ctx, session_id,
				 eCSR_ASSOC_STATE_TYPE_WDS_CONNECTED);
}

bool csr_is_conn_state_connected_infra_ap(struct mac_context *mac_ctx,
					  uint32_t session_id)
{
	return csr_is_conn_state(mac_ctx, session_id,
				 eCSR_ASSOC_STATE_TYPE_INFRA_CONNECTED) ||
		csr_is_conn_state(mac_ctx, session_id,
				  eCSR_ASSOC_STATE_TYPE_INFRA_DISCONNECTED);
}

bool csr_is_conn_state_disconnected_wds(struct mac_context *mac_ctx,
					uint32_t session_id)
{
	return csr_is_conn_state(mac_ctx, session_id,
				 eCSR_ASSOC_STATE_TYPE_WDS_DISCONNECTED);
}

bool csr_is_conn_state_wds(struct mac_context *mac, uint32_t sessionId)
{
	return csr_is_conn_state_connected_wds(mac, sessionId) ||
	       csr_is_conn_state_disconnected_wds(mac, sessionId);
}

enum csr_cfgdot11mode
csr_get_vdev_dot11_mode(struct mac_context *mac,
			enum QDF_OPMODE device_mode,
			enum csr_cfgdot11mode curr_dot11_mode)
{
	enum mlme_vdev_dot11_mode vdev_dot11_mode;
	uint8_t dot11_mode_indx;
	enum csr_cfgdot11mode dot11_mode = curr_dot11_mode;
	uint32_t vdev_type_dot11_mode =
				mac->mlme_cfg->dot11_mode.vdev_type_dot11_mode;

	sme_debug("curr_dot11_mode %d, vdev_dot11 %08X, dev_mode %d",
		  curr_dot11_mode, vdev_type_dot11_mode, device_mode);

	switch (device_mode) {
	case QDF_STA_MODE:
		dot11_mode_indx = STA_DOT11_MODE_INDX;
		break;
	case QDF_P2P_CLIENT_MODE:
	case QDF_P2P_DEVICE_MODE:
		dot11_mode_indx = P2P_DEV_DOT11_MODE_INDX;
		break;
	case QDF_TDLS_MODE:
		dot11_mode_indx = TDLS_DOT11_MODE_INDX;
		break;
	case QDF_NAN_DISC_MODE:
		dot11_mode_indx = NAN_DISC_DOT11_MODE_INDX;
		break;
	case QDF_NDI_MODE:
		dot11_mode_indx = NDI_DOT11_MODE_INDX;
		break;
	case QDF_OCB_MODE:
		dot11_mode_indx = OCB_DOT11_MODE_INDX;
		break;
	default:
		return dot11_mode;
	}
	vdev_dot11_mode = QDF_GET_BITS(vdev_type_dot11_mode,
				       dot11_mode_indx, 4);
	if (vdev_dot11_mode == MLME_VDEV_DOT11_MODE_AUTO)
		dot11_mode = curr_dot11_mode;

	if (CSR_IS_DOT11_MODE_11N(curr_dot11_mode) &&
	    vdev_dot11_mode == MLME_VDEV_DOT11_MODE_11N)
		dot11_mode = eCSR_CFG_DOT11_MODE_11N;

	if (CSR_IS_DOT11_MODE_11AC(curr_dot11_mode) &&
	    vdev_dot11_mode == MLME_VDEV_DOT11_MODE_11AC)
		dot11_mode = eCSR_CFG_DOT11_MODE_11AC;

	if (CSR_IS_DOT11_MODE_11AX(curr_dot11_mode) &&
	    vdev_dot11_mode == MLME_VDEV_DOT11_MODE_11AX)
		dot11_mode = eCSR_CFG_DOT11_MODE_11AX;
#ifdef WLAN_FEATURE_11BE
	if (CSR_IS_DOT11_MODE_11BE(curr_dot11_mode) &&
	    vdev_dot11_mode == MLME_VDEV_DOT11_MODE_11BE)
		dot11_mode = eCSR_CFG_DOT11_MODE_11BE;
#endif
	sme_debug("INI vdev_dot11_mode %d new dot11_mode %d",
		  vdev_dot11_mode, dot11_mode);

	return dot11_mode;
}

static bool csr_is_conn_state_ap(struct mac_context *mac, uint32_t sessionId)
{
	struct csr_roam_session *pSession;

	pSession = CSR_GET_SESSION(mac, sessionId);
	if (!pSession)
		return false;
	if (CSR_IS_INFRA_AP(&pSession->connectedProfile))
		return true;
	return false;
}

bool csr_is_any_session_in_connect_state(struct mac_context *mac)
{
	uint32_t i;

	for (i = 0; i < WLAN_MAX_VDEVS; i++) {
		if (CSR_IS_SESSION_VALID(mac, i) &&
		/* This is temp ifdef will be removed in near future */
#ifdef FEATURE_CM_ENABLE
		    (cm_is_vdevid_connected(mac->pdev, i) ||
#else
		    (csr_is_conn_state_infra(mac, i) ||
#endif
		     csr_is_conn_state_ap(mac, i))) {
			return true;
		}
	}

	return false;
}

qdf_freq_t csr_get_concurrent_operation_freq(struct mac_context *mac_ctx)
{
	uint8_t i = 0;
	qdf_freq_t freq;
	enum QDF_OPMODE op_mode;

	for (i = 0; i < WLAN_MAX_VDEVS; i++) {
		op_mode = wlan_get_opmode_from_vdev_id(mac_ctx->pdev, i);
		/* check only for STA, CLI, GO and SAP */
		if (op_mode != QDF_STA_MODE && op_mode != QDF_P2P_CLIENT_MODE &&
		    op_mode != QDF_P2P_GO_MODE && op_mode != QDF_SAP_MODE)
			continue;

		freq = wlan_get_operation_chan_freq_vdev_id(mac_ctx->pdev, i);
		if (!freq)
			continue;

		return freq;
	}

	return 0;
}

uint32_t csr_get_beaconing_concurrent_channel(struct mac_context *mac_ctx,
					     uint8_t vdev_id_to_skip)
{
	struct csr_roam_session *session = NULL;
	uint8_t i = 0;
	enum QDF_OPMODE persona;

	for (i = 0; i < WLAN_MAX_VDEVS; i++) {
		if (i == vdev_id_to_skip)
			continue;
		if (!CSR_IS_SESSION_VALID(mac_ctx, i))
			continue;
		session = CSR_GET_SESSION(mac_ctx, i);
		persona = wlan_get_opmode_from_vdev_id(mac_ctx->pdev, i);
		if (((persona == QDF_P2P_GO_MODE) ||
		     (persona == QDF_SAP_MODE)) &&
		     (session->connectState !=
		      eCSR_ASSOC_STATE_TYPE_NOT_CONNECTED))
			return wlan_get_operation_chan_freq_vdev_id(mac_ctx->pdev, i);
	}

	return 0;
}

#ifdef FEATURE_WLAN_MCC_TO_SCC_SWITCH

#define HALF_BW_OF(eCSR_bw_val) ((eCSR_bw_val)/2)

/* calculation of center channel based on V/HT BW and WIFI channel bw=5MHz) */

#define CSR_GET_HT40_PLUS_CCH(och) ((och) + 10)
#define CSR_GET_HT40_MINUS_CCH(och) ((och) - 10)

#define CSR_GET_HT80_PLUS_LL_CCH(och) ((och) + 30)
#define CSR_GET_HT80_PLUS_HL_CCH(och) ((och) + 30)
#define CSR_GET_HT80_MINUS_LH_CCH(och) ((och) - 10)
#define CSR_GET_HT80_MINUS_HH_CCH(och) ((och) - 30)

/**
 * csr_calc_chb_for_sap_phymode() - to calc channel bandwidth for sap phymode
 * @mac_ctx: pointer to mac context
 * @sap_ch: SAP operating channel
 * @sap_phymode: SAP physical mode
 * @sap_cch: concurrency channel
 * @sap_hbw: SAP half bw
 * @chb: channel bandwidth
 *
 * This routine is called to calculate channel bandwidth
 *
 * Return: none
 */
static void csr_calc_chb_for_sap_phymode(struct mac_context *mac_ctx,
		uint32_t *sap_ch, eCsrPhyMode *sap_phymode,
		uint32_t *sap_cch, uint32_t *sap_hbw, uint8_t *chb)
{
	if (*sap_phymode == eCSR_DOT11_MODE_11n ||
			*sap_phymode == eCSR_DOT11_MODE_11n_ONLY) {

		*sap_hbw = HALF_BW_OF(eCSR_BW_40MHz_VAL);
		if (*chb == PHY_DOUBLE_CHANNEL_LOW_PRIMARY)
			*sap_cch = CSR_GET_HT40_PLUS_CCH(*sap_ch);
		else if (*chb == PHY_DOUBLE_CHANNEL_HIGH_PRIMARY)
			*sap_cch = CSR_GET_HT40_MINUS_CCH(*sap_ch);

	} else if (*sap_phymode == eCSR_DOT11_MODE_11ac ||
		   *sap_phymode == eCSR_DOT11_MODE_11ac_ONLY ||
		   *sap_phymode == eCSR_DOT11_MODE_11ax ||
		   *sap_phymode == eCSR_DOT11_MODE_11ax_ONLY ||
		   CSR_IS_DOT11_PHY_MODE_11BE(*sap_phymode) ||
		   CSR_IS_DOT11_PHY_MODE_11BE_ONLY(*sap_phymode)) {
		/*11AC only 80/40/20 Mhz supported in Rome */
		if (mac_ctx->roam.configParam.nVhtChannelWidth ==
				(WNI_CFG_VHT_CHANNEL_WIDTH_80MHZ + 1)) {
			*sap_hbw = HALF_BW_OF(eCSR_BW_80MHz_VAL);
			if (*chb ==
				(PHY_QUADRUPLE_CHANNEL_20MHZ_LOW_40MHZ_LOW - 1))
				*sap_cch = CSR_GET_HT80_PLUS_LL_CCH(*sap_ch);
			else if (*chb ==
				(PHY_QUADRUPLE_CHANNEL_20MHZ_HIGH_40MHZ_LOW
				     - 1))
				*sap_cch = CSR_GET_HT80_PLUS_HL_CCH(*sap_ch);
			else if (*chb ==
				 (PHY_QUADRUPLE_CHANNEL_20MHZ_LOW_40MHZ_HIGH
				     - 1))
				*sap_cch = CSR_GET_HT80_MINUS_LH_CCH(*sap_ch);
			else if (*chb ==
				(PHY_QUADRUPLE_CHANNEL_20MHZ_HIGH_40MHZ_HIGH
				     - 1))
				*sap_cch = CSR_GET_HT80_MINUS_HH_CCH(*sap_ch);
		} else {
			*sap_hbw = HALF_BW_OF(eCSR_BW_40MHz_VAL);
			if (*chb == (PHY_QUADRUPLE_CHANNEL_20MHZ_LOW_40MHZ_LOW
					- 1))
				*sap_cch = CSR_GET_HT40_PLUS_CCH(*sap_ch);
			else if (*chb ==
				(PHY_QUADRUPLE_CHANNEL_20MHZ_HIGH_40MHZ_LOW
				     - 1))
				*sap_cch = CSR_GET_HT40_MINUS_CCH(*sap_ch);
			else if (*chb ==
				(PHY_QUADRUPLE_CHANNEL_20MHZ_LOW_40MHZ_HIGH
				     - 1))
				*sap_cch = CSR_GET_HT40_PLUS_CCH(*sap_ch);
			else if (*chb ==
				(PHY_QUADRUPLE_CHANNEL_20MHZ_HIGH_40MHZ_HIGH
				     - 1))
				*sap_cch = CSR_GET_HT40_MINUS_CCH(*sap_ch);
		}
	}
}

static eCSR_BW_Val csr_get_half_bw(enum phy_ch_width ch_width)
{
	eCSR_BW_Val hw_bw = HALF_BW_OF(eCSR_BW_20MHz_VAL);

	switch (ch_width) {
	case CH_WIDTH_40MHZ:
		hw_bw = HALF_BW_OF(eCSR_BW_40MHz_VAL);
		break;
	case CH_WIDTH_80MHZ:
		hw_bw = HALF_BW_OF(eCSR_BW_80MHz_VAL);
		break;
	case CH_WIDTH_160MHZ:
	case CH_WIDTH_80P80MHZ:
		hw_bw = HALF_BW_OF(eCSR_BW_160MHz_VAL);
		break;
	default:
		break;
	}

	return hw_bw;
}

/**
 * csr_handle_conc_chnl_overlap_for_sap_go - To handle overlap for AP+AP
 * @mac_ctx: pointer to mac context
 * @session: Current session
 * @sap_ch_freq: SAP/GO operating channel frequency
 * @sap_hbw: SAP/GO half bw
 * @sap_cfreq: SAP/GO channel frequency
 * @intf_ch_freq: concurrent SAP/GO operating channel frequency
 * @intf_hbw: concurrent SAP/GO half bw
 * @intf_cfreq: concurrent SAP/GO channel frequency
 * @op_mode: opmode
 *
 * This routine is called to check if one SAP/GO channel is overlapping with
 * other SAP/GO channel
 *
 * Return: none
 */
static void csr_handle_conc_chnl_overlap_for_sap_go(
		struct mac_context *mac_ctx,
		struct csr_roam_session *session,
		uint32_t *sap_ch_freq, uint32_t *sap_hbw, uint32_t *sap_cfreq,
		uint32_t *intf_ch_freq, uint32_t *intf_hbw,
		uint32_t *intf_cfreq, enum QDF_OPMODE op_mode)
{
	qdf_freq_t op_chan_freq;
	qdf_freq_t freq_seg_0;
	enum phy_ch_width ch_width;

	wlan_get_op_chan_freq_info_vdev_id(mac_ctx->pdev, session->vdev_id,
					   &op_chan_freq, &freq_seg_0,
					   &ch_width);
	sme_debug("op_chan_freq:%d freq_seg_0:%d ch_width:%d",
		  op_chan_freq, freq_seg_0, ch_width);
	/*
	 * if conc_custom_rule1 is defined then we don't
	 * want p2pgo to follow SAP's channel or SAP to
	 * follow P2PGO's channel.
	 */
	if (0 == mac_ctx->roam.configParam.conc_custom_rule1 &&
		0 == mac_ctx->roam.configParam.conc_custom_rule2) {
		if (*sap_ch_freq == 0) {
			*sap_ch_freq = op_chan_freq;
			*sap_cfreq = freq_seg_0;
			*sap_hbw = csr_get_half_bw(ch_width);
		} else if (*sap_ch_freq != op_chan_freq) {
			*intf_ch_freq = op_chan_freq;
			*intf_cfreq = freq_seg_0;
			*intf_hbw = csr_get_half_bw(ch_width);
		}
	} else if (*sap_ch_freq == 0 && op_mode == QDF_SAP_MODE) {
		*sap_ch_freq = op_chan_freq;
		*sap_cfreq = freq_seg_0;
		*sap_hbw = csr_get_half_bw(ch_width);
	}
}


/**
 * csr_check_concurrent_channel_overlap() - To check concurrent overlap chnls
 * @mac_ctx: Pointer to mac context
 * @sap_ch: SAP channel
 * @sap_phymode: SAP phy mode
 * @cc_switch_mode: concurrent switch mode
 *
 * This routine will be called to check concurrent overlap channels
 *
 * Return: uint16_t
 */
uint16_t csr_check_concurrent_channel_overlap(struct mac_context *mac_ctx,
			uint32_t sap_ch_freq, eCsrPhyMode sap_phymode,
			uint8_t cc_switch_mode)
{
	struct csr_roam_session *session = NULL;
	uint8_t i = 0, chb = PHY_SINGLE_CHANNEL_CENTERED;
	uint32_t intf_ch_freq = 0, sap_hbw = 0, intf_hbw = 0, intf_cfreq = 0;
	uint32_t sap_cfreq = 0;
	uint32_t sap_lfreq, sap_hfreq, intf_lfreq, intf_hfreq;
	QDF_STATUS status;
	enum QDF_OPMODE op_mode;
	enum phy_ch_width ch_width;

	if (mac_ctx->roam.configParam.cc_switch_mode ==
			QDF_MCC_TO_SCC_SWITCH_DISABLE)
		return 0;

	if (sap_ch_freq != 0) {
		sap_cfreq = sap_ch_freq;
		sap_hbw = HALF_BW_OF(eCSR_BW_20MHz_VAL);

		if (!WLAN_REG_IS_24GHZ_CH_FREQ(sap_ch_freq))
			chb = mac_ctx->roam.configParam.channelBondingMode5GHz;
		else
			chb = mac_ctx->roam.configParam.channelBondingMode24GHz;

		if (chb)
			csr_calc_chb_for_sap_phymode(mac_ctx, &sap_ch_freq,
						     &sap_phymode, &sap_cfreq,
						     &sap_hbw, &chb);
	}

	sme_debug("sap_ch:%d sap_phymode:%d sap_cch:%d sap_hbw:%d chb:%d",
		  sap_ch_freq, sap_phymode, sap_cfreq, sap_hbw, chb);

	for (i = 0; i < WLAN_MAX_VDEVS; i++) {
		if (!CSR_IS_SESSION_VALID(mac_ctx, i))
			continue;

		session = CSR_GET_SESSION(mac_ctx, i);
		op_mode = wlan_get_opmode_from_vdev_id(mac_ctx->pdev, i);
		if ((op_mode == QDF_STA_MODE ||
		     op_mode == QDF_P2P_CLIENT_MODE) &&
		/* This is temp ifdef will be removed in near future */
#ifdef FEATURE_CM_ENABLE
		    cm_is_vdevid_connected(mac_ctx->pdev, i)
#else
		    (session->connectState ==
		     eCSR_ASSOC_STATE_TYPE_INFRA_ASSOCIATED)
#endif
		    ) {
			wlan_get_op_chan_freq_info_vdev_id(mac_ctx->pdev,
					   session->vdev_id,
					   &intf_ch_freq, &intf_cfreq,
					   &ch_width);
			intf_hbw = csr_get_half_bw(ch_width);
			sme_debug("%d: intf_ch:%d intf_cfreq:%d intf_hbw:%d ch_width %d",
				  i, intf_ch_freq, intf_cfreq, intf_hbw,
				  ch_width);
		} else if ((op_mode == QDF_P2P_GO_MODE ||
			    op_mode == QDF_SAP_MODE) &&
			   (session->connectState !=
			     eCSR_ASSOC_STATE_TYPE_NOT_CONNECTED)) {

			if (session->ch_switch_in_progress)
				continue;

			csr_handle_conc_chnl_overlap_for_sap_go(mac_ctx,
					session, &sap_ch_freq, &sap_hbw,
					&sap_cfreq, &intf_ch_freq, &intf_hbw,
					&intf_cfreq, op_mode);
		}
		if (intf_ch_freq &&
		    ((intf_ch_freq <= wlan_reg_ch_to_freq(CHAN_ENUM_2484) &&
		     sap_ch_freq <= wlan_reg_ch_to_freq(CHAN_ENUM_2484)) ||
		    (intf_ch_freq > wlan_reg_ch_to_freq(CHAN_ENUM_2484) &&
		     sap_ch_freq > wlan_reg_ch_to_freq(CHAN_ENUM_2484))))
			break;
	}

	sme_debug("intf_ch:%d sap_ch:%d cc_switch_mode:%d, dbs:%d",
		  intf_ch_freq, sap_ch_freq, cc_switch_mode,
		  policy_mgr_is_hw_dbs_capable(mac_ctx->psoc));

	if (intf_ch_freq && sap_ch_freq != intf_ch_freq &&
	    !policy_mgr_is_force_scc(mac_ctx->psoc)) {
		sap_lfreq = sap_cfreq - sap_hbw;
		sap_hfreq = sap_cfreq + sap_hbw;
		intf_lfreq = intf_cfreq - intf_hbw;
		intf_hfreq = intf_cfreq + intf_hbw;

		sme_debug("SAP:  OCH: %03d CCH: %03d BW: %d LF: %d HF: %d INTF: OCH: %03d CF: %d BW: %d LF: %d HF: %d",
			sap_ch_freq, sap_cfreq, sap_hbw * 2,
			sap_lfreq, sap_hfreq, intf_ch_freq,
			intf_cfreq, intf_hbw * 2, intf_lfreq, intf_hfreq);

		if (!(((sap_lfreq > intf_lfreq && sap_lfreq < intf_hfreq) ||
			(sap_hfreq > intf_lfreq && sap_hfreq < intf_hfreq)) ||
			((intf_lfreq > sap_lfreq && intf_lfreq < sap_hfreq) ||
			(intf_hfreq > sap_lfreq && intf_hfreq < sap_hfreq))))
			intf_ch_freq = 0;
	} else if (intf_ch_freq && sap_ch_freq != intf_ch_freq &&
		   (policy_mgr_is_force_scc(mac_ctx->psoc))) {
		if (!((intf_ch_freq <= wlan_reg_ch_to_freq(CHAN_ENUM_2484) &&
		       sap_ch_freq <= wlan_reg_ch_to_freq(CHAN_ENUM_2484)) ||
		     (intf_ch_freq > wlan_reg_ch_to_freq(CHAN_ENUM_2484) &&
		      sap_ch_freq > wlan_reg_ch_to_freq(CHAN_ENUM_2484)))) {
			if (policy_mgr_is_hw_dbs_capable(mac_ctx->psoc) ||
			    cc_switch_mode ==
			    QDF_MCC_TO_SCC_WITH_PREFERRED_BAND)
				intf_ch_freq = 0;
		} else if (policy_mgr_is_hw_dbs_capable(mac_ctx->psoc) &&
			   cc_switch_mode ==
				QDF_MCC_TO_SCC_SWITCH_WITH_FAVORITE_CHANNEL) {
			status = policy_mgr_get_sap_mandatory_channel(
					mac_ctx->psoc, sap_ch_freq,
					&intf_ch_freq);
			if (QDF_IS_STATUS_ERROR(status))
				sme_err("no mandatory channels (%d, %d)",
					sap_ch_freq, intf_ch_freq);
		}
	} else if ((intf_ch_freq == sap_ch_freq) && (cc_switch_mode ==
				QDF_MCC_TO_SCC_SWITCH_WITH_FAVORITE_CHANNEL)) {
		if (WLAN_REG_IS_24GHZ_CH_FREQ(intf_ch_freq) ||
		    WLAN_REG_IS_6GHZ_CHAN_FREQ(sap_ch_freq)) {
			status =
				policy_mgr_get_sap_mandatory_channel(
					mac_ctx->psoc, sap_ch_freq,
					&intf_ch_freq);
			if (QDF_IS_STATUS_ERROR(status))
				sme_err("no mandatory channel");
		}
	}

	if (intf_ch_freq == sap_ch_freq)
		intf_ch_freq = 0;

	sme_debug("##Concurrent Channels (%d, %d) %s Interfering", sap_ch_freq,
		  intf_ch_freq,
		  intf_ch_freq == 0 ? "Not" : "Are");

	return intf_ch_freq;
}
#endif

bool csr_is_all_session_disconnected(struct mac_context *mac)
{
	uint32_t i;
	bool fRc = true;

	for (i = 0; i < WLAN_MAX_VDEVS; i++) {
		if (CSR_IS_SESSION_VALID(mac, i)
		    && !csr_is_conn_state_disconnected(mac, i)) {
			fRc = false;
			break;
		}
	}

	return fRc;
}

bool csr_is_concurrent_session_running(struct mac_context *mac)
{
	uint8_t vdev_id, noOfCocurrentSession = 0;
	bool fRc = false;

	for (vdev_id = 0; vdev_id < WLAN_MAX_VDEVS; vdev_id++) {
		if (!CSR_IS_SESSION_VALID(mac, vdev_id))
			continue;
		/* This is temp ifdef will be removed in near future */
#ifdef FEATURE_CM_ENABLE
		if (csr_is_conn_state_connected_infra_ap(mac, vdev_id) ||
		    cm_is_vdevid_connected(mac->pdev, vdev_id))
#else
		if (csr_is_conn_state_connected_infra_ap(mac, vdev_id) ||
		    csr_is_conn_state_connected_infra(mac, vdev_id))
#endif
			++noOfCocurrentSession;
	}

	/* More than one session is Up and Running */
	if (noOfCocurrentSession > 1)
		fRc = true;
	return fRc;
}

bool csr_is_infra_ap_started(struct mac_context *mac)
{
	uint32_t sessionId;
	bool fRc = false;

	for (sessionId = 0; sessionId < WLAN_MAX_VDEVS; sessionId++) {
		if (CSR_IS_SESSION_VALID(mac, sessionId) &&
				(csr_is_conn_state_connected_infra_ap(mac,
					sessionId))) {
			fRc = true;
			break;
		}
	}

	return fRc;

}

#ifdef FEATURE_CM_ENABLE
bool csr_is_conn_state_disconnected(struct mac_context *mac, uint8_t vdev_id)
{
	enum QDF_OPMODE opmode;

	opmode = wlan_get_opmode_from_vdev_id(mac->pdev, vdev_id);

	if (opmode == QDF_STA_MODE || opmode == QDF_P2P_CLIENT_MODE)
		return !cm_is_vdevid_connected(mac->pdev, vdev_id);

	return eCSR_ASSOC_STATE_TYPE_NOT_CONNECTED ==
	       mac->roam.roamSession[vdev_id].connectState;
}
#else
bool csr_is_conn_state_disconnected(struct mac_context *mac, uint8_t vdev_id)
{
	return eCSR_ASSOC_STATE_TYPE_NOT_CONNECTED ==
	       mac->roam.roamSession[vdev_id].connectState;
}
#endif

bool csr_is_infra_bss_desc(struct bss_description *pSirBssDesc)
{
	tSirMacCapabilityInfo dot11Caps = csr_get_bss_capabilities(pSirBssDesc);

	return (bool) dot11Caps.ess;
}

static bool csr_is_qos_bss_desc(struct bss_description *pSirBssDesc)
{
	tSirMacCapabilityInfo dot11Caps = csr_get_bss_capabilities(pSirBssDesc);

	return (bool) dot11Caps.qos;
}

bool csr_is11h_supported(struct mac_context *mac)
{
	return mac->mlme_cfg->gen.enabled_11h;
}

bool csr_is11e_supported(struct mac_context *mac)
{
	return mac->roam.configParam.Is11eSupportEnabled;
}

bool csr_is_wmm_supported(struct mac_context *mac)
{
	if (eCsrRoamWmmNoQos == mac->roam.configParam.WMMSupportMode)
		return false;
	else
		return true;
}

#ifndef FEATURE_CM_ENABLE
/* pIes is the IEs for pSirBssDesc2 */
bool csr_is_ssid_equal(struct mac_context *mac,
		       struct bss_description *pSirBssDesc1,
		       struct bss_description *pSirBssDesc2,
		       tDot11fBeaconIEs *pIes2)
{
	bool fEqual = false;
	tSirMacSSid Ssid1, Ssid2;
	tDot11fBeaconIEs *pIes1 = NULL;
	tDot11fBeaconIEs *pIesLocal = pIes2;

	do {
		if ((!pSirBssDesc1) || (!pSirBssDesc2))
			break;
		if (!pIesLocal
		    &&
		    !QDF_IS_STATUS_SUCCESS(csr_get_parsed_bss_description_ies
						   (mac, pSirBssDesc2,
						    &pIesLocal))) {
			sme_err("fail to parse IEs");
			break;
		}
		if (!QDF_IS_STATUS_SUCCESS
			(csr_get_parsed_bss_description_ies(mac,
				pSirBssDesc1, &pIes1))) {
			break;
		}
		if ((!pIes1->SSID.present) || (!pIesLocal->SSID.present))
			break;
		if (pIes1->SSID.num_ssid != pIesLocal->SSID.num_ssid)
			break;
		qdf_mem_copy(Ssid1.ssId, pIes1->SSID.ssid,
			     pIes1->SSID.num_ssid);
		qdf_mem_copy(Ssid2.ssId, pIesLocal->SSID.ssid,
			     pIesLocal->SSID.num_ssid);

		fEqual = (!qdf_mem_cmp(Ssid1.ssId, Ssid2.ssId,
					pIesLocal->SSID.num_ssid));

	} while (0);
	if (pIes1)
		qdf_mem_free(pIes1);
	if (pIesLocal && !pIes2)
		qdf_mem_free(pIesLocal);

	return fEqual;
}
#endif
/* pIes can be passed in as NULL if the caller doesn't have one prepared */
static bool csr_is_bss_description_wme(struct mac_context *mac,
				       struct bss_description *pSirBssDesc,
				       tDot11fBeaconIEs *pIes)
{
	/* Assume that WME is found... */
	bool fWme = true;
	tDot11fBeaconIEs *pIesTemp = pIes;

	do {
		if (!pIesTemp) {
			if (!QDF_IS_STATUS_SUCCESS
				    (csr_get_parsed_bss_description_ies
					    (mac, pSirBssDesc, &pIesTemp))) {
				fWme = false;
				break;
			}
		}
		/* if the Wme Info IE is found, then WME is supported... */
		if (CSR_IS_QOS_BSS(pIesTemp))
			break;
		/* if none of these are found, then WME is NOT supported... */
		fWme = false;
	} while (0);
	if (!csr_is_wmm_supported(mac) && fWme)
		if (!pIesTemp->HTCaps.present)
			fWme = false;

	if ((!pIes) && (pIesTemp))
		/* we allocate memory here so free it before returning */
		qdf_mem_free(pIesTemp);

	return fWme;
}

eCsrMediaAccessType
csr_get_qos_from_bss_desc(struct mac_context *mac_ctx,
			  struct bss_description *pSirBssDesc,
			  tDot11fBeaconIEs *pIes)
{
	eCsrMediaAccessType qosType = eCSR_MEDIUM_ACCESS_DCF;

	if (!pIes) {
		QDF_ASSERT(pIes);
		return qosType;
	}

	do {
		/* If we find WMM in the Bss Description, then we let this
		 * override and use WMM.
		 */
		if (csr_is_bss_description_wme(mac_ctx, pSirBssDesc, pIes))
			qosType = eCSR_MEDIUM_ACCESS_WMM_eDCF_DSCP;
		else {
			/* If the QoS bit is on, then the AP is
			 * advertising 11E QoS.
			 */
			if (csr_is_qos_bss_desc(pSirBssDesc))
				qosType = eCSR_MEDIUM_ACCESS_11e_eDCF;
			else
				qosType = eCSR_MEDIUM_ACCESS_DCF;

			/* Scale back based on the types turned on
			 * for the adapter.
			 */
			if (eCSR_MEDIUM_ACCESS_11e_eDCF == qosType
			    && !csr_is11e_supported(mac_ctx))
				qosType = eCSR_MEDIUM_ACCESS_DCF;
		}

	} while (0);

	return qosType;
}

/* Caller allocates memory for pIEStruct */
QDF_STATUS csr_parse_bss_description_ies(struct mac_context *mac_ctx,
					 struct bss_description *bss_desc,
					 tDot11fBeaconIEs *pIEStruct)
{
	return wlan_parse_bss_description_ies(mac_ctx, bss_desc, pIEStruct);
}

/* This function will allocate memory for the parsed IEs to the caller.
 * Caller must free the memory after it is done with the data only if
 * this function succeeds
 */
QDF_STATUS csr_get_parsed_bss_description_ies(struct mac_context *mac_ctx,
					      struct bss_description *bss_desc,
					      tDot11fBeaconIEs **ppIEStruct)
{
	return wlan_get_parsed_bss_description_ies(mac_ctx, bss_desc,
						   ppIEStruct);
}

bool csr_is_nullssid(uint8_t *pBssSsid, uint8_t len)
{
	bool fNullSsid = false;

	uint32_t SsidLength;
	uint8_t *pSsidStr;

	do {
		if (0 == len) {
			fNullSsid = true;
			break;
		}
		/* Consider 0 or space for hidden SSID */
		if (0 == pBssSsid[0]) {
			fNullSsid = true;
			break;
		}

		SsidLength = len;
		pSsidStr = pBssSsid;

		while (SsidLength) {
			if (*pSsidStr)
				break;

			pSsidStr++;
			SsidLength--;
		}

		if (0 == SsidLength) {
			fNullSsid = true;
			break;
		}
	} while (0);

	return fNullSsid;
}

uint32_t csr_get_frag_thresh(struct mac_context *mac_ctx)
{
	return mac_ctx->mlme_cfg->threshold.frag_threshold;
}

uint32_t csr_get_rts_thresh(struct mac_context *mac_ctx)
{
	return mac_ctx->mlme_cfg->threshold.rts_threshold;
}

static eCsrPhyMode
csr_translate_to_phy_mode_from_bss_desc(struct mac_context *mac_ctx,
					struct bss_description *pSirBssDesc,
					tDot11fBeaconIEs *ies)
{
	eCsrPhyMode phyMode;
	uint8_t i;

	switch (pSirBssDesc->nwType) {
	case eSIR_11A_NW_TYPE:
		phyMode = eCSR_DOT11_MODE_11a;
		break;

	case eSIR_11B_NW_TYPE:
		phyMode = eCSR_DOT11_MODE_11b;
		break;

	case eSIR_11G_NW_TYPE:
		phyMode = eCSR_DOT11_MODE_11g_ONLY;

		/* Check if the BSS is in b/g mixed mode or g_only mode */
		if (!ies || !ies->SuppRates.present) {
			sme_debug("Unable to get rates, assume G only mode");
			break;
		}

		for (i = 0; i < ies->SuppRates.num_rates; i++) {
			if (csr_rates_is_dot11_rate11b_supported_rate(
			    ies->SuppRates.rates[i])) {
				sme_debug("One B rate is supported");
				phyMode = eCSR_DOT11_MODE_11g;
				break;
			}
		}
		break;
	case eSIR_11N_NW_TYPE:
		phyMode = eCSR_DOT11_MODE_11n;
		break;
	case eSIR_11AX_NW_TYPE:
		phyMode = eCSR_DOT11_MODE_11ax;
		break;
#ifdef WLAN_FEATURE_11BE
	case eSIR_11BE_NW_TYPE:
		phyMode = eCSR_DOT11_MODE_11be;
		break;
#endif
	case eSIR_11AC_NW_TYPE:
	default:
		phyMode = eCSR_DOT11_MODE_11ac;
		break;
	}
	return phyMode;
}

uint32_t csr_translate_to_wni_cfg_dot11_mode(struct mac_context *mac,
					     enum csr_cfgdot11mode csrDot11Mode)
{
	uint32_t ret;

	switch (csrDot11Mode) {
	case eCSR_CFG_DOT11_MODE_AUTO:
#ifdef WLAN_FEATURE_11BE
		if (IS_FEATURE_SUPPORTED_BY_FW(DOT11BE))
			ret = MLME_DOT11_MODE_11BE;
		else
#endif
		if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AX))
			ret = MLME_DOT11_MODE_11AX;
		else if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AC))
			ret = MLME_DOT11_MODE_11AC;
		else
			ret = MLME_DOT11_MODE_11N;
		break;
	case eCSR_CFG_DOT11_MODE_11A:
		ret = MLME_DOT11_MODE_11A;
		break;
	case eCSR_CFG_DOT11_MODE_11B:
		ret = MLME_DOT11_MODE_11B;
		break;
	case eCSR_CFG_DOT11_MODE_11G:
		ret = MLME_DOT11_MODE_11G;
		break;
	case eCSR_CFG_DOT11_MODE_11N:
		ret = MLME_DOT11_MODE_11N;
		break;
	case eCSR_CFG_DOT11_MODE_11G_ONLY:
		ret = MLME_DOT11_MODE_11G_ONLY;
		break;
	case eCSR_CFG_DOT11_MODE_11N_ONLY:
		ret = MLME_DOT11_MODE_11N_ONLY;
		break;
	case eCSR_CFG_DOT11_MODE_11AC_ONLY:
		if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AC))
			ret = MLME_DOT11_MODE_11AC_ONLY;
		else
			ret = MLME_DOT11_MODE_11N;
		break;
	case eCSR_CFG_DOT11_MODE_11AC:
		if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AC))
			ret = MLME_DOT11_MODE_11AC;
		else
			ret = MLME_DOT11_MODE_11N;
		break;
	case eCSR_CFG_DOT11_MODE_11AX_ONLY:
		if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AX))
			ret = MLME_DOT11_MODE_11AX_ONLY;
		else if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AC))
			ret = MLME_DOT11_MODE_11AC;
		else
			ret = MLME_DOT11_MODE_11N;
		break;
	case eCSR_CFG_DOT11_MODE_11AX:
		if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AX))
			ret = MLME_DOT11_MODE_11AX;
		else if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AC))
			ret = MLME_DOT11_MODE_11AC;
		else
			ret = MLME_DOT11_MODE_11N;
		break;
#ifdef WLAN_FEATURE_11BE
	case eCSR_CFG_DOT11_MODE_11BE_ONLY:
		if (IS_FEATURE_SUPPORTED_BY_FW(DOT11BE))
			ret = MLME_DOT11_MODE_11BE_ONLY;
		else if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AX))
			ret = MLME_DOT11_MODE_11AX_ONLY;
		else if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AC))
			ret = MLME_DOT11_MODE_11AC;
		else
			ret = MLME_DOT11_MODE_11N;
		break;
	case eCSR_CFG_DOT11_MODE_11BE:
		if (IS_FEATURE_SUPPORTED_BY_FW(DOT11BE))
			ret = MLME_DOT11_MODE_11BE;
		else if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AX))
			ret = MLME_DOT11_MODE_11AX;
		else if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AC))
			ret = MLME_DOT11_MODE_11AC;
		else
			ret = MLME_DOT11_MODE_11N;
		break;
#endif
	default:
		sme_warn("doesn't expect %d as csrDo11Mode", csrDot11Mode);
		if (BAND_2G == mac->mlme_cfg->gen.band)
			ret = MLME_DOT11_MODE_11G;
		else
			ret = MLME_DOT11_MODE_11A;
		break;
	}

	return ret;
}

/**
 * csr_get_phy_mode_from_bss() - Get Phy Mode
 * @mac:           Global MAC context
 * @pBSSDescription: BSS Descriptor
 * @pPhyMode:        Physical Mode
 * @pIes:            Pointer to the IE fields
 *
 * This function should only return the super set of supported modes
 * 11n implies 11b/g/a/n.
 *
 * Return: success
 **/
QDF_STATUS csr_get_phy_mode_from_bss(struct mac_context *mac,
		struct bss_description *pBSSDescription,
		eCsrPhyMode *pPhyMode, tDot11fBeaconIEs *pIes)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	eCsrPhyMode phyMode =
		csr_translate_to_phy_mode_from_bss_desc(mac, pBSSDescription,
							pIes);

	if (pIes) {
		if (pIes->HTCaps.present) {
			phyMode = eCSR_DOT11_MODE_11n;
			if (IS_BSS_VHT_CAPABLE(pIes->VHTCaps) ||
				IS_BSS_VHT_CAPABLE(pIes->vendor_vht_ie.VHTCaps))
				phyMode = eCSR_DOT11_MODE_11ac;
			if (pIes->he_cap.present)
				phyMode = eCSR_DOT11_MODE_11ax;
			if (pIes->eht_cap.present)
				phyMode = eCSR_DOT11_MODE_11be;
		} else if (WLAN_REG_IS_6GHZ_CHAN_FREQ(
					pBSSDescription->chan_freq)) {
			if (pIes->eht_cap.present)
				phyMode = eCSR_DOT11_MODE_11be;
			else if (pIes->he_cap.present)
				phyMode = eCSR_DOT11_MODE_11ax;
			else
				sme_debug("Warning - 6Ghz AP no he cap");
		} else {
			if (pIes->he_cap.present)
				phyMode = eCSR_DOT11_MODE_11ax;
			if (pIes->eht_cap.present)
				phyMode = eCSR_DOT11_MODE_11be;
		}

		*pPhyMode = phyMode;
	}

	return status;
}

/**
 * csr_get_phy_mode_in_use() - to get phymode
 * @phyModeIn: physical mode
 * @bssPhyMode: physical mode in bss
 * @f5GhzBand: 5Ghz band
 * @pCfgDot11ModeToUse: dot11 mode in use
 *
 * This function returns the correct eCSR_CFG_DOT11_MODE is the two phyModes
 * matches. bssPhyMode is the mode derived from the BSS description
 * f5GhzBand is derived from the channel id of BSS description
 *
 * Return: true or false
 */
static bool csr_get_phy_mode_in_use(struct mac_context *mac_ctx,
				    eCsrPhyMode phyModeIn,
				    eCsrPhyMode bssPhyMode,
				    bool f5GhzBand,
				    enum csr_cfgdot11mode *pCfgDot11ModeToUse)
{
	bool fMatch = false;
	enum csr_cfgdot11mode cfgDot11Mode;

	cfgDot11Mode = eCSR_CFG_DOT11_MODE_11N;
	switch (phyModeIn) {
	/* 11a or 11b or 11g */
	case eCSR_DOT11_MODE_abg:
		fMatch = true;
		if (f5GhzBand)
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11A;
		else if (eCSR_DOT11_MODE_11b == bssPhyMode)
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11B;
		else
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11G;
		break;

	case eCSR_DOT11_MODE_11a:
		if (f5GhzBand) {
			fMatch = true;
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11A;
		}
		break;

	case eCSR_DOT11_MODE_11g:
		if (!f5GhzBand) {
			fMatch = true;
			if (eCSR_DOT11_MODE_11b == bssPhyMode)
				cfgDot11Mode = eCSR_CFG_DOT11_MODE_11B;
			else
				cfgDot11Mode = eCSR_CFG_DOT11_MODE_11G;
		}
		break;

	case eCSR_DOT11_MODE_11g_ONLY:
		if ((bssPhyMode == eCSR_DOT11_MODE_11g) ||
		    (bssPhyMode == eCSR_DOT11_MODE_11g_ONLY)) {
			fMatch = true;
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11G;
		}
		break;

	case eCSR_DOT11_MODE_11b:
	case eCSR_DOT11_MODE_11b_ONLY:
		if (!f5GhzBand && (bssPhyMode != eCSR_DOT11_MODE_11g_ONLY)) {
			fMatch = true;
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11B;
		}
		break;

	case eCSR_DOT11_MODE_11n:
		fMatch = true;
		switch (bssPhyMode) {
		case eCSR_DOT11_MODE_11g:
		case eCSR_DOT11_MODE_11g_ONLY:
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11G;
			break;
		case eCSR_DOT11_MODE_11b:
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11B;
			break;
		case eCSR_DOT11_MODE_11a:
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11A;
			break;
		case eCSR_DOT11_MODE_11n:
		case eCSR_DOT11_MODE_11ac:
		case eCSR_DOT11_MODE_11ax:
		case eCSR_DOT11_MODE_11be:
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11N;
			break;

		default:
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11AC;
			break;
		}
		break;

	case eCSR_DOT11_MODE_11n_ONLY:
		if (eCSR_DOT11_MODE_11n == bssPhyMode ||
			bssPhyMode >= eCSR_DOT11_MODE_11ac) {
			fMatch = true;
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11N;

		}

		break;
	case eCSR_DOT11_MODE_11ac:
		fMatch = true;
		switch (bssPhyMode) {
		case eCSR_DOT11_MODE_11g:
		case eCSR_DOT11_MODE_11g_ONLY:
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11G;
			break;
		case eCSR_DOT11_MODE_11b:
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11B;
			break;
		case eCSR_DOT11_MODE_11a:
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11A;
			break;
		case eCSR_DOT11_MODE_11n:
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11N;
			break;
		case eCSR_DOT11_MODE_11ac:
		case eCSR_DOT11_MODE_11ax:
		case eCSR_DOT11_MODE_11be:
		default:
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11AC;
			break;
		}
		break;

	case eCSR_DOT11_MODE_11ac_ONLY:
		if (eCSR_DOT11_MODE_11ac == bssPhyMode) {
			fMatch = true;
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11AC;
		}
		break;
	case eCSR_DOT11_MODE_11ax:
		fMatch = true;
		switch (bssPhyMode) {
		case eCSR_DOT11_MODE_11g:
		case eCSR_DOT11_MODE_11g_ONLY:
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11G;
			break;
		case eCSR_DOT11_MODE_11b:
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11B;
			break;
		case eCSR_DOT11_MODE_11a:
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11A;
			break;
		case eCSR_DOT11_MODE_11n:
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11N;
			break;
		case eCSR_DOT11_MODE_11ac:
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11AC;
			break;
		case eCSR_DOT11_MODE_11ax:
		case eCSR_DOT11_MODE_11be:
		default:
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11AX;
			break;
		}
		break;

	case eCSR_DOT11_MODE_11ax_ONLY:
		if (eCSR_DOT11_MODE_11ax == bssPhyMode) {
			fMatch = true;
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11AX;
		}
		break;

	case eCSR_DOT11_MODE_11be:
		fMatch = true;
		switch (bssPhyMode) {
		case eCSR_DOT11_MODE_11g:
		case eCSR_DOT11_MODE_11g_ONLY:
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11G;
			break;
		case eCSR_DOT11_MODE_11b:
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11B;
			break;
		case eCSR_DOT11_MODE_11a:
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11A;
			break;
		case eCSR_DOT11_MODE_11n:
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11N;
			break;
		case eCSR_DOT11_MODE_11ac:
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11AC;
			break;
		case eCSR_DOT11_MODE_11ax:
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11AX;
			break;
		case eCSR_DOT11_MODE_11be:
		default:
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11BE;
			break;
		}
		break;

	case eCSR_DOT11_MODE_11be_ONLY:
		if (CSR_IS_DOT11_PHY_MODE_11BE(bssPhyMode)) {
			fMatch = true;
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11BE;
		}
		break;

	default:
		fMatch = true;
		switch (bssPhyMode) {
		case eCSR_DOT11_MODE_11g:
		case eCSR_DOT11_MODE_11g_ONLY:
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11G;
			break;
		case eCSR_DOT11_MODE_11b:
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11B;
			break;
		case eCSR_DOT11_MODE_11a:
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11A;
			break;
		case eCSR_DOT11_MODE_11n:
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11N;
			break;
		case eCSR_DOT11_MODE_11ac:
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11AC;
			break;
		case eCSR_DOT11_MODE_11ax:
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11AX;
			break;
		case eCSR_DOT11_MODE_11be:
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11BE;
			break;
		default:
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_AUTO;
			break;
		}
		break;
	}

	if (fMatch && pCfgDot11ModeToUse) {
		if (CSR_IS_CFG_DOT11_PHY_MODE_11BE(cfgDot11Mode)) {
#ifdef WLAN_FEATURE_11BE
			if (IS_FEATURE_SUPPORTED_BY_FW(DOT11BE))
				*pCfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_11BE;
			else
#endif
			if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AX))
				*pCfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_11AX;
			else if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AC))
				*pCfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_11AC;
			else
				*pCfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_11N;
		} else if (cfgDot11Mode == eCSR_CFG_DOT11_MODE_11AX) {
			if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AX))
				*pCfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_11AX;
			else if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AC))
				*pCfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_11AC;
			else
				*pCfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_11N;
		} else {
			if (cfgDot11Mode == eCSR_CFG_DOT11_MODE_11AC
			    && (!IS_FEATURE_SUPPORTED_BY_FW(DOT11AC)))
				*pCfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_11N;
			else
				*pCfgDot11ModeToUse = cfgDot11Mode;
		}
	}
	return fMatch;
}

/**
 * csr_is_phy_mode_match() - to find if phy mode matches
 * @mac: pointer to mac context
 * @phyMode: physical mode
 * @pSirBssDesc: bss description
 * @pProfile: pointer to roam profile
 * @pReturnCfgDot11Mode: dot1 mode to return
 * @pIes: pointer to IEs
 *
 * This function decides whether the one of the bit of phyMode is matching the
 * mode in the BSS and allowed by the user setting
 *
 * Return: true or false based on mode that fits the criteria
 */
bool csr_is_phy_mode_match(struct mac_context *mac, uint32_t phyMode,
			   struct bss_description *pSirBssDesc,
			   struct csr_roam_profile *pProfile,
			   enum csr_cfgdot11mode *pReturnCfgDot11Mode,
			   tDot11fBeaconIEs *pIes)
{
	bool fMatch = false;
	eCsrPhyMode phyModeInBssDesc = eCSR_DOT11_MODE_AUTO;
	eCsrPhyMode phyMode2 = eCSR_DOT11_MODE_AUTO;
	enum csr_cfgdot11mode cfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_AUTO;
	uint32_t bitMask, loopCount;
	uint32_t bss_chan_freq;

	if (!pProfile) {
		sme_err("profile not found");
		return fMatch;
	}

	if (!QDF_IS_STATUS_SUCCESS(csr_get_phy_mode_from_bss(mac, pSirBssDesc,
					&phyModeInBssDesc, pIes)))
		return fMatch;

	bss_chan_freq = pSirBssDesc->chan_freq;

	if ((0 == phyMode) || (eCSR_DOT11_MODE_AUTO & phyMode)) {
		if (eCSR_CFG_DOT11_MODE_ABG ==
				mac->roam.configParam.uCfgDot11Mode) {
			phyMode = eCSR_DOT11_MODE_abg;
		} else if (eCSR_CFG_DOT11_MODE_AUTO ==
				mac->roam.configParam.uCfgDot11Mode) {
#ifdef WLAN_FEATURE_11BE
			if (IS_FEATURE_SUPPORTED_BY_FW(DOT11BE))
				phyMode = eCSR_DOT11_MODE_11be;
			else
#endif
			if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AX))
				phyMode = eCSR_DOT11_MODE_11ax;
			else if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AC))
				phyMode = eCSR_DOT11_MODE_11ac;
			else
				phyMode = eCSR_DOT11_MODE_11n;
		} else {
			/* user's pick */
			phyMode = mac->roam.configParam.phyMode;
		}
	}

	if ((0 == phyMode) || (eCSR_DOT11_MODE_AUTO & phyMode)) {
		if (0 != phyMode) {
			if (eCSR_DOT11_MODE_AUTO & phyMode) {
				phyMode2 =
					eCSR_DOT11_MODE_AUTO & phyMode;
			}
		} else {
			phyMode2 = phyMode;
		}
		fMatch = csr_get_phy_mode_in_use(mac, phyMode2,
						 phyModeInBssDesc,
						 !WLAN_REG_IS_24GHZ_CH_FREQ
							(bss_chan_freq),
						 &cfgDot11ModeToUse);
	} else {
		bitMask = 1;
		loopCount = 0;
		while (loopCount < eCSR_NUM_PHY_MODE) {
			phyMode2 = (phyMode & (bitMask << loopCount++));
			if (0 != phyMode2 &&
			    csr_get_phy_mode_in_use(mac, phyMode2,
			    phyModeInBssDesc,
			    !WLAN_REG_IS_24GHZ_CH_FREQ(bss_chan_freq),
			    &cfgDot11ModeToUse)) {
				fMatch = true;
				break;
			}
		}
	}

	cfgDot11ModeToUse = csr_get_vdev_dot11_mode(mac, pProfile->csrPersona,
						    cfgDot11ModeToUse);
	if (fMatch && pReturnCfgDot11Mode) {
		/*
		 * IEEE 11n spec (8.4.3): HT STA shall
		 * eliminate TKIP as a choice for the pairwise
		 * cipher suite if CCMP is advertised by the AP
		 * or if the AP included an HT capabilities
		 * element in its Beacons and Probe Response.
		 */
		if ((!CSR_IS_11n_ALLOWED(
				pProfile->negotiatedUCEncryptionType))
				&& ((eCSR_CFG_DOT11_MODE_11N ==
					cfgDot11ModeToUse) ||
				(eCSR_CFG_DOT11_MODE_11AC ==
					cfgDot11ModeToUse) ||
				(eCSR_CFG_DOT11_MODE_11AX ==
					cfgDot11ModeToUse) ||
				CSR_IS_CFG_DOT11_PHY_MODE_11BE(
					cfgDot11ModeToUse))) {
			/* We cannot do 11n here */
			if (WLAN_REG_IS_24GHZ_CH_FREQ(bss_chan_freq)) {
				cfgDot11ModeToUse =
					eCSR_CFG_DOT11_MODE_11G;
			} else {
				cfgDot11ModeToUse =
					eCSR_CFG_DOT11_MODE_11A;
			}
		}
		*pReturnCfgDot11Mode = cfgDot11ModeToUse;
	}

	return fMatch;
}

enum csr_cfgdot11mode csr_find_best_phy_mode(struct mac_context *mac,
			uint32_t phyMode)
{
	enum csr_cfgdot11mode cfgDot11ModeToUse;
	enum band_info band = mac->mlme_cfg->gen.band;

	if ((0 == phyMode) ||
	    (eCSR_DOT11_MODE_AUTO & phyMode) ||
	    (eCSR_DOT11_MODE_11be & phyMode)) {
#ifdef WLAN_FEATURE_11BE
		if (IS_FEATURE_SUPPORTED_BY_FW(DOT11BE)) {
			cfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_11BE;
		} else
#endif
		if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AX)) {
			cfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_11AX;
		} else if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AC)) {
			cfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_11AC;
		} else {
			/* Default to 11N mode if user has configured 11ac mode
			 * and FW doesn't supports 11ac mode .
			 */
			cfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_11N;
		}
	} else if (eCSR_DOT11_MODE_11ax & phyMode) {
		if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AX)) {
			cfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_11AX;
		} else if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AC)) {
			cfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_11AC;
		} else {
			/* Default to 11N mode if user has configured 11ac mode
			 * and FW doesn't supports 11ac mode .
			 */
			cfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_11N;
		}
	} else if (eCSR_DOT11_MODE_11ac & phyMode) {
		if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AC)) {
			cfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_11AC;
		} else {
			/* Default to 11N mode if user has configured 11ac mode
			 * and FW doesn't supports 11ac mode .
			 */
		}	cfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_11N;
	} else {
		if ((eCSR_DOT11_MODE_11n | eCSR_DOT11_MODE_11n_ONLY) & phyMode)
			cfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_11N;
		else if (eCSR_DOT11_MODE_abg & phyMode) {
			if (BAND_2G != band)
				cfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_11A;
			else
				cfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_11G;
		} else if (eCSR_DOT11_MODE_11a & phyMode)
			cfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_11A;
		else if ((eCSR_DOT11_MODE_11g | eCSR_DOT11_MODE_11g_ONLY) &
			   phyMode)
			cfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_11G;
		else
			cfgDot11ModeToUse = eCSR_CFG_DOT11_MODE_11B;
	}

	return cfgDot11ModeToUse;
}

enum reg_phymode csr_convert_to_reg_phy_mode(eCsrPhyMode csr_phy_mode,
				       qdf_freq_t freq)
{
	if (csr_phy_mode == eCSR_DOT11_MODE_AUTO)
		return REG_PHYMODE_MAX - 1;
#ifdef WLAN_FEATURE_11BE
	else if (CSR_IS_DOT11_PHY_MODE_11BE(csr_phy_mode) ||
		 CSR_IS_DOT11_PHY_MODE_11BE_ONLY(csr_phy_mode))
		return REG_PHYMODE_11BE;
#endif
	else if (csr_phy_mode == eCSR_DOT11_MODE_11ax ||
		 csr_phy_mode == eCSR_DOT11_MODE_11ax_ONLY)
		return REG_PHYMODE_11AX;
	else if (csr_phy_mode == eCSR_DOT11_MODE_11ac ||
		 csr_phy_mode == eCSR_DOT11_MODE_11ac_ONLY)
		return REG_PHYMODE_11AC;
	else if (csr_phy_mode == eCSR_DOT11_MODE_11n ||
		 csr_phy_mode == eCSR_DOT11_MODE_11n_ONLY)
		return REG_PHYMODE_11N;
	else if (csr_phy_mode == eCSR_DOT11_MODE_11a)
		return REG_PHYMODE_11A;
	else if (csr_phy_mode == eCSR_DOT11_MODE_11g ||
		 csr_phy_mode == eCSR_DOT11_MODE_11g_ONLY)
		return REG_PHYMODE_11G;
	else if (csr_phy_mode == eCSR_DOT11_MODE_11b ||
		 csr_phy_mode == eCSR_DOT11_MODE_11b_ONLY)
		return REG_PHYMODE_11B;
	else if (csr_phy_mode == eCSR_DOT11_MODE_abg) {
		if (WLAN_REG_IS_24GHZ_CH_FREQ(freq))
			return REG_PHYMODE_11G;
		else
			return REG_PHYMODE_11A;
	} else {
		sme_err("Invalid eCsrPhyMode");
		return REG_PHYMODE_INVALID;
	}
}

eCsrPhyMode csr_convert_from_reg_phy_mode(enum reg_phymode phymode)
{
	switch (phymode) {
	case REG_PHYMODE_INVALID:
		return eCSR_DOT11_MODE_AUTO;
	case REG_PHYMODE_11B:
		return eCSR_DOT11_MODE_11b;
	case REG_PHYMODE_11G:
		return eCSR_DOT11_MODE_11g;
	case REG_PHYMODE_11A:
		return eCSR_DOT11_MODE_11a;
	case REG_PHYMODE_11N:
		return eCSR_DOT11_MODE_11n;
	case REG_PHYMODE_11AC:
		return eCSR_DOT11_MODE_11ac;
	case REG_PHYMODE_11AX:
		return eCSR_DOT11_MODE_11ax;
#ifdef WLAN_FEATURE_11BE
	case REG_PHYMODE_11BE:
		return eCSR_DOT11_MODE_11be;
#endif
	case REG_PHYMODE_MAX:
		return eCSR_DOT11_MODE_AUTO;
	default:
		return eCSR_DOT11_MODE_AUTO;
	}
}

#ifndef FEATURE_CM_ENABLE
bool csr_is_profile_wpa(struct csr_roam_profile *pProfile)
{
	bool fWpaProfile = false;

	switch (pProfile->negotiatedAuthType) {
	case eCSR_AUTH_TYPE_WPA:
	case eCSR_AUTH_TYPE_WPA_PSK:
	case eCSR_AUTH_TYPE_WPA_NONE:
#ifdef FEATURE_WLAN_ESE
	case eCSR_AUTH_TYPE_CCKM_WPA:
#endif
		fWpaProfile = true;
		break;

	default:
		fWpaProfile = false;
		break;
	}

	if (fWpaProfile) {
		switch (pProfile->negotiatedUCEncryptionType) {
		case eCSR_ENCRYPT_TYPE_WEP40:
		case eCSR_ENCRYPT_TYPE_WEP104:
		case eCSR_ENCRYPT_TYPE_TKIP:
		case eCSR_ENCRYPT_TYPE_AES:
			fWpaProfile = true;
			break;

		default:
			fWpaProfile = false;
			break;
		}
	}
	return fWpaProfile;
}

bool csr_is_profile_rsn(struct csr_roam_profile *pProfile)
{
	bool fRSNProfile = false;

	switch (pProfile->negotiatedAuthType) {
	case eCSR_AUTH_TYPE_RSN:
	case eCSR_AUTH_TYPE_RSN_PSK:
	case eCSR_AUTH_TYPE_FT_RSN:
	case eCSR_AUTH_TYPE_FT_RSN_PSK:
#ifdef FEATURE_WLAN_ESE
	case eCSR_AUTH_TYPE_CCKM_RSN:
#endif
	case eCSR_AUTH_TYPE_RSN_PSK_SHA256:
	case eCSR_AUTH_TYPE_RSN_8021X_SHA256:
	/* fallthrough */
	case eCSR_AUTH_TYPE_FILS_SHA256:
	case eCSR_AUTH_TYPE_FILS_SHA384:
	case eCSR_AUTH_TYPE_FT_FILS_SHA256:
	case eCSR_AUTH_TYPE_FT_FILS_SHA384:
	case eCSR_AUTH_TYPE_DPP_RSN:
	case eCSR_AUTH_TYPE_OSEN:
		fRSNProfile = true;
		break;

	case eCSR_AUTH_TYPE_OWE:
	case eCSR_AUTH_TYPE_SUITEB_EAP_SHA256:
	case eCSR_AUTH_TYPE_SUITEB_EAP_SHA384:
	case eCSR_AUTH_TYPE_FT_SUITEB_EAP_SHA384:
		fRSNProfile = true;
		break;
	case eCSR_AUTH_TYPE_SAE:
	case eCSR_AUTH_TYPE_FT_SAE:
		fRSNProfile = true;
		break;

	default:
		fRSNProfile = false;
		break;
	}

	if (fRSNProfile) {
		switch (pProfile->negotiatedUCEncryptionType) {
		/* !!REVIEW - For WPA2, use of RSN IE mandates */
		/* use of AES as encryption. Here, we qualify */
		/* even if encryption type is WEP or TKIP */
		case eCSR_ENCRYPT_TYPE_WEP40:
		case eCSR_ENCRYPT_TYPE_WEP104:
		case eCSR_ENCRYPT_TYPE_TKIP:
		case eCSR_ENCRYPT_TYPE_AES:
		case eCSR_ENCRYPT_TYPE_AES_GCMP:
		case eCSR_ENCRYPT_TYPE_AES_GCMP_256:
			fRSNProfile = true;
			break;

		default:
			fRSNProfile = false;
			break;
		}
	}
	return fRSNProfile;
}

#ifdef FEATURE_WLAN_ESE
/* Function to return true if the profile is ESE */
bool csr_is_profile_ese(struct csr_roam_profile *pProfile)
{
	return csr_is_auth_type_ese(pProfile->negotiatedAuthType);
}
#endif

#endif

bool csr_is_auth_type_ese(enum csr_akm_type AuthType)
{
	switch (AuthType) {
	case eCSR_AUTH_TYPE_CCKM_WPA:
	case eCSR_AUTH_TYPE_CCKM_RSN:
		return true;
	default:
		break;
	}
	return false;
}

bool csr_is_pmkid_found_for_peer(struct mac_context *mac,
				 struct csr_roam_session *session,
				 tSirMacAddr peer_mac_addr,
				 uint8_t *pmkid,
				 uint16_t pmkid_count)
{
	uint32_t i;
	uint8_t *session_pmkid;
	struct wlan_crypto_pmksa *pmkid_cache;

	pmkid_cache = qdf_mem_malloc(sizeof(*pmkid_cache));
	if (!pmkid_cache)
		return false;

	qdf_mem_copy(pmkid_cache->bssid.bytes, peer_mac_addr,
		     QDF_MAC_ADDR_SIZE);

	if (!cm_lookup_pmkid_using_bssid(mac->psoc, session->vdev_id,
					 pmkid_cache)) {
		qdf_mem_free(pmkid_cache);
		return false;
	}

	session_pmkid = pmkid_cache->pmkid;
	for (i = 0; i < pmkid_count; i++) {
		if (!qdf_mem_cmp(pmkid + (i * PMKID_LEN),
				 session_pmkid, PMKID_LEN)) {
			qdf_mem_free(pmkid_cache);
			return true;
		}
	}

	sme_debug("PMKID in PmkidCacheInfo doesn't match with PMKIDs of peer");
	qdf_mem_free(pmkid_cache);

	return false;
}

#ifndef FEATURE_CM_ENABLE
#ifdef FEATURE_WLAN_WAPI
bool csr_is_profile_wapi(struct csr_roam_profile *pProfile)
{
	bool fWapiProfile = false;

	switch (pProfile->negotiatedAuthType) {
	case eCSR_AUTH_TYPE_WAPI_WAI_CERTIFICATE:
	case eCSR_AUTH_TYPE_WAPI_WAI_PSK:
		fWapiProfile = true;
		break;

	default:
		fWapiProfile = false;
		break;
	}

	if (fWapiProfile) {
		switch (pProfile->negotiatedUCEncryptionType) {
		case eCSR_ENCRYPT_TYPE_WPI:
			fWapiProfile = true;
			break;

		default:
			fWapiProfile = false;
			break;
		}
	}
	return fWapiProfile;
}
#endif /* FEATURE_WLAN_WAPI */
bool csr_lookup_fils_pmkid(struct mac_context *mac,
			   uint8_t vdev_id, uint8_t *cache_id,
			   uint8_t *ssid, uint8_t ssid_len,
			   struct qdf_mac_addr *bssid)
{
	struct wlan_crypto_pmksa *fils_ssid_pmksa, *bssid_lookup_pmksa;
	struct wlan_objmgr_vdev *vdev;

	vdev = wlan_objmgr_get_vdev_by_id_from_psoc(mac->psoc, vdev_id,
						    WLAN_LEGACY_SME_ID);
	if (!vdev) {
		sme_err("Invalid vdev");
		return false;
	}

	bssid_lookup_pmksa = wlan_crypto_get_pmksa(vdev, bssid);
	fils_ssid_pmksa =
		wlan_crypto_get_fils_pmksa(vdev, cache_id, ssid, ssid_len);
	if (!fils_ssid_pmksa && !bssid_lookup_pmksa) {
		sme_err("FILS_PMKSA: Lookup failed");
		wlan_objmgr_vdev_release_ref(vdev, WLAN_LEGACY_SME_ID);
		return false;
	}
	wlan_objmgr_vdev_release_ref(vdev, WLAN_LEGACY_SME_ID);

	return true;
}

#ifdef WLAN_FEATURE_FILS_SK
/**
 * csr_update_pmksa_to_profile() - update pmk and pmkid to profile which will be
 * used in case of fils session
 * @profile: profile
 * @pmkid_cache: pmksa cache
 *
 * Return: None
 */
static inline void csr_update_pmksa_to_profile(struct wlan_objmgr_vdev *vdev,
					       struct wlan_crypto_pmksa *pmksa)
{
	struct mlme_legacy_priv *mlme_priv;

	mlme_priv = wlan_vdev_mlme_get_ext_hdl(vdev);
	if (!mlme_priv) {
		mlme_err("vdev legacy private object is NULL");
		return;
	}
	if (!mlme_priv->connect_info.fils_con_info)
		return;
	mlme_priv->connect_info.fils_con_info->pmk_len = pmksa->pmk_len;
	qdf_mem_copy(mlme_priv->connect_info.fils_con_info->pmk,
		     pmksa->pmk, pmksa->pmk_len);
	qdf_mem_copy(mlme_priv->connect_info.fils_con_info->pmkid,
		     pmksa->pmkid, PMKID_LEN);
}
#else
static inline void csr_update_pmksa_to_profile(struct wlan_objmgr_vdev *vdev,
					       struct wlan_crypto_pmksa *pmksa)
{
}
#endif

uint8_t csr_construct_rsn_ie(struct mac_context *mac, uint32_t sessionId,
			     struct csr_roam_profile *pProfile,
			     struct bss_description *pSirBssDesc,
			     tDot11fBeaconIEs *ap_ie, tCsrRSNIe *pRSNIe)
{
	struct wlan_objmgr_vdev *vdev;
	uint8_t *rsn_ie_end = NULL;
	uint8_t *rsn_ie = (uint8_t *)pRSNIe;
	uint8_t ie_len = 0;
	tDot11fBeaconIEs *local_ap_ie = ap_ie;
	uint16_t rsn_cap = 0, self_rsn_cap;
	int32_t rsn_val;
	struct wlan_crypto_pmksa pmksa, *pmksa_peer;

	if (!local_ap_ie &&
	    (!QDF_IS_STATUS_SUCCESS(csr_get_parsed_bss_description_ies
	     (mac, pSirBssDesc, &local_ap_ie))))
		return ie_len;

	/* get AP RSN cap */
	qdf_mem_copy(&rsn_cap, local_ap_ie->RSN.RSN_Cap, sizeof(rsn_cap));
	if (!ap_ie && local_ap_ie)
		/* locally allocated */
		qdf_mem_free(local_ap_ie);

	vdev = wlan_objmgr_get_vdev_by_id_from_psoc(mac->psoc, sessionId,
						    WLAN_LEGACY_SME_ID);
	if (!vdev) {
		sme_err("Invalid vdev");
		return ie_len;
	}

	rsn_val = wlan_crypto_get_param(vdev, WLAN_CRYPTO_PARAM_RSN_CAP);
	if (rsn_val < 0) {
		sme_err("Invalid mgmt cipher");
		wlan_objmgr_vdev_release_ref(vdev, WLAN_LEGACY_SME_ID);
		return ie_len;
	}
	self_rsn_cap = (uint16_t)rsn_val;

	/* If AP is capable then use self capability else set PMF as 0 */
	if (rsn_cap & WLAN_CRYPTO_RSN_CAP_MFP_ENABLED &&
	    pProfile->MFPCapable) {
		self_rsn_cap |= WLAN_CRYPTO_RSN_CAP_MFP_ENABLED;
		if (pProfile->MFPRequired)
			self_rsn_cap |= WLAN_CRYPTO_RSN_CAP_MFP_REQUIRED;
		if (!(rsn_cap & WLAN_CRYPTO_RSN_CAP_OCV_SUPPORTED))
			self_rsn_cap &= ~WLAN_CRYPTO_RSN_CAP_OCV_SUPPORTED;
	} else {
		self_rsn_cap &= ~WLAN_CRYPTO_RSN_CAP_MFP_ENABLED;
		self_rsn_cap &= ~WLAN_CRYPTO_RSN_CAP_MFP_REQUIRED;
		self_rsn_cap &= ~WLAN_CRYPTO_RSN_CAP_OCV_SUPPORTED;
	}
	wlan_crypto_set_vdev_param(vdev, WLAN_CRYPTO_PARAM_RSN_CAP,
				   self_rsn_cap);
	qdf_mem_zero(&pmksa, sizeof(pmksa));
	if (pSirBssDesc->fils_info_element.is_cache_id_present) {
		pmksa.ssid_len =
			pProfile->SSIDs.SSIDList[0].SSID.length;
		qdf_mem_copy(pmksa.ssid,
			     pProfile->SSIDs.SSIDList[0].SSID.ssId,
			     pProfile->SSIDs.SSIDList[0].SSID.length);
		qdf_mem_copy(pmksa.cache_id,
			     pSirBssDesc->fils_info_element.cache_id,
			     CACHE_ID_LEN);
		qdf_mem_copy(&pmksa.bssid,
			     pSirBssDesc->bssId, QDF_MAC_ADDR_SIZE);
	} else {
		qdf_mem_copy(&pmksa.bssid,
			     pSirBssDesc->bssId, QDF_MAC_ADDR_SIZE);
	}
	pmksa_peer = wlan_crypto_get_peer_pmksa(vdev, &pmksa);
	qdf_mem_zero(&pmksa, sizeof(pmksa));
	/*
	 * TODO: Add support for Adaptive 11r connection after
	 * call to csr_get_rsn_information is added here
	 */
	rsn_ie_end = wlan_crypto_build_rsnie_with_pmksa(vdev, rsn_ie,
							pmksa_peer);
	if (rsn_ie_end)
		ie_len = rsn_ie_end - rsn_ie;

	/*
	 * If a PMK cache is found for the BSSID, then
	 * update the PMK in CSR session also as this
	 * will be sent to the FW during RSO.
	 */
	if (pmksa_peer) {
		wlan_cm_set_psk_pmk(mac->pdev, sessionId,
				    pmksa_peer->pmk, pmksa_peer->pmk_len);
		csr_update_pmksa_to_profile(vdev, pmksa_peer);
	}

	wlan_objmgr_vdev_release_ref(vdev, WLAN_LEGACY_SME_ID);

	return ie_len;
}

uint8_t csr_construct_wpa_ie(struct mac_context *mac, uint8_t session_id,
			     struct csr_roam_profile *pProfile,
			     struct bss_description *pSirBssDesc,
			     tDot11fBeaconIEs *pIes, tCsrWpaIe *pWpaIe)
{
	struct wlan_objmgr_vdev *vdev;
	uint8_t *wpa_ie_end = NULL;
	uint8_t *wpa_ie = (uint8_t *)pWpaIe;
	uint8_t ie_len = 0;

	vdev = wlan_objmgr_get_vdev_by_id_from_psoc(mac->psoc, session_id,
						    WLAN_LEGACY_SME_ID);
	if (!vdev) {
		sme_err("Invalid vdev");
		return ie_len;
	}
	wpa_ie_end = wlan_crypto_build_wpaie(vdev, wpa_ie);
	if (wpa_ie_end)
		ie_len = wpa_ie_end - wpa_ie;

	wlan_objmgr_vdev_release_ref(vdev, WLAN_LEGACY_SME_ID);

	return ie_len;
}

#ifdef FEATURE_WLAN_WAPI
uint8_t csr_construct_wapi_ie(struct mac_context *mac, uint32_t sessionId,
			      struct csr_roam_profile *pProfile,
			      struct bss_description *pSirBssDesc,
			      tDot11fBeaconIEs *pIes, tCsrWapiIe *pWapiIe)
{
	struct wlan_objmgr_vdev *vdev;
	uint8_t *wapi_ie_end = NULL;
	uint8_t *wapi_ie = (uint8_t *)pWapiIe;
	uint8_t ie_len = 0;

	vdev = wlan_objmgr_get_vdev_by_id_from_psoc(mac->psoc, sessionId,
						    WLAN_LEGACY_SME_ID);
	if (!vdev) {
		sme_err("Invalid vdev");
		return ie_len;
	}
	wapi_ie_end = wlan_crypto_build_wapiie(vdev, wapi_ie);
	if (wapi_ie_end)
		ie_len = wapi_ie_end - wapi_ie;

	wlan_objmgr_vdev_release_ref(vdev, WLAN_LEGACY_SME_ID);

	return ie_len;
}
#endif

/* If a WPAIE exists in the profile, just use it. Or else construct
 * one from the BSS Caller allocated memory for pWpaIe and guarrantee
 * it can contain a max length WPA IE
 */
uint8_t csr_retrieve_wpa_ie(struct mac_context *mac, uint8_t session_id,
			    struct csr_roam_profile *pProfile,
			    struct bss_description *pSirBssDesc,
			    tDot11fBeaconIEs *pIes, tCsrWpaIe *pWpaIe)
{
	uint8_t cbWpaIe = 0;

	do {
		if (!csr_is_profile_wpa(pProfile))
			break;
		if (pProfile->nWPAReqIELength && pProfile->pWPAReqIE) {
			if (pProfile->nWPAReqIELength <=
					DOT11F_IE_RSN_MAX_LEN) {
				cbWpaIe = (uint8_t) pProfile->nWPAReqIELength;
				qdf_mem_copy(pWpaIe, pProfile->pWPAReqIE,
					     cbWpaIe);
			} else {
				sme_warn("Invalid WPA IE length %d",
					 pProfile->nWPAReqIELength);
			}
			break;
		}
		cbWpaIe = csr_construct_wpa_ie(mac, session_id, pProfile,
					       pSirBssDesc, pIes, pWpaIe);
	} while (0);

	return cbWpaIe;
}

/* If a RSNIE exists in the profile, just use it. Or else construct
 * one from the BSS Caller allocated memory for pWpaIe and guarrantee
 * it can contain a max length WPA IE
 */
uint8_t csr_retrieve_rsn_ie(struct mac_context *mac, uint32_t sessionId,
			    struct csr_roam_profile *pProfile,
			    struct bss_description *pSirBssDesc,
			    tDot11fBeaconIEs *pIes, tCsrRSNIe *pRsnIe)
{
	uint8_t cbRsnIe = 0;

	do {
		if (!csr_is_profile_rsn(pProfile))
			break;
		/* copy RSNIE from user as it is if test mode is enabled */
		if (pProfile->force_rsne_override &&
		    pProfile->nRSNReqIELength && pProfile->pRSNReqIE) {
			sme_debug("force_rsne_override, copy RSN IE provided by user");
			if (pProfile->nRSNReqIELength <=
					DOT11F_IE_RSN_MAX_LEN) {
				cbRsnIe = (uint8_t) pProfile->nRSNReqIELength;
				qdf_mem_copy(pRsnIe, pProfile->pRSNReqIE,
					     cbRsnIe);
			} else {
				sme_warn("Invalid RSN IE length: %d",
					 pProfile->nRSNReqIELength);
			}
			break;
		}
		cbRsnIe = csr_construct_rsn_ie(mac, sessionId, pProfile,
					       pSirBssDesc, pIes, pRsnIe);
	} while (0);

	return cbRsnIe;
}

#ifdef FEATURE_WLAN_WAPI
/* If a WAPI IE exists in the profile, just use it. Or else construct
 * one from the BSS Caller allocated memory for pWapiIe and guarrantee
 * it can contain a max length WAPI IE
 */
uint8_t csr_retrieve_wapi_ie(struct mac_context *mac, uint32_t sessionId,
			     struct csr_roam_profile *pProfile,
			     struct bss_description *pSirBssDesc,
			     tDot11fBeaconIEs *pIes, tCsrWapiIe *pWapiIe)
{
	uint8_t cbWapiIe = 0;

	do {
		if (!csr_is_profile_wapi(pProfile))
			break;
		if (pProfile->nWAPIReqIELength && pProfile->pWAPIReqIE) {
			if (DOT11F_IE_WAPI_MAX_LEN >=
			    pProfile->nWAPIReqIELength) {
				cbWapiIe = (uint8_t) pProfile->nWAPIReqIELength;
				qdf_mem_copy(pWapiIe, pProfile->pWAPIReqIE,
					     cbWapiIe);
			} else {
				sme_warn("Invalid WAPI IE length %d",
					 pProfile->nWAPIReqIELength);
			}
			break;
		}
		cbWapiIe = csr_construct_wapi_ie(mac, sessionId,
						 pProfile, pSirBssDesc,
						 pIes, pWapiIe);
	} while (0);

	return cbWapiIe;
}
#endif /* FEATURE_WLAN_WAPI */
#endif

bool csr_rates_is_dot11_rate11b_supported_rate(uint8_t dot11Rate)
{
	bool fSupported = false;
	uint16_t nonBasicRate =
		(uint16_t) (BITS_OFF(dot11Rate, CSR_DOT11_BASIC_RATE_MASK));

	switch (nonBasicRate) {
	case SUPP_RATE_1_MBPS:
	case SUPP_RATE_2_MBPS:
	case SUPP_RATE_5_MBPS:
	case SUPP_RATE_11_MBPS:
		fSupported = true;
		break;

	default:
		break;
	}

	return fSupported;
}

bool csr_rates_is_dot11_rate11a_supported_rate(uint8_t dot11Rate)
{
	bool fSupported = false;
	uint16_t nonBasicRate =
		(uint16_t) (BITS_OFF(dot11Rate, CSR_DOT11_BASIC_RATE_MASK));

	switch (nonBasicRate) {
	case SUPP_RATE_6_MBPS:
	case SUPP_RATE_9_MBPS:
	case SUPP_RATE_12_MBPS:
	case SUPP_RATE_18_MBPS:
	case SUPP_RATE_24_MBPS:
	case SUPP_RATE_36_MBPS:
	case SUPP_RATE_48_MBPS:
	case SUPP_RATE_54_MBPS:
		fSupported = true;
		break;

	default:
		break;
	}

	return fSupported;
}

tAniEdType csr_translate_encrypt_type_to_ed_type(eCsrEncryptionType EncryptType)
{
	tAniEdType edType;

	switch (EncryptType) {
	default:
	case eCSR_ENCRYPT_TYPE_NONE:
		edType = eSIR_ED_NONE;
		break;

	case eCSR_ENCRYPT_TYPE_WEP40_STATICKEY:
	case eCSR_ENCRYPT_TYPE_WEP40:
		edType = eSIR_ED_WEP40;
		break;

	case eCSR_ENCRYPT_TYPE_WEP104_STATICKEY:
	case eCSR_ENCRYPT_TYPE_WEP104:
		edType = eSIR_ED_WEP104;
		break;

	case eCSR_ENCRYPT_TYPE_TKIP:
		edType = eSIR_ED_TKIP;
		break;

	case eCSR_ENCRYPT_TYPE_AES:
		edType = eSIR_ED_CCMP;
		break;
#ifdef FEATURE_WLAN_WAPI
	case eCSR_ENCRYPT_TYPE_WPI:
		edType = eSIR_ED_WPI;
		break;
#endif
	/* 11w BIP */
	case eCSR_ENCRYPT_TYPE_AES_CMAC:
		edType = eSIR_ED_AES_128_CMAC;
		break;
	case eCSR_ENCRYPT_TYPE_AES_GCMP:
		edType = eSIR_ED_GCMP;
		break;
	case eCSR_ENCRYPT_TYPE_AES_GCMP_256:
		edType = eSIR_ED_GCMP_256;
		break;
	case eCSR_ENCRYPT_TYPE_AES_GMAC_128:
		edType = eSIR_ED_AES_GMAC_128;
		break;
	case eCSR_ENCRYPT_TYPE_AES_GMAC_256:
		edType = eSIR_ED_AES_GMAC_256;
		break;
	}

	return edType;
}

bool csr_is_ssid_match(struct mac_context *mac, uint8_t *ssid1, uint8_t ssid1Len,
		       uint8_t *bssSsid, uint8_t bssSsidLen, bool fSsidRequired)
{
	bool fMatch = false;

	do {
		/*
		 * Check for the specification of the Broadcast SSID at the
		 * beginning of the list. If specified, then all SSIDs are
		 * matches (broadcast SSID means accept all SSIDs).
		 */
		if (ssid1Len == 0) {
			fMatch = true;
			break;
		}

		/* There are a few special cases.  If the Bss description has
		 * a Broadcast SSID, then our Profile must have a single SSID
		 * without Wildcards so we can program the SSID.
		 *
		 * SSID could be suppressed in beacons. In that case SSID IE
		 * has valid length but the SSID value is all NULL characters.
		 * That condition is trated same as NULL SSID
		 */
		if (csr_is_nullssid(bssSsid, bssSsidLen)) {
			if (false == fSsidRequired) {
				fMatch = true;
				break;
			}
		}

		if (ssid1Len != bssSsidLen)
			break;
		if (!qdf_mem_cmp(bssSsid, ssid1, bssSsidLen)) {
			fMatch = true;
			break;
		}

	} while (0);

	return fMatch;
}

#ifndef FEATURE_CM_ENABLE
/* Null ssid means match */
bool csr_is_ssid_in_list(tSirMacSSid *pSsid, tCsrSSIDs *pSsidList)
{
	bool fMatch = false;
	uint32_t i;

	if (pSsidList && pSsid) {
		for (i = 0; i < pSsidList->numOfSSIDs; i++) {
			if (csr_is_nullssid
				    (pSsidList->SSIDList[i].SSID.ssId,
				    pSsidList->SSIDList[i].SSID.length)
			    ||
			    ((pSsidList->SSIDList[i].SSID.length ==
			      pSsid->length)
			     && (!qdf_mem_cmp(pSsid->ssId,
						pSsidList->SSIDList[i].SSID.
						ssId, pSsid->length)))) {
				fMatch = true;
				break;
			}
		}
	}

	return fMatch;
}
#endif
bool csr_is_bssid_match(struct qdf_mac_addr *pProfBssid,
			struct qdf_mac_addr *BssBssid)
{
	bool fMatch = false;
	struct qdf_mac_addr ProfileBssid;

	/* for efficiency of the MAC_ADDRESS functions, move the */
	/* Bssid's into MAC_ADDRESS structs. */
	qdf_mem_copy(&ProfileBssid, pProfBssid, sizeof(struct qdf_mac_addr));

	do {
		/* Give the profile the benefit of the doubt... accept
		 * either all 0 or the real broadcast Bssid (all 0xff)
		 * as broadcast Bssids (meaning to match any Bssids).
		 */
		if (qdf_is_macaddr_zero(&ProfileBssid) ||
		    qdf_is_macaddr_broadcast(&ProfileBssid)) {
			fMatch = true;
			break;
		}

		if (qdf_is_macaddr_equal(BssBssid, &ProfileBssid)) {
			fMatch = true;
			break;
		}

	} while (0);

	return fMatch;
}

bool csr_rates_is_dot11_rate_supported(struct mac_context *mac_ctx,
				       uint8_t rate)
{
	return wlan_rates_is_dot11_rate_supported(mac_ctx, rate);
}

#ifndef FEATURE_CM_ENABLE
#ifdef WLAN_FEATURE_FILS_SK
static inline
void csr_free_fils_profile_info(struct mac_context *mac,
				struct csr_roam_profile *profile)
{
	if (profile->fils_con_info) {
		qdf_mem_free(profile->fils_con_info);
		profile->fils_con_info = NULL;
	}
}
#else
static inline void csr_free_fils_profile_info(struct mac_context *mac,
					      struct csr_roam_profile *profile)
{ }
#endif
#endif

void csr_release_profile(struct mac_context *mac,
			 struct csr_roam_profile *pProfile)
{
	if (pProfile) {
		if (pProfile->BSSIDs.bssid) {
			qdf_mem_free(pProfile->BSSIDs.bssid);
			pProfile->BSSIDs.bssid = NULL;
		}
		if (pProfile->SSIDs.SSIDList) {
			qdf_mem_free(pProfile->SSIDs.SSIDList);
			pProfile->SSIDs.SSIDList = NULL;
		}

		if (pProfile->ChannelInfo.freq_list) {
			qdf_mem_free(pProfile->ChannelInfo.freq_list);
			pProfile->ChannelInfo.freq_list = NULL;
		}
		if (pProfile->pRSNReqIE) {
			qdf_mem_free(pProfile->pRSNReqIE);
			pProfile->pRSNReqIE = NULL;
		}
#ifndef FEATURE_CM_ENABLE
		if (pProfile->pWPAReqIE) {
			qdf_mem_free(pProfile->pWPAReqIE);
			pProfile->pWPAReqIE = NULL;
		}
#ifdef FEATURE_WLAN_WAPI
		if (pProfile->pWAPIReqIE) {
			qdf_mem_free(pProfile->pWAPIReqIE);
			pProfile->pWAPIReqIE = NULL;
		}
#endif /* FEATURE_WLAN_WAPI */

		if (pProfile->pAddIEAssoc) {
			qdf_mem_free(pProfile->pAddIEAssoc);
			pProfile->pAddIEAssoc = NULL;
		}

		if (pProfile->pAddIEScan) {
			qdf_mem_free(pProfile->pAddIEScan);
			pProfile->pAddIEScan = NULL;
		}
		csr_free_fils_profile_info(mac, pProfile);
#endif
		qdf_mem_zero(pProfile, sizeof(struct csr_roam_profile));
	}
}

void csr_free_roam_profile(struct mac_context *mac, uint32_t sessionId)
{
	struct csr_roam_session *pSession = &mac->roam.roamSession[sessionId];

	if (pSession->pCurRoamProfile) {
		csr_release_profile(mac, pSession->pCurRoamProfile);
		qdf_mem_free(pSession->pCurRoamProfile);
		pSession->pCurRoamProfile = NULL;
	}
}

#ifndef FEATURE_CM_ENABLE
void csr_free_connect_bss_desc(struct mac_context *mac, uint32_t sessionId)
{
	struct csr_roam_session *pSession = &mac->roam.roamSession[sessionId];

	if (pSession->pConnectBssDesc) {
		qdf_mem_free(pSession->pConnectBssDesc);
		pSession->pConnectBssDesc = NULL;
	}
}
#endif

tSirResultCodes csr_get_de_auth_rsp_status_code(struct deauth_rsp *pSmeRsp)
{
	uint8_t *pBuffer = (uint8_t *) pSmeRsp;
	uint32_t ret;

	pBuffer +=
		(sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint8_t) +
		 sizeof(uint16_t));
	/* tSirResultCodes is an enum, assuming is 32bit */
	/* If we cannot make this assumption, use copymemory */
	qdf_get_u32(pBuffer, &ret);

	return (tSirResultCodes) ret;
}

enum bss_type csr_translate_bsstype_to_mac_type(eCsrRoamBssType csrtype)
{
	enum bss_type ret;

	switch (csrtype) {
	case eCSR_BSS_TYPE_INFRASTRUCTURE:
		ret = eSIR_INFRASTRUCTURE_MODE;
		break;
	case eCSR_BSS_TYPE_INFRA_AP:
		ret = eSIR_INFRA_AP_MODE;
		break;
	case eCSR_BSS_TYPE_NDI:
		ret = eSIR_NDI_MODE;
		break;
	case eCSR_BSS_TYPE_ANY:
	default:
		ret = eSIR_AUTO_MODE;
		break;
	}

	return ret;
}

/* This function use the parameters to decide the CFG value. */
/* CSR never sets MLME_DOT11_MODE_ALL to the CFG */
/* So PE should not see MLME_DOT11_MODE_ALL when it gets the CFG value */
enum csr_cfgdot11mode
csr_get_cfg_dot11_mode_from_csr_phy_mode(struct csr_roam_profile *pProfile,
					 eCsrPhyMode phyMode,
					 bool fProprietary)
{
	uint32_t cfgDot11Mode = eCSR_CFG_DOT11_MODE_ABG;

	switch (phyMode) {
	case eCSR_DOT11_MODE_11a:
		cfgDot11Mode = eCSR_CFG_DOT11_MODE_11A;
		break;
	case eCSR_DOT11_MODE_11b:
	case eCSR_DOT11_MODE_11b_ONLY:
		cfgDot11Mode = eCSR_CFG_DOT11_MODE_11B;
		break;
	case eCSR_DOT11_MODE_11g:
	case eCSR_DOT11_MODE_11g_ONLY:
		if (pProfile && (CSR_IS_INFRA_AP(pProfile))
		    && (phyMode == eCSR_DOT11_MODE_11g_ONLY))
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11G_ONLY;
		else
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11G;
		break;
	case eCSR_DOT11_MODE_11n:
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11N;
		break;
	case eCSR_DOT11_MODE_11n_ONLY:
		if (pProfile && CSR_IS_INFRA_AP(pProfile))
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11N_ONLY;
		else
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11N;
		break;
	case eCSR_DOT11_MODE_abg:
		cfgDot11Mode = eCSR_CFG_DOT11_MODE_ABG;
		break;
	case eCSR_DOT11_MODE_AUTO:
		cfgDot11Mode = eCSR_CFG_DOT11_MODE_AUTO;
		break;

	case eCSR_DOT11_MODE_11ac:
		if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AC))
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11AC;
		else
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11N;
		break;
	case eCSR_DOT11_MODE_11ac_ONLY:
		if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AC))
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11AC_ONLY;
		else
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11N;
		break;
	case eCSR_DOT11_MODE_11ax:
		if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AX))
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11AX;
		else if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AC))
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11AC;
		else
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11N;
		break;
	case eCSR_DOT11_MODE_11ax_ONLY:
		if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AX))
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11AX_ONLY;
		else if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AC))
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11AC;
		else
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11N;
		break;
#ifdef WLAN_FEATURE_11BE
	case eCSR_DOT11_MODE_11be:
		if (IS_FEATURE_SUPPORTED_BY_FW(DOT11BE))
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11BE;
		else if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AX))
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11AX;
		else if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AC))
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11AC;
		else
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11N;
		break;
	case eCSR_DOT11_MODE_11be_ONLY:
		if (IS_FEATURE_SUPPORTED_BY_FW(DOT11BE))
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11BE_ONLY;
		else if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AX))
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11AX_ONLY;
		else if (IS_FEATURE_SUPPORTED_BY_FW(DOT11AC))
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11AC;
		else
			cfgDot11Mode = eCSR_CFG_DOT11_MODE_11N;
		break;
#endif
	default:
		/* No need to assign anything here */
		break;
	}

	return cfgDot11Mode;
}

QDF_STATUS csr_get_regulatory_domain_for_country(struct mac_context *mac,
						 uint8_t *pCountry,
						 v_REGDOMAIN_t *pDomainId,
						 enum country_src source)
{
	QDF_STATUS status = QDF_STATUS_E_INVAL;
	QDF_STATUS qdf_status;
	uint8_t countryCode[CDS_COUNTRY_CODE_LEN + 1];
	v_REGDOMAIN_t domainId;

	if (pCountry) {
		countryCode[0] = pCountry[0];
		countryCode[1] = pCountry[1];
		qdf_status = wlan_reg_get_domain_from_country_code(&domainId,
								  countryCode,
								  source);

		if (QDF_IS_STATUS_SUCCESS(qdf_status)) {
			if (pDomainId)
				*pDomainId = domainId;
			status = QDF_STATUS_SUCCESS;
		} else {
			sme_warn("Couldn't find domain for country code %c%c",
				pCountry[0], pCountry[1]);
			status = QDF_STATUS_E_INVAL;
		}
	}

	return status;
}

QDF_STATUS csr_get_modify_profile_fields(struct mac_context *mac,
					uint32_t sessionId,
					 tCsrRoamModifyProfileFields *
					 pModifyProfileFields)
{
	if (!pModifyProfileFields)
		return QDF_STATUS_E_FAILURE;

	qdf_mem_copy(pModifyProfileFields,
		     &mac->roam.roamSession[sessionId].connectedProfile.
		     modifyProfileFields, sizeof(tCsrRoamModifyProfileFields));

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS csr_set_modify_profile_fields(struct mac_context *mac,
					uint32_t sessionId,
					 tCsrRoamModifyProfileFields *
					 pModifyProfileFields)
{
	struct csr_roam_session *pSession = CSR_GET_SESSION(mac, sessionId);

	qdf_mem_copy(&pSession->connectedProfile.modifyProfileFields,
		     pModifyProfileFields, sizeof(tCsrRoamModifyProfileFields));

	return QDF_STATUS_SUCCESS;
}

/* no need to acquire lock for this basic function */
uint16_t sme_chn_to_freq(uint8_t chanNum)
{
	int i;

	for (i = 0; i < NUM_CHANNELS; i++) {
		if (WLAN_REG_CH_NUM(i) == chanNum)
			return WLAN_REG_CH_TO_FREQ(i);
	}

	return 0;
}

struct lim_channel_status *
csr_get_channel_status(struct mac_context *mac, uint32_t chan_freq)
{
	uint8_t i;
	struct lim_scan_channel_status *channel_status;
	struct lim_channel_status *entry;

	if (!mac->sap.acs_with_more_param)
		return NULL;

	channel_status = &mac->lim.scan_channel_status;
	for (i = 0; i < channel_status->total_channel; i++) {
		entry = &channel_status->channel_status_list[i];
		if (entry->channelfreq == chan_freq)
			return entry;
	}
	sme_err("Channel %d status info not exist", chan_freq);

	return NULL;
}

void csr_clear_channel_status(struct mac_context *mac)
{
	struct lim_scan_channel_status *channel_status;

	if (!mac->sap.acs_with_more_param)
		return;

	channel_status = &mac->lim.scan_channel_status;
	channel_status->total_channel = 0;

	return;
}

/**
 * sme_bsstype_to_string() - converts bss type to string.
 * @bss_type: bss type enum
 *
 * Return: printable string for bss type
 */
const char *sme_bss_type_to_string(const uint8_t bss_type)
{
	switch (bss_type) {
	CASE_RETURN_STRING(eCSR_BSS_TYPE_INFRASTRUCTURE);
	CASE_RETURN_STRING(eCSR_BSS_TYPE_INFRA_AP);
	CASE_RETURN_STRING(eCSR_BSS_TYPE_ANY);
	default:
		return "unknown bss type";
	}
}

/**
 * csr_wait_for_connection_update() - Wait for hw mode update
 * @mac: Pointer to the MAC context
 * @do_release_reacquire_lock: Indicates whether release and
 * re-acquisition of SME global lock is required.
 *
 * Waits for CONNECTION_UPDATE_TIMEOUT time so that the
 * hw mode update can get processed.
 *
 * Return: True if the wait was successful, false otherwise
 */
bool csr_wait_for_connection_update(struct mac_context *mac,
		bool do_release_reacquire_lock)
{
	QDF_STATUS status, ret;

	if (do_release_reacquire_lock == true) {
		ret = sme_release_global_lock(&mac->sme);
		if (!QDF_IS_STATUS_SUCCESS(ret)) {
			cds_err("lock release fail %d", ret);
			return false;
		}
	}

	status = policy_mgr_wait_for_connection_update(mac->psoc);

	if (do_release_reacquire_lock == true) {
		ret = sme_acquire_global_lock(&mac->sme);
		if (!QDF_IS_STATUS_SUCCESS(ret))
			return false;
	}

	if (!QDF_IS_STATUS_SUCCESS(status)) {
		cds_err("wait for event failed");
		return false;
	}

	return true;
}

/**
 * csr_is_ndi_started() - function to check if NDI is started
 * @mac_ctx: handle to mac context
 * @session_id: session identifier
 *
 * returns: true if NDI is started, false otherwise
 */
bool csr_is_ndi_started(struct mac_context *mac_ctx, uint32_t session_id)
{
	struct csr_roam_session *session = CSR_GET_SESSION(mac_ctx, session_id);

	if (!session)
		return false;

	return eCSR_CONNECT_STATE_TYPE_NDI_STARTED == session->connectState;
}

bool csr_is_mcc_channel(struct mac_context *mac_ctx, uint32_t chan_freq)
{
	struct csr_roam_session *session;
	enum QDF_OPMODE oper_mode;
	uint32_t oper_chan_freq = 0;
	uint8_t vdev_id;
	bool hw_dbs_capable, same_band_freqs;

	if (chan_freq == 0)
		return false;

	hw_dbs_capable = policy_mgr_is_hw_dbs_capable(mac_ctx->psoc);
	for (vdev_id = 0; vdev_id < WLAN_MAX_VDEVS; vdev_id++) {
		if (!CSR_IS_SESSION_VALID(mac_ctx, vdev_id))
			continue;

		session = CSR_GET_SESSION(mac_ctx, vdev_id);
		oper_mode =
			wlan_get_opmode_from_vdev_id(mac_ctx->pdev, vdev_id);
		if ((((oper_mode == QDF_STA_MODE) ||
		     (oper_mode == QDF_P2P_CLIENT_MODE)) &&
		/* This is temp ifdef will be removed in near future */
#ifdef FEATURE_CM_ENABLE
		    cm_is_vdevid_connected(mac_ctx->pdev, vdev_id)
#else
		    (session->connectState ==
		    eCSR_ASSOC_STATE_TYPE_INFRA_ASSOCIATED)
#endif
		    ) ||
		    (((oper_mode == QDF_P2P_GO_MODE) ||
		      (oper_mode == QDF_SAP_MODE)) &&
		     (session->connectState !=
		      eCSR_ASSOC_STATE_TYPE_NOT_CONNECTED)))
			oper_chan_freq =
			    wlan_get_operation_chan_freq_vdev_id(mac_ctx->pdev,
								 vdev_id);

		if (!oper_chan_freq)
			continue;
		same_band_freqs = WLAN_REG_IS_SAME_BAND_FREQS(
			chan_freq, oper_chan_freq);

		if (oper_chan_freq && chan_freq != oper_chan_freq &&
		    (!hw_dbs_capable || same_band_freqs))
			return true;
	}

	return false;
}

enum csr_cfgdot11mode csr_phy_mode_to_dot11mode(enum wlan_phymode phy_mode)
{
	switch (phy_mode) {
	case WLAN_PHYMODE_AUTO:
		return eCSR_CFG_DOT11_MODE_AUTO;
	case WLAN_PHYMODE_11A:
		return eCSR_CFG_DOT11_MODE_11A;
	case WLAN_PHYMODE_11B:
		return eCSR_CFG_DOT11_MODE_11B;
	case WLAN_PHYMODE_11G:
		return eCSR_CFG_DOT11_MODE_11G;
	case WLAN_PHYMODE_11G_ONLY:
		return eCSR_CFG_DOT11_MODE_11G_ONLY;
	case WLAN_PHYMODE_11NA_HT20:
	case WLAN_PHYMODE_11NG_HT20:
	case WLAN_PHYMODE_11NA_HT40:
	case WLAN_PHYMODE_11NG_HT40PLUS:
	case WLAN_PHYMODE_11NG_HT40MINUS:
	case WLAN_PHYMODE_11NG_HT40:
		return eCSR_CFG_DOT11_MODE_11N;
	case WLAN_PHYMODE_11AC_VHT20:
	case WLAN_PHYMODE_11AC_VHT20_2G:
	case WLAN_PHYMODE_11AC_VHT40:
	case WLAN_PHYMODE_11AC_VHT40PLUS_2G:
	case WLAN_PHYMODE_11AC_VHT40MINUS_2G:
	case WLAN_PHYMODE_11AC_VHT40_2G:
	case WLAN_PHYMODE_11AC_VHT80:
	case WLAN_PHYMODE_11AC_VHT80_2G:
	case WLAN_PHYMODE_11AC_VHT160:
	case WLAN_PHYMODE_11AC_VHT80_80:
		return eCSR_CFG_DOT11_MODE_11AC;
	case WLAN_PHYMODE_11AXA_HE20:
	case WLAN_PHYMODE_11AXG_HE20:
	case WLAN_PHYMODE_11AXA_HE40:
	case WLAN_PHYMODE_11AXG_HE40PLUS:
	case WLAN_PHYMODE_11AXG_HE40MINUS:
	case WLAN_PHYMODE_11AXG_HE40:
	case WLAN_PHYMODE_11AXA_HE80:
	case WLAN_PHYMODE_11AXG_HE80:
	case WLAN_PHYMODE_11AXA_HE160:
	case WLAN_PHYMODE_11AXA_HE80_80:
		return eCSR_CFG_DOT11_MODE_11AX;
#ifdef WLAN_FEATURE_11BE
	case WLAN_PHYMODE_11BEA_EHT20:
	case WLAN_PHYMODE_11BEG_EHT20:
	case WLAN_PHYMODE_11BEA_EHT40:
	case WLAN_PHYMODE_11BEG_EHT40PLUS:
	case WLAN_PHYMODE_11BEG_EHT40MINUS:
	case WLAN_PHYMODE_11BEG_EHT40:
	case WLAN_PHYMODE_11BEA_EHT80:
	case WLAN_PHYMODE_11BEG_EHT80:
	case WLAN_PHYMODE_11BEA_EHT160:
		return eCSR_CFG_DOT11_MODE_11BE;
#endif
	default:
		sme_err("invalid phy mode %d", phy_mode);
		return eCSR_CFG_DOT11_MODE_MAX;
	}
}

QDF_STATUS csr_mlme_vdev_disconnect_all_p2p_client_event(uint8_t vdev_id)
{
	struct mac_context *mac_ctx = cds_get_context(QDF_MODULE_ID_SME);

	if (!mac_ctx)
		return QDF_STATUS_E_FAILURE;

	return csr_roam_call_callback(mac_ctx, vdev_id, NULL, 0,
				      eCSR_ROAM_DISCONNECT_ALL_P2P_CLIENTS,
				      eCSR_ROAM_RESULT_NONE);
}

QDF_STATUS csr_mlme_vdev_stop_bss(uint8_t vdev_id)
{
	struct mac_context *mac_ctx = cds_get_context(QDF_MODULE_ID_SME);

	if (!mac_ctx)
		return QDF_STATUS_E_FAILURE;

	return csr_roam_call_callback(mac_ctx, vdev_id, NULL, 0,
				      eCSR_ROAM_SEND_P2P_STOP_BSS,
				      eCSR_ROAM_RESULT_NONE);
}

qdf_freq_t csr_mlme_get_concurrent_operation_freq(void)
{
	struct mac_context *mac_ctx = cds_get_context(QDF_MODULE_ID_SME);

	if (!mac_ctx)
		return QDF_STATUS_E_FAILURE;

	return csr_get_concurrent_operation_freq(mac_ctx);
}
