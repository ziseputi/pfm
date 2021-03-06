#include "pfm.h"
#include "pfm_comm.h"
#include "pfm_log.h"
#include "cuup.h"
#include "tunnel.h"
#include "ue_ctx.h"
#include "pdus.h"
#include "drb.h"
#include "e1ap_comm.h"
#include "e1ap_bearer_setup.h"
static void  
pdus_setup_succ_rsp_create(tunnel_t *tunnel_entry,pdus_setup_succ_rsp_info_t *succ_rsp)
{
	succ_rsp->pdus_id 			= tunnel_entry->pdus_info.pdus_id;
	succ_rsp->drb_setup_succ_count 	= 0;
	succ_rsp->drb_setup_fail_count	= 0;
	succ_rsp->pdus_dl_ip_addr		= tunnel_entry->key.ip_addr;
	succ_rsp->pdus_dl_teid		= tunnel_entry->key.te_id;
	return;
}

void
pdus_setup_fail_rsp_create(pdus_setup_req_info_t* req,
			   pdus_setup_fail_rsp_info_t* fail_rsp,
			   e1ap_fail_cause_t cause)
{
	fail_rsp->pdus_id = req->pdus_id;
	// TD how to assign cause
	fail_rsp->cause   = cause;
	return;
}


pfm_retval_t 
pdus_setup(ue_ctx_t* ue_ctx,
	   pdus_setup_req_info_t* req,
	   pdus_setup_succ_rsp_info_t *succ_rsp,
	   pdus_setup_fail_rsp_info_t *fail_rsp)
{
	pfm_retval_t ret;
	tunnel_key_t tunnel_key;
	tunnel_t *tunnel_entry;

	drb_setup_succ_rsp_info_t* drb_succ_rsp;
	drb_setup_fail_rsp_info_t* drb_fail_rsp;
	
	// Assign a tunnel key with pdus_ul_ip_addr
	ret = tunnel_key_alloc(req->pdus_ul_ip_addr,TUNNEL_TYPE_PDUS,&tunnel_key);
	if (ret == PFM_FAILED)
	{
		pfm_log_msg(PFM_LOG_ERR,"Error allocating tunnel_key");
		// TD assign cause properly
		pdus_setup_fail_rsp_create(req,fail_rsp,FAIL_CAUSE_RNL_RESOURCE_UNAVAIL);
		return PFM_FAILED;
	}

	// Allocate a free entry from the tunnel table 
	
	tunnel_entry = tunnel_add(&tunnel_key);
	if (tunnel_entry ==  NULL)
	{
		pfm_log_msg(PFM_LOG_ERR,"Error allocating tunnel_entry");
		// TD assign cause properly
		tunnel_key_free(&tunnel_key);
		pdus_setup_fail_rsp_create(req,fail_rsp,FAIL_CAUSE_RNL_RESOURCE_UNAVAIL);
		return PFM_FAILED;
	}
	
	// Fill in the pdus details to the entry
	tunnel_entry->remote_ip    	= req->pdus_ul_ip_addr;
	tunnel_entry->remote_te_id 	= req->pdus_ul_teid;
	tunnel_entry->tunnel_type 	= TUNNEL_TYPE_PDUS;
	tunnel_entry->pdus_info.pdus_id = req->pdus_id;
	
	// Setup up the succ_rsp since it's gone fine so far
	// Rest of the info is added as the drb's are processed

	pdus_setup_succ_rsp_create(tunnel_entry,succ_rsp);
	
	for (int i = 0;i < req->drb_count;i++)	{
		// Storing frequently used pointers to make the variable names smaller
		drb_succ_rsp = &(succ_rsp->drb_succ_list[succ_rsp->drb_setup_succ_count]);
		drb_fail_rsp = &(succ_rsp->drb_fail_list[succ_rsp->drb_setup_fail_count]);

		// Process this drb
		ret = drb_setup(ue_ctx,&(req->drb_list[i]),drb_succ_rsp,drb_fail_rsp);
		
		// If failed log and increment the drb_setup_fail_count
		if (ret ==  PFM_FAILED){
			pfm_log_msg(PFM_LOG_ERR,"Error creating DRB entry");
			succ_rsp->drb_setup_fail_count++;
			continue;
		}
		succ_rsp->drb_setup_succ_count++;
	}
	// If all drb requests fail consider the pdu request a fail
	if (succ_rsp->drb_setup_fail_count == req->drb_count)
	{
		pfm_log_msg(PFM_LOG_ERR,
			"all drb_req in pdus_req failed pdus id :: %s:",
				req->pdus_id);
		tunnel_remove(&tunnel_key);
		tunnel_commit(tunnel_entry);
		
		pdus_setup_fail_rsp_create(req,fail_rsp,FAIL_CAUSE_RNL_RESOURCE_UNAVAIL);
		return PFM_FAILED;
	}

	// Assign tunnel_entry to the next vacant spot
	ue_ctx->pdus_tunnel_list[ue_ctx->pdus_count] = tunnel_entry;
	ue_ctx->pdus_count++;
	return PFM_SUCCESS;
}


//---------------------------------------------------pdus_modify------------------------

