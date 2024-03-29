/*
 * This file is part of wl1271
 *
 * Copyright (C) 2008-2010 Nokia Corporation
 * Copyright (C) 2012 Sony Mobile Communications AB
 *
 * Contact: Luciano Coelho <luciano.coelho@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <linux/module.h>
#include <linux/firmware.h>
#include <linux/delay.h>
#include <linux/spi/spi.h>
#include <linux/crc32.h>
#include <linux/etherdevice.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/wl12xx.h>
#include <linux/sched.h>

#include "wl12xx.h"
#include "wl12xx_80211.h"
#include "reg.h"
#include "io.h"
#include "event.h"
#include "tx.h"
#include "rx.h"
#include "ps.h"
#include "init.h"
#include "debugfs.h"
#include "cmd.h"
#include "boot.h"
#include "testmode.h"
#include "scan.h"

#define WL1271_BOOT_RETRIES 3

static struct conf_drv_settings default_conf = {
	.sg = {
		.params = {
			[CONF_SG_ACL_BT_MASTER_MIN_BR] = 10,
			[CONF_SG_ACL_BT_MASTER_MAX_BR] = 180,
			[CONF_SG_ACL_BT_SLAVE_MIN_BR] = 10,
			[CONF_SG_ACL_BT_SLAVE_MAX_BR] = 180,
			[CONF_SG_ACL_BT_MASTER_MIN_EDR] = 10,
			[CONF_SG_ACL_BT_MASTER_MAX_EDR] = 80,
			[CONF_SG_ACL_BT_SLAVE_MIN_EDR] = 10,
			[CONF_SG_ACL_BT_SLAVE_MAX_EDR] = 80,
			[CONF_SG_ACL_WLAN_PS_MASTER_BR] = 8,
			[CONF_SG_ACL_WLAN_PS_SLAVE_BR] = 8,
			[CONF_SG_ACL_WLAN_PS_MASTER_EDR] = 20,
			[CONF_SG_ACL_WLAN_PS_SLAVE_EDR] = 20,
			[CONF_SG_ACL_WLAN_ACTIVE_MASTER_MIN_BR] = 20,
			[CONF_SG_ACL_WLAN_ACTIVE_MASTER_MAX_BR] = 35,
			[CONF_SG_ACL_WLAN_ACTIVE_SLAVE_MIN_BR] = 16,
			[CONF_SG_ACL_WLAN_ACTIVE_SLAVE_MAX_BR] = 35,
			[CONF_SG_ACL_WLAN_ACTIVE_MASTER_MIN_EDR] = 32,
			[CONF_SG_ACL_WLAN_ACTIVE_MASTER_MAX_EDR] = 50,
			[CONF_SG_ACL_WLAN_ACTIVE_SLAVE_MIN_EDR] = 28,
			[CONF_SG_ACL_WLAN_ACTIVE_SLAVE_MAX_EDR] = 50,
			[CONF_SG_ACL_ACTIVE_SCAN_WLAN_BR] = 10,
			[CONF_SG_ACL_ACTIVE_SCAN_WLAN_EDR] = 20,
			[CONF_SG_ACL_PASSIVE_SCAN_BT_BR] = 75,
			[CONF_SG_ACL_PASSIVE_SCAN_WLAN_BR] = 15,
			[CONF_SG_ACL_PASSIVE_SCAN_BT_EDR] = 27,
			[CONF_SG_ACL_PASSIVE_SCAN_WLAN_EDR] = 17,
			/* active scan params */
			[CONF_SG_AUTO_SCAN_PROBE_REQ] = 170,
			[CONF_SG_ACTIVE_SCAN_DURATION_FACTOR_HV3] = 50,
			[CONF_SG_ACTIVE_SCAN_DURATION_FACTOR_A2DP] = 100,
			/* passive scan params */
			[CONF_SG_PASSIVE_SCAN_DURATION_FACTOR_A2DP_BR] = 800,
			[CONF_SG_PASSIVE_SCAN_DURATION_FACTOR_A2DP_EDR] = 200,
			[CONF_SG_PASSIVE_SCAN_DURATION_FACTOR_HV3] = 200,
			/* passive scan in dual antenna params */
			[CONF_SG_CONSECUTIVE_HV3_IN_PASSIVE_SCAN] = 0,
			[CONF_SG_BCN_HV3_COLLISION_THRESH_IN_PASSIVE_SCAN] = 0,
			[CONF_SG_TX_RX_PROTECTION_BWIDTH_IN_PASSIVE_SCAN] = 0,
			/* general params */
			[CONF_SG_STA_FORCE_PS_IN_BT_SCO] = 1,
			[CONF_SG_ANTENNA_CONFIGURATION] = 0,
			[CONF_SG_BEACON_MISS_PERCENT] = 60,
			[CONF_SG_DHCP_TIME] = 5000,
			[CONF_SG_RXT] = 1200,
			[CONF_SG_TXT] = 1000,
			[CONF_SG_ADAPTIVE_RXT_TXT] = 1,
			[CONF_SG_GENERAL_USAGE_BIT_MAP] = 3,
			[CONF_SG_HV3_MAX_SERVED] = 6,
			[CONF_SG_PS_POLL_TIMEOUT] = 10,
			[CONF_SG_UPSD_TIMEOUT] = 10,
			[CONF_SG_CONSECUTIVE_CTS_THRESHOLD] = 2,
			[CONF_SG_STA_RX_WINDOW_AFTER_DTIM] = 5,
			[CONF_SG_STA_CONNECTION_PROTECTION_TIME] = 30,
			/* AP params */
			[CONF_AP_BEACON_MISS_TX] = 3,
			[CONF_AP_RX_WINDOW_AFTER_BEACON] = 10,
			[CONF_AP_BEACON_WINDOW_INTERVAL] = 2,
			[CONF_AP_CONNECTION_PROTECTION_TIME] = 0,
			[CONF_AP_BT_ACL_VAL_BT_SERVE_TIME] = 25,
			[CONF_AP_BT_ACL_VAL_WL_SERVE_TIME] = 25,
		},
		.state = CONF_SG_PROTECTIVE,
	},
	.rx = {
		.rx_msdu_life_time           = 512000,
		.packet_detection_threshold  = 0,
		.ps_poll_timeout             = 15,
		.upsd_timeout                = 15,
		.rts_threshold               = IEEE80211_MAX_RTS_THRESHOLD,
		.rx_cca_threshold            = 0,
		.irq_blk_threshold           = 0xFFFF,
		.irq_pkt_threshold           = 0,
		.irq_timeout                 = 600,
		.queue_type                  = CONF_RX_QUEUE_TYPE_LOW_PRIORITY,
	},
	.tx = {
		.tx_energy_detection         = 0,
		.sta_rc_conf                 = {
			.enabled_rates       = 0,
			.short_retry_limit   = 10,
			.long_retry_limit    = 10,
			.aflags              = 0,
		},
		.ac_conf_count               = 4,
		.ac_conf                     = {
			[CONF_TX_AC_BE] = {
				.ac          = CONF_TX_AC_BE,
				.cw_min      = 15,
				.cw_max      = 63,
				.aifsn       = 3,
				.tx_op_limit = 0,
			},
			[CONF_TX_AC_BK] = {
				.ac          = CONF_TX_AC_BK,
				.cw_min      = 15,
				.cw_max      = 63,
				.aifsn       = 7,
				.tx_op_limit = 0,
			},
			[CONF_TX_AC_VI] = {
				.ac          = CONF_TX_AC_VI,
				.cw_min      = 15,
				.cw_max      = 63,
				.aifsn       = CONF_TX_AIFS_PIFS,
				.tx_op_limit = 3008,
			},
			[CONF_TX_AC_VO] = {
				.ac          = CONF_TX_AC_VO,
				.cw_min      = 15,
				.cw_max      = 63,
				.aifsn       = CONF_TX_AIFS_PIFS,
				.tx_op_limit = 1504,
			},
		},
		.max_tx_retries = 100,
		.ap_aging_period = 300,
		.tid_conf_count = 4,
		.tid_conf = {
			[CONF_TX_AC_BE] = {
				.queue_id    = CONF_TX_AC_BE,
				.channel_type = CONF_CHANNEL_TYPE_EDCF,
				.tsid        = CONF_TX_AC_BE,
				.ps_scheme   = CONF_PS_SCHEME_LEGACY,
				.ack_policy  = CONF_ACK_POLICY_LEGACY,
				.apsd_conf   = {0, 0},
			},
			[CONF_TX_AC_BK] = {
				.queue_id    = CONF_TX_AC_BK,
				.channel_type = CONF_CHANNEL_TYPE_EDCF,
				.tsid        = CONF_TX_AC_BK,
				.ps_scheme   = CONF_PS_SCHEME_LEGACY,
				.ack_policy  = CONF_ACK_POLICY_LEGACY,
				.apsd_conf   = {0, 0},
			},
			[CONF_TX_AC_VI] = {
				.queue_id    = CONF_TX_AC_VI,
				.channel_type = CONF_CHANNEL_TYPE_EDCF,
				.tsid        = CONF_TX_AC_VI,
				.ps_scheme   = CONF_PS_SCHEME_LEGACY,
				.ack_policy  = CONF_ACK_POLICY_LEGACY,
				.apsd_conf   = {0, 0},
			},
			[CONF_TX_AC_VO] = {
				.queue_id    = CONF_TX_AC_VO,
				.channel_type = CONF_CHANNEL_TYPE_EDCF,
				.tsid        = CONF_TX_AC_VO,
				.ps_scheme   = CONF_PS_SCHEME_LEGACY,
				.ack_policy  = CONF_ACK_POLICY_LEGACY,
				.apsd_conf   = {0, 0},
			},
		},
		.frag_threshold              = IEEE80211_MAX_FRAG_THRESHOLD,
		.tx_compl_timeout            = 700,
		.tx_compl_threshold          = 4,
		.basic_rate                  = CONF_HW_BIT_RATE_1MBPS,
		.basic_rate_5                = CONF_HW_BIT_RATE_6MBPS,
		.tmpl_short_retry_limit      = 10,
		.tmpl_long_retry_limit       = 10,
	},
	.conn = {
		.wake_up_event               = CONF_WAKE_UP_EVENT_DTIM,
		.listen_interval             = 1,
		.suspend_wake_up_event       = CONF_WAKE_UP_EVENT_N_DTIM,
		.suspend_listen_interval     = 3,
		.bcn_filt_mode               = CONF_BCN_FILT_MODE_ENABLED,
		.bcn_filt_ie_count           = 2,
		.bcn_filt_ie = {
			[0] = {
				.ie          = WLAN_EID_CHANNEL_SWITCH,
				.rule        = CONF_BCN_RULE_PASS_ON_APPEARANCE,
			},
			[1] = {
				.ie          = WLAN_EID_HT_INFORMATION,
				.rule        = CONF_BCN_RULE_PASS_ON_CHANGE,
			},
		},
		.synch_fail_thold            = 20,
		.bss_lose_timeout            = 100,
		.beacon_rx_timeout           = 10000,
		.broadcast_timeout           = 20000,
		.rx_broadcast_in_ps          = 1,
		.ps_poll_threshold           = 10,
		.ps_poll_recovery_period     = 700,
		.bet_enable                  = CONF_BET_MODE_ENABLE,
		.bet_max_consecutive         = 50,
		.psm_entry_retries           = 8,
		.psm_exit_retries            = 16,
		.psm_entry_nullfunc_retries  = 3,
		.keep_alive_interval         = 55000,
		.max_listen_interval         = 20,
	},
	.itrim = {
		.enable = false,
		.timeout = 50000,
	},
	.pm_config = {
		.host_clk_settling_time = 5000,
		.host_fast_wakeup_support = false
	},
	.roam_trigger = {
		.trigger_pacing               = 1,
		.avg_weight_rssi_beacon       = 20,
		.avg_weight_rssi_data         = 10,
		.avg_weight_snr_beacon        = 20,
		.avg_weight_snr_data          = 10,
	},
	.scan = {
		.min_dwell_time_active        = 7500,
		.max_dwell_time_active        = 30000,
		.min_dwell_time_passive       = 100000,
		.max_dwell_time_passive       = 100000,
		.num_probe_reqs               = 2,
	},
	.sched_scan = {
		/* sched_scan requires dwell times in TU instead of TU/1000 */
		.min_dwell_time_active = 30,
		.max_dwell_time_active = 60,
		.dwell_time_passive    = 100,
		.num_probe_reqs        = 2,
		.rssi_threshold        = -90,
		.snr_threshold         = 0,
	},
	.rf = {
		.tx_per_channel_power_compensation_2 = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		},
		.tx_per_channel_power_compensation_5 = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		},
	},
	.ht = {
		.tx_ba_win_size = 8,
		.inactivity_timeout = 10000,
		.tx_ba_tid_bitmap = CONF_TX_BA_ENABLED_TID_BITMAP,
	},
	.mem_wl127x = {
		.num_stations                 = 1,
		.ssid_profiles                = 1,
		.rx_block_num                 = 70,
		.tx_min_block_num             = 40,
		.dynamic_memory               = 0,
		.min_req_tx_blocks            = 100,
		.min_req_rx_blocks            = 22,
		.tx_min                       = 27,
	},
	.mem_wl128x = {
		.num_stations                 = 1,
		.ssid_profiles                = 1,
		.rx_block_num                 = 40,
		.tx_min_block_num             = 40,
		.dynamic_memory               = 1,
		.min_req_tx_blocks            = 45,
		.min_req_rx_blocks            = 22,
		.tx_min                       = 27,
	},
	.fm_coex = {
		.enable                       = true,
		.swallow_period               = 5,
		.n_divider_fref_set_1         = 0xff,       /* default */
		.n_divider_fref_set_2         = 12,
		.m_divider_fref_set_1         = 148,
		.m_divider_fref_set_2         = 0xffff,     /* default */
		.coex_pll_stabilization_time  = 0xffffffff, /* default */
		.ldo_stabilization_time       = 0xffff,     /* default */
		.fm_disturbed_band_margin     = 0xff,       /* default */
		.swallow_clk_diff             = 0xff,       /* default */
	},
	.rx_streaming = {
		.duration                      = 150,
		.queues                        = 0x1,
		.interval                      = 20,
		.always                        = 0,
	},
	.hci_io_ds = HCI_IO_DS_6MA,
	.rate = {
		.rate_retry_score = 32000,
		.per_add = 8192,
		.per_th1 = 2048,
		.per_th2 = 4096,
		.max_per = 8100,
		.inverse_curiosity_factor = 5,
		.tx_fail_low_th = 4,
		.tx_fail_high_th = 10,
		.per_alpha_shift = 4,
		.per_add_shift = 13,
		.per_beta1_shift = 10,
		.per_beta2_shift = 8,
		.rate_check_up = 2,
		.rate_check_down = 12,
		.rate_retry_policy = {
			0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00,
		},
	},
	.hangover = {
		.recover_time               = 0,
		.hangover_period            = 20,
		.dynamic_mode               = 1,
		.early_termination_mode     = 1,
		.max_period                 = 20,
		.min_period                 = 1,
		.increase_delta             = 1,
		.decrease_delta             = 2,
		.quiet_time                 = 4,
		.increase_time              = 1,
		.window_size                = 16,
	},
	.fwlog = {
		.mode                         = WL12XX_FWLOG_ON_DEMAND,
		.mem_blocks                   = 2,
		.severity                     = 0,
		.timestamp                    = WL12XX_FWLOG_TIMESTAMP_DISABLED,
		.output                       = WL12XX_FWLOG_OUTPUT_HOST,
		.threshold                    = 0,
	},
};

static char *fwlog_param;
static char *fref_param;
static char *tcxo_param;

static void __wl1271_op_remove_interface(struct wl1271 *wl,
					 bool reset_tx_queues);
static void wl1271_free_ap_keys(struct wl1271 *wl);


static void wl1271_device_release(struct device *dev)
{

}

static struct platform_device wl1271_device = {
	.name           = "wl1271",
	.id             = -1,

	/* device model insists to have a release function */
	.dev            = {
		.release = wl1271_device_release,
	},
};

static DEFINE_MUTEX(wl_list_mutex);
static LIST_HEAD(wl_list);
static bool bug_on_recovery = false;

static int wl1271_check_operstate(struct wl1271 *wl, unsigned char operstate)
{
	int ret;
	if (operstate != IF_OPER_UP)
		return 0;

	if (test_and_set_bit(WL1271_FLAG_STA_STATE_SENT, &wl->flags))
		return 0;

	ret = wl1271_cmd_set_peer_state(wl, wl->sta_hlid);
	if (ret < 0)
		return ret;

	wl1271_croc(wl, wl->role_id);

	wl1271_info("Association completed.");
	return 0;
}
static int wl1271_dev_notify(struct notifier_block *me, unsigned long what,
			     void *arg)
{
	struct net_device *dev = arg;
	struct wireless_dev *wdev;
	struct wiphy *wiphy;
	struct ieee80211_hw *hw;
	struct wl1271 *wl;
	struct wl1271 *wl_temp;
	int ret = 0;

	/* Check that this notification is for us. */
	if (what != NETDEV_CHANGE)
		return NOTIFY_DONE;

	wdev = dev->ieee80211_ptr;
	if (wdev == NULL)
		return NOTIFY_DONE;

	wiphy = wdev->wiphy;
	if (wiphy == NULL)
		return NOTIFY_DONE;

	hw = wiphy_priv(wiphy);
	if (hw == NULL)
		return NOTIFY_DONE;

	wl_temp = hw->priv;
	mutex_lock(&wl_list_mutex);
	list_for_each_entry(wl, &wl_list, list) {
		if (wl == wl_temp)
			break;
	}
	mutex_unlock(&wl_list_mutex);
	if (wl != wl_temp)
		return NOTIFY_DONE;

	mutex_lock(&wl->mutex);

	if (wl->state == WL1271_STATE_OFF)
		goto out;

	if (!test_bit(WL1271_FLAG_STA_ASSOCIATED, &wl->flags))
		goto out;

	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		goto out;

	wl1271_check_operstate(wl, dev->operstate);

	wl1271_ps_elp_sleep(wl);

out:
	mutex_unlock(&wl->mutex);

	return NOTIFY_OK;
}

static int wl1271_reg_notify(struct wiphy *wiphy,
			     struct regulatory_request *request)
{
	struct ieee80211_supported_band *band;
	struct ieee80211_channel *ch;
	int i;

	band = wiphy->bands[IEEE80211_BAND_5GHZ];
	for (i = 0; i < band->n_channels; i++) {
		ch = &band->channels[i];
		if (ch->flags & IEEE80211_CHAN_DISABLED)
			continue;

		if (ch->flags & IEEE80211_CHAN_RADAR)
			ch->flags |= IEEE80211_CHAN_NO_IBSS |
				     IEEE80211_CHAN_PASSIVE_SCAN;

	}

	return 0;
}

static int wl1271_set_rx_streaming(struct wl1271 *wl, bool enable)
{
	int ret = 0;

	/* we should hold wl->mutex */
	ret = wl1271_acx_ps_rx_streaming(wl, enable);
	if (ret < 0)
		goto out;

	if (enable)
		set_bit(WL1271_FLAG_RX_STREAMING_STARTED, &wl->flags);
	else
		clear_bit(WL1271_FLAG_RX_STREAMING_STARTED, &wl->flags);
out:
	return ret;
}

/*
 * this function is being called when the rx_streaming interval
 * has beed changed or rx_streaming should be disabled
 */
int wl1271_recalc_rx_streaming(struct wl1271 *wl)
{
	int ret = 0;
	int period = wl->conf.rx_streaming.interval;

	/* don't reconfigure if rx_streaming is disabled */
	if (!test_bit(WL1271_FLAG_RX_STREAMING_STARTED, &wl->flags))
		goto out;

	/* reconfigure/disable according to new streaming_period */
	if (period &&
	    test_bit(WL1271_FLAG_STA_ASSOCIATED, &wl->flags) &&
	    (wl->conf.rx_streaming.always ||
	     test_bit(WL1271_FLAG_SOFT_GEMINI, &wl->flags)))
		ret = wl1271_set_rx_streaming(wl, true);
	else {
		ret = wl1271_set_rx_streaming(wl, false);
		/* don't cancel_work_sync since we might deadlock */
		del_timer_sync(&wl->rx_streaming_timer);
	}
out:
	return ret;
}

static void wl1271_rx_streaming_enable_work(struct work_struct *work)
{
	int ret;
	struct wl1271 *wl =
		container_of(work, struct wl1271, rx_streaming_enable_work);

	mutex_lock(&wl->mutex);

	if (test_bit(WL1271_FLAG_RX_STREAMING_STARTED, &wl->flags) ||
	    !test_bit(WL1271_FLAG_STA_ASSOCIATED, &wl->flags) ||
	    (!wl->conf.rx_streaming.always &&
	     !test_bit(WL1271_FLAG_SOFT_GEMINI, &wl->flags)))
		goto out;

	if (!wl->conf.rx_streaming.interval)
		goto out;

	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		goto out;

	ret = wl1271_set_rx_streaming(wl, true);
	if (ret < 0)
		goto out_sleep;

	/* stop it after some time of inactivity */
	mod_timer(&wl->rx_streaming_timer,
		  jiffies + msecs_to_jiffies(wl->conf.rx_streaming.duration));

out_sleep:
	wl1271_ps_elp_sleep(wl);
out:
	mutex_unlock(&wl->mutex);
}

static void wl1271_rx_streaming_disable_work(struct work_struct *work)
{
	int ret;
	struct wl1271 *wl =
		container_of(work, struct wl1271, rx_streaming_disable_work);

	mutex_lock(&wl->mutex);

	if (!test_bit(WL1271_FLAG_RX_STREAMING_STARTED, &wl->flags))
		goto out;

	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		goto out;

	ret = wl1271_set_rx_streaming(wl, false);
	if (ret)
		goto out_sleep;

out_sleep:
	wl1271_ps_elp_sleep(wl);
out:
	mutex_unlock(&wl->mutex);
}

static void wl1271_rx_streaming_timer(unsigned long data)
{
	struct wl1271 *wl = (struct wl1271 *)data;
	ieee80211_queue_work(wl->hw, &wl->rx_streaming_disable_work);
}

static void wl1271_conf_init(struct wl1271 *wl)
{

	/*
	 * This function applies the default configuration to the driver. This
	 * function is invoked upon driver load (spi probe.)
	 *
	 * The configuration is stored in a run-time structure in order to
	 * facilitate for run-time adjustment of any of the parameters. Making
	 * changes to the configuration structure will apply the new values on
	 * the next interface up (wl1271_op_start.)
	 */

	/* apply driver default configuration */
	memcpy(&wl->conf, &default_conf, sizeof(default_conf));

	/* Adjust settings according to optional module parameters */
	if (fwlog_param) {
		if (!strcmp(fwlog_param, "continuous")) {
			wl->conf.fwlog.mode = WL12XX_FWLOG_CONTINUOUS;
		} else if (!strcmp(fwlog_param, "ondemand")) {
			wl->conf.fwlog.mode = WL12XX_FWLOG_ON_DEMAND;
		} else if (!strcmp(fwlog_param, "dbgpins")) {
			wl->conf.fwlog.mode = WL12XX_FWLOG_CONTINUOUS;
			wl->conf.fwlog.output = WL12XX_FWLOG_OUTPUT_DBG_PINS;
		} else if (!strcmp(fwlog_param, "disable")) {
			wl->conf.fwlog.mem_blocks = 0;
			wl->conf.fwlog.output = WL12XX_FWLOG_OUTPUT_NONE;
		} else {
			wl1271_error("Unknown fwlog parameter %s", fwlog_param);
		}
	}

	wl->ref_clock = -1;
	if (fref_param) {
		if (!strcmp(fref_param, "19.2"))
			wl->ref_clock = WL12XX_REFCLOCK_19;
		else if (!strcmp(fref_param, "26"))
			wl->ref_clock = WL12XX_REFCLOCK_26;
		else if (!strcmp(fref_param, "26x"))
			wl->ref_clock = WL12XX_REFCLOCK_26_XTAL;
		else if (!strcmp(fref_param, "38.4"))
			wl->ref_clock = WL12XX_REFCLOCK_38;
		else if (!strcmp(fref_param, "38.4x"))
			wl->ref_clock = WL12XX_REFCLOCK_38_XTAL;
		else if (!strcmp(fref_param, "52"))
			wl->ref_clock = WL12XX_REFCLOCK_52;
		else
			wl1271_error("Invalid fref parameter %s", fref_param);
	}

	wl->tcxo_clock = -1;
	if (tcxo_param) {
		if (!strcmp(tcxo_param, "19.2"))
			wl->tcxo_clock = WL12XX_TCXOCLOCK_19_2;
		else if (!strcmp(tcxo_param, "26"))
			wl->tcxo_clock = WL12XX_TCXOCLOCK_26;
		else if (!strcmp(tcxo_param, "38.4"))
			wl->tcxo_clock = WL12XX_TCXOCLOCK_38_4;
		else if (!strcmp(tcxo_param, "52"))
			wl->tcxo_clock = WL12XX_TCXOCLOCK_52;
		else if (!strcmp(tcxo_param, "16.368"))
			wl->tcxo_clock = WL12XX_TCXOCLOCK_16_368;
		else if (!strcmp(tcxo_param, "32.736"))
			wl->tcxo_clock = WL12XX_TCXOCLOCK_32_736;
		else if (!strcmp(tcxo_param, "16.8"))
			wl->tcxo_clock = WL12XX_TCXOCLOCK_16_8;
		else if (!strcmp(tcxo_param, "33.6"))
			wl->tcxo_clock = WL12XX_TCXOCLOCK_33_6;
		else
			wl1271_error("Invalid tcxo parameter %s", tcxo_param);
	}
}

static int wl1271_plt_init(struct wl1271 *wl)
{
	struct conf_tx_ac_category *conf_ac;
	struct conf_tx_tid *conf_tid;
	int ret, i;

	if (wl->chip.id == CHIP_ID_1283_PG20)
		ret = wl128x_cmd_general_parms(wl);
	else
		ret = wl1271_cmd_general_parms(wl);
	if (ret < 0)
		return ret;

	if (wl->chip.id == CHIP_ID_1283_PG20)
		ret = wl128x_cmd_radio_parms(wl);
	else
		ret = wl1271_cmd_radio_parms(wl);
	if (ret < 0)
		return ret;

	if (wl->chip.id != CHIP_ID_1283_PG20) {
		ret = wl1271_cmd_ext_radio_parms(wl);
		if (ret < 0)
			return ret;
	}
	if (ret < 0)
		return ret;

	/* Chip-specific initializations */
	ret = wl1271_chip_specific_init(wl);
	if (ret < 0)
		return ret;

	ret = wl1271_sta_init_templates_config(wl);
	if (ret < 0)
		return ret;

	ret = wl1271_acx_init_mem_config(wl);
	if (ret < 0)
		return ret;

	/* PHY layer config */
	ret = wl1271_init_phy_config(wl);
	if (ret < 0)
		goto out_free_memmap;

	ret = wl1271_acx_dco_itrim_params(wl);
	if (ret < 0)
		goto out_free_memmap;

	/* Initialize connection monitoring thresholds */
	ret = wl1271_acx_conn_monit_params(wl, false);
	if (ret < 0)
		goto out_free_memmap;

	/* Bluetooth WLAN coexistence */
	ret = wl1271_init_pta(wl);
	if (ret < 0)
		goto out_free_memmap;

	/* FM WLAN coexistence */
	ret = wl1271_acx_fm_coex(wl);
	if (ret < 0)
		goto out_free_memmap;

	/* Energy detection */
	ret = wl1271_init_energy_detection(wl);
	if (ret < 0)
		goto out_free_memmap;

	ret = wl1271_acx_mem_cfg(wl);
	if (ret < 0)
		goto out_free_memmap;

	/* Default fragmentation threshold */
	ret = wl1271_acx_frag_threshold(wl, wl->conf.tx.frag_threshold);
	if (ret < 0)
		goto out_free_memmap;

	/* Default TID/AC configuration */
	BUG_ON(wl->conf.tx.tid_conf_count != wl->conf.tx.ac_conf_count);
	for (i = 0; i < wl->conf.tx.tid_conf_count; i++) {
		conf_ac = &wl->conf.tx.ac_conf[i];
		ret = wl1271_acx_ac_cfg(wl, conf_ac->ac, conf_ac->cw_min,
					conf_ac->cw_max, conf_ac->aifsn,
					conf_ac->tx_op_limit);
		if (ret < 0)
			goto out_free_memmap;

		conf_tid = &wl->conf.tx.tid_conf[i];
		ret = wl1271_acx_tid_cfg(wl, conf_tid->queue_id,
					 conf_tid->channel_type,
					 conf_tid->tsid,
					 conf_tid->ps_scheme,
					 conf_tid->ack_policy,
					 conf_tid->apsd_conf[0],
					 conf_tid->apsd_conf[1]);
		if (ret < 0)
			goto out_free_memmap;
	}

	/* Enable data path */
	ret = wl1271_cmd_data_path(wl, 1);
	if (ret < 0)
		goto out_free_memmap;

	/* Configure for CAM power saving (ie. always active) */
	ret = wl1271_acx_sleep_auth(wl, WL1271_PSM_CAM);
	if (ret < 0)
		goto out_free_memmap;

	/* configure PM */
	ret = wl1271_acx_pm_config(wl);
	if (ret < 0)
		goto out_free_memmap;

	return 0;

 out_free_memmap:
	kfree(wl->target_mem_map);
	wl->target_mem_map = NULL;

	return ret;
}

static void wl1271_irq_ps_regulate_link(struct wl1271 *wl, u8 hlid, u8 tx_pkts)
{
	bool fw_ps, single_sta;

	/* only regulate station links */
	if (hlid < WL1271_AP_STA_HLID_START)
		return;

	fw_ps = test_bit(hlid, (unsigned long *)&wl->ap_fw_ps_map);
	single_sta = (wl->active_sta_count == 1);

	/*
	 * Wake up from high level PS if the STA is asleep with too little
	 * packets in FW or if the STA is awake.
	 */
	if (!fw_ps || tx_pkts < WL1271_PS_STA_MAX_PACKETS)
		wl1271_ps_link_end(wl, hlid);

	/*
	 * Start high-level PS if the STA is asleep with enough blocks in FW.
	 * Make an exception if this is the only connected station. In this
	 * case FW-memory congestion is not a problem.
	 */
	else if (!single_sta && fw_ps && tx_pkts >= WL1271_PS_STA_MAX_PACKETS)
		wl1271_ps_link_start(wl, hlid, true);
}

bool wl1271_is_active_sta(struct wl1271 *wl, u8 hlid)
{
	int id;

	/* global/broadcast "stations" are always active */
	if (hlid < WL1271_AP_STA_HLID_START)
		return true;

	id = hlid - WL1271_AP_STA_HLID_START;
	return test_bit(id, wl->ap_hlid_map);
}

static void wl1271_irq_update_links_status(struct wl1271 *wl,
					   struct wl1271_fw_status *status)
{
	u32 cur_fw_ps_map;
	u8 hlid, cnt;

	/* TODO: also use link_fast_bitmap here */

	cur_fw_ps_map = le32_to_cpu(status->link_ps_bitmap);
	if (wl->ap_fw_ps_map != cur_fw_ps_map) {
		wl1271_debug(DEBUG_PSM,
			     "link ps prev 0x%x cur 0x%x changed 0x%x",
			     wl->ap_fw_ps_map, cur_fw_ps_map,
			     wl->ap_fw_ps_map ^ cur_fw_ps_map);

		wl->ap_fw_ps_map = cur_fw_ps_map;
	}

	for (hlid = WL1271_AP_STA_HLID_START; hlid < AP_MAX_LINKS; hlid++) {
		if (!wl1271_is_active_sta(wl, hlid))
			continue;

		cnt = status->tx_lnk_free_pkts[hlid] -
		      wl->links[hlid].prev_freed_pkts;

		wl->links[hlid].prev_freed_pkts =
			status->tx_lnk_free_pkts[hlid];
		wl->links[hlid].allocated_pkts -= cnt;

		wl1271_irq_ps_regulate_link(wl, hlid,
					    wl->links[hlid].allocated_pkts);
	}
}

static void wl1271_fw_status(struct wl1271 *wl,
			     struct wl1271_fw_status *status)
{
	struct timespec ts;
	u32 old_tx_blk_count = wl->tx_blocks_available;
	int avail, freed_blocks;
	int i;

	wl1271_raw_read(wl, FW_STATUS_ADDR, status, sizeof(*status), false);

	wl1271_debug(DEBUG_IRQ, "intr: 0x%x (fw_rx_counter = %d, "
		     "drv_rx_counter = %d, tx_results_counter = %d)",
		     status->intr,
		     status->fw_rx_counter,
		     status->drv_rx_counter,
		     status->tx_results_counter);

	for (i = 0; i < NUM_TX_QUEUES; i++) {
		/* prevent wrap-around in freed-packets counter */
		wl->tx_allocated_pkts[i] -=
				(status->tx_released_pkts[i] -
				wl->tx_pkts_freed[i] + 256) % 256;

		wl->tx_pkts_freed[i] = status->tx_released_pkts[i];
	}

	/* prevent wrap-around in total blocks counter */
	if (likely(wl->tx_blocks_freed <=
		   le32_to_cpu(status->total_released_blks)))
		freed_blocks = le32_to_cpu(status->total_released_blks) -
			       wl->tx_blocks_freed;
	else
		freed_blocks = 0x100000000LL - wl->tx_blocks_freed +
			       le32_to_cpu(status->total_released_blks);

	wl->tx_blocks_freed = le32_to_cpu(status->total_released_blks);

	wl->tx_allocated_blocks -= freed_blocks;

	avail = status->tx_total - wl->tx_allocated_blocks;

	/*
	 * The FW might change the total number of TX memblocks before
	 * we get a notification about blocks being released. Thus, the
	 * available blocks calculation might yield a temporary result
	 * which is lower than the actual available blocks. Keeping in
	 * mind that only blocks that were allocated can be moved from
	 * TX to RX, tx_blocks_available should never decrease here.
	 */
	wl->tx_blocks_available = max((int)wl->tx_blocks_available,
				      avail);

	/* if more blocks are available now, tx work can be scheduled */
	if (wl->tx_blocks_available > old_tx_blk_count)
		clear_bit(WL1271_FLAG_FW_TX_BUSY, &wl->flags);

	wl1271_debug(DEBUG_TX, "avail_blocks: %d allocated_blks: %d",
		     wl->tx_blocks_available, wl->tx_allocated_blocks);

	/* for AP update num of allocated TX blocks per link and ps status */
	if (wl->bss_type == BSS_TYPE_AP_BSS)
		wl1271_irq_update_links_status(wl, status);

	/* update the host-chipset time offset */
	getnstimeofday(&ts);
	wl->time_offset = (timespec_to_ns(&ts) >> 10) -
		(s64)le32_to_cpu(status->fw_localtime);
}

static void wl1271_flush_deferred_work(struct wl1271 *wl)
{
	struct sk_buff *skb;

	/* Pass all received frames to the network stack */
	while ((skb = skb_dequeue(&wl->deferred_rx_queue)))
		ieee80211_rx_ni(wl->hw, skb);

	/* Return sent skbs to the network stack */
	while ((skb = skb_dequeue(&wl->deferred_tx_queue)))
		ieee80211_tx_status(wl->hw, skb);
}

static void wl1271_netstack_work(struct work_struct *work)
{
	struct wl1271 *wl =
		container_of(work, struct wl1271, netstack_work);

	do {
		wl1271_flush_deferred_work(wl);
	} while (skb_queue_len(&wl->deferred_rx_queue));
}

#define WL1271_IRQ_MAX_LOOPS 256

irqreturn_t wl1271_irq(int irq, void *cookie)
{
	int ret;
	u32 intr;
	int loopcount = WL1271_IRQ_MAX_LOOPS;
	struct wl1271 *wl = (struct wl1271 *)cookie;
	bool done = false;
	unsigned int defer_count;
	unsigned long flags;

	/* TX might be handled here, avoid redundant work */
	set_bit(WL1271_FLAG_TX_PENDING, &wl->flags);
	cancel_work_sync(&wl->tx_work);

	/*
	 * In case edge triggered interrupt must be used, we cannot iterate
	 * more than once without introducing race conditions with the hardirq.
	 */
	if (wl->platform_quirks & WL12XX_PLATFORM_QUIRK_EDGE_IRQ)
		loopcount = 1;

	mutex_lock(&wl->mutex);

	wl1271_debug(DEBUG_IRQ, "IRQ work");

	if (unlikely(wl->state == WL1271_STATE_OFF))
		goto out;

	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		goto out;

	while (!done && loopcount--) {
		/*
		 * In order to avoid a race with the hardirq, clear the flag
		 * before acknowledging the chip. Since the mutex is held,
		 * wl1271_ps_elp_wakeup cannot be called concurrently.
		 */
		clear_bit(WL1271_FLAG_IRQ_RUNNING, &wl->flags);
		smp_mb__after_clear_bit();

		wl1271_fw_status(wl, wl->fw_status);
		intr = le32_to_cpu(wl->fw_status->intr);
		intr &= WL1271_INTR_MASK;
		if (!intr) {
			done = true;
			continue;
		}

		if (unlikely(intr & WL1271_ACX_INTR_WATCHDOG)) {
			wl1271_error("watchdog interrupt received! "
				     "starting recovery.");
			wl1271_queue_recovery_work(wl);

			/* restarting the chip. ignore any other interrupt. */
			goto out;
		}

		if (likely(intr & WL1271_ACX_INTR_DATA)) {
			wl1271_debug(DEBUG_IRQ, "WL1271_ACX_INTR_DATA");

			wl1271_rx(wl, wl->fw_status);

			/* Check if any tx blocks were freed */
			spin_lock_irqsave(&wl->wl_lock, flags);
			if (!test_bit(WL1271_FLAG_FW_TX_BUSY, &wl->flags) &&
			    wl1271_tx_total_queue_count(wl) > 0) {
				spin_unlock_irqrestore(&wl->wl_lock, flags);
				/*
				 * In order to avoid starvation of the TX path,
				 * call the work function directly.
				 */
				wl1271_tx_work_locked(wl);
			} else {
				spin_unlock_irqrestore(&wl->wl_lock, flags);
			}

			/* check for tx results */
			if (wl->fw_status->tx_results_counter !=
			    (wl->tx_results_count & 0xff))
				wl1271_tx_complete(wl);

			/* Make sure the deferred queues don't get too long */
			defer_count = skb_queue_len(&wl->deferred_tx_queue) +
				      skb_queue_len(&wl->deferred_rx_queue);
			if (defer_count > WL1271_DEFERRED_QUEUE_LIMIT)
				wl1271_flush_deferred_work(wl);
		}

		if (intr & WL1271_ACX_INTR_EVENT_A) {
			wl1271_debug(DEBUG_IRQ, "WL1271_ACX_INTR_EVENT_A");
			wl1271_event_handle(wl, 0);
		}

		if (intr & WL1271_ACX_INTR_EVENT_B) {
			wl1271_debug(DEBUG_IRQ, "WL1271_ACX_INTR_EVENT_B");
			wl1271_event_handle(wl, 1);
		}

		if (intr & WL1271_ACX_INTR_INIT_COMPLETE)
			wl1271_debug(DEBUG_IRQ,
				     "WL1271_ACX_INTR_INIT_COMPLETE");

		if (intr & WL1271_ACX_INTR_HW_AVAILABLE)
			wl1271_debug(DEBUG_IRQ, "WL1271_ACX_INTR_HW_AVAILABLE");
	}

	wl1271_ps_elp_sleep(wl);

out:
	spin_lock_irqsave(&wl->wl_lock, flags);
	/* In case TX was not handled here, queue TX work */
	clear_bit(WL1271_FLAG_TX_PENDING, &wl->flags);
	if (!test_bit(WL1271_FLAG_FW_TX_BUSY, &wl->flags) &&
	    wl1271_tx_total_queue_count(wl) > 0)
		ieee80211_queue_work(wl->hw, &wl->tx_work);

#ifdef CONFIG_HAS_WAKELOCK
	if (test_and_clear_bit(WL1271_FLAG_WAKE_LOCK, &wl->flags))
		wake_unlock(&wl->wake_lock);
#endif
	spin_unlock_irqrestore(&wl->wl_lock, flags);

	mutex_unlock(&wl->mutex);

	return IRQ_HANDLED;
}
EXPORT_SYMBOL_GPL(wl1271_irq);

static int wl12xx_fetch_firmware(struct wl1271 *wl, bool plt)
{
	const struct firmware *fw;
	const char *fw_name;
	int ret;

	if (plt) {
		if (wl->fw_type == WL12XX_FW_TYPE_PLT)
			return 0;

		if (wl->chip.id == CHIP_ID_1283_PG20)
			fw_name = WL128X_PLT_FW_NAME;
		else
			fw_name	= WL1271_PLT_FW_NAME;
	} else {
		if (wl->fw_type == WL12XX_FW_TYPE_NORMAL)
			return 0;

		if (wl->chip.id == CHIP_ID_1283_PG20)
			fw_name = WL128X_FW_NAME;
		else
			fw_name	= WL1271_FW_NAME;
	}

	wl1271_debug(DEBUG_BOOT, "booting firmware %s", fw_name);

	ret = request_firmware(&fw, fw_name, wl1271_wl_to_dev(wl));

	if (ret < 0) {
		wl1271_error("could not get firmware: %d", ret);
		return ret;
	}

	if (fw->size % 4) {
		wl1271_error("firmware size is not multiple of 32 bits: %zu",
			     fw->size);
		ret = -EILSEQ;
		goto out;
	}

	vfree(wl->fw);
	wl->fw_type = WL12XX_FW_TYPE_NONE;
	wl->fw_len = fw->size;
	wl->fw = vmalloc(wl->fw_len);

	if (!wl->fw) {
		wl1271_error("could not allocate memory for the firmware");
		ret = -ENOMEM;
		goto out;
	}

	memcpy(wl->fw, fw->data, wl->fw_len);
	ret = 0;
	if (plt)
		wl->fw_type = WL12XX_FW_TYPE_PLT;
	else
		wl->fw_type = WL12XX_FW_TYPE_NORMAL;

out:
	release_firmware(fw);

	return ret;
}

static int wl1271_fetch_nvs(struct wl1271 *wl)
{
	const struct firmware *fw;
	int ret;

	ret = request_firmware(&fw, WL12XX_NVS_NAME, wl1271_wl_to_dev(wl));

	if (ret < 0) {
		wl1271_error("could not get nvs file: %d", ret);
		return ret;
	}

	wl->nvs = kmemdup(fw->data, fw->size, GFP_KERNEL);

	if (!wl->nvs) {
		wl1271_error("could not allocate memory for the nvs file");
		ret = -ENOMEM;
		goto out;
	}

	wl->nvs_len = fw->size;

out:
	release_firmware(fw);

	return ret;
}

void wl1271_queue_recovery_work(struct wl1271 *wl)
{
	if (!test_bit(WL1271_FLAG_RECOVERY_IN_PROGRESS, &wl->flags))
		ieee80211_queue_work(wl->hw, &wl->recovery_work);
}

size_t wl12xx_copy_fwlog(struct wl1271 *wl, u8 *memblock, size_t maxlen)
{
	size_t len = 0;

	/* The FW log is a length-value list, find where the log end */
	while (len < maxlen) {
		if (memblock[len] == 0)
			break;
		if (len + memblock[len] + 1 > maxlen)
			break;
		len += memblock[len] + 1;
	}

	/* Make sure we have enough room */
	len = min(len, (size_t)(PAGE_SIZE - wl->fwlog_size));

	/* Fill the FW log file, consumed by the sysfs fwlog entry */
	memcpy(wl->fwlog + wl->fwlog_size, memblock, len);
	wl->fwlog_size += len;

	return len;
}

static void wl12xx_read_fwlog_panic(struct wl1271 *wl)
{
	u32 addr;
	u32 first_addr;
	u8 *block;

	if ((wl->conf.fwlog.mode != WL12XX_FWLOG_ON_DEMAND) ||
	    (wl->conf.fwlog.mem_blocks == 0))
		return;

	wl1271_info("Reading FW panic log");

	block = kmalloc(WL12XX_HW_BLOCK_SIZE, GFP_KERNEL);
	if (!block)
		return;

	/*
	 * Make sure the chip is awake and the logger isn't active.
	 * This might fail if the firmware hanged.
	 */
	if (!wl1271_ps_elp_wakeup(wl))
		wl12xx_cmd_stop_fwlog(wl);

	/* Read the first memory block address */
	wl1271_fw_status(wl, wl->fw_status);
	first_addr = __le32_to_cpu(wl->fw_status->log_start_addr);
	printk("First address of fwlogger = 0x%x\n", first_addr);
	if (!first_addr)
		goto out;

	/* Traverse the memory blocks linked list */
	addr = first_addr;
	do {
		memset(block, 0, WL12XX_HW_BLOCK_SIZE);
		wl1271_read_hwaddr(wl, addr, block, WL12XX_HW_BLOCK_SIZE,
				   false);

		/*
		 * Memory blocks are linked to one another. The first 4 bytes
		 * of each memory block hold the hardware address of the next
		 * one. The last memory block points to the first one.
		 */
		addr = __le32_to_cpup((__le32 *)block);
		if (!wl12xx_copy_fwlog(wl, block + sizeof(addr),
				       WL12XX_HW_BLOCK_SIZE - sizeof(addr)))
			break;
	} while (addr && (addr != first_addr));

	wake_up_interruptible(&wl->fwlog_waitq);

out:
	kfree(block);
}

static void wl1271_recovery_work(struct work_struct *work)
{
	struct wl1271 *wl =
		container_of(work, struct wl1271, recovery_work);

	mutex_lock(&wl->mutex);

	if (wl->state != WL1271_STATE_ON)
		goto out;

	/* Avoid a recursive recovery */
	set_bit(WL1271_FLAG_RECOVERY_IN_PROGRESS, &wl->flags);

	/* Only read FW crash log if debugging. */
	if (wl12xx_debug_level)
		wl12xx_read_fwlog_panic(wl);

	wl1271_info("Hardware recovery in progress. FW ver: %s pc: 0x%x",
		    wl->chip.fw_ver_str, wl1271_read32(wl, SCR_PAD4));

	BUG_ON(bug_on_recovery);

	/*
	 * Advance security sequence number to overcome potential progress
	 * in the firmware during recovery. This doens't hurt if the network is
	 * not encrypted.
	 */
	if (test_bit(WL1271_FLAG_STA_ASSOCIATED, &wl->flags) ||
	    test_bit(WL1271_FLAG_AP_STARTED, &wl->flags))
		wl->tx_security_seq += WL1271_TX_SQN_POST_RECOVERY_PADDING;

	/* Prevent spurious TX during FW restart */
	ieee80211_stop_queues(wl->hw);

	if (wl->sched_scanning) {
		ieee80211_sched_scan_stopped(wl->hw);
		wl->sched_scanning = false;
	}

	/* reboot the chipset */
	__wl1271_op_remove_interface(wl, false);

	clear_bit(WL1271_FLAG_RECOVERY_IN_PROGRESS, &wl->flags);

	ieee80211_restart_hw(wl->hw);

	/*
	 * Its safe to enable TX now - the queues are stopped after a request
	 * to restart the HW.
	 */
	ieee80211_wake_queues(wl->hw);

out:
	mutex_unlock(&wl->mutex);
}

static void wl1271_fw_wakeup(struct wl1271 *wl)
{
	u32 elp_reg;

	elp_reg = ELPCTRL_WAKE_UP;
	wl1271_raw_write32(wl, HW_ACCESS_ELP_CTRL_REG_ADDR, elp_reg);
}

static int wl1271_setup(struct wl1271 *wl)
{
	wl->fw_status = kmalloc(sizeof(*wl->fw_status), GFP_KERNEL);
	if (!wl->fw_status)
		return -ENOMEM;

	wl->tx_res_if = kmalloc(sizeof(*wl->tx_res_if), GFP_KERNEL);
	if (!wl->tx_res_if) {
		kfree(wl->fw_status);
		return -ENOMEM;
	}

	return 0;
}

static int wl12xx_chip_wakeup(struct wl1271 *wl, bool plt)
{
	struct wl1271_partition_set partition;
	int ret = 0;

	msleep(WL1271_PRE_POWER_ON_SLEEP);
	ret = wl1271_power_on(wl);
	if (ret < 0)
		goto out;
	msleep(WL1271_POWER_ON_SLEEP);
	wl1271_io_reset(wl);
	wl1271_io_init(wl);

	/* We don't need a real memory partition here, because we only want
	 * to use the registers at this point. */
	memset(&partition, 0, sizeof(partition));
	partition.reg.start = REGISTERS_BASE;
	partition.reg.size = REGISTERS_DOWN_SIZE;
	wl1271_set_partition(wl, &partition);

	/* ELP module wake up */
	wl1271_fw_wakeup(wl);

	/* whal_FwCtrl_BootSm() */

	/* 0. read chip id from CHIP_ID */
	wl->chip.id = wl1271_read32(wl, CHIP_ID_B);

	/* 1. check if chip id is valid */

	switch (wl->chip.id) {
	case CHIP_ID_1271_PG10:
		wl1271_warning("chip id 0x%x (1271 PG10) support is obsolete",
			       wl->chip.id);

		ret = wl1271_setup(wl);
		if (ret < 0)
			goto out;
		break;
	case CHIP_ID_1271_PG20:
		wl1271_debug(DEBUG_BOOT, "chip id 0x%x (1271 PG20)",
			     wl->chip.id);

		/*
		 * 'end-of-transaction flag' and 'LPD mode flag'
		 * should be set in wl127x AP mode only
		 */
		if (wl->bss_type == BSS_TYPE_AP_BSS)
			wl->quirks |= (WL12XX_QUIRK_END_OF_TRANSACTION |
				       WL12XX_QUIRK_LPD_MODE);

		ret = wl1271_setup(wl);
		if (ret < 0)
			goto out;
		break;
	case CHIP_ID_1283_PG20:
		wl1271_debug(DEBUG_BOOT, "chip id 0x%x (1283 PG20)",
			     wl->chip.id);

		ret = wl1271_setup(wl);
		if (ret < 0)
			goto out;

		if (wl1271_set_block_size(wl))
			wl->quirks |= WL12XX_QUIRK_BLOCKSIZE_ALIGNMENT;
		break;
	case CHIP_ID_1283_PG10:
	default:
		wl1271_warning("unsupported chip id: 0x%x", wl->chip.id);
		ret = -ENODEV;
		goto out;
	}

	ret = wl12xx_fetch_firmware(wl, plt);
	if (ret < 0)
		goto out;

	/* No NVS from netlink, try to get it from the filesystem */
	if (wl->nvs == NULL) {
		ret = wl1271_fetch_nvs(wl);
		if (ret < 0)
			goto out;
	}

out:
	return ret;
}

int wl1271_plt_start(struct wl1271 *wl)
{
	int retries = WL1271_BOOT_RETRIES;
	struct wiphy *wiphy = wl->hw->wiphy;
	int ret;

	mutex_lock(&wl->mutex);

	wl1271_notice("power up");

	if (wl->state != WL1271_STATE_OFF) {
		wl1271_error("cannot go into PLT state because not "
			     "in off state: %d", wl->state);
		ret = -EBUSY;
		goto out;
	}

	wl->bss_type = BSS_TYPE_STA_BSS;

	while (retries) {
		retries--;
		ret = wl12xx_chip_wakeup(wl, true);
		if (ret < 0)
			goto power_off;

		ret = wl1271_boot(wl);
		if (ret < 0)
			goto power_off;

		ret = wl1271_plt_init(wl);
		if (ret < 0)
			goto irq_disable;

		wl->state = WL1271_STATE_PLT;
		wl1271_notice("firmware booted in PLT mode (%s)",
			      wl->chip.fw_ver_str);

		/* update hw/fw version info in wiphy struct */
		wiphy->hw_version = wl->chip.id;
		strncpy(wiphy->fw_version, wl->chip.fw_ver_str,
			sizeof(wiphy->fw_version));

		goto out;

irq_disable:
		mutex_unlock(&wl->mutex);
		/* Unlocking the mutex in the middle of handling is
		   inherently unsafe. In this case we deem it safe to do,
		   because we need to let any possibly pending IRQ out of
		   the system (and while we are WL1271_STATE_OFF the IRQ
		   work function will not do anything.) Also, any other
		   possible concurrent operations will fail due to the
		   current state, hence the wl1271 struct should be safe. */
		wl1271_disable_interrupts(wl);
		wl1271_flush_deferred_work(wl);
		cancel_work_sync(&wl->netstack_work);
		mutex_lock(&wl->mutex);
power_off:
		wl1271_power_off(wl);
	}

	wl1271_error("firmware boot in PLT mode failed despite %d retries",
		     WL1271_BOOT_RETRIES);
out:
	mutex_unlock(&wl->mutex);

	return ret;
}

static int __wl1271_plt_stop(struct wl1271 *wl)
{
	int ret = 0;

	wl1271_notice("power down");

	if (wl->state != WL1271_STATE_PLT) {
		wl1271_error("cannot power down because not in PLT "
			     "state: %d", wl->state);
		ret = -EBUSY;
		goto out;
	}

	mutex_unlock(&wl->mutex);
	wl1271_disable_interrupts(wl);
	mutex_lock(&wl->mutex);

	wl1271_power_off(wl);

	wl->state = WL1271_STATE_OFF;
	wl->rx_counter = 0;

	mutex_unlock(&wl->mutex);
	wl1271_flush_deferred_work(wl);
	cancel_work_sync(&wl->netstack_work);
	cancel_work_sync(&wl->recovery_work);
	mutex_lock(&wl->mutex);
out:
	return ret;
}

int wl1271_plt_stop(struct wl1271 *wl)
{
	int ret;

	mutex_lock(&wl->mutex);
	ret = __wl1271_plt_stop(wl);
	mutex_unlock(&wl->mutex);
	return ret;
}

static void wl1271_op_tx(struct ieee80211_hw *hw, struct sk_buff *skb)
{
	struct wl1271 *wl = hw->priv;
	unsigned long flags;
	int q, mapping;
	u8 hlid = 0;

	mapping = skb_get_queue_mapping(skb);
	q = wl1271_tx_get_queue(mapping);

	if (wl->bss_type == BSS_TYPE_AP_BSS)
		hlid = wl1271_tx_get_hlid(wl, skb);

	spin_lock_irqsave(&wl->wl_lock, flags);

	/* queue the packet */
	if (wl->bss_type == BSS_TYPE_AP_BSS) {
		if (!wl1271_is_active_sta(wl, hlid)) {
			wl1271_debug(DEBUG_TX, "DROP skb hlid %d q %d",
				     hlid, q);
			dev_kfree_skb(skb);
			goto out;
		}

		wl1271_debug(DEBUG_TX, "queue skb hlid %d q %d", hlid, q);
		skb_queue_tail(&wl->links[hlid].tx_queue[q], skb);
	} else {
		skb_queue_tail(&wl->tx_queue[q], skb);
	}

	wl->tx_queue_count[q]++;

	/*
	 * The workqueue is slow to process the tx_queue and we need stop
	 * the queue here, otherwise the queue will get too long.
	 */
	if (wl->tx_queue_count[q] >= WL1271_TX_QUEUE_HIGH_WATERMARK) {
		wl1271_debug(DEBUG_TX, "op_tx: stopping queues for q %d", q);
		ieee80211_stop_queue(wl->hw, mapping);
		set_bit(q, &wl->stopped_queues_map);
	}

	/*
	 * The chip specific setup must run before the first TX packet -
	 * before that, the tx_work will not be initialized!
	 */

	if (!test_bit(WL1271_FLAG_FW_TX_BUSY, &wl->flags) &&
	    !test_bit(WL1271_FLAG_TX_PENDING, &wl->flags))
		ieee80211_queue_work(wl->hw, &wl->tx_work);

out:
	spin_unlock_irqrestore(&wl->wl_lock, flags);
}

int wl1271_tx_dummy_packet(struct wl1271 *wl)
{
	unsigned long flags;
	int q;

	/* no need to queue a new dummy packet if one is already pending */
	if (test_bit(WL1271_FLAG_DUMMY_PACKET_PENDING, &wl->flags))
		return 0;

	q = wl1271_tx_get_queue(skb_get_queue_mapping(wl->dummy_packet));

	spin_lock_irqsave(&wl->wl_lock, flags);
	set_bit(WL1271_FLAG_DUMMY_PACKET_PENDING, &wl->flags);
	wl->tx_queue_count[q]++;
	spin_unlock_irqrestore(&wl->wl_lock, flags);

	/* The FW is low on RX memory blocks, so send the dummy packet asap */
	if (!test_bit(WL1271_FLAG_FW_TX_BUSY, &wl->flags))
		wl1271_tx_work_locked(wl);

	/*
	 * If the FW TX is busy, TX work will be scheduled by the threaded
	 * interrupt handler function
	 */
	return 0;
}

/*
 * The size of the dummy packet should be at least 1400 bytes. However, in
 * order to minimize the number of bus transactions, aligning it to 512 bytes
 * boundaries could be beneficial, performance wise
 */
#define TOTAL_TX_DUMMY_PACKET_SIZE (ALIGN(1400, 512))

static struct sk_buff *wl12xx_alloc_dummy_packet(struct wl1271 *wl)
{
	struct sk_buff *skb;
	struct ieee80211_hdr_3addr *hdr;
	unsigned int dummy_packet_size;

	dummy_packet_size = TOTAL_TX_DUMMY_PACKET_SIZE -
			    sizeof(struct wl1271_tx_hw_descr) - sizeof(*hdr);

	skb = dev_alloc_skb(TOTAL_TX_DUMMY_PACKET_SIZE);
	if (!skb) {
		wl1271_warning("Failed to allocate a dummy packet skb");
		return NULL;
	}

	skb_reserve(skb, sizeof(struct wl1271_tx_hw_descr));

	hdr = (struct ieee80211_hdr_3addr *) skb_put(skb, sizeof(*hdr));
	memset(hdr, 0, sizeof(*hdr));
	hdr->frame_control = cpu_to_le16(IEEE80211_FTYPE_DATA |
					 IEEE80211_STYPE_NULLFUNC |
					 IEEE80211_FCTL_TODS);

	memset(skb_put(skb, dummy_packet_size), 0, dummy_packet_size);

	/* Dummy packets require the TID to be management */
	skb->priority = WL1271_TID_MGMT;

	/* Initialize all fields that might be used */
	skb_set_queue_mapping(skb, 0);
	memset(IEEE80211_SKB_CB(skb), 0, sizeof(struct ieee80211_tx_info));

	return skb;
}


static struct notifier_block wl1271_dev_notifier = {
	.notifier_call = wl1271_dev_notify,
};

int wl1271_validate_wowlan_pattern(struct cfg80211_wowlan_trig_pkt_pattern *p)
{
	if (p->pattern_len > WL1271_RX_DATA_FILTER_MAX_PATTERN_SIZE) {
		wl1271_warning("WoWLAN pattern too big");
		return -E2BIG;
	}

	if (!p->mask) {
		wl1271_warning("No mask in WoWLAN pattern");
		return -EINVAL;
	}

	return 0;
}

int wl1271_build_rx_filter_field(struct wl12xx_rx_data_filter_field *field,
				 u8 *pattern, u8 len, u8 offset,
				 u8 *bitmask, u8 flags)
{
	u8 *mask;
	int i;

	field->flags = flags | WL1271_RX_DATA_FILTER_FLAG_MASK;

	/* Not using the capability to set offset within an RX filter field.
	 * The offset param is used to access pattern and bitmask.
	 */
	field->offset = 0;
	field->len = len;
	memcpy(field->pattern, &pattern[offset], len);

	/* Translate the WowLAN bitmask (in bits) to the FW RX filter field
	   mask which is in bytes */

	mask = field->pattern + len;

	for (i = offset; i < (offset+len); i++) {
		if (test_bit(i, (unsigned long *)bitmask))
			*mask = 0xFF;

		mask++;
	}

	return sizeof(*field) + len + (bitmask ? len : 0);
}

/* Allocates an RX filter returned through f
   which needs to be freed using kfree() */
int wl1271_convert_wowlan_pattern_to_rx_filter(
	struct cfg80211_wowlan_trig_pkt_pattern *p,
	struct wl12xx_rx_data_filter **f)
{
	int filter_size, num_fields, fields_size;
	int first_field_size;
	int ret = 0;
	struct wl12xx_rx_data_filter_field *field;
	struct wl12xx_rx_data_filter *filter;

	/* If pattern is longer then the ETHERNET header we split it into
	 * 2 fields in the rx filter as you can't have a single
	 * field across ETH header boundary. The first field will filter
	 * anything in the ETH header and the 2nd one from the IP header.
	 * Each field will contain pattern bytes and mask bytes
	 */
	if (p->pattern_len > WL1271_RX_DATA_FILTER_ETH_HEADER_SIZE)
		num_fields = 2;
	else
		num_fields = 1;

	fields_size = (sizeof(*field) * num_fields) + (2 * p->pattern_len);
	filter_size = sizeof(*filter) + fields_size;

	filter = kzalloc(filter_size, GFP_KERNEL);
	if (!filter) {
		wl1271_warning("Failed to alloc rx filter");
		ret = -ENOMEM;
		goto err;
	}

	field = &filter->fields[0];
	first_field_size = wl1271_build_rx_filter_field(field,
			p->pattern,
			min(p->pattern_len,
			    WL1271_RX_DATA_FILTER_ETH_HEADER_SIZE),
			0,
			p->mask,
			WL1271_RX_DATA_FILTER_FLAG_ETHERNET_HEADER);

	field = (struct wl12xx_rx_data_filter_field *)
		  (((u8 *)filter->fields) + first_field_size);

	if (num_fields > 1) {
		wl1271_build_rx_filter_field(field,
		     p->pattern,
		     p->pattern_len - WL1271_RX_DATA_FILTER_ETH_HEADER_SIZE,
		     WL1271_RX_DATA_FILTER_ETH_HEADER_SIZE,
		     p->mask,
		     WL1271_RX_DATA_FILTER_FLAG_IP_HEADER);
	}

	filter->action = FILTER_SIGNAL;
	filter->num_fields = num_fields;
	filter->fields_size = fields_size;

	*f = filter;
	return 0;

err:
	kfree(filter);
	*f = NULL;

	return ret;
}

static int wl1271_configure_wowlan(struct wl1271 *wl,
				   struct cfg80211_wowlan *wow)
{
	int i, ret;

	if (!wow) {
		wl1271_rx_data_filtering_enable(wl, 0, FILTER_SIGNAL);
		wl1271_rx_data_filters_clear_all(wl);
		return 0;
	}

	WARN_ON(wow->n_patterns > WL1271_MAX_RX_DATA_FILTERS);
	if (wow->any || !wow->n_patterns)
		return 0;

	/* Translate WoWLAN patterns into filters */
	for (i = 0; i < wow->n_patterns; i++) {
		struct cfg80211_wowlan_trig_pkt_pattern *p;
		struct wl12xx_rx_data_filter *filter = NULL;

		p = &wow->patterns[i];

		ret = wl1271_validate_wowlan_pattern(p);
		if (ret) {
			wl1271_warning("validate_wowlan_pattern "
				       "failed (%d)", ret);
			goto out;
		}

		ret = wl1271_convert_wowlan_pattern_to_rx_filter(p, &filter);
		if (ret) {
			wl1271_warning("convert_wowlan_pattern_to_rx_filter "
				       "failed (%d)", ret);
			goto out;
		}

		ret = wl1271_rx_data_filter_enable(wl, i, 1, filter);

		kfree(filter);
		if (ret) {
			wl1271_warning("rx_data_filter_enable "
				       " failed (%d)", ret);
			goto out;
		}
	}

	ret = wl1271_rx_data_filtering_enable(wl, 1, FILTER_DROP);
	if (ret) {
		wl1271_warning("rx_data_filtering_enable failed (%d)", ret);
		goto out;
	}

out:
	return ret;
}

static int wl1271_configure_suspend_sta(struct wl1271 *wl,
					struct cfg80211_wowlan *wow)
{
	int ret = 0;

	mutex_lock(&wl->mutex);

	if (!test_bit(WL1271_FLAG_STA_ASSOCIATED, &wl->flags))
		goto out_unlock;

	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		goto out_unlock;

	/* enter psm if needed*/
	if (!test_bit(WL1271_FLAG_PSM, &wl->flags)) {
		DECLARE_COMPLETION_ONSTACK(compl);

		wl->ps_compl = &compl;
		ret = wl1271_ps_set_mode(wl, STATION_POWER_SAVE_MODE,
				   wl->basic_rate, true);
		if (ret < 0)
			goto out_sleep;

		/* we must unlock here so we will be able to get events */
		wl1271_ps_elp_sleep(wl);
		mutex_unlock(&wl->mutex);

		ret = wait_for_completion_timeout(
			&compl, msecs_to_jiffies(WL1271_PS_COMPLETE_TIMEOUT));

		mutex_lock(&wl->mutex);
		if (ret <= 0) {
			wl1271_warning("couldn't enter ps mode!");
			ret = -EBUSY;
			goto out_cleanup;
		}

		ret = wl1271_ps_elp_wakeup(wl);
		if (ret < 0)
			goto out_cleanup;
	}

	ret = wl1271_configure_wowlan(wl, wow);
	if (ret < 0)
		wl1271_error("suspend: Could not configure WoWLAN: %d", ret);

	/* In any case set wake up conditions to suspend values to conserve
	 *  more power while willing to lose some broadcast traffic
	 */
	ret = wl1271_acx_wake_up_conditions(wl,
			    wl->conf.conn.suspend_wake_up_event,
			    wl->conf.conn.suspend_listen_interval);

	if (ret < 0)
		wl1271_error("suspend: set wake up conditions failed: %d", ret);

out_sleep:
	wl1271_ps_elp_sleep(wl);
out_cleanup:
	wl->ps_compl = NULL;
out_unlock:
	mutex_unlock(&wl->mutex);
	return ret;
}

static int wl1271_configure_suspend_ap(struct wl1271 *wl)
{
	int ret = 0;

	mutex_lock(&wl->mutex);

	if (!test_bit(WL1271_FLAG_AP_STARTED, &wl->flags))
		goto out_unlock;

	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		goto out_unlock;

	ret = wl1271_acx_beacon_filter_opt(wl, true);

	wl1271_ps_elp_sleep(wl);
out_unlock:
	mutex_unlock(&wl->mutex);
	return ret;
}

static int wl1271_configure_suspend(struct wl1271 *wl,
				    struct cfg80211_wowlan *wow)
{
	if (wl->bss_type == BSS_TYPE_STA_BSS)
		return wl1271_configure_suspend_sta(wl, wow);
	if (wl->bss_type == BSS_TYPE_AP_BSS)
		return wl1271_configure_suspend_ap(wl);
	return 0;
}

static void wl1271_configure_resume(struct wl1271 *wl)
{
	int ret;
	bool is_sta = wl->bss_type == BSS_TYPE_STA_BSS;
	bool is_ap = wl->bss_type == BSS_TYPE_AP_BSS;

	if (!is_sta && !is_ap)
		return;

	mutex_lock(&wl->mutex);
	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		goto out;

	if (is_sta) {
		/* exit psm if it wasn't configured */
		if (!test_bit(WL1271_FLAG_PSM_REQUESTED, &wl->flags))
			wl1271_ps_set_mode(wl, STATION_ACTIVE_MODE,
					   wl->basic_rate, true);

		/* Remove WoWLAN filtering */
		wl1271_configure_wowlan(wl, NULL);

		/* switch back to normal wake up conditions */
		ret = wl1271_acx_wake_up_conditions(wl,
				    wl->conf.conn.wake_up_event,
				    wl->conf.conn.listen_interval);

		if (ret < 0)
			wl1271_error("resume: wake up conditions failed: %d",
				     ret);
	} else if (is_ap) {
		wl1271_acx_beacon_filter_opt(wl, false);
	}

	wl1271_ps_elp_sleep(wl);
out:
	mutex_unlock(&wl->mutex);
}

static int wl1271_op_suspend(struct ieee80211_hw *hw,
			    struct cfg80211_wowlan *wow)
{
	struct wl1271 *wl = hw->priv;
	int ret;

	wl1271_debug(DEBUG_MAC80211, "mac80211 suspend wow=%d", !!wow);
	WARN_ON(!wow);

	wl->wow_enabled = true;
	ret = wl1271_configure_suspend(wl, wow);
	if (ret < 0) {
		wl1271_warning("couldn't prepare device to suspend");
		return ret;
	}
	/* flush any remaining work */
	wl1271_debug(DEBUG_MAC80211, "flushing remaining works");

	/*
	 * disable and re-enable interrupts in order to flush
	 * the threaded_irq
	 */
	wl1271_disable_interrupts(wl);

	/*
	 * set suspended flag to avoid triggering a new threaded_irq
	 * work. no need for spinlock as interrupts are disabled.
	 */
	set_bit(WL1271_FLAG_SUSPENDED, &wl->flags);

	wl1271_enable_interrupts(wl);
	flush_work(&wl->tx_work);
	flush_delayed_work(&wl->pspoll_work);
	flush_delayed_work(&wl->elp_work);

	return 0;
}

static int wl1271_op_resume(struct ieee80211_hw *hw)
{
	struct wl1271 *wl = hw->priv;
	unsigned long flags;
	bool run_irq_work = false;

	wl1271_debug(DEBUG_MAC80211, "mac80211 resume wow=%d",
		     wl->wow_enabled);
	WARN_ON(!wl->wow_enabled);

	/*
	 * re-enable irq_work enqueuing, and call irq_work directly if
	 * there is a pending work.
	 */
	spin_lock_irqsave(&wl->wl_lock, flags);
	clear_bit(WL1271_FLAG_SUSPENDED, &wl->flags);
	if (test_and_clear_bit(WL1271_FLAG_PENDING_WORK, &wl->flags))
		run_irq_work = true;
	spin_unlock_irqrestore(&wl->wl_lock, flags);

	if (run_irq_work) {
		wl1271_debug(DEBUG_MAC80211,
			     "run postponed irq_work directly");
		wl1271_irq(0, wl);
		wl1271_enable_interrupts(wl);
	}
	wl1271_configure_resume(wl);
	wl->wow_enabled = false;

	return 0;
}

static int wl1271_op_start(struct ieee80211_hw *hw)
{
	wl1271_debug(DEBUG_MAC80211, "mac80211 start");

	/*
	 * We have to delay the booting of the hardware because
	 * we need to know the local MAC address before downloading and
	 * initializing the firmware. The MAC address cannot be changed
	 * after boot, and without the proper MAC address, the firmware
	 * will not function properly.
	 *
	 * The MAC address is first known when the corresponding interface
	 * is added. That is where we will initialize the hardware.
	 */

	return 0;
}

static void wl1271_op_stop(struct ieee80211_hw *hw)
{
	wl1271_debug(DEBUG_MAC80211, "mac80211 stop");
}

static u8 wl1271_get_role_type(struct wl1271 *wl)
{
	switch (wl->bss_type) {
	case BSS_TYPE_AP_BSS:
		if (wl->p2p)
			return WL1271_ROLE_P2P_GO;
		else
			return WL1271_ROLE_AP;

	case BSS_TYPE_STA_BSS:
		if (wl->p2p)
			return WL1271_ROLE_P2P_CL;
		else
			return WL1271_ROLE_STA;

	case BSS_TYPE_IBSS:
		return WL1271_ROLE_IBSS;

	default:
		wl1271_info("invalid bss_type: %d", wl->bss_type);
	}
	return 0xff;
}

static int wl1271_op_add_interface(struct ieee80211_hw *hw,
				   struct ieee80211_vif *vif)
{
	struct wl1271 *wl = hw->priv;
	struct wiphy *wiphy = hw->wiphy;
	int retries = WL1271_BOOT_RETRIES;
	int ret = 0;
	bool booted = false;

	wl1271_debug(DEBUG_MAC80211, "mac80211 add interface type %d mac %pM",
		     ieee80211_vif_type_p2p(vif), vif->addr);

	mutex_lock(&wl->mutex);
	if (wl->vif) {
		wl1271_debug(DEBUG_MAC80211,
			     "multiple vifs are not supported yet");
		ret = -EBUSY;
		goto out;
	}

	/*
	 * in some very corner case HW recovery scenarios its possible to
	 * get here before __wl1271_op_remove_interface is complete, so
	 * opt out if that is the case.
	 */
	if (test_bit(WL1271_FLAG_IF_INITIALIZED, &wl->flags)) {
		ret = -EBUSY;
		goto out;
	}

	switch (ieee80211_vif_type_p2p(vif)) {
	case NL80211_IFTYPE_P2P_CLIENT:
		wl->p2p = 1;
		/* fall-through */
	case NL80211_IFTYPE_STATION:
		wl->bss_type = BSS_TYPE_STA_BSS;
		wl->set_bss_type = BSS_TYPE_STA_BSS;
		break;
	case NL80211_IFTYPE_ADHOC:
		wl->bss_type = BSS_TYPE_IBSS;
		wl->set_bss_type = BSS_TYPE_STA_BSS;
		break;
	case NL80211_IFTYPE_P2P_GO:
		wl->p2p = 1;
		/* fall-through */
	case NL80211_IFTYPE_AP:
		wl->bss_type = BSS_TYPE_AP_BSS;
		break;
	default:
		ret = -EOPNOTSUPP;
		goto out;
	}

	memcpy(wl->mac_addr, vif->addr, ETH_ALEN);

	if (wl->state != WL1271_STATE_OFF) {
		wl1271_error("cannot start because not in off state: %d",
			     wl->state);
		ret = -EBUSY;
		goto out;
	}

	while (retries) {
		retries--;
		ret = wl12xx_chip_wakeup(wl, false);
		if (ret < 0)
			goto power_off;

		ret = wl1271_boot(wl);
		if (ret < 0)
			goto power_off;

		if (wl->bss_type == BSS_TYPE_STA_BSS ||
		    wl->bss_type == BSS_TYPE_IBSS) {
			ret = wl1271_cmd_role_enable(wl,
							 WL1271_ROLE_DEVICE,
							 &wl->dev_role_id);
			if (ret < 0)
				goto irq_disable;
		}

		ret = wl1271_cmd_role_enable(wl, wl1271_get_role_type(wl),
					     &wl->role_id);
		if (ret < 0)
			goto irq_disable;

		/* TODO: disable role here if failed */
		ret = wl1271_hw_init(wl);
		if (ret < 0)
			goto irq_disable;

		booted = true;
		break;

irq_disable:
		mutex_unlock(&wl->mutex);
		/* Unlocking the mutex in the middle of handling is
		   inherently unsafe. In this case we deem it safe to do,
		   because we need to let any possibly pending IRQ out of
		   the system (and while we are WL1271_STATE_OFF the IRQ
		   work function will not do anything.) Also, any other
		   possible concurrent operations will fail due to the
		   current state, hence the wl1271 struct should be safe. */
		wl1271_disable_interrupts(wl);
		wl1271_flush_deferred_work(wl);
		cancel_work_sync(&wl->netstack_work);
		mutex_lock(&wl->mutex);
power_off:
		wl1271_power_off(wl);
	}

	if (!booted) {
		wl1271_error("firmware boot failed despite %d retries",
			     WL1271_BOOT_RETRIES);
		goto out;
	}

	wl->vif = vif;
	wl->state = WL1271_STATE_ON;
	set_bit(WL1271_FLAG_IF_INITIALIZED, &wl->flags);
	wl1271_info("firmware booted (%s)", wl->chip.fw_ver_str);

	/* update hw/fw version info in wiphy struct */
	wiphy->hw_version = wl->chip.id;
	strncpy(wiphy->fw_version, wl->chip.fw_ver_str,
		sizeof(wiphy->fw_version));

	/*
	 * Now we know if 11a is supported (info from the NVS), so disable
	 * 11a channels if not supported
	 */
	if (!wl->enable_11a)
		wiphy->bands[IEEE80211_BAND_5GHZ]->n_channels = 0;

	wl1271_debug(DEBUG_MAC80211, "11a is %ssupported",
		     wl->enable_11a ? "" : "not ");

out:
	mutex_unlock(&wl->mutex);

	mutex_lock(&wl_list_mutex);
	if (!ret)
		list_add(&wl->list, &wl_list);
	mutex_unlock(&wl_list_mutex);

	return ret;
}

static void __wl1271_op_remove_interface(struct wl1271 *wl,
					 bool reset_tx_queues)
{
	int ret, i;

	wl1271_debug(DEBUG_MAC80211, "mac80211 remove interface");

	/* because of hardware recovery, we may get here twice */
	if (wl->state != WL1271_STATE_ON)
		return;

	/* Protect from re-entry that can happen if recovery is triggered while
	   this op is running */
	if (test_and_set_bit(WL1271_FLAG_HW_GOING_DOWN, &wl->flags))
		return;

	wl1271_info("down");

	mutex_lock(&wl_list_mutex);
	list_del(&wl->list);
	mutex_unlock(&wl_list_mutex);

	/* enable dyn ps just in case (if left on due to fw crash etc) */
	if (wl->bss_type == BSS_TYPE_STA_BSS)
		ieee80211_enable_dyn_ps(wl->vif);

	if (wl->scan.state != WL1271_SCAN_STATE_IDLE) {
		wl->scan.state = WL1271_SCAN_STATE_IDLE;
		memset(wl->scan.scanned_ch, 0, sizeof(wl->scan.scanned_ch));
		wl->scan.req = NULL;
		ieee80211_scan_completed(wl->hw, true);
	}

	if (!test_bit(WL1271_FLAG_RECOVERY_IN_PROGRESS, &wl->flags)) {
		/* TODO: put all this in a separate function? */
		/* disable active roles */
		ret = wl1271_ps_elp_wakeup(wl);
		if (ret < 0) {
			/*
			 * do nothing. we are going to power_off the chip anyway.
			 * handle this case when we'll really support multi-role...
			 */
		}
		if (wl->bss_type == BSS_TYPE_STA_BSS)
			wl1271_cmd_role_disable(wl, &wl->dev_role_id);
		wl1271_cmd_role_disable(wl, &wl->role_id);

		/* TODO: this obviously shouldn't always be called */
		wl1271_ps_elp_sleep(wl);
	} else {
		/* clear all hlids (except system_hlid) */
		wl->sta_hlid = WL1271_INVALID_LINK_ID;
		wl->dev_hlid = WL1271_INVALID_LINK_ID;
		wl->ap_bcast_hlid = WL1271_INVALID_LINK_ID;
		wl->ap_global_hlid = WL1271_INVALID_LINK_ID;
	}

	/*
	 * this must be before the cancel_work calls below, so that the work
	 * functions don't perform further work.
	 */
	mutex_unlock(&wl->mutex);

	/*
	 * Interrupts must be disabled before setting the state to OFF.
	 * Otherwise, the interrupt handler might be called and exit without
	 * reading the interrupt status.
	 */
	wl1271_disable_interrupts(wl);

	mutex_lock(&wl->mutex);
	wl->state = WL1271_STATE_OFF;
	mutex_unlock(&wl->mutex);

	wl1271_flush_deferred_work(wl);
	cancel_delayed_work_sync(&wl->scan_complete_work);
	cancel_work_sync(&wl->netstack_work);
	cancel_work_sync(&wl->tx_work);
	del_timer_sync(&wl->rx_streaming_timer);
	cancel_work_sync(&wl->rx_streaming_enable_work);
	cancel_work_sync(&wl->rx_streaming_disable_work);
	cancel_delayed_work_sync(&wl->pspoll_work);
	cancel_work_sync(&wl->ap_start_work);
	cancel_delayed_work_sync(&wl->elp_work);

	mutex_lock(&wl->mutex);

	/* let's notify MAC80211 about the remaining pending TX frames */
	wl1271_tx_reset(wl, reset_tx_queues);
	wl1271_power_off(wl);

	memset(wl->bssid, 0, ETH_ALEN);
	memset(wl->ssid, 0, IW_ESSID_MAX_SIZE + 1);
	wl->ssid_len = 0;
	wl->bss_type = MAX_BSS_TYPE;
	wl->set_bss_type = MAX_BSS_TYPE;
	wl->p2p = 0;
	wl->band = IEEE80211_BAND_2GHZ;

	wl->rx_counter = 0;
	wl->psm_entry_retry = 0;
	wl->power_level = WL1271_DEFAULT_POWER_LEVEL;
	wl->tx_blocks_available = 0;
	wl->tx_results_count = 0;
	wl->tx_packets_count = 0;
	wl->time_offset = 0;
	wl->session_counter = 0;
	wl->rate_set = CONF_TX_RATE_MASK_BASIC;
	wl->bitrate_masks[IEEE80211_BAND_2GHZ] = wl->conf.tx.basic_rate;
	wl->bitrate_masks[IEEE80211_BAND_5GHZ] = wl->conf.tx.basic_rate_5;
	wl->vif = NULL;
	wl->filters = 0;
	wl->tx_spare_blocks = TX_HW_BLOCK_SPARE_DEFAULT;
	wl1271_free_ap_keys(wl);
	memset(wl->ap_hlid_map, 0, sizeof(wl->ap_hlid_map));
	wl->ap_fw_ps_map = 0;
	wl->ap_ps_map = 0;
	wl->sched_scanning = false;
	wl->role_id = WL1271_INVALID_ROLE_ID;
	wl->dev_role_id = WL1271_INVALID_ROLE_ID;
	memset(wl->roles_map, 0, sizeof(wl->roles_map));
	memset(wl->links_map, 0, sizeof(wl->links_map));
	memset(wl->roc_map, 0, sizeof(wl->roc_map));
	wl->active_sta_count = 0;

	/* The system link is always allocated */
	__set_bit(WL1271_SYSTEM_HLID, wl->links_map);

	/*
	 * this is performed after the cancel_work calls and the associated
	 * mutex_lock, so that wl1271_op_add_interface does not accidentally
	 * get executed before all these vars have been reset.
	 */
	wl->flags = 0;

	wl->tx_blocks_freed = 0;
	wl->tx_allocated_blocks = 0;

	for (i = 0; i < NUM_TX_QUEUES; i++) {
		wl->tx_pkts_freed[i] = 0;
		wl->tx_allocated_pkts[i] = 0;
	}

	wl1271_debugfs_reset(wl);

	kfree(wl->fw_status);
	wl->fw_status = NULL;
	kfree(wl->tx_res_if);
	wl->tx_res_if = NULL;
	kfree(wl->target_mem_map);
	wl->target_mem_map = NULL;
}

static void wl1271_op_remove_interface(struct ieee80211_hw *hw,
				       struct ieee80211_vif *vif)
{
	struct wl1271 *wl = hw->priv;

	mutex_lock(&wl->mutex);
	/*
	 * wl->vif can be null here if someone shuts down the interface
	 * just when hardware recovery has been started.
	 */
	if (wl->vif) {
		WARN_ON(wl->vif != vif);
		__wl1271_op_remove_interface(wl, true);
	}

	mutex_unlock(&wl->mutex);
	cancel_work_sync(&wl->recovery_work);
}

void wl1271_configure_filters(struct wl1271 *wl, unsigned int filters)
{
	wl1271_set_default_filters(wl);

	/* combine requested filters with current filter config */
	filters = wl->filters | filters;

	wl1271_debug(DEBUG_FILTERS, "RX filters set: ");

	if (filters & FIF_PROMISC_IN_BSS) {
		wl1271_debug(DEBUG_FILTERS, " - FIF_PROMISC_IN_BSS");
		wl->rx_config &= ~CFG_UNI_FILTER_EN;
		wl->rx_config |= CFG_BSSID_FILTER_EN;
	}
	if (filters & FIF_BCN_PRBRESP_PROMISC) {
		wl1271_debug(DEBUG_FILTERS, " - FIF_BCN_PRBRESP_PROMISC");
		wl->rx_config &= ~CFG_BSSID_FILTER_EN;
		wl->rx_config &= ~CFG_SSID_FILTER_EN;
	}
	if (filters & FIF_OTHER_BSS) {
		wl1271_debug(DEBUG_FILTERS, " - FIF_OTHER_BSS");
		wl->rx_config &= ~CFG_BSSID_FILTER_EN;
	}
	if (filters & FIF_CONTROL) {
		wl1271_debug(DEBUG_FILTERS, " - FIF_CONTROL");
		wl->rx_filter |= CFG_RX_CTL_EN;
	}
	if (filters & FIF_FCSFAIL) {
		wl1271_debug(DEBUG_FILTERS, " - FIF_FCSFAIL");
		wl->rx_filter |= CFG_RX_FCS_ERROR;
	}
}

static int wl1271_join(struct wl1271 *wl, bool set_assoc)
{
	int ret;
	bool is_ibss = (wl->bss_type == BSS_TYPE_IBSS);

	/*
	 * One of the side effects of the JOIN command is that is clears
	 * WPA/WPA2 keys from the chipset. Performing a JOIN while associated
	 * to a WPA/WPA2 access point will therefore kill the data-path.
	 * Currently the only valid scenario for JOIN during association
	 * is on roaming, in which case we will also be given new keys.
	 * Keep the below message for now, unless it starts bothering
	 * users who really like to roam a lot :)
	 */
	if (test_bit(WL1271_FLAG_STA_ASSOCIATED, &wl->flags))
		wl1271_info("JOIN while associated.");

	/* clear encryption type */
	wl->encryption_type = KEY_NONE;

	if (set_assoc)
		set_bit(WL1271_FLAG_STA_ASSOCIATED, &wl->flags);

	if (is_ibss)
		ret = wl1271_cmd_role_start_ibss(wl);
	else
		ret = wl1271_cmd_role_start_sta(wl);
	if (ret < 0)
		goto out;

	if (!test_bit(WL1271_FLAG_STA_ASSOCIATED, &wl->flags))
		goto out;

	/*
	 * The join command disable the keep-alive mode, shut down its process,
	 * and also clear the template config, so we need to reset it all after
	 * the join. The acx_aid starts the keep-alive process, and the order
	 * of the commands below is relevant.
	 */
	ret = wl1271_acx_keep_alive_mode(wl, true);
	if (ret < 0)
		goto out;

	ret = wl1271_acx_aid(wl, wl->aid);
	if (ret < 0)
		goto out;

	ret = wl1271_cmd_build_klv_null_data(wl);
	if (ret < 0)
		goto out;

	ret = wl1271_acx_keep_alive_config(wl, CMD_TEMPL_KLV_IDX_NULL_DATA,
					   ACX_KEEP_ALIVE_TPL_VALID);
	if (ret < 0)
		goto out;

out:
	return ret;
}

static int wl1271_unjoin(struct wl1271 *wl)
{
	int ret;

	if (test_and_clear_bit(WL1271_FLAG_CS_PROGRESS, &wl->flags)) {
		wl12xx_cmd_stop_channel_switch(wl);
		ieee80211_chswitch_done(wl->vif, false);
	}

	/* to stop listening to a channel, we disconnect */
	ret = wl1271_cmd_role_stop_sta(wl);
	if (ret < 0)
		goto out;

	memset(wl->bssid, 0, ETH_ALEN);

	/* reset TX security counters on a clean disconnect */
	wl->tx_security_last_seq_lsb = 0;
	wl->tx_security_seq = 0;

out:
	return ret;
}

static void wl1271_set_band_rate(struct wl1271 *wl)
{
	wl->basic_rate_set = wl->bitrate_masks[wl->band];
	wl->rate_set = wl->basic_rate_set;
}

static int wl1271_sta_handle_idle(struct wl1271 *wl, bool idle)
{
	int ret;

	if (idle) {
		if (test_bit(WL1271_FLAG_ROC, &wl->flags)) {
			ret = wl1271_croc(wl, wl->dev_role_id);
			if (ret < 0)
				goto out;

			ret = wl1271_cmd_role_stop_dev(wl);
			if (ret < 0)
				goto out;
		}
		wl->rate_set = wl1271_tx_min_rate_get(wl, wl->basic_rate_set);
		ret = wl1271_acx_sta_rate_policies(wl);
		if (ret < 0)
			goto out;
		ret = wl1271_acx_keep_alive_config(
			wl, CMD_TEMPL_KLV_IDX_NULL_DATA,
			ACX_KEEP_ALIVE_TPL_INVALID);
		if (ret < 0)
			goto out;
		set_bit(WL1271_FLAG_IDLE, &wl->flags);
	} else {
		ret = wl1271_cmd_role_start_dev(wl);
		if (ret < 0)
			goto out;

		ret = wl1271_roc(wl, wl->dev_role_id);
		if (ret < 0)
			goto out;
		clear_bit(WL1271_FLAG_IDLE, &wl->flags);
	}

out:
	return ret;
}

static int wl1271_op_config(struct ieee80211_hw *hw, u32 changed)
{
	struct wl1271 *wl = hw->priv;
	struct ieee80211_conf *conf = &hw->conf;
	int channel, ret = 0;
	bool is_ap;

	channel = ieee80211_frequency_to_channel(conf->channel->center_freq);

	wl1271_debug(DEBUG_MAC80211, "mac80211 config ch %d psm %s power %d %s"
		     " changed 0x%x",
		     channel,
		     conf->flags & IEEE80211_CONF_PS ? "on" : "off",
		     conf->power_level,
		     conf->flags & IEEE80211_CONF_IDLE ? "idle" : "in use",
			 changed);

	/*
	 * mac80211 will go to idle nearly immediately after transmitting some
	 * frames, such as the deauth. To make sure those frames reach the air,
	 * wait here until the TX queue is fully flushed.
	 */
	if ((changed & IEEE80211_CONF_CHANGE_IDLE) &&
	    (conf->flags & IEEE80211_CONF_IDLE))
		wl1271_tx_flush(wl);

	mutex_lock(&wl->mutex);

	if (unlikely(wl->state == WL1271_STATE_OFF)) {
		/* we support configuring the channel and band while off */
		if ((changed & IEEE80211_CONF_CHANGE_CHANNEL)) {
			wl->band = conf->channel->band;
			wl->channel = channel;
		}

		if ((changed & IEEE80211_CONF_CHANGE_POWER))
			wl->power_level = conf->power_level;

		goto out;
	}

	is_ap = (wl->bss_type == BSS_TYPE_AP_BSS);

	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		goto out;

	/* if the channel changes while joined, join again */
	if (changed & IEEE80211_CONF_CHANGE_CHANNEL &&
	    ((wl->band != conf->channel->band) ||
	     (wl->channel != channel))) {
		/* send all pending packets */
		wl1271_tx_work_locked(wl);
		wl->band = conf->channel->band;
		wl->channel = channel;

		if (!is_ap) {
			/*
			 * FIXME: the mac80211 should really provide a fixed
			 * rate to use here. for now, just use the smallest
			 * possible rate for the band as a fixed rate for
			 * association frames and other control messages.
			 */
			if (!test_bit(WL1271_FLAG_STA_ASSOCIATED, &wl->flags))
				wl1271_set_band_rate(wl);

			wl->basic_rate =
				wl1271_tx_min_rate_get(wl, wl->basic_rate_set);
			ret = wl1271_acx_sta_rate_policies(wl);
			if (ret < 0)
				wl1271_warning("rate policy for channel "
					       "failed %d", ret);

			if (test_bit(WL1271_FLAG_STA_ASSOCIATED, &wl->flags)) {
				if (test_bit(WL1271_FLAG_ROC, &wl->flags)) {
					/* roaming */
					ret = wl1271_croc(wl, wl->dev_role_id);
					if (ret < 0)
						goto out_sleep;
				}
				ret = wl1271_join(wl, false);
				if (ret < 0)
					wl1271_warning("cmd join on channel "
						       "failed %d", ret);
			} else {
				/*
				 * change the ROC channel. do it only if we are
				 * not idle. otherwise, CROC will be called
				 * anyway.
				 */
				if (test_bit(WL1271_FLAG_ROC, &wl->flags) &&
				    !(conf->flags & IEEE80211_CONF_IDLE)) {
					ret = wl1271_croc(wl, wl->dev_role_id);
					if (ret < 0)
						goto out_sleep;

					ret = wl1271_roc(wl, wl->dev_role_id);
					if (ret < 0)
						wl1271_warning("roc failed %d",
							       ret);
				}
			}
		}
	}

	if (changed & IEEE80211_CONF_CHANGE_IDLE && !is_ap) {
		ret = wl1271_sta_handle_idle(wl,
					conf->flags & IEEE80211_CONF_IDLE);
		if (ret < 0)
			wl1271_warning("idle mode change failed %d", ret);
	}

	/*
	 * if mac80211 changes the PSM mode, make sure the mode is not
	 * incorrectly changed after the pspoll failure active window.
	 */
	if (changed & IEEE80211_CONF_CHANGE_PS)
		clear_bit(WL1271_FLAG_PSPOLL_FAILURE, &wl->flags);

	if (conf->flags & IEEE80211_CONF_PS &&
	    !test_bit(WL1271_FLAG_PSM_REQUESTED, &wl->flags)) {
		set_bit(WL1271_FLAG_PSM_REQUESTED, &wl->flags);

		/*
		 * We enter PSM only if we're already associated.
		 * If we're not, we'll enter it when joining an SSID,
		 * through the bss_info_changed() hook.
		 */
		if (test_bit(WL1271_FLAG_STA_ASSOCIATED, &wl->flags)) {
			wl1271_debug(DEBUG_PSM, "psm enabled");
			ret = wl1271_ps_set_mode(wl, STATION_POWER_SAVE_MODE,
						 wl->basic_rate, true);
		}
	} else if (!(conf->flags & IEEE80211_CONF_PS) &&
		   test_bit(WL1271_FLAG_PSM_REQUESTED, &wl->flags)) {
		wl1271_debug(DEBUG_PSM, "psm disabled");

		clear_bit(WL1271_FLAG_PSM_REQUESTED, &wl->flags);

		if (test_bit(WL1271_FLAG_PSM, &wl->flags))
			ret = wl1271_ps_set_mode(wl, STATION_ACTIVE_MODE,
						 wl->basic_rate, true);
	}

	if (conf->power_level != wl->power_level) {
		ret = wl1271_acx_tx_power(wl, conf->power_level);
		if (ret < 0)
			goto out_sleep;

		wl->power_level = conf->power_level;
	}

out_sleep:
	wl1271_ps_elp_sleep(wl);

out:
	mutex_unlock(&wl->mutex);

	return ret;
}

struct wl1271_filter_params {
	bool enabled;
	int mc_list_length;
	u8 mc_list[ACX_MC_ADDRESS_GROUP_MAX][ETH_ALEN];
};

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35))
static u64 wl1271_op_prepare_multicast(struct ieee80211_hw *hw,
				       struct netdev_hw_addr_list *mc_list)
#else
static u64 wl1271_op_prepare_multicast(struct ieee80211_hw *hw, int mc_count,
				       struct dev_addr_list *mc_list)
#endif
{
	struct wl1271_filter_params *fp;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35))
	struct netdev_hw_addr *ha;
#else
	int i;
#endif
	struct wl1271 *wl = hw->priv;

	if (unlikely(wl->state == WL1271_STATE_OFF))
		return 0;

	fp = kzalloc(sizeof(*fp), GFP_ATOMIC);
	if (!fp) {
		wl1271_error("Out of memory setting filters.");
		return 0;
	}

	/* update multicast filtering parameters */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35))
	fp->mc_list_length = 0;
	if (netdev_hw_addr_list_count(mc_list) > ACX_MC_ADDRESS_GROUP_MAX) {
#else
	fp->enabled = true;
	if (mc_count > ACX_MC_ADDRESS_GROUP_MAX) {
		mc_count = 0;
#endif
		fp->enabled = false;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35))
	} else {
		fp->enabled = true;
		netdev_hw_addr_list_for_each(ha, mc_list) {
#else
	}

	fp->mc_list_length = 0;
	for (i = 0; i < mc_count; i++) {
		if (mc_list->da_addrlen == ETH_ALEN) {
#endif
			memcpy(fp->mc_list[fp->mc_list_length],
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35))
					ha->addr, ETH_ALEN);
#else
			       mc_list->da_addr, ETH_ALEN);
#endif
			fp->mc_list_length++;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35))
		}
#else
		} else
			wl1271_warning("Unknown mc address length.");
		mc_list = mc_list->next;
#endif
	}

	return (u64)(unsigned long)fp;
}

#define WL1271_SUPPORTED_FILTERS (FIF_PROMISC_IN_BSS | \
				  FIF_ALLMULTI | \
				  FIF_FCSFAIL | \
				  FIF_BCN_PRBRESP_PROMISC | \
				  FIF_CONTROL | \
				  FIF_OTHER_BSS)

static void wl1271_op_configure_filter(struct ieee80211_hw *hw,
				       unsigned int changed,
				       unsigned int *total, u64 multicast)
{
	struct wl1271_filter_params *fp = (void *)(unsigned long)multicast;
	struct wl1271 *wl = hw->priv;
	int ret;

	wl1271_debug(DEBUG_MAC80211, "mac80211 configure filter changed %x"
		     " total %x", changed, *total);

	mutex_lock(&wl->mutex);

	*total &= WL1271_SUPPORTED_FILTERS;
	changed &= WL1271_SUPPORTED_FILTERS;

	if (unlikely(wl->state == WL1271_STATE_OFF))
		goto out;

	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		goto out;

	if (wl->bss_type != BSS_TYPE_AP_BSS) {
		if (*total & FIF_ALLMULTI)
			ret = wl1271_acx_group_address_tbl(wl, false, NULL, 0);
		else if (fp)
			ret = wl1271_acx_group_address_tbl(wl, fp->enabled,
							   fp->mc_list,
							   fp->mc_list_length);
		if (ret < 0)
			goto out_sleep;
	}

	/* determine, whether supported filter values have changed */
	if (changed == 0)
		goto out_sleep;

	/* configure filters */
	wl->filters = *total;
	wl1271_configure_filters(wl, 0);

	/* apply configured filters */
#if 0
	ret = wl1271_acx_rx_config(wl, wl->rx_config, wl->rx_filter);
	if (ret < 0)
		goto out_sleep;
#endif

out_sleep:
	wl1271_ps_elp_sleep(wl);

out:
	mutex_unlock(&wl->mutex);
	kfree(fp);
}

static int wl1271_record_ap_key(struct wl1271 *wl, u8 id, u8 key_type,
			u8 key_size, const u8 *key, u8 hlid, u32 tx_seq_32,
			u16 tx_seq_16)
{
	struct wl1271_ap_key *ap_key;
	int i;

	wl1271_debug(DEBUG_CRYPT, "record ap key id %d", (int)id);

	if (key_size > MAX_KEY_SIZE)
		return -EINVAL;

	/*
	 * Find next free entry in ap_keys. Also check we are not replacing
	 * an existing key.
	 */
	for (i = 0; i < MAX_NUM_KEYS; i++) {
		if (wl->recorded_ap_keys[i] == NULL)
			break;

		if (wl->recorded_ap_keys[i]->id == id) {
			wl1271_warning("trying to record key replacement");
			return -EINVAL;
		}
	}

	if (i == MAX_NUM_KEYS)
		return -EBUSY;

	ap_key = kzalloc(sizeof(*ap_key), GFP_KERNEL);
	if (!ap_key)
		return -ENOMEM;

	ap_key->id = id;
	ap_key->key_type = key_type;
	ap_key->key_size = key_size;
	memcpy(ap_key->key, key, key_size);
	ap_key->hlid = hlid;
	ap_key->tx_seq_32 = tx_seq_32;
	ap_key->tx_seq_16 = tx_seq_16;

	wl->recorded_ap_keys[i] = ap_key;
	return 0;
}

static void wl1271_free_ap_keys(struct wl1271 *wl)
{
	int i;

	for (i = 0; i < MAX_NUM_KEYS; i++) {
		kfree(wl->recorded_ap_keys[i]);
		wl->recorded_ap_keys[i] = NULL;
	}
}

static int wl1271_ap_init_hwenc(struct wl1271 *wl)
{
	int i, ret = 0;
	struct wl1271_ap_key *key;
	bool wep_key_added = false;

	for (i = 0; i < MAX_NUM_KEYS; i++) {
		u8 hlid;
		if (wl->recorded_ap_keys[i] == NULL)
			break;

		key = wl->recorded_ap_keys[i];
		hlid = key->hlid;
		if (hlid == WL1271_INVALID_LINK_ID)
			hlid = wl->ap_bcast_hlid;
			
		ret = wl1271_cmd_set_ap_key(wl, KEY_ADD_OR_REPLACE,
					    key->id, key->key_type,
					    key->key_size, key->key,
					    hlid, key->tx_seq_32,
					    key->tx_seq_16);
		if (ret < 0)
			goto out;

		if (key->key_type == KEY_WEP)
			wep_key_added = true;
	}

	if (wep_key_added) {
		ret = wl1271_cmd_set_ap_default_wep_key(wl, wl->default_key);
		if (ret < 0)
			goto out;
	}

out:
	wl1271_free_ap_keys(wl);
	return ret;
}

static int wl1271_set_key(struct wl1271 *wl, u16 action, u8 id, u8 key_type,
		       u8 key_size, const u8 *key, u32 tx_seq_32,
		       u16 tx_seq_16, struct ieee80211_sta *sta)
{
	int ret;
	bool is_ap = (wl->bss_type == BSS_TYPE_AP_BSS);

	if (is_ap) {
		struct wl1271_station *wl_sta;
		u8 hlid;

		if (sta) {
			wl_sta = (struct wl1271_station *)sta->drv_priv;
			hlid = wl_sta->hlid;
		} else {
			hlid = wl->ap_bcast_hlid;
		}

		if (!test_bit(WL1271_FLAG_AP_STARTED, &wl->flags)) {
			/*
			 * We do not support removing keys after AP shutdown.
			 * Pretend we do to make mac80211 happy.
			 */
			if (action != KEY_ADD_OR_REPLACE)
				return 0;

			ret = wl1271_record_ap_key(wl, id,
					     key_type, key_size,
					     key, hlid, tx_seq_32,
					     tx_seq_16);
		} else {
			ret = wl1271_cmd_set_ap_key(wl, action,
					     id, key_type, key_size,
					     key, hlid, tx_seq_32,
					     tx_seq_16);
		}

		if (ret < 0)
			return ret;
	} else {
		const u8 *addr;
		static const u8 bcast_addr[ETH_ALEN] = {
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff
		};

		/*
		 * A STA set to GEM cipher requires 2 tx spare blocks.
		 * Return to default value when GEM cipher key is removed
		 */
		if (key_type == KEY_GEM) {
			if (action == KEY_ADD_OR_REPLACE)
				wl->tx_spare_blocks = 2;
			else if (action == KEY_REMOVE)
				wl->tx_spare_blocks = TX_HW_BLOCK_SPARE_DEFAULT;
		}

		addr = sta ? sta->addr : bcast_addr;

		if (is_zero_ether_addr(addr)) {
			/* We dont support TX only encryption */
			return -EOPNOTSUPP;
		}

		/*
		 * The wl1271 does not allow to remove unicast keys - they
		 * will be cleared automatically on next CMD_JOIN. Ignore the
		 * request silently, as we dont want the mac80211 to emit
		 * an error message.
		 *
		 * The current fw fails to remove unicast keys as well, so
		 * just drop any KEY_REMOVE request (revert it when fw
		 * will be fixed)
		 */
		if (action == KEY_REMOVE)
			return 0;

		/* don't remove key if hlid was already deleted */
		if (action == KEY_REMOVE &&
		    wl->sta_hlid == WL1271_INVALID_LINK_ID)
			return 0;

		ret = wl1271_cmd_set_sta_key(wl, action,
					     id, key_type, key_size,
					     key, addr, tx_seq_32,
					     tx_seq_16);
		if (ret < 0)
			return ret;

		/* the default WEP key needs to be configured at least once */
		if (key_type == KEY_WEP) {
			ret = wl1271_cmd_set_sta_default_wep_key(wl,
							wl->default_key);
			if (ret < 0)
				return ret;
		}
	}

	return 0;
}

static int wl1271_op_set_key(struct ieee80211_hw *hw, enum set_key_cmd cmd,
			     struct ieee80211_vif *vif,
			     struct ieee80211_sta *sta,
			     struct ieee80211_key_conf *key_conf)
{
	struct wl1271 *wl = hw->priv;
	int ret;
	u32 tx_seq_32 = 0;
	u16 tx_seq_16 = 0;
	u8 key_type;

	wl1271_debug(DEBUG_MAC80211, "mac80211 set key");

	wl1271_debug(DEBUG_CRYPT, "CMD: 0x%x sta: %p", cmd, sta);
	wl1271_debug(DEBUG_CRYPT, "Key: algo:0x%x, id:%d, len:%d flags 0x%x",
		     key_conf->cipher, key_conf->keyidx,
		     key_conf->keylen, key_conf->flags);
	wl1271_dump(DEBUG_CRYPT, "KEY: ", key_conf->key, key_conf->keylen);

	mutex_lock(&wl->mutex);

	if (unlikely(wl->state == WL1271_STATE_OFF)) {
		ret = -EAGAIN;
		goto out_unlock;
	}

	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		goto out_unlock;

	switch (key_conf->cipher) {
	case WLAN_CIPHER_SUITE_WEP40:
	case WLAN_CIPHER_SUITE_WEP104:
		key_type = KEY_WEP;

		key_conf->hw_key_idx = key_conf->keyidx;
		break;
	case WLAN_CIPHER_SUITE_TKIP:
		key_type = KEY_TKIP;

		key_conf->hw_key_idx = key_conf->keyidx;
		tx_seq_32 = WL1271_TX_SECURITY_HI32(wl->tx_security_seq);
		tx_seq_16 = WL1271_TX_SECURITY_LO16(wl->tx_security_seq);
		break;
	case WLAN_CIPHER_SUITE_CCMP:
		key_type = KEY_AES;

		key_conf->flags |= IEEE80211_KEY_FLAG_GENERATE_IV;
		tx_seq_32 = WL1271_TX_SECURITY_HI32(wl->tx_security_seq);
		tx_seq_16 = WL1271_TX_SECURITY_LO16(wl->tx_security_seq);
		break;
	case WL1271_CIPHER_SUITE_GEM:
		key_type = KEY_GEM;
		tx_seq_32 = WL1271_TX_SECURITY_HI32(wl->tx_security_seq);
		tx_seq_16 = WL1271_TX_SECURITY_LO16(wl->tx_security_seq);
		break;
	default:
		wl1271_error("Unknown key algo 0x%x", key_conf->cipher);

		ret = -EOPNOTSUPP;
		goto out_sleep;
	}

	switch (cmd) {
	case SET_KEY:
		ret = wl1271_set_key(wl, KEY_ADD_OR_REPLACE,
				 key_conf->keyidx, key_type,
				 key_conf->keylen, key_conf->key,
				 tx_seq_32, tx_seq_16, sta);
		if (ret < 0) {
			wl1271_error("Could not add or replace key");
			goto out_sleep;
		}

		/*
		 * reconfiguring arp response if the unicast (or common)
		 * encryption key type was changed
		 */
		if (wl->bss_type == BSS_TYPE_STA_BSS &&
		    (sta || key_type == KEY_WEP) &&
		    wl->encryption_type != key_type) {
			wl->encryption_type = key_type;
			ret = wl1271_cmd_build_arp_rsp(wl, wl->ip_addr);
			if (ret < 0) {
				wl1271_warning("build arp rsp failed: %d", ret);
				goto out_sleep;
			}
		}
		break;

	case DISABLE_KEY:
		ret = wl1271_set_key(wl, KEY_REMOVE,
				     key_conf->keyidx, key_type,
				     key_conf->keylen, key_conf->key,
				     0, 0, sta);
		if (ret < 0) {
			wl1271_error("Could not remove key");
			goto out_sleep;
		}
		break;

	default:
		wl1271_error("Unsupported key cmd 0x%x", cmd);
		ret = -EOPNOTSUPP;
		break;
	}

out_sleep:
	wl1271_ps_elp_sleep(wl);

out_unlock:
	mutex_unlock(&wl->mutex);

	return ret;
}

static int wl1271_op_hw_scan(struct ieee80211_hw *hw,
			     struct ieee80211_vif *vif,
			     struct cfg80211_scan_request *req)
{
	struct wl1271 *wl = hw->priv;
	int ret;
	u8 *ssid = NULL;
	size_t len = 0;

	wl1271_debug(DEBUG_MAC80211, "mac80211 hw scan");

	if (req->n_ssids) {
		ssid = req->ssids[0].ssid;
		len = req->ssids[0].ssid_len;
	}

	mutex_lock(&wl->mutex);

	if (wl->state == WL1271_STATE_OFF) {
		/*
		 * We cannot return -EBUSY here because cfg80211 will expect
		 * a call to ieee80211_scan_completed if we do - in this case
		 * there won't be any call.
		 */
		ret = -EAGAIN;
		goto out;
	}

	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		goto out;

	/* cancel ROC before scanning */
	if (test_bit(WL1271_FLAG_ROC, &wl->flags)) {
		if (test_bit(WL1271_FLAG_STA_ASSOCIATED, &wl->flags)) {
			/* don't allow scanning right now (?) */
			ret = -EBUSY;
			goto out_sleep;
		}
	}

	ret = wl1271_scan(hw->priv, ssid, len, req);
out_sleep:
	wl1271_ps_elp_sleep(wl);
out:
	mutex_unlock(&wl->mutex);

	return ret;
}

static void wl1271_op_cancel_hw_scan(struct ieee80211_hw *hw,
				     struct ieee80211_vif *vif)
{
	struct wl1271 *wl = hw->priv;
	int ret;

	wl1271_debug(DEBUG_MAC80211, "mac80211 cancel hw scan");

	mutex_lock(&wl->mutex);

	if (wl->state == WL1271_STATE_OFF)
		goto out;

	if (wl->scan.state == WL1271_SCAN_STATE_IDLE)
		goto out;

	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		goto out;

	if (wl->scan.state != WL1271_SCAN_STATE_DONE) {
		ret = wl1271_scan_stop(wl);
		if (ret < 0)
			goto out_sleep;
	}
	wl->scan.state = WL1271_SCAN_STATE_IDLE;
	memset(wl->scan.scanned_ch, 0, sizeof(wl->scan.scanned_ch));
	wl->scan.req = NULL;
	ieee80211_scan_completed(wl->hw, true);

out_sleep:
	wl1271_ps_elp_sleep(wl);
out:
	mutex_unlock(&wl->mutex);

	cancel_delayed_work_sync(&wl->scan_complete_work);
}

static int wl1271_op_sched_scan_start(struct ieee80211_hw *hw,
				      struct ieee80211_vif *vif,
				      struct cfg80211_sched_scan_request *req,
				      struct ieee80211_sched_scan_ies *ies)
{
	struct wl1271 *wl = hw->priv;
	int ret;

	wl1271_debug(DEBUG_MAC80211, "wl1271_op_sched_scan_start");

	mutex_lock(&wl->mutex);

	if (wl->state == WL1271_STATE_OFF) {
		ret = -EAGAIN;
		goto out;
	}

	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		goto out;

	ret = wl1271_scan_sched_scan_config(wl, req, ies);
	if (ret < 0)
		goto out_sleep;

	ret = wl1271_scan_sched_scan_start(wl);
	if (ret < 0)
		goto out_sleep;

	wl->sched_scanning = true;

out_sleep:
	wl1271_ps_elp_sleep(wl);
out:
	mutex_unlock(&wl->mutex);
	return ret;
}

static void wl1271_op_sched_scan_stop(struct ieee80211_hw *hw,
				      struct ieee80211_vif *vif)
{
	struct wl1271 *wl = hw->priv;
	int ret;

	wl1271_debug(DEBUG_MAC80211, "wl1271_op_sched_scan_stop");

	mutex_lock(&wl->mutex);

	if (wl->state == WL1271_STATE_OFF)
		goto out;

	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		goto out;

	wl1271_scan_sched_scan_stop(wl);

	wl1271_ps_elp_sleep(wl);
out:
	mutex_unlock(&wl->mutex);
}

static int wl1271_op_set_frag_threshold(struct ieee80211_hw *hw, u32 value)
{
	struct wl1271 *wl = hw->priv;
	int ret = 0;

	mutex_lock(&wl->mutex);

	if (unlikely(wl->state == WL1271_STATE_OFF)) {
		ret = -EAGAIN;
		goto out;
	}

	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		goto out;

	ret = wl1271_acx_frag_threshold(wl, value);
	if (ret < 0)
		wl1271_warning("wl1271_op_set_frag_threshold failed: %d", ret);

	wl1271_ps_elp_sleep(wl);

out:
	mutex_unlock(&wl->mutex);

	return ret;
}

static int wl1271_op_set_rts_threshold(struct ieee80211_hw *hw, u32 value)
{
	struct wl1271 *wl = hw->priv;
	int ret = 0;

	mutex_lock(&wl->mutex);

	if (unlikely(wl->state == WL1271_STATE_OFF)) {
		ret = -EAGAIN;
		goto out;
	}

	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		goto out;

	ret = wl1271_acx_rts_threshold(wl, value);
	if (ret < 0)
		wl1271_warning("wl1271_op_set_rts_threshold failed: %d", ret);

	wl1271_ps_elp_sleep(wl);

out:
	mutex_unlock(&wl->mutex);

	return ret;
}

static int wl1271_ssid_set(struct wl1271 *wl, struct sk_buff *skb,
			    int offset)
{
	u8 ssid_len;
	const u8 *ptr = cfg80211_find_ie(WLAN_EID_SSID, skb->data + offset,
					 skb->len - offset);

	if (!ptr) {
		wl1271_error("No SSID in IEs!");
		return -ENOENT;
	}

	ssid_len = ptr[1];
	if (ssid_len > IEEE80211_MAX_SSID_LEN) {
		wl1271_error("SSID is too long!");
		return -EINVAL;
	}

	wl->ssid_len = ssid_len;
	memcpy(wl->ssid, ptr+2, ssid_len);
	return 0;
}

static void wl1271_remove_ie(struct sk_buff *skb, u8 eid, int ieoffset)
{
	int len;
	const u8 *next, *end = skb->data + skb->len;
	u8 *ie = (u8 *)cfg80211_find_ie(eid, skb->data + ieoffset,
					skb->len - ieoffset);
	if (!ie)
		return;
	len = ie[1] + 2;
	next = ie + len;
	memmove(ie, next, end - next);
	skb_trim(skb, skb->len - len);
}

static void wl1271_remove_vendor_ie(struct sk_buff *skb,
					    unsigned int oui, u8 oui_type,
					    int ieoffset)
{
	int len;
	const u8 *next, *end = skb->data + skb->len;
	u8 *ie = (u8 *)cfg80211_find_vendor_ie(oui, oui_type,
					       skb->data + ieoffset,
					       skb->len - ieoffset);
	if (!ie)
		return;
	len = ie[1] + 2;
	next = ie + len;
	memmove(ie, next, end - next);
	skb_trim(skb, skb->len - len);
}

static int wl1271_ap_set_probe_resp_tmpl(struct wl1271 *wl, u32 rates)
{
	struct sk_buff *skb;
	int ret;

	skb = ieee80211_proberesp_get(wl->hw, wl->vif);
	if (!skb)
		return -EINVAL;

	ret = wl1271_cmd_template_set(wl,
				      CMD_TEMPL_AP_PROBE_RESPONSE,
				      skb->data,
				      skb->len, 0,
				      rates);

	if (!ret)
		set_bit(WL1271_FLAG_PROBE_RESP_SET, &wl->flags);

	dev_kfree_skb(skb);
	return ret;
}

static int wl1271_ap_set_probe_resp_tmpl_legacy(struct wl1271 *wl,
					     u8 *probe_rsp_data,
					     size_t probe_rsp_len,
					     u32 rates)
{
	struct ieee80211_bss_conf *bss_conf = &wl->vif->bss_conf;
	u8 probe_rsp_templ[WL1271_CMD_TEMPL_MAX_SIZE];
	int ssid_ie_offset, ie_offset, templ_len;
	const u8 *ptr;

	/* no need to change probe response if the SSID is set correctly */
	if (wl->ssid_len > 0)
		return wl1271_cmd_template_set(wl,
					       CMD_TEMPL_AP_PROBE_RESPONSE,
					       probe_rsp_data,
					       probe_rsp_len, 0,
					       rates);

	if (probe_rsp_len + bss_conf->ssid_len > WL1271_CMD_TEMPL_MAX_SIZE) {
		wl1271_error("probe_rsp template too big");
		return -EINVAL;
	}

	/* start searching from IE offset */
	ie_offset = offsetof(struct ieee80211_mgmt, u.probe_resp.variable);

	ptr = cfg80211_find_ie(WLAN_EID_SSID, probe_rsp_data + ie_offset,
			       probe_rsp_len - ie_offset);
	if (!ptr) {
		wl1271_error("No SSID in beacon!");
		return -EINVAL;
	}

	ssid_ie_offset = ptr - probe_rsp_data;
	ptr += (ptr[1] + 2);

	memcpy(probe_rsp_templ, probe_rsp_data, ssid_ie_offset);

	/* insert SSID from bss_conf */
	probe_rsp_templ[ssid_ie_offset] = WLAN_EID_SSID;
	probe_rsp_templ[ssid_ie_offset + 1] = bss_conf->ssid_len;
	memcpy(probe_rsp_templ + ssid_ie_offset + 2,
	       bss_conf->ssid, bss_conf->ssid_len);
	templ_len = ssid_ie_offset + 2 + bss_conf->ssid_len;

	memcpy(probe_rsp_templ + ssid_ie_offset + 2 + bss_conf->ssid_len,
	       ptr, probe_rsp_len - (ptr - probe_rsp_data));
	templ_len += probe_rsp_len - (ptr - probe_rsp_data);

	return wl1271_cmd_template_set(wl,
				       CMD_TEMPL_AP_PROBE_RESPONSE,
				       probe_rsp_templ,
				       templ_len, 0,
				       rates);
}

static int wl1271_bss_erp_info_changed(struct wl1271 *wl,
				       struct ieee80211_bss_conf *bss_conf,
				       u32 changed)
{
	int ret = 0;

	if (changed & BSS_CHANGED_ERP_SLOT) {
		if (bss_conf->use_short_slot)
			ret = wl1271_acx_slot(wl, SLOT_TIME_SHORT);
		else
			ret = wl1271_acx_slot(wl, SLOT_TIME_LONG);
		if (ret < 0) {
			wl1271_warning("Set slot time failed %d", ret);
			goto out;
		}
	}

	if (changed & BSS_CHANGED_ERP_PREAMBLE) {
		if (bss_conf->use_short_preamble)
			wl1271_acx_set_preamble(wl, ACX_PREAMBLE_SHORT);
		else
			wl1271_acx_set_preamble(wl, ACX_PREAMBLE_LONG);
	}

	if (changed & BSS_CHANGED_ERP_CTS_PROT) {
		if (bss_conf->use_cts_prot)
			ret = wl1271_acx_cts_protect(wl, CTSPROTECT_ENABLE);
		else
			ret = wl1271_acx_cts_protect(wl, CTSPROTECT_DISABLE);
		if (ret < 0) {
			wl1271_warning("Set ctsprotect failed %d", ret);
			goto out;
		}
	}

out:
	return ret;
}

static int wl1271_bss_beacon_info_changed(struct wl1271 *wl,
					  struct ieee80211_vif *vif,
					  struct ieee80211_bss_conf *bss_conf,
					  u32 changed)
{
	bool is_ap = (wl->bss_type == BSS_TYPE_AP_BSS);
	int ret = 0;

	if ((changed & BSS_CHANGED_BEACON_INT)) {
		wl1271_debug(DEBUG_MASTER, "beacon interval updated: %d",
			bss_conf->beacon_int);

		wl->beacon_int = bss_conf->beacon_int;
	}

	if ((changed & BSS_CHANGED_AP_PROBE_RESP) && is_ap) {
		ret = wl1271_ap_set_probe_resp_tmpl(wl,
			      wl1271_tx_min_rate_get(wl, wl->basic_rate_set));

		/* if BSS_CHANGED_BEACON is also marked then fall through
		 * and the probe response will be set from the beacon */
		if ((ret < 0) &&
		    !(changed & BSS_CHANGED_BEACON))
			goto out;
	}

	if ((changed & BSS_CHANGED_BEACON)) {
		struct ieee80211_hdr *hdr;
		u32 min_rate;
		int ieoffset = offsetof(struct ieee80211_mgmt,
					u.beacon.variable);
		struct sk_buff *beacon = ieee80211_beacon_get(wl->hw, vif);
		u16 tmpl_id;

		if (!beacon) {
			ret = -EINVAL;
			goto out;
		}

		wl1271_debug(DEBUG_MASTER, "beacon updated");

		ret = wl1271_ssid_set(wl, beacon, ieoffset);
		if (ret < 0) {
			dev_kfree_skb(beacon);
			goto out;
		}
		min_rate = wl1271_tx_min_rate_get(wl, wl->basic_rate_set);
		tmpl_id = is_ap ? CMD_TEMPL_AP_BEACON :
				  CMD_TEMPL_BEACON;
		ret = wl1271_cmd_template_set(wl, tmpl_id,
					      beacon->data,
					      beacon->len, 0,
					      min_rate);
		if (ret < 0) {
			dev_kfree_skb(beacon);
			goto out;
		}

		/*
		 * In case we already have a probe-resp beacon set explicitly
		 * by usermode, don't use the beacon data.
		 */
		if (test_bit(WL1271_FLAG_PROBE_RESP_SET, &wl->flags))
			goto end_bcn;

		/* remove TIM ie from probe response */
		wl1271_remove_ie(beacon, WLAN_EID_TIM, ieoffset);

		/*
		 * remove p2p ie from probe response.
		 * the fw reponds to probe requests that don't include the p2p ie.
		 * probe requests with p2p ie will be passed, and get responded
		 * by the supplicant (the spec forbids including the p2p ie when
		 * responding to probe requests that didn't include it).
		 */
		wl1271_remove_vendor_ie(beacon, WLAN_OUI_WFA,
					WLAN_OUI_TYPE_WFA_P2P, ieoffset);

		hdr = (struct ieee80211_hdr *) beacon->data;
		hdr->frame_control = cpu_to_le16(IEEE80211_FTYPE_MGMT |
						 IEEE80211_STYPE_PROBE_RESP);
		if (is_ap)
			ret = wl1271_ap_set_probe_resp_tmpl_legacy(wl,
						beacon->data,
						beacon->len,
						min_rate);
		else
			ret = wl1271_cmd_template_set(wl,
						CMD_TEMPL_PROBE_RESPONSE,
						beacon->data,
						beacon->len, 0,
						min_rate);
end_bcn:
		dev_kfree_skb(beacon);
		if (ret < 0)
			goto out;
	}

out:
	if (ret != 0)
		wl1271_error("beacon info change failed: %d", ret);
	return ret;
}

/* forward declaration */
static int wl1271_op_sta_add(struct ieee80211_hw *hw,
			     struct ieee80211_vif *vif,
			     struct ieee80211_sta *sta);

static void wl12xx_add_sta_iterator(struct ieee80211_hw *hw,
				    struct ieee80211_vif *vif,
				    struct ieee80211_sta *sta,
				    void *data)
{
	struct wl1271_station *wl_sta = (struct wl1271_station *)sta->drv_priv;

	if (wl_sta->added)
		return;

	wl1271_op_sta_add(hw, vif, sta);
}

static void wl12xx_inconn_sta_iterator(struct ieee80211_hw *hw,
				       struct ieee80211_vif *vif,
				       struct ieee80211_sta *sta,
				       void *data)
{
	struct wl1271_station *wl_sta = (struct wl1271_station *)sta->drv_priv;
	struct wl1271 *wl = hw->priv;
	int ret;

	if (wl_sta->added)
		return;

	mutex_lock(&wl->mutex);

	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		goto out;

	wl1271_acx_set_inconnection_sta(wl, sta->addr);

	wl1271_ps_elp_sleep(wl);
out:
	mutex_unlock(&wl->mutex);
}

static void wl12xx_ap_start_work(struct work_struct *work)
{
	int ret;
	struct wl1271 *wl =
		container_of(work, struct wl1271, ap_start_work);

	/*
	 * Cause existing stations to be "in-connection" to prevent the FW
	 * from de-authenticating them.
	 */
	ieee80211_iterate_sta(wl->vif, wl12xx_inconn_sta_iterator, NULL);

	/* only lock wl->mutex here to prevent lock inversion */
	mutex_lock(&wl->mutex);

	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		goto out;

	ret = wl1271_cmd_role_start_ap(wl);
	if (ret < 0)
		goto out_sleep;

	ret = wl1271_ap_init_hwenc(wl);
	if (ret < 0)
		goto out_sleep;

	set_bit(WL1271_FLAG_AP_STARTED, &wl->flags);
	wl1271_debug(DEBUG_AP, "started AP");

out_sleep:
	wl1271_ps_elp_sleep(wl);
out:
	mutex_unlock(&wl->mutex);

	/* actually add the stations - with wl->mutex unlocked */
	if (!ret)
		ieee80211_iterate_sta(wl->vif, wl12xx_add_sta_iterator, NULL);
}

/* AP mode changes */
static void wl1271_bss_info_changed_ap(struct wl1271 *wl,
				       struct ieee80211_vif *vif,
				       struct ieee80211_bss_conf *bss_conf,
				       u32 changed)
{
	int ret = 0;

	if ((changed & BSS_CHANGED_BASIC_RATES)) {
		u32 rates = bss_conf->basic_rates;

		wl->basic_rate_set = wl1271_tx_enabled_rates_get(wl, rates,
								 wl->band);
		wl->basic_rate = wl1271_tx_min_rate_get(wl,
							wl->basic_rate_set);

		ret = wl1271_init_ap_rates(wl);
		if (ret < 0) {
			wl1271_error("AP rate policy change failed %d", ret);
			goto out;
		}

		ret = wl1271_ap_init_templates(wl);
		if (ret < 0)
			goto out;
	}

	ret = wl1271_bss_beacon_info_changed(wl, vif, bss_conf, changed);
	if (ret < 0)
		goto out;

	if ((changed & BSS_CHANGED_BEACON_ENABLED)) {
		if (bss_conf->enable_beacon) {
			if (!test_bit(WL1271_FLAG_AP_STARTED, &wl->flags)) {
				/*
				 * Actually start the AP in a separate
				 * work to avoid wl->mutex deadlocks
				 */
				ieee80211_queue_work(wl->hw,
						     &wl->ap_start_work);
			}
		} else {
			if (test_bit(WL1271_FLAG_AP_STARTED, &wl->flags)) {
				ret = wl1271_cmd_role_stop_ap(wl);
				if (ret < 0)
					goto out;

				clear_bit(WL1271_FLAG_AP_STARTED, &wl->flags);
				clear_bit(WL1271_FLAG_PROBE_RESP_SET,
					    &wl->flags);
				wl1271_debug(DEBUG_AP, "stopped AP");
			}
		}
	}

	ret = wl1271_bss_erp_info_changed(wl, bss_conf, changed);
	if (ret < 0)
		goto out;

	/* Handle HT information change */
	if ((changed & BSS_CHANGED_HT) &&
	    (bss_conf->channel_type != NL80211_CHAN_NO_HT)) {
		ret = wl1271_acx_set_ht_information(wl,
					bss_conf->ht_operation_mode);
		if (ret < 0) {
			wl1271_warning("Set ht information failed %d", ret);
			goto out;
		}
	}

out:
	return;
}

/* STA/IBSS mode changes */
static void wl1271_bss_info_changed_sta(struct wl1271 *wl,
					struct ieee80211_vif *vif,
					struct ieee80211_bss_conf *bss_conf,
					u32 changed)
{
	bool do_join = false, set_assoc = false;
	bool is_ibss = (wl->bss_type == BSS_TYPE_IBSS);
	bool ibss_joined = false;
	u32 sta_rate_set = 0;
	int ret;
	struct ieee80211_sta *sta;
	bool sta_exists = false;
	struct ieee80211_sta_ht_cap sta_ht_cap;

	if (is_ibss) {
		ret = wl1271_bss_beacon_info_changed(wl, vif, bss_conf,
						     changed);
		if (ret < 0)
			goto out;
	}

	if (changed & BSS_CHANGED_IBSS) {
		if (bss_conf->ibss_joined) {
			set_bit(WL1271_FLAG_IBSS_JOINED, &wl->flags);
			ibss_joined = true;
		} else {
			if (test_and_clear_bit(WL1271_FLAG_IBSS_JOINED,
					       &wl->flags)) {
				wl1271_unjoin(wl);
				wl1271_cmd_role_start_dev(wl);
				wl1271_roc(wl, wl->dev_role_id);
			}
		}
	}

	if ((changed & BSS_CHANGED_BEACON_INT) && ibss_joined)
		do_join = true;

	/* Need to update the SSID (for filtering etc) */
	if ((changed & BSS_CHANGED_BEACON) && ibss_joined)
		do_join = true;

	if ((changed & BSS_CHANGED_BEACON_ENABLED) && ibss_joined) {
		wl1271_debug(DEBUG_ADHOC, "ad-hoc beaconing: %s",
			     bss_conf->enable_beacon ? "enabled" : "disabled");

		if (bss_conf->enable_beacon)
			wl->set_bss_type = BSS_TYPE_IBSS;
		else
			wl->set_bss_type = BSS_TYPE_STA_BSS;
		do_join = true;
	}

	if ((changed & BSS_CHANGED_CQM)) {
		bool enable = false;
		if (bss_conf->cqm_rssi_thold)
			enable = true;
		ret = wl1271_acx_rssi_snr_trigger(wl, enable,
						  bss_conf->cqm_rssi_thold,
						  bss_conf->cqm_rssi_hyst);
		if (ret < 0)
			goto out;
		wl->rssi_thold = bss_conf->cqm_rssi_thold;
	}

	if ((changed & BSS_CHANGED_BSSID) &&
	    /*
	     * Now we know the correct bssid, so we send a new join command
	     * and enable the BSSID filter
	     */
	    memcmp(wl->bssid, bss_conf->bssid, ETH_ALEN)) {
		memcpy(wl->bssid, bss_conf->bssid, ETH_ALEN);

		if (!is_zero_ether_addr(wl->bssid)) {
			ret = wl1271_cmd_build_null_data(wl);
			if (ret < 0)
				goto out;

			ret = wl1271_build_qos_null_data(wl);
			if (ret < 0)
				goto out;

			/* Need to update the BSSID (for filtering etc) */
			if (bss_conf->assoc)
				do_join = true;
		}
	}

	if (changed & (BSS_CHANGED_ASSOC | BSS_CHANGED_HT)) {
		rcu_read_lock();
		sta = ieee80211_find_sta(vif, bss_conf->bssid);
		if (!sta)
			goto sta_not_found;

		/* save the supp_rates of the ap */
		sta_rate_set = sta->supp_rates[wl->hw->conf.channel->band];
		if (sta->ht_cap.ht_supported)
			sta_rate_set |=
			    (sta->ht_cap.mcs.rx_mask[0] << HW_HT_RATES_OFFSET);
		sta_ht_cap = sta->ht_cap;
		sta_exists = true;

sta_not_found:
		rcu_read_unlock();
	}

	if ((changed & BSS_CHANGED_ASSOC)) {
		if (bss_conf->assoc) {
			u32 rates;
			int ieoffset;
			wl->aid = bss_conf->aid;
			set_assoc = true;

			wl->ps_poll_failures = 0;

			/*
			 * use basic rates from AP, and determine lowest rate
			 * to use with control frames.
			 */
			rates = bss_conf->basic_rates;
			wl->basic_rate_set =
				wl1271_tx_enabled_rates_get(wl, rates,
							    wl->band);
			wl->basic_rate =
				wl1271_tx_min_rate_get(wl, wl->basic_rate_set);
			if (sta_rate_set) {
				wl->rate_set = wl1271_tx_enabled_rates_get(wl,
								sta_rate_set,
								wl->band);
				wl1271_tx_set_max_rate(wl, wl->rate_set,
						       &sta_ht_cap);
			}

			ret = wl1271_acx_sta_rate_policies(wl);
			if (ret < 0)
				goto out;

			/*
			 * with wl1271, we don't need to update the
			 * beacon_int and dtim_period, because the firmware
			 * updates it by itself when the first beacon is
			 * received after a join.
			 */
			ret = wl1271_cmd_build_ps_poll(wl, wl->aid);
			if (ret < 0)
				goto out;

			/*
			 * Get a template for hardware connection maintenance
			 */
			dev_kfree_skb(wl->probereq);
			wl->probereq = wl1271_cmd_build_ap_probe_req(wl, NULL);
			ieoffset = offsetof(struct ieee80211_mgmt,
					    u.probe_req.variable);
			wl1271_ssid_set(wl, wl->probereq, ieoffset);

			/* enable the connection monitoring feature */
			ret = wl1271_acx_conn_monit_params(wl, true);
			if (ret < 0)
				goto out;
		} else {
			/* use defaults when not associated */
			bool was_assoc =
			    !!test_and_clear_bit(WL1271_FLAG_STA_ASSOCIATED,
						 &wl->flags);
			bool was_ifup =
			    !!test_and_clear_bit(WL1271_FLAG_STA_STATE_SENT,
						 &wl->flags);
			wl->aid = 0;

			/* free probe-request template */
			dev_kfree_skb(wl->probereq);
			wl->probereq = NULL;

			/* re-enable dynamic ps - just in case */
			ieee80211_enable_dyn_ps(wl->vif);

			/* revert back to minimum rates for the current band */
			wl1271_set_band_rate(wl);
			wl->basic_rate =
				wl1271_tx_min_rate_get(wl, wl->basic_rate_set);
			ret = wl1271_acx_sta_rate_policies(wl);
			if (ret < 0)
				goto out;

			/* disable connection monitor features */
			ret = wl1271_acx_conn_monit_params(wl, false);

			/* Disable the keep-alive feature */
			ret = wl1271_acx_keep_alive_mode(wl, false);
			if (ret < 0)
				goto out;

			/* restore the bssid filter and go to dummy bssid */
			if (was_assoc) {
				u32 conf_flags = wl->hw->conf.flags;
				/*
				 * we might have to disable roc, if there was
				 * no IF_OPER_UP notification.
				 */
				if (!was_ifup) {
					ret = wl1271_croc(wl, wl->role_id);
					if (ret < 0)
						goto out;
				}
				/*
				 * (we also need to disable roc in case of
				 * roaming on the same channel. until we will
				 * have a better flow...)
				 */
				if (test_bit(wl->dev_role_id, wl->roc_map)) {
					ret = wl1271_croc(wl, wl->dev_role_id);
					if (ret < 0)
						goto out;
				}

				wl1271_unjoin(wl);
				if (!(conf_flags & IEEE80211_CONF_IDLE)) {
					wl1271_cmd_role_start_dev(wl);
					wl1271_roc(wl, wl->dev_role_id);
				}
			}
		}
	}

	if (changed & BSS_CHANGED_IBSS) {
		wl1271_debug(DEBUG_ADHOC, "ibss_joined: %d",
			     bss_conf->ibss_joined);

		if (bss_conf->ibss_joined) {
			u32 rates = bss_conf->basic_rates;
			wl->basic_rate_set =
				wl1271_tx_enabled_rates_get(wl, rates,
							    wl->band);
			wl->basic_rate =
				wl1271_tx_min_rate_get(wl, wl->basic_rate_set);

			/* by default, use 11b rates */
			wl->rate_set = CONF_TX_IBSS_DEFAULT_RATES;
			ret = wl1271_acx_sta_rate_policies(wl);
			if (ret < 0)
				goto out;
		}
	}

	ret = wl1271_bss_erp_info_changed(wl, bss_conf, changed);
	if (ret < 0)
		goto out;

	if (do_join) {
		ret = wl1271_join(wl, set_assoc);
		if (ret < 0) {
			wl1271_warning("cmd join failed %d", ret);
			goto out;
		}

		/* ROC until interface is up (after EAPOL exchange) */
		if (!is_ibss) {
			ret = wl1271_roc(wl, wl->role_id);
			if (ret < 0)
				goto out;

			wl1271_check_operstate(wl, ieee80211_get_operstate(vif));
		}
		/*
		 * stop device role if started (we might already be in
		 * STA role). TODO: make it better.
		 */
		if (wl->dev_hlid != WL1271_INVALID_LINK_ID) {
			ret = wl1271_croc(wl, wl->dev_role_id);
			if (ret < 0)
				goto out;

			ret = wl1271_cmd_role_stop_dev(wl);
			if (ret < 0)
				goto out;
		}

		/* If we want to go in PSM but we're not there yet */
		if (test_bit(WL1271_FLAG_PSM_REQUESTED, &wl->flags) &&
		    !test_bit(WL1271_FLAG_PSM, &wl->flags)) {
			enum wl1271_cmd_ps_mode mode;

			mode = STATION_POWER_SAVE_MODE;
			ret = wl1271_ps_set_mode(wl, mode,
						 wl->basic_rate,
						 true);
			if (ret < 0)
				goto out;
		}
	}

	/* Handle new association with HT. Do this after join. */
	if (sta_exists) {
		if ((changed & BSS_CHANGED_HT) &&
		    (bss_conf->channel_type != NL80211_CHAN_NO_HT)) {
			ret = wl1271_acx_set_ht_capabilities(wl,
							     &sta_ht_cap,
							     true,
							     wl->sta_hlid);
			if (ret < 0) {
				wl1271_warning("Set ht cap true failed %d",
					       ret);
				goto out;
			}
		}
		/* handle new association without HT and disassociation */
		else if (changed & BSS_CHANGED_ASSOC) {
			ret = wl1271_acx_set_ht_capabilities(wl,
							     &sta_ht_cap,
							     false,
							     wl->sta_hlid);
			if (ret < 0) {
				wl1271_warning("Set ht cap false failed %d",
					       ret);
				goto out;
			}
		}
	}

	/* Handle HT information change. Done after join. */
	if ((changed & BSS_CHANGED_HT) &&
	    (bss_conf->channel_type != NL80211_CHAN_NO_HT)) {
		ret = wl1271_acx_set_ht_information(wl,
					bss_conf->ht_operation_mode);
		if (ret < 0) {
			wl1271_warning("Set ht information failed %d", ret);
			goto out;
		}
	}

	/* Handle arp filtering. Done after join. */
	if ((changed & BSS_CHANGED_ARP_FILTER) ||
	    (!is_ibss && (changed & BSS_CHANGED_QOS))) {
		__be32 addr = bss_conf->arp_addr_list[0];
		wl->qos = bss_conf->qos;
		WARN_ON(wl->bss_type != BSS_TYPE_STA_BSS);

		if (bss_conf->arp_addr_cnt == 1 &&
		    bss_conf->arp_filter_enabled) {
			wl->ip_addr = addr;
			/*
			 * The template should have been configured only upon
			 * association. however, it seems that the correct ip
			 * isn't being set (when sending), so we have to
			 * reconfigure the template upon every ip change.
			 */
			ret = wl1271_cmd_build_arp_rsp(wl, addr);
			if (ret < 0) {
				wl1271_warning("build arp rsp failed: %d", ret);
				goto out;
			}

			ret = wl1271_acx_arp_ip_filter(wl,
				(ACX_ARP_FILTER_ARP_FILTERING |
				 ACX_ARP_FILTER_AUTO_ARP),
				addr);
		} else {
			wl->ip_addr = 0;
			ret = wl1271_acx_arp_ip_filter(wl, 0, addr);
		}

		if (ret < 0)
			goto out;
	}

out:
	return;
}

static void wl1271_op_bss_info_changed(struct ieee80211_hw *hw,
				       struct ieee80211_vif *vif,
				       struct ieee80211_bss_conf *bss_conf,
				       u32 changed)
{
	struct wl1271 *wl = hw->priv;
	bool is_ap = (wl->bss_type == BSS_TYPE_AP_BSS);
	int ret;

	wl1271_debug(DEBUG_MAC80211, "mac80211 bss info changed 0x%x",
		     (int)changed);

	mutex_lock(&wl->mutex);

	if (unlikely(wl->state == WL1271_STATE_OFF))
		goto out;

	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		goto out;

	if (is_ap)
		wl1271_bss_info_changed_ap(wl, vif, bss_conf, changed);
	else
		wl1271_bss_info_changed_sta(wl, vif, bss_conf, changed);

	wl1271_ps_elp_sleep(wl);

out:
	mutex_unlock(&wl->mutex);
}

static int wl1271_op_conf_tx(struct ieee80211_hw *hw, u16 queue,
			     const struct ieee80211_tx_queue_params *params)
{
	struct wl1271 *wl = hw->priv;
	u8 ps_scheme;
	int ret = 0;

	mutex_lock(&wl->mutex);

	wl1271_debug(DEBUG_MAC80211, "mac80211 conf tx %d", queue);

	if (params->uapsd)
		ps_scheme = CONF_PS_SCHEME_UPSD_TRIGGER;
	else
		ps_scheme = CONF_PS_SCHEME_LEGACY;

	if (wl->state == WL1271_STATE_OFF) {
		/*
		 * If the state is off, the parameters will be recorded and
		 * configured on init. This happens in AP-mode.
		 */
		struct conf_tx_ac_category *conf_ac =
			&wl->conf.tx.ac_conf[wl1271_tx_get_queue(queue)];
		struct conf_tx_tid *conf_tid =
			&wl->conf.tx.tid_conf[wl1271_tx_get_queue(queue)];

		conf_ac->ac = wl1271_tx_get_queue(queue);
		conf_ac->cw_min = (u8)params->cw_min;
		conf_ac->cw_max = params->cw_max;
		conf_ac->aifsn = params->aifs;
		conf_ac->tx_op_limit = params->txop << 5;

		conf_tid->queue_id = wl1271_tx_get_queue(queue);
		conf_tid->channel_type = CONF_CHANNEL_TYPE_EDCF;
		conf_tid->tsid = wl1271_tx_get_queue(queue);
		conf_tid->ps_scheme = ps_scheme;
		conf_tid->ack_policy = CONF_ACK_POLICY_LEGACY;
		conf_tid->apsd_conf[0] = 0;
		conf_tid->apsd_conf[1] = 0;
		goto out;
	}

	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		goto out;

	/*
	 * the txop is confed in units of 32us by the mac80211,
	 * we need us
	 */
	ret = wl1271_acx_ac_cfg(wl, wl1271_tx_get_queue(queue),
				params->cw_min, params->cw_max,
				params->aifs, params->txop << 5);
	if (ret < 0)
		goto out_sleep;

	ret = wl1271_acx_tid_cfg(wl, wl1271_tx_get_queue(queue),
				 CONF_CHANNEL_TYPE_EDCF,
				 wl1271_tx_get_queue(queue),
				 ps_scheme, CONF_ACK_POLICY_LEGACY,
				 0, 0);

out_sleep:
	wl1271_ps_elp_sleep(wl);

out:
	mutex_unlock(&wl->mutex);

	return ret;
}

static u64 wl1271_op_get_tsf(struct ieee80211_hw *hw)
{

	struct wl1271 *wl = hw->priv;
	u64 mactime = ULLONG_MAX;
	int ret;

	wl1271_debug(DEBUG_MAC80211, "mac80211 get tsf");

	mutex_lock(&wl->mutex);

	if (unlikely(wl->state == WL1271_STATE_OFF))
		goto out;

	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		goto out;

	ret = wl1271_acx_tsf_info(wl, &mactime);
	if (ret < 0)
		goto out_sleep;

out_sleep:
	wl1271_ps_elp_sleep(wl);

out:
	mutex_unlock(&wl->mutex);
	return mactime;
}

static int wl1271_op_get_survey(struct ieee80211_hw *hw, int idx,
				struct survey_info *survey)
{
	struct wl1271 *wl = hw->priv;
	struct ieee80211_conf *conf = &hw->conf;

	if (idx != 0)
		return -ENOENT;

	survey->channel = conf->channel;
	survey->filled = SURVEY_INFO_NOISE_DBM;
	survey->noise = wl->noise;

	return 0;
}

static int wl1271_allocate_sta(struct wl1271 *wl,
			     struct ieee80211_sta *sta,
			     u8 *hlid)
{
	struct wl1271_station *wl_sta;
	int id;

	id = find_first_zero_bit(wl->ap_hlid_map, AP_MAX_STATIONS);
	if (id >= AP_MAX_STATIONS) {
		wl1271_warning("could not allocate HLID - too much stations");
		return -EBUSY;
	}

	wl_sta = (struct wl1271_station *)sta->drv_priv;
	set_bit(id, wl->ap_hlid_map);
	wl_sta->hlid = WL1271_AP_STA_HLID_START + id;
	*hlid = wl_sta->hlid;
	memcpy(wl->links[wl_sta->hlid].addr, sta->addr, ETH_ALEN);
	wl->active_sta_count++;
	return 0;
}

void wl1271_free_sta(struct wl1271 *wl, u8 hlid)
{
	int id = hlid - WL1271_AP_STA_HLID_START;

	if (hlid < WL1271_AP_STA_HLID_START)
		return;

	if (!test_bit(id, wl->ap_hlid_map))
		return;

	clear_bit(id, wl->ap_hlid_map);
	memset(wl->links[hlid].addr, 0, ETH_ALEN);
	wl->links[hlid].ba_bitmap = 0;
	wl1271_tx_reset_link_queues(wl, hlid);
	__clear_bit(hlid, &wl->ap_ps_map);
	__clear_bit(hlid, (unsigned long *)&wl->ap_fw_ps_map);
	wl->active_sta_count--;
}

static int wl12xx_sta_authorize(struct wl1271 *wl,
				struct ieee80211_sta *sta)
{
	struct wl1271_station *wl_sta;
	int ret;

	wl_sta = (struct wl1271_station *)sta->drv_priv;
	if (wl_sta->authorized)
		return -EALREADY;

	ret = wl1271_acx_set_ht_capabilities(wl, &sta->ht_cap, true,
					     wl_sta->hlid);
	wl_sta->authorized = true;

	return ret;
}

static int wl1271_op_sta_add_locked(struct ieee80211_hw *hw,
				    struct ieee80211_vif *vif,
				    struct ieee80211_sta *sta)
{
	struct wl1271 *wl = hw->priv;
	struct wl1271_station *wl_sta;
	int ret = 0;
	u8 hlid;
	ret = wl1271_allocate_sta(wl, sta, &hlid);
	if (ret < 0)
		return ret;

	ret = wl1271_cmd_add_peer(wl, sta, hlid);
	if (ret < 0)
		goto out;

	ret = wl1271_cmd_set_peer_state(wl, hlid);
	if (ret < 0)
		goto out;

	if (sta->authorized) {
		ret = wl12xx_sta_authorize(wl, sta);
		if (ret < 0)
			goto out;
	}

	wl_sta = (struct wl1271_station *)sta->drv_priv;
	wl_sta->added = true;

	return 0;
out:
	wl1271_free_sta(wl, hlid);
	return ret;
}

static void wl12xx_op_sta_authorize(struct ieee80211_hw *hw,
				    struct ieee80211_vif *vif,
				    struct ieee80211_sta *sta)
{
	struct wl1271 *wl = hw->priv;
	int ret;

	wl1271_debug(DEBUG_MAC80211, "mac80211 auth sta %d", (int)sta->aid);
	mutex_lock(&wl->mutex);

	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		goto out;

	ret = wl12xx_sta_authorize(wl, sta);

	wl1271_ps_elp_sleep(wl);
out:
	mutex_unlock(&wl->mutex);

}
static int wl1271_op_sta_add(struct ieee80211_hw *hw,
			     struct ieee80211_vif *vif,
			     struct ieee80211_sta *sta)
{
	struct wl1271 *wl = hw->priv;
	int ret = 0;

	wl1271_debug(DEBUG_MAC80211, "mac80211 add sta %d", (int)sta->aid);

	mutex_lock(&wl->mutex);

	if (unlikely(wl->state == WL1271_STATE_OFF))
		goto out;

	if (wl->bss_type != BSS_TYPE_AP_BSS)
		goto out;

	if (!test_bit(WL1271_FLAG_AP_STARTED, &wl->flags)) {
		wl1271_debug(DEBUG_MAC80211, "skipping add sta "
			     "while ap is not started");
		goto out;
	}

	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		goto out;

	ret = wl1271_op_sta_add_locked(hw, vif, sta);

	wl1271_ps_elp_sleep(wl);
out:
	mutex_unlock(&wl->mutex);
	return ret;
}

static int wl1271_op_sta_remove(struct ieee80211_hw *hw,
				struct ieee80211_vif *vif,
				struct ieee80211_sta *sta)
{
	struct wl1271 *wl = hw->priv;
	struct wl1271_station *wl_sta = (struct wl1271_station *)sta->drv_priv;
	int ret = 0, id;

	wl1271_debug(DEBUG_MAC80211, "mac80211 remove sta %d", (int)sta->aid);

	mutex_lock(&wl->mutex);

	if (unlikely(wl->state == WL1271_STATE_OFF))
		goto out;

	if (wl->bss_type != BSS_TYPE_AP_BSS)
		goto out;

	if (unlikely(!wl_sta->added))
		goto out;

	id = wl_sta->hlid - WL1271_AP_STA_HLID_START;
	if (WARN_ON(!test_bit(id, wl->ap_hlid_map)))
		goto out;

	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		goto out;

	ret = wl1271_cmd_remove_peer(wl, wl_sta->hlid);
	if (ret < 0)
		goto out_sleep;

	wl1271_free_sta(wl, wl_sta->hlid);

out_sleep:
	wl1271_ps_elp_sleep(wl);

out:
	mutex_unlock(&wl->mutex);
	wl_sta->added = false;
	return ret;
}

static int wl1271_op_ampdu_action(struct ieee80211_hw *hw,
				  struct ieee80211_vif *vif,
				  enum ieee80211_ampdu_mlme_action action,
				  struct ieee80211_sta *sta, u16 tid, u16 *ssn,
				  u8 buf_size)
{
	struct wl1271 *wl = hw->priv;
	int ret;
	u8 hlid, *ba_bitmap;

	wl1271_debug(DEBUG_MAC80211, "mac80211 ampdu action %d tid %d", action,
		     tid);

	/* sanity check - the fields in FW are only 8bits wide */
	if (WARN_ON(tid > 0xFF))
		return -ENOTSUPP;

	mutex_lock(&wl->mutex);

	if (unlikely(wl->state == WL1271_STATE_OFF)) {
		ret = -EAGAIN;
		goto out;
	}

	if (wl->bss_type == BSS_TYPE_STA_BSS) {
		hlid = wl->sta_hlid;
		ba_bitmap = &wl->ba_rx_bitmap;
	} else if (wl->bss_type == BSS_TYPE_AP_BSS) {
		struct wl1271_station *wl_sta;

		wl_sta = (struct wl1271_station *)sta->drv_priv;
		hlid = wl_sta->hlid;
		ba_bitmap = &wl->links[hlid].ba_bitmap;
	} else {
		ret = -EINVAL;
		goto out;
	}

	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		goto out;

	switch (action) {
	case IEEE80211_AMPDU_RX_START:
		if (!(wl->ba_support) || !(wl->ba_allowed)) {
			ret = -ENOTSUPP;
			break;
		}

		if (wl->ba_rx_session_count >= RX_BA_MAX_SESSIONS) {
			ret = -EBUSY;
			wl1271_error("exceeded max RX BA sessions");
			break;
		}

		if (*ba_bitmap & BIT(tid)) {
			ret = -EINVAL;
			wl1271_error("cannot enable RX BA session on active "
				     "tid: %d", tid);
			break;
		}

		ret = wl1271_acx_set_ba_receiver_session(wl, tid, *ssn, true,
							 hlid);
		if (!ret) {
			*ba_bitmap |= BIT(tid);
			wl->ba_rx_session_count++;
		}
		break;

	case IEEE80211_AMPDU_RX_STOP:
		if (!(*ba_bitmap & BIT(tid))) {
			ret = -EINVAL;
			wl1271_error("no active RX BA session on tid: %d",
				     tid);
			break;
		}

		ret = wl1271_acx_set_ba_receiver_session(wl, tid, 0, false,
							 hlid);
		if (!ret) {
			*ba_bitmap &= ~BIT(tid);
			wl->ba_rx_session_count--;
		}
		break;

	/*
	 * The BA initiator session management in FW independently.
	 * Falling break here on purpose for all TX APDU commands.
	 */
	case IEEE80211_AMPDU_TX_START:
	case IEEE80211_AMPDU_TX_STOP:
	case IEEE80211_AMPDU_TX_OPERATIONAL:
		ret = -EINVAL;
		break;

	default:
		wl1271_error("Incorrect ampdu action id=%x\n", action);
		ret = -EINVAL;
	}

	wl1271_ps_elp_sleep(wl);

out:
	mutex_unlock(&wl->mutex);

	return ret;
}

static void wl12xx_op_channel_switch(struct ieee80211_hw *hw,
				     struct ieee80211_channel_switch *ch_switch)
{
	struct wl1271 *wl = hw->priv;
	int ret;

	wl1271_debug(DEBUG_MAC80211, "mac80211 channel switch");

	mutex_lock(&wl->mutex);

	if (unlikely(wl->state == WL1271_STATE_OFF)) {
		/* we support configuring the channel and band while off */
		wl->channel = ch_switch->channel->hw_value;
		wl->band = ch_switch->channel->band;
		goto out;
	}

	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		goto out;

	/* send all pending packets */
	wl1271_tx_work_locked(wl);

	ret = wl12xx_cmd_channel_switch(wl, ch_switch);

	if (!ret)
		set_bit(WL1271_FLAG_CS_PROGRESS, &wl->flags);

	wl1271_ps_elp_sleep(wl);

out:
	mutex_unlock(&wl->mutex);
}

static int wl12xx_set_bitrate_mask(struct ieee80211_hw *hw,
				   struct ieee80211_vif *vif,
				   const struct cfg80211_bitrate_mask *mask)
{
	struct wl1271 *wl = hw->priv;
	int i;

	wl1271_debug(DEBUG_MAC80211, "mac80211 set_bitrate_mask 0x%x 0x%x",
		mask->control[NL80211_BAND_2GHZ].legacy,
		mask->control[NL80211_BAND_5GHZ].legacy);

	mutex_lock(&wl->mutex);

	for (i = 0; i < IEEE80211_NUM_BANDS; i++)
		wl->bitrate_masks[i] =
			wl1271_tx_enabled_rates_get(wl,
						    mask->control[i].legacy,
						    i);
	mutex_unlock(&wl->mutex);

	return 0;
}

static bool wl1271_tx_frames_pending(struct ieee80211_hw *hw)
{
	struct wl1271 *wl = hw->priv;
	bool ret = false;

	mutex_lock(&wl->mutex);

	if (unlikely(wl->state == WL1271_STATE_OFF))
		goto out;

	/* packets are considered pending if in the TX queue or the FW */
	ret = (wl1271_tx_total_queue_count(wl) > 0) || (wl->tx_frames_cnt > 0);

	/* the above is appropriate for STA mode for PS purposes */
	WARN_ON(wl->bss_type != BSS_TYPE_STA_BSS);

out:
	mutex_unlock(&wl->mutex);

	return ret;
}

/* can't be const, mac80211 writes to this */
static struct ieee80211_rate wl1271_rates[] = {
	{ .bitrate = 10,
	  .hw_value = CONF_HW_BIT_RATE_1MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_1MBPS, },
	{ .bitrate = 20,
	  .hw_value = CONF_HW_BIT_RATE_2MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_2MBPS,
	  .flags = IEEE80211_RATE_SHORT_PREAMBLE },
	{ .bitrate = 55,
	  .hw_value = CONF_HW_BIT_RATE_5_5MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_5_5MBPS,
	  .flags = IEEE80211_RATE_SHORT_PREAMBLE },
	{ .bitrate = 110,
	  .hw_value = CONF_HW_BIT_RATE_11MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_11MBPS,
	  .flags = IEEE80211_RATE_SHORT_PREAMBLE },
	{ .bitrate = 60,
	  .hw_value = CONF_HW_BIT_RATE_6MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_6MBPS, },
	{ .bitrate = 90,
	  .hw_value = CONF_HW_BIT_RATE_9MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_9MBPS, },
	{ .bitrate = 120,
	  .hw_value = CONF_HW_BIT_RATE_12MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_12MBPS, },
	{ .bitrate = 180,
	  .hw_value = CONF_HW_BIT_RATE_18MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_18MBPS, },
	{ .bitrate = 240,
	  .hw_value = CONF_HW_BIT_RATE_24MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_24MBPS, },
	{ .bitrate = 360,
	 .hw_value = CONF_HW_BIT_RATE_36MBPS,
	 .hw_value_short = CONF_HW_BIT_RATE_36MBPS, },
	{ .bitrate = 480,
	  .hw_value = CONF_HW_BIT_RATE_48MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_48MBPS, },
	{ .bitrate = 540,
	  .hw_value = CONF_HW_BIT_RATE_54MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_54MBPS, },
};

/* can't be const, mac80211 writes to this */
static struct ieee80211_channel wl1271_channels[] = {
	{ .hw_value = 1, .center_freq = 2412, .max_power = 25 },
	{ .hw_value = 2, .center_freq = 2417, .max_power = 25 },
	{ .hw_value = 3, .center_freq = 2422, .max_power = 25 },
	{ .hw_value = 4, .center_freq = 2427, .max_power = 25 },
	{ .hw_value = 5, .center_freq = 2432, .max_power = 25 },
	{ .hw_value = 6, .center_freq = 2437, .max_power = 25 },
	{ .hw_value = 7, .center_freq = 2442, .max_power = 25 },
	{ .hw_value = 8, .center_freq = 2447, .max_power = 25 },
	{ .hw_value = 9, .center_freq = 2452, .max_power = 25 },
	{ .hw_value = 10, .center_freq = 2457, .max_power = 25 },
	{ .hw_value = 11, .center_freq = 2462, .max_power = 25 },
	{ .hw_value = 12, .center_freq = 2467, .max_power = 25 },
	{ .hw_value = 13, .center_freq = 2472, .max_power = 25 },
	{ .hw_value = 14, .center_freq = 2484, .max_power = 25 },
};

/* mapping to indexes for wl1271_rates */
static const u8 wl1271_rate_to_idx_2ghz[] = {
	/* MCS rates are used only with 11n */
	7,                            /* CONF_HW_RXTX_RATE_MCS7_SGI */
	7,                            /* CONF_HW_RXTX_RATE_MCS7 */
	6,                            /* CONF_HW_RXTX_RATE_MCS6 */
	5,                            /* CONF_HW_RXTX_RATE_MCS5 */
	4,                            /* CONF_HW_RXTX_RATE_MCS4 */
	3,                            /* CONF_HW_RXTX_RATE_MCS3 */
	2,                            /* CONF_HW_RXTX_RATE_MCS2 */
	1,                            /* CONF_HW_RXTX_RATE_MCS1 */
	0,                            /* CONF_HW_RXTX_RATE_MCS0 */

	11,                            /* CONF_HW_RXTX_RATE_54   */
	10,                            /* CONF_HW_RXTX_RATE_48   */
	9,                             /* CONF_HW_RXTX_RATE_36   */
	8,                             /* CONF_HW_RXTX_RATE_24   */

	/* TI-specific rate */
	CONF_HW_RXTX_RATE_UNSUPPORTED, /* CONF_HW_RXTX_RATE_22   */

	7,                             /* CONF_HW_RXTX_RATE_18   */
	6,                             /* CONF_HW_RXTX_RATE_12   */
	3,                             /* CONF_HW_RXTX_RATE_11   */
	5,                             /* CONF_HW_RXTX_RATE_9    */
	4,                             /* CONF_HW_RXTX_RATE_6    */
	2,                             /* CONF_HW_RXTX_RATE_5_5  */
	1,                             /* CONF_HW_RXTX_RATE_2    */
	0                              /* CONF_HW_RXTX_RATE_1    */
};

/* 11n STA capabilities */
#define HW_RX_HIGHEST_RATE	72

#define WL12XX_HT_CAP { \
	.cap = IEEE80211_HT_CAP_GRN_FLD | IEEE80211_HT_CAP_SGI_20 | \
	       (1 << IEEE80211_HT_CAP_RX_STBC_SHIFT), \
	.ht_supported = true, \
	.ampdu_factor = IEEE80211_HT_MAX_AMPDU_8K, \
	.ampdu_density = IEEE80211_HT_MPDU_DENSITY_8, \
	.mcs = { \
		.rx_mask = { 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, }, \
		.rx_highest = cpu_to_le16(HW_RX_HIGHEST_RATE), \
		.tx_params = IEEE80211_HT_MCS_TX_DEFINED, \
		}, \
}

/* can't be const, mac80211 writes to this */
static struct ieee80211_supported_band wl1271_band_2ghz = {
	.channels = wl1271_channels,
	.n_channels = ARRAY_SIZE(wl1271_channels),
	.bitrates = wl1271_rates,
	.n_bitrates = ARRAY_SIZE(wl1271_rates),
	.ht_cap	= WL12XX_HT_CAP,
};

/* 5 GHz data rates for WL1273 */
static struct ieee80211_rate wl1271_rates_5ghz[] = {
	{ .bitrate = 60,
	  .hw_value = CONF_HW_BIT_RATE_6MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_6MBPS, },
	{ .bitrate = 90,
	  .hw_value = CONF_HW_BIT_RATE_9MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_9MBPS, },
	{ .bitrate = 120,
	  .hw_value = CONF_HW_BIT_RATE_12MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_12MBPS, },
	{ .bitrate = 180,
	  .hw_value = CONF_HW_BIT_RATE_18MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_18MBPS, },
	{ .bitrate = 240,
	  .hw_value = CONF_HW_BIT_RATE_24MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_24MBPS, },
	{ .bitrate = 360,
	 .hw_value = CONF_HW_BIT_RATE_36MBPS,
	 .hw_value_short = CONF_HW_BIT_RATE_36MBPS, },
	{ .bitrate = 480,
	  .hw_value = CONF_HW_BIT_RATE_48MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_48MBPS, },
	{ .bitrate = 540,
	  .hw_value = CONF_HW_BIT_RATE_54MBPS,
	  .hw_value_short = CONF_HW_BIT_RATE_54MBPS, },
};

/* 5 GHz band channels for WL1273 */
static struct ieee80211_channel wl1271_channels_5ghz[] = {
	{ .hw_value = 7, .center_freq = 5035, .max_power = 25 },
	{ .hw_value = 8, .center_freq = 5040, .max_power = 25 },
	{ .hw_value = 9, .center_freq = 5045, .max_power = 25 },
	{ .hw_value = 11, .center_freq = 5055, .max_power = 25 },
	{ .hw_value = 12, .center_freq = 5060, .max_power = 25 },
	{ .hw_value = 16, .center_freq = 5080, .max_power = 25 },
	{ .hw_value = 34, .center_freq = 5170, .max_power = 25 },
	{ .hw_value = 36, .center_freq = 5180, .max_power = 25 },
	{ .hw_value = 38, .center_freq = 5190, .max_power = 25 },
	{ .hw_value = 40, .center_freq = 5200, .max_power = 25 },
	{ .hw_value = 42, .center_freq = 5210, .max_power = 25 },
	{ .hw_value = 44, .center_freq = 5220, .max_power = 25 },
	{ .hw_value = 46, .center_freq = 5230, .max_power = 25 },
	{ .hw_value = 48, .center_freq = 5240, .max_power = 25 },
	{ .hw_value = 52, .center_freq = 5260, .max_power = 25 },
	{ .hw_value = 56, .center_freq = 5280, .max_power = 25 },
	{ .hw_value = 60, .center_freq = 5300, .max_power = 25 },
	{ .hw_value = 64, .center_freq = 5320, .max_power = 25 },
	{ .hw_value = 100, .center_freq = 5500, .max_power = 25 },
	{ .hw_value = 104, .center_freq = 5520, .max_power = 25 },
	{ .hw_value = 108, .center_freq = 5540, .max_power = 25 },
	{ .hw_value = 112, .center_freq = 5560, .max_power = 25 },
	{ .hw_value = 116, .center_freq = 5580, .max_power = 25 },
	{ .hw_value = 120, .center_freq = 5600, .max_power = 25 },
	{ .hw_value = 124, .center_freq = 5620, .max_power = 25 },
	{ .hw_value = 128, .center_freq = 5640, .max_power = 25 },
	{ .hw_value = 132, .center_freq = 5660, .max_power = 25 },
	{ .hw_value = 136, .center_freq = 5680, .max_power = 25 },
	{ .hw_value = 140, .center_freq = 5700, .max_power = 25 },
	{ .hw_value = 149, .center_freq = 5745, .max_power = 25 },
	{ .hw_value = 153, .center_freq = 5765, .max_power = 25 },
	{ .hw_value = 157, .center_freq = 5785, .max_power = 25 },
	{ .hw_value = 161, .center_freq = 5805, .max_power = 25 },
	{ .hw_value = 165, .center_freq = 5825, .max_power = 25 },
};

/* mapping to indexes for wl1271_rates_5ghz */
static const u8 wl1271_rate_to_idx_5ghz[] = {
	/* MCS rates are used only with 11n */
	7,                            /* CONF_HW_RXTX_RATE_MCS7_SGI */
	7,                            /* CONF_HW_RXTX_RATE_MCS7 */
	6,                            /* CONF_HW_RXTX_RATE_MCS6 */
	5,                            /* CONF_HW_RXTX_RATE_MCS5 */
	4,                            /* CONF_HW_RXTX_RATE_MCS4 */
	3,                            /* CONF_HW_RXTX_RATE_MCS3 */
	2,                            /* CONF_HW_RXTX_RATE_MCS2 */
	1,                            /* CONF_HW_RXTX_RATE_MCS1 */
	0,                            /* CONF_HW_RXTX_RATE_MCS0 */

	7,                             /* CONF_HW_RXTX_RATE_54   */
	6,                             /* CONF_HW_RXTX_RATE_48   */
	5,                             /* CONF_HW_RXTX_RATE_36   */
	4,                             /* CONF_HW_RXTX_RATE_24   */

	/* TI-specific rate */
	CONF_HW_RXTX_RATE_UNSUPPORTED, /* CONF_HW_RXTX_RATE_22   */

	3,                             /* CONF_HW_RXTX_RATE_18   */
	2,                             /* CONF_HW_RXTX_RATE_12   */
	CONF_HW_RXTX_RATE_UNSUPPORTED, /* CONF_HW_RXTX_RATE_11   */
	1,                             /* CONF_HW_RXTX_RATE_9    */
	0,                             /* CONF_HW_RXTX_RATE_6    */
	CONF_HW_RXTX_RATE_UNSUPPORTED, /* CONF_HW_RXTX_RATE_5_5  */
	CONF_HW_RXTX_RATE_UNSUPPORTED, /* CONF_HW_RXTX_RATE_2    */
	CONF_HW_RXTX_RATE_UNSUPPORTED  /* CONF_HW_RXTX_RATE_1    */
};

static struct ieee80211_supported_band wl1271_band_5ghz = {
	.channels = wl1271_channels_5ghz,
	.n_channels = ARRAY_SIZE(wl1271_channels_5ghz),
	.bitrates = wl1271_rates_5ghz,
	.n_bitrates = ARRAY_SIZE(wl1271_rates_5ghz),
	.ht_cap	= WL12XX_HT_CAP,
};

static const u8 *wl1271_band_rate_to_idx[] = {
	[IEEE80211_BAND_2GHZ] = wl1271_rate_to_idx_2ghz,
	[IEEE80211_BAND_5GHZ] = wl1271_rate_to_idx_5ghz
};

static const struct ieee80211_ops wl1271_ops = {
	.start = wl1271_op_start,
	.stop = wl1271_op_stop,
	.add_interface = wl1271_op_add_interface,
	.remove_interface = wl1271_op_remove_interface,
	.suspend = wl1271_op_suspend,
	.resume = wl1271_op_resume,
	.config = wl1271_op_config,
	.prepare_multicast = wl1271_op_prepare_multicast,
	.configure_filter = wl1271_op_configure_filter,
	.tx = wl1271_op_tx,
	.set_key = wl1271_op_set_key,
	.hw_scan = wl1271_op_hw_scan,
	.cancel_hw_scan = wl1271_op_cancel_hw_scan,
	.sched_scan_start = wl1271_op_sched_scan_start,
	.sched_scan_stop = wl1271_op_sched_scan_stop,
	.bss_info_changed = wl1271_op_bss_info_changed,
	.set_frag_threshold = wl1271_op_set_frag_threshold,
	.set_rts_threshold = wl1271_op_set_rts_threshold,
	.conf_tx = wl1271_op_conf_tx,
	.get_tsf = wl1271_op_get_tsf,
	.get_survey = wl1271_op_get_survey,
	.sta_add = wl1271_op_sta_add,
	.sta_remove = wl1271_op_sta_remove,
	.sta_authorize = wl12xx_op_sta_authorize,
	.ampdu_action = wl1271_op_ampdu_action,
	.tx_frames_pending = wl1271_tx_frames_pending,
	.channel_switch = wl12xx_op_channel_switch,
	.set_bitrate_mask = wl12xx_set_bitrate_mask,
	CFG80211_TESTMODE_CMD(wl1271_tm_cmd)
};


u8 wl1271_rate_to_idx(int rate, enum ieee80211_band band)
{
	u8 idx;

	BUG_ON(band >= sizeof(wl1271_band_rate_to_idx)/sizeof(u8 *));

	if (unlikely(rate >= CONF_HW_RXTX_RATE_MAX)) {
		wl1271_error("Illegal RX rate from HW: %d", rate);
		return 0;
	}

	idx = wl1271_band_rate_to_idx[band][rate];
	if (unlikely(idx == CONF_HW_RXTX_RATE_UNSUPPORTED)) {
		wl1271_error("Unsupported RX rate from HW: %d", rate);
		return 0;
	}

	return idx;
}

static ssize_t wl1271_sysfs_show_bt_coex_state(struct device *dev,
					       struct device_attribute *attr,
					       char *buf)
{
	struct wl1271 *wl = dev_get_drvdata(dev);
	ssize_t len;

	len = PAGE_SIZE;

	mutex_lock(&wl->mutex);
	len = snprintf(buf, len, "%d\n\n0 - off\n1 - on\n",
		       wl->sg_enabled);
	mutex_unlock(&wl->mutex);

	return len;

}

static ssize_t wl1271_sysfs_store_bt_coex_state(struct device *dev,
						struct device_attribute *attr,
						const char *buf, size_t count)
{
	struct wl1271 *wl = dev_get_drvdata(dev);
	unsigned long res;
	int ret;

	ret = kstrtoul(buf, 10, &res);
	if (ret < 0) {
		wl1271_warning("incorrect value written to bt_coex_mode");
		return count;
	}

	mutex_lock(&wl->mutex);

	res = !!res;

	if (res == wl->sg_enabled)
		goto out;

	wl->sg_enabled = res;

	if (wl->state == WL1271_STATE_OFF)
		goto out;

	ret = wl1271_ps_elp_wakeup(wl);
	if (ret < 0)
		goto out;

	wl1271_acx_sg_enable(wl, wl->sg_enabled);
	wl1271_ps_elp_sleep(wl);

 out:
	mutex_unlock(&wl->mutex);
	return count;
}

static DEVICE_ATTR(bt_coex_state, S_IRUGO | S_IWUSR,
		   wl1271_sysfs_show_bt_coex_state,
		   wl1271_sysfs_store_bt_coex_state);

static ssize_t wl1271_sysfs_show_hw_pg_ver(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct wl1271 *wl = dev_get_drvdata(dev);
	ssize_t len;

	len = PAGE_SIZE;

	mutex_lock(&wl->mutex);
	if (wl->hw_pg_ver >= 0)
		len = snprintf(buf, len, "%d\n", wl->hw_pg_ver);
	else
		len = snprintf(buf, len, "n/a\n");
	mutex_unlock(&wl->mutex);

	return len;
}

static DEVICE_ATTR(hw_pg_ver, S_IRUGO,
		   wl1271_sysfs_show_hw_pg_ver, NULL);

static ssize_t wl1271_sysfs_read_fwlog(struct kobject *kobj,
				       struct bin_attribute *bin_attr,
				       char *buffer, loff_t pos, size_t count)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct wl1271 *wl = dev_get_drvdata(dev);
	ssize_t len;
	int ret;

	ret = mutex_lock_interruptible(&wl->mutex);
	if (ret < 0)
		return -ERESTARTSYS;

	/* Let only one thread read the log at a time, blocking others */
	while (wl->fwlog_size == 0) {
		DEFINE_WAIT(wait);

		prepare_to_wait_exclusive(&wl->fwlog_waitq,
					  &wait,
					  TASK_INTERRUPTIBLE);

		if (wl->fwlog_size != 0) {
			finish_wait(&wl->fwlog_waitq, &wait);
			break;
		}

		mutex_unlock(&wl->mutex);

		schedule();
		finish_wait(&wl->fwlog_waitq, &wait);

		if (signal_pending(current))
			return -ERESTARTSYS;

		ret = mutex_lock_interruptible(&wl->mutex);
		if (ret < 0)
			return -ERESTARTSYS;
	}

	/* Check if the fwlog is still valid */
	if (wl->fwlog_size < 0) {
		mutex_unlock(&wl->mutex);
		return 0;
	}

	/* Seeking is not supported - old logs are not kept. Disregard pos. */
	len = min(count, (size_t)wl->fwlog_size);
	wl->fwlog_size -= len;
	memcpy(buffer, wl->fwlog, len);

	/* Make room for new messages */
	memmove(wl->fwlog, wl->fwlog + len, wl->fwlog_size);

	mutex_unlock(&wl->mutex);

	return len;
}

static struct bin_attribute fwlog_attr = {
	.attr = {.name = "fwlog", .mode = S_IRUSR},
	.read = wl1271_sysfs_read_fwlog,
};

int wl1271_register_hw(struct wl1271 *wl)
{
	int ret;

	if (wl->mac80211_registered)
		return 0;

	ret = wl1271_fetch_nvs(wl);
	if (ret == 0) {
		/* NOTE: The wl->nvs->nvs element must be first, in
		 * order to simplify the casting, we assume it is at
		 * the beginning of the wl->nvs structure.
		 */
		u8 *nvs_ptr = (u8 *)wl->nvs;

		wl->mac_addr[0] = nvs_ptr[11];
		wl->mac_addr[1] = nvs_ptr[10];
		wl->mac_addr[2] = nvs_ptr[6];
		wl->mac_addr[3] = nvs_ptr[5];
		wl->mac_addr[4] = nvs_ptr[4];
		wl->mac_addr[5] = nvs_ptr[3];
	}

	SET_IEEE80211_PERM_ADDR(wl->hw, wl->mac_addr);

	ret = ieee80211_register_hw(wl->hw);
	if (ret < 0) {
		wl1271_error("unable to register mac80211 hw: %d", ret);
		return ret;
	}

	wl->mac80211_registered = true;

	wl1271_debugfs_init(wl);

	register_netdevice_notifier(&wl1271_dev_notifier);

	wl1271_notice("loaded");

	return 0;
}
EXPORT_SYMBOL_GPL(wl1271_register_hw);

void wl1271_unregister_hw(struct wl1271 *wl)
{
	if (wl->state == WL1271_STATE_PLT)
		__wl1271_plt_stop(wl);

	unregister_netdevice_notifier(&wl1271_dev_notifier);
	ieee80211_unregister_hw(wl->hw);
	wl->mac80211_registered = false;

}
EXPORT_SYMBOL_GPL(wl1271_unregister_hw);

int wl1271_init_ieee80211(struct wl1271 *wl)
{
	static const u32 cipher_suites[] = {
		WLAN_CIPHER_SUITE_WEP40,
		WLAN_CIPHER_SUITE_WEP104,
		WLAN_CIPHER_SUITE_TKIP,
		WLAN_CIPHER_SUITE_CCMP,
		WL1271_CIPHER_SUITE_GEM,
	};

	/* The tx descriptor buffer and the TKIP space. */
	wl->hw->extra_tx_headroom = WL1271_EXTRA_SPACE_TKIP +
		sizeof(struct wl1271_tx_hw_descr);

	/* unit us */
	/* FIXME: find a proper value */
	wl->hw->channel_change_time = 10000;
	wl->hw->max_listen_interval = wl->conf.conn.max_listen_interval;

	wl->hw->flags = IEEE80211_HW_SIGNAL_DBM |
		IEEE80211_HW_BEACON_FILTER |
		IEEE80211_HW_SUPPORTS_PS |
		IEEE80211_HW_SUPPORTS_UAPSD |
		IEEE80211_HW_HAS_RATE_CONTROL |
		IEEE80211_HW_CONNECTION_MONITOR |
		IEEE80211_HW_SUPPORTS_CQM_RSSI |
		IEEE80211_HW_REPORTS_TX_ACK_STATUS |
		IEEE80211_HW_SPECTRUM_MGMT |
		IEEE80211_HW_AP_LINK_PS |
		IEEE80211_HW_AMPDU_AGGREGATION |
		IEEE80211_HW_TX_AMPDU_IN_HW_ONLY |
		IEEE80211_HW_SCAN_WHILE_IDLE;

	wl->hw->wiphy->cipher_suites = cipher_suites;
	wl->hw->wiphy->n_cipher_suites = ARRAY_SIZE(cipher_suites);

	wl->hw->wiphy->interface_modes = BIT(NL80211_IFTYPE_STATION) |
		BIT(NL80211_IFTYPE_ADHOC) | BIT(NL80211_IFTYPE_AP) |
		BIT(NL80211_IFTYPE_P2P_CLIENT) | BIT(NL80211_IFTYPE_P2P_GO);
	wl->hw->wiphy->max_scan_ssids = 1;
	wl->hw->wiphy->max_sched_scan_ssids = 16;
	wl->hw->wiphy->max_match_sets = 16;
	/*
	 * Maximum length of elements in scanning probe request templates
	 * should be the maximum length possible for a template, without
	 * the IEEE80211 header of the template
	 */
	wl->hw->wiphy->max_scan_ie_len = WL1271_CMD_TEMPL_DFLT_SIZE -
			sizeof(struct ieee80211_header);

	wl->hw->wiphy->max_sched_scan_ie_len = WL1271_CMD_TEMPL_DFLT_SIZE -
		sizeof(struct ieee80211_header);

	/* make sure all our channels fit in the scanned_ch bitmask */
	BUILD_BUG_ON(ARRAY_SIZE(wl1271_channels) +
		     ARRAY_SIZE(wl1271_channels_5ghz) >
		     WL1271_MAX_CHANNELS);
	/*
	 * We keep local copies of the band structs because we need to
	 * modify them on a per-device basis.
	 */
	memcpy(&wl->bands[IEEE80211_BAND_2GHZ], &wl1271_band_2ghz,
	       sizeof(wl1271_band_2ghz));
	memcpy(&wl->bands[IEEE80211_BAND_5GHZ], &wl1271_band_5ghz,
	       sizeof(wl1271_band_5ghz));

	wl->hw->wiphy->bands[IEEE80211_BAND_2GHZ] =
		&wl->bands[IEEE80211_BAND_2GHZ];
	wl->hw->wiphy->bands[IEEE80211_BAND_5GHZ] =
		&wl->bands[IEEE80211_BAND_5GHZ];

	wl->hw->queues = 4;
	wl->hw->max_rates = 1;

	wl->hw->wiphy->reg_notifier = wl1271_reg_notify;

	SET_IEEE80211_DEV(wl->hw, wl1271_wl_to_dev(wl));

	wl->hw->sta_data_size = sizeof(struct wl1271_station);

	wl->hw->max_rx_aggregation_subframes = 8;

	return 0;
}
EXPORT_SYMBOL_GPL(wl1271_init_ieee80211);

#define WL1271_DEFAULT_CHANNEL 0

struct ieee80211_hw *wl1271_alloc_hw(void)
{
	struct ieee80211_hw *hw;
	struct platform_device *plat_dev = NULL;
	struct wl1271 *wl;
	int i, j, ret;
	unsigned int order;

	BUILD_BUG_ON(AP_MAX_LINKS > WL1271_MAX_LINKS);

	hw = ieee80211_alloc_hw(sizeof(*wl), &wl1271_ops);
	if (!hw) {
		wl1271_error("could not alloc ieee80211_hw");
		ret = -ENOMEM;
		goto err_hw_alloc;
	}

	plat_dev = kmemdup(&wl1271_device, sizeof(wl1271_device), GFP_KERNEL);
	if (!plat_dev) {
		wl1271_error("could not allocate platform_device");
		ret = -ENOMEM;
		goto err_plat_alloc;
	}

	wl = hw->priv;
	memset(wl, 0, sizeof(*wl));

	INIT_LIST_HEAD(&wl->list);

	wl->hw = hw;
	wl->plat_dev = plat_dev;

	for (i = 0; i < NUM_TX_QUEUES; i++)
		skb_queue_head_init(&wl->tx_queue[i]);

	for (i = 0; i < NUM_TX_QUEUES; i++)
		for (j = 0; j < AP_MAX_LINKS; j++)
			skb_queue_head_init(&wl->links[j].tx_queue[i]);

	skb_queue_head_init(&wl->deferred_rx_queue);
	skb_queue_head_init(&wl->deferred_tx_queue);

	INIT_DELAYED_WORK(&wl->elp_work, wl1271_elp_work);
	INIT_DELAYED_WORK(&wl->pspoll_work, wl1271_pspoll_work);
	INIT_WORK(&wl->netstack_work, wl1271_netstack_work);
	INIT_WORK(&wl->tx_work, wl1271_tx_work);
	INIT_WORK(&wl->recovery_work, wl1271_recovery_work);
	INIT_DELAYED_WORK(&wl->scan_complete_work, wl1271_scan_complete_work);
	INIT_WORK(&wl->rx_streaming_enable_work,
		  wl1271_rx_streaming_enable_work);
	INIT_WORK(&wl->rx_streaming_disable_work,
		  wl1271_rx_streaming_disable_work);
	INIT_WORK(&wl->ap_start_work,
		  wl12xx_ap_start_work);

	wl->freezable_wq = create_freezable_workqueue("wl12xx_wq");
	if (!wl->freezable_wq) {
		ret = -ENOMEM;
		goto err_hw;
	}

	wl->channel = WL1271_DEFAULT_CHANNEL;
	wl->beacon_int = WL1271_DEFAULT_BEACON_INT;
	wl->default_key = 0;
	wl->rx_counter = 0;
	wl->rx_config = WL1271_DEFAULT_STA_RX_CONFIG;
	wl->rx_filter = WL1271_DEFAULT_STA_RX_FILTER;
	wl->psm_entry_retry = 0;
	wl->power_level = WL1271_DEFAULT_POWER_LEVEL;
	wl->basic_rate_set = CONF_TX_RATE_MASK_BASIC;
	wl->basic_rate = CONF_TX_RATE_MASK_BASIC;
	wl->rate_set = CONF_TX_RATE_MASK_BASIC;
	wl->band = IEEE80211_BAND_2GHZ;
	wl->vif = NULL;
	wl->flags = 0;
	wl->sg_enabled = true;
	wl->hw_pg_ver = -1;
	wl->bss_type = MAX_BSS_TYPE;
	wl->set_bss_type = MAX_BSS_TYPE;
	wl->last_tx_hlid = 0;
	wl->ap_ps_map = 0;
	wl->ap_fw_ps_map = 0;
	wl->quirks = 0;
	wl->platform_quirks = 0;
	wl->sched_scanning = false;
	wl->role_id = WL1271_INVALID_ROLE_ID;
	wl->system_hlid = WL1271_SYSTEM_HLID;
	wl->sta_hlid = WL1271_INVALID_LINK_ID;
	wl->dev_role_id = WL1271_INVALID_ROLE_ID;
	wl->dev_hlid = WL1271_INVALID_LINK_ID;
	wl->session_counter = 0;
	wl->ap_bcast_hlid = WL1271_INVALID_LINK_ID;
	wl->ap_global_hlid = WL1271_INVALID_LINK_ID;
	wl->tx_security_seq = 0;
	wl->tx_security_last_seq_lsb = 0;
	wl->active_sta_count = 0;
	wl->tx_spare_blocks = TX_HW_BLOCK_SPARE_DEFAULT;

	setup_timer(&wl->rx_streaming_timer, wl1271_rx_streaming_timer,
		    (unsigned long) wl);
	wl->fwlog_size = 0;
	init_waitqueue_head(&wl->fwlog_waitq);

	memset(wl->links_map, 0, sizeof(wl->links_map));

	/* The system link is always allocated */
	__set_bit(WL1271_SYSTEM_HLID, wl->links_map);

	memset(wl->tx_frames_map, 0, sizeof(wl->tx_frames_map));
	for (i = 0; i < ACX_TX_DESCRIPTORS; i++)
		wl->tx_frames[i] = NULL;

	spin_lock_init(&wl->wl_lock);
#ifdef CONFIG_HAS_WAKELOCK
	wake_lock_init(&wl->wake_lock, WAKE_LOCK_SUSPEND, "wl1271_wake");
	wake_lock_init(&wl->rx_wake, WAKE_LOCK_SUSPEND, "rx_wake");
#endif

	wl->state = WL1271_STATE_OFF;
	wl->fw_type = WL12XX_FW_TYPE_NONE;
	mutex_init(&wl->mutex);

	/* Apply default driver configuration. */
	wl1271_conf_init(wl);
	wl->bitrate_masks[IEEE80211_BAND_2GHZ] = wl->conf.tx.basic_rate;
	wl->bitrate_masks[IEEE80211_BAND_5GHZ] = wl->conf.tx.basic_rate_5;

	order = get_order(WL1271_AGGR_BUFFER_SIZE);
	wl->aggr_buf = (u8 *)__get_free_pages(GFP_KERNEL, order);
	if (!wl->aggr_buf) {
		ret = -ENOMEM;
		goto err_wq;
	}

	wl->dummy_packet = wl12xx_alloc_dummy_packet(wl);
	if (!wl->dummy_packet) {
		ret = -ENOMEM;
		goto err_aggr;
	}

	/* Allocate one page for the FW log */
	wl->fwlog = (u8 *)get_zeroed_page(GFP_KERNEL);
	if (!wl->fwlog) {
		ret = -ENOMEM;
		goto err_dummy_packet;
	}

	/* Register platform device */
	ret = platform_device_register(wl->plat_dev);
	if (ret) {
		wl1271_error("couldn't register platform device");
		goto err_fwlog;
	}
	dev_set_drvdata(&wl->plat_dev->dev, wl);

	/* Create sysfs file to control bt coex state */
	ret = device_create_file(&wl->plat_dev->dev, &dev_attr_bt_coex_state);
	if (ret < 0) {
		wl1271_error("failed to create sysfs file bt_coex_state");
		goto err_platform;
	}

	/* Create sysfs file to get HW PG version */
	ret = device_create_file(&wl->plat_dev->dev, &dev_attr_hw_pg_ver);
	if (ret < 0) {
		wl1271_error("failed to create sysfs file hw_pg_ver");
		goto err_bt_coex_state;
	}

	/* Create sysfs file for the FW log */
	ret = device_create_bin_file(&wl->plat_dev->dev, &fwlog_attr);
	if (ret < 0) {
		wl1271_error("failed to create sysfs file fwlog");
		goto err_hw_pg_ver;
	}

	return hw;

err_hw_pg_ver:
	device_remove_file(&wl->plat_dev->dev, &dev_attr_hw_pg_ver);

err_bt_coex_state:
	device_remove_file(&wl->plat_dev->dev, &dev_attr_bt_coex_state);

err_platform:
	platform_device_unregister(wl->plat_dev);

err_fwlog:
	free_page((unsigned long)wl->fwlog);

err_dummy_packet:
	dev_kfree_skb(wl->dummy_packet);

err_aggr:
	free_pages((unsigned long)wl->aggr_buf, order);

err_wq:
	destroy_workqueue(wl->freezable_wq);

err_hw:
	wl1271_debugfs_exit(wl);
	kfree(plat_dev);

err_plat_alloc:
	ieee80211_free_hw(hw);

err_hw_alloc:

	return ERR_PTR(ret);
}
EXPORT_SYMBOL_GPL(wl1271_alloc_hw);

int wl1271_free_hw(struct wl1271 *wl)
{
	/* Unblock any fwlog readers */
	mutex_lock(&wl->mutex);
	wl->fwlog_size = -1;
	wake_up_interruptible_all(&wl->fwlog_waitq);
	mutex_unlock(&wl->mutex);

	device_remove_bin_file(&wl->plat_dev->dev, &fwlog_attr);

	device_remove_file(&wl->plat_dev->dev, &dev_attr_hw_pg_ver);

	device_remove_file(&wl->plat_dev->dev, &dev_attr_bt_coex_state);
#ifdef CONFIG_HAS_WAKELOCK
	wake_lock_destroy(&wl->wake_lock);
	wake_lock_destroy(&wl->rx_wake);
#endif
	platform_device_unregister(wl->plat_dev);
	free_page((unsigned long)wl->fwlog);
	dev_kfree_skb(wl->dummy_packet);
	free_pages((unsigned long)wl->aggr_buf,
			get_order(WL1271_AGGR_BUFFER_SIZE));
	kfree(wl->plat_dev);

	wl1271_debugfs_exit(wl);

	vfree(wl->fw);
	wl->fw = NULL;
	kfree(wl->nvs);
	wl->nvs = NULL;

	kfree(wl->fw_status);
	kfree(wl->tx_res_if);
	destroy_workqueue(wl->freezable_wq);

	ieee80211_free_hw(wl->hw);

	return 0;
}
EXPORT_SYMBOL_GPL(wl1271_free_hw);

u32 wl12xx_debug_level = DEBUG_NONE;
EXPORT_SYMBOL_GPL(wl12xx_debug_level);
module_param_named(debug_level, wl12xx_debug_level, uint, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(debug_level, "wl12xx debugging level");

module_param(bug_on_recovery, bool, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(bug_on_recovery, "BUG() on fw recovery");

module_param_named(fwlog, fwlog_param, charp, 0);
MODULE_PARM_DESC(keymap,
		 "FW logger options: continuous, ondemand, dbgpins or disable");

module_param_named(fref, fref_param, charp, 0);
MODULE_PARM_DESC(fref, "FREF clock: 19.2, 26, 26x, 38.4, 38.4x, 52");

module_param_named(tcxo, tcxo_param, charp, 0);
MODULE_PARM_DESC(tcxo,
		 "TCXO clock: 19.2, 26, 38.4, 52, 16.368, 32.736, 16.8, 33.6");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Luciano Coelho <coelho@ti.com>");
MODULE_AUTHOR("Juuso Oikarinen <juuso.oikarinen@nokia.com>");
