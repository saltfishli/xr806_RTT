/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if PRJCONF_NET_EN

#include <string.h>
#include <stdlib.h>
#include "lwip/tcpip.h"
#include "lwip/inet.h"
#include "lwip/dhcp.h"
#include "lwip/netifapi.h"
#include "net/wlan/wlan.h"
#include "net/udhcp/usr_dhcpd.h"

#include "common/framework/sys_ctrl/sys_ctrl.h"
#include "common/framework/sysinfo.h"
#if PRJCONF_TEMP_MONITOR_EN
#include "common/framework/sys_monitor.h"
#endif
#include "net_ctrl.h"
#include "net_ctrl_debug.h"

struct netif_conf {
	uint8_t     bring_up;   // bring up or down
	uint8_t     use_dhcp;   // use DHCP or not
#ifdef CONFIG_LWIP_V1
	ip_addr_t   ipaddr;
	ip_addr_t   netmask;
	ip_addr_t   gw;
#elif LWIP_IPV4 /* now only for IPv4 */
	ip4_addr_t  ipaddr;
	ip4_addr_t  netmask;
	ip4_addr_t  gw;
#else
	#error "IPv4 not support!"
#endif
};

#if (defined(CONFIG_LWIP_V1) || LWIP_IPV4)

static void netif_up_handler(struct netif *nif)
{
	if (nif == NULL) {
		NET_DBG("netif is NULL\n");
		return;
	}

	enum wlan_mode mode = wlan_netif_get_mode(nif);
	switch (mode) {
#ifdef CONFIG_WLAN_STA
	case WLAN_MODE_STA:
		wlan_set_ps_mode(nif, 1);
  #if LWIP_AUTOIP
		wlan_set_ip_addr(nif, NULL, 0);
  #else
		uint32_t addr = NET_IP_ADDR_GET_IP4U32(&nif->ip_addr);
		wlan_set_ip_addr(nif, (uint8_t *)&addr, sizeof(addr));
  #endif
		break;
#endif
#ifdef CONFIG_WLAN_AP
	case WLAN_MODE_HOSTAP:
		dhcp_server_start(NULL);
		break;
#endif
#ifdef CONFIG_WLAN_MONITOR
	case WLAN_MODE_MONITOR:
		break;
#endif
	default:
		NET_ERR("Invalid wlan mode %d\n", mode);
		break;
	}
}

static void netif_down_handler(struct netif *nif)
{
#if (!LWIP_AUTOIP)
	if (nif == NULL) {
		NET_DBG("netif is NULL\n");
		return;
	}

	enum wlan_mode mode = wlan_netif_get_mode(nif);
	if (mode == WLAN_MODE_STA) {
		wlan_set_ip_addr(nif, NULL, 0);
	}
#endif
}

#else /* (defined(CONFIG_LWIP_V1) || LWIP_IPV4) */

#define netif_up_handler(nif)   do { } while (0)
#define netif_down_handler(nif) do { } while (0)

#endif /* (defined(CONFIG_LWIP_V1) || LWIP_IPV4) */

#if LWIP_NETIF_LINK_CALLBACK
static void netif_link_callback(struct netif *netif)
{
	if (netif_is_link_up(netif)) {
		NET_INF("netif is link up\n");
	} else {
		NET_INF("netif is link down\n");
	}
}
#endif /* LWIP_NETIF_LINK_CALLBACK */

#ifdef CONFIG_LWIP_V1

#if LWIP_NETIF_STATUS_CALLBACK
static void netif_status_callback(struct netif *netif)
{
	if (NET_IS_IP4_VALID(netif)) {
		NET_INF("netif is up\n");
		NET_INF("address: %s\n", inet_ntoa(netif->ip_addr));
		NET_INF("gateway: %s\n", inet_ntoa(netif->gw));
		NET_INF("netmask: %s\n", inet_ntoa(netif->netmask));
		net_ctrl_msg_send(NET_CTRL_MSG_NETWORK_UP, 0);
	} else {
		NET_INF("netif is down\n");
		netif_down_handler(netif);
		net_ctrl_msg_send(NET_CTRL_MSG_NETWORK_DOWN, 0);
	}
}
#endif /* LWIP_NETIF_STATUS_CALLBACK */

#else /* CONFIG_LWIP_V1 */

#if LWIP_NETIF_STATUS_CALLBACK
static struct in_addr m_ipv4_addr;
#if LWIP_IPV6
static uint8_t m_ipv6_addr_state;
#if (LWIP_IPV6_NUM_ADDRESSES > 8)
#error "MUST enlarge sizeof(m_ipv6_addr_state)!"
#endif
#endif

static void netif_status_callback(struct netif *netif)
{
	struct in_addr ipv4_addr;

	/* netif is always up, check IPv4 addr status */
	inet_addr_from_ip4addr(&ipv4_addr, ip_2_ip4(&netif->ip_addr));
	if (m_ipv4_addr.s_addr != ipv4_addr.s_addr) {
		/* only send message when IP changed, different with lwip-1.4.1 */
		m_ipv4_addr.s_addr = ipv4_addr.s_addr;
		if (NET_IS_IP4_VALID(netif)) {
			NET_INF("netif (IPv4) is up\n");
			NET_INF("address: %s\n", inet_ntoa(netif->ip_addr));
			NET_INF("gateway: %s\n", inet_ntoa(netif->gw));
			NET_INF("netmask: %s\n", inet_ntoa(netif->netmask));
			net_ctrl_msg_send(NET_CTRL_MSG_NETWORK_UP, 0);
		} else {
			NET_INF("netif (IPv4) is down\n");
			netif_down_handler(netif);
			net_ctrl_msg_send(NET_CTRL_MSG_NETWORK_DOWN, 0);
		}
	}

#if LWIP_IPV6
	int i;
	uint8_t ipv6_addr_state = 0;

	for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; ++i) {
		if (ip6_addr_isvalid(netif_ip6_addr_state(netif, i))) {
			ipv6_addr_state |= (1 << i);
		}
	}

	if (m_ipv6_addr_state != ipv6_addr_state) {
		NET_INF("IPv6 addr state change: 0x%x --> 0x%x\n",
		        m_ipv6_addr_state, ipv6_addr_state);
		m_ipv6_addr_state = ipv6_addr_state;
		net_ctrl_msg_send(NET_CTRL_MSG_NETWORK_IPV6_STATE, ipv6_addr_state);
	}
#endif /* LWIP_IPV6 */
}
#endif /* LWIP_NETIF_STATUS_CALLBACK */

#endif /* CONFIG_LWIP_V1 */

#if LWIP_NETIF_REMOVE_CALLBACK
static void netif_remove_callback(struct netif *netif)
{
	NET_DBG("%s()\n", __func__);
}
#endif /* LWIP_NETIF_REMOVE_CALLBACK */

#ifdef CONFIG_LWIP_V1
#define NET_DHCP_DATA(nif)      (nif->dhcp)
#define NET_DHCP_STATE(nif)     (nif->dhcp->state)
#define NET_DHCP_STATE_OFF      DHCP_OFF
#define NET_DHCP_STATE_BOUND    DHCP_BOUND
//#define NET_IP4_ADDR_ANY        IP_ADDR_ANY
#elif LWIP_IPV4 /* now only for IPv4 */
#include "lwip/prot/dhcp.h"
#define NET_DHCP_DATA(nif)      netif_dhcp_data(nif)
#define NET_DHCP_STATE(nif)     (netif_dhcp_data(nif)->state)
#define NET_DHCP_STATE_OFF      DHCP_STATE_OFF
#define NET_DHCP_STATE_BOUND    DHCP_STATE_BOUND
//#define NET_IP4_ADDR_ANY        IP4_ADDR_ANY4
#else
#error "IPv4 not support!"
#endif

static void netif_config(struct netif *nif, struct netif_conf *conf)
{
	if (conf->bring_up) {
		if (NET_IS_IP4_VALID(nif)) {
			NET_INF("netif is already up\n");
#ifdef CONFIG_LWIP_V1
	#if LWIP_NETIF_STATUS_CALLBACK
			if (!conf->use_dhcp) {
				netif_status_callback(nif);
			}
	#endif
#else /* CONFIG_LWIP_V1 */
			/* Maybe no new NET_CTRL_MSG_NETWORK_UP message due to the IP
			 * address is the same as before, do netif_up_handler() first.
			 */
			netif_up_handler(nif);
#endif /* CONFIG_LWIP_V1 */
			return;
		}
		if (conf->use_dhcp) {
			if (NET_DHCP_DATA(nif) &&
			    NET_DHCP_STATE(nif) != NET_DHCP_STATE_OFF) {
				NET_INF("DHCP is already started\n");
				return;
			}

			NET_INF("start DHCP...\n");
			if (netifapi_dhcp_start(nif) != ERR_OK) {
				NET_ERR("DHCP start failed!\n");
				return;
			}
		} else {
			NET_INF("bring up netif\n");
			netifapi_netif_set_addr(nif, &conf->ipaddr, &conf->netmask, &conf->gw);
#ifdef CONFIG_LWIP_V1
			netifapi_netif_set_up(nif);
#endif
		}
	} else {
		if (conf->use_dhcp) {
			if (NET_DHCP_DATA(nif) == NULL) {
				NET_INF("DHCP is not started\n");
				return;
			}
			if (netif_is_link_up(nif)) {
				NET_INF("release DHCP\n");
				netifapi_dhcp_release(nif);
			} else {
				NET_INF("bring down netif\n");
#ifdef CONFIG_LWIP_V1
				netifapi_netif_set_down(nif);
#endif
				netifapi_netif_set_addr(nif, NULL, NULL, NULL);
			}

			NET_INF("stop DHCP\n");
			netifapi_dhcp_stop(nif);
		} else {
			if (!NET_IS_IP4_VALID(nif)) {
				NET_INF("netif is already down\n");
				return;
			}
			NET_INF("bring down netif\n");
#ifdef CONFIG_LWIP_V1
			netifapi_netif_set_down(nif);
#endif
			netifapi_netif_set_addr(nif, NULL, NULL, NULL);
		}
	}
}

/* bring up/down network */
void net_config(struct netif *nif, uint8_t bring_up)
{
	struct sysinfo *sysinfo = sysinfo_get();
	if (sysinfo == NULL) {
		NET_ERR("failed to get sysinfo %p\n", sysinfo);
		return;
	}

	struct netif_conf net_conf;
	memset(&net_conf, 0, sizeof(struct netif_conf));
	net_conf.bring_up = bring_up;

	if (sysinfo->wlan_mode == WLAN_MODE_STA) {
		if (sysinfo->sta_use_dhcp) {
			net_conf.use_dhcp = 1;
		} else {
			net_conf.use_dhcp = 0;
			memcpy(&net_conf.ipaddr, &sysinfo->netif_sta_param.ip_addr, sizeof(net_conf.ipaddr));
			memcpy(&net_conf.netmask, &sysinfo->netif_sta_param.net_mask, sizeof(net_conf.netmask));
			memcpy(&net_conf.gw, &sysinfo->netif_sta_param.gateway, sizeof(net_conf.gw));
		}
	} else if (sysinfo->wlan_mode == WLAN_MODE_HOSTAP) {
		net_conf.use_dhcp = 0;
		memcpy(&net_conf.ipaddr, &sysinfo->netif_ap_param.ip_addr, sizeof(net_conf.ipaddr));
		memcpy(&net_conf.netmask, &sysinfo->netif_ap_param.net_mask, sizeof(net_conf.netmask));
		memcpy(&net_conf.gw, &sysinfo->netif_ap_param.gateway, sizeof(net_conf.gw));
	} else {
		NET_ERR("invalid wlan mode %d\n", sysinfo->wlan_mode);
		return;
	}

	netif_config(nif, &net_conf);
}

struct netif *net_open(enum wlan_mode mode)
{
	struct netif *nif;

	NET_DBG("%s(), mode %d\n", __func__, mode);

	nif = wlan_netif_create(mode);
	if (nif == NULL) {
		return NULL;
	}

#if LWIP_NETIF_LINK_CALLBACK
	netif_set_link_callback(nif, netif_link_callback);
#endif
#if LWIP_NETIF_STATUS_CALLBACK
	netif_set_status_callback(nif, netif_status_callback);
#endif
#if LWIP_NETIF_REMOVE_CALLBACK
	netif_set_remove_callback(nif, netif_remove_callback);
#endif

#ifndef CONFIG_LWIP_V1
	m_ipv4_addr.s_addr = 0;
#if LWIP_IPV6
	m_ipv6_addr_state = 0;
	/* enable IPv6 for station only */
	if (mode == WLAN_MODE_STA) {
		netif_create_ip6_linklocal_address(nif, 1);
#if LWIP_IPV6_AUTOCONFIG
		netif_set_ip6_autoconfig_enabled(nif, 1);
#endif
	}
#endif /* LWIP_IPV6 */
	/* set netif up, but no valid ip address, required by lwip-2.x.x */
	netifapi_netif_set_up(nif);
#endif /* CONFIG_LWIP_V1 */

	wlan_start(nif);

	struct sysinfo *sysinfo = sysinfo_get();
	if (sysinfo) {
		sysinfo->wlan_mode = mode;
	}

#if PRJCONF_TEMP_MONITOR_EN
	temp_monitor_init();
#endif

	return nif;
}

void net_close(struct netif *nif)
{
	if (!nif) {
		NET_ERR("%s() has closed!\n", __func__);
		return ;
	}

	NET_DBG("%s(), mode %d\n", __func__, wlan_netif_get_mode(nif));

	wlan_stop(nif);
#if 0
#if LWIP_NETIF_REMOVE_CALLBACK
	netif_set_remove_callback(nif, NULL);
#endif
#if LWIP_NETIF_STATUS_CALLBACK
	netif_set_status_callback(nif, NULL);
#endif
#if LWIP_NETIF_LINK_CALLBACK
	netif_set_link_callback(nif, NULL);
#endif
#endif
	wlan_netif_delete(nif);
}

int net_switch_mode(enum wlan_mode mode)
{
	int ret = -1;
	enum wlan_mode cur_mode;
	struct netif *nif = wlan_netif_get(WLAN_MODE_NONE);

	NET_DBG("%s(), mode %d --> %d\n", __func__, wlan_netif_get_mode(nif), mode);

	if (nif == NULL) {
		nif = net_open(mode);
		return nif ? 0 : -1;
	}

	if (mode >= WLAN_MODE_NUM) {
		NET_WRN("invalid wlan mode %d\n", mode);
		return ret;
	}

	cur_mode = wlan_netif_get_mode(nif);
	if (mode == cur_mode) {
		NET_INF("no need to switch wlan mode %d\n", cur_mode);
		return 0;
	}

	if (NET_IS_IP4_VALID(nif)) {
		net_config(nif, 0); /* bring down netif */
	}

	if (cur_mode == WLAN_MODE_STA && netif_is_link_up(nif)) {
		wlan_sta_disable(); /* disconnect wlan */
	}

	switch (mode) {
	case WLAN_MODE_HOSTAP:
	case WLAN_MODE_STA:
	case WLAN_MODE_MONITOR:
		net_close(nif);
		nif = net_open(mode);
		if (nif) {
			ret = 0;
		}
		break;
	default:
		break;
	}

	return ret;
}

void net_ctrl_msg_send(uint32_t type, uint32_t data)
{
	sys_event_send(CTRL_MSG_TYPE_NETWORK, (uint16_t)type, data, OS_WAIT_FOREVER);
}

int net_ctrl_msg_send_with_free(uint16_t type, void *data)
{
	return sys_event_send_with_free(CTRL_MSG_TYPE_NETWORK, type, (void *)data, OS_WAIT_FOREVER);
}

#if NET_INF_ON
static const char * const net_ctrl_msg_str[] = {
	"wlan connected",
	"wlan disconnected",
	"wlan scan success",
	"wlan scan failed",
	"wlan 4way handshake failed",
	"wlan connect failed",
	"wlan connect loss",
	"network up",
	"network down",
#if (!defined(CONFIG_LWIP_V1) && LWIP_IPV6)
	"network IPv6 state",
#endif
};
#endif

int net_ctrl_connect_ap(void)
{
	return 0;
}

int net_ctrl_disconnect_ap(void)
{
	return 0;
}

void net_ctrl_msg_process(uint32_t event, uint32_t data, void *arg)
{
	struct netif *nif = wlan_netif_get(WLAN_MODE_NONE);
	uint16_t type = EVENT_SUBTYPE(event);

	NET_INF("msg <%s>\n", net_ctrl_msg_str[type]);

	switch (type) {
	case NET_CTRL_MSG_WLAN_CONNECTED:
		if (nif && !netif_is_link_up(nif)) {
			netifapi_netif_set_link_up(nif); /* set link up */
#if (defined(CONFIG_LWIP_V1) || LWIP_IPV4)
			net_config(nif, 1); /* bring up network */
#endif
		}
		break;
	case NET_CTRL_MSG_WLAN_DISCONNECTED:
		if (nif) {
			if (netif_is_link_up(nif)) {
				netifapi_netif_set_link_down(nif); /* set link down */
			}
#if (defined(CONFIG_LWIP_V1) || LWIP_IPV4)
			/* if dhcp is started and not bound, stop it */
			if (NET_DHCP_DATA(nif) &&
				NET_DHCP_STATE(nif) != NET_DHCP_STATE_OFF &&
				NET_DHCP_STATE(nif) != NET_DHCP_STATE_BOUND) {
				net_config(nif, 0);
			}
#endif
		}
		break;
	case NET_CTRL_MSG_WLAN_SCAN_SUCCESS:
		break;
	case NET_CTRL_MSG_WLAN_SCAN_FAILED:
		break;
	case NET_CTRL_MSG_WLAN_4WAY_HANDSHAKE_FAILED:
		break;
	case NET_CTRL_MSG_WLAN_CONNECT_FAILED:
		break;
	case NET_CTRL_MSG_CONNECTION_LOSS:
		break;
	case NET_CTRL_MSG_NETWORK_UP:
		netif_up_handler(nif);
		break;
	case NET_CTRL_MSG_NETWORK_DOWN:
		break;
#if (!defined(CONFIG_LWIP_V1) && LWIP_IPV6)
	case NET_CTRL_MSG_NETWORK_IPV6_STATE:
		break;
#endif
	default:
		NET_WRN("unknown msg (%u, %u)\n", type, data);
		break;
	}
}

int net_ctrl_init(void)
{
	observer_base *ob = sys_callback_observer_create(CTRL_MSG_TYPE_NETWORK,
	                                                 NET_CTRL_MSG_ALL,
	                                                 net_ctrl_msg_process,
	                                                 NULL);
	if (ob == NULL)
		return -1;
	if (sys_ctrl_attach(ob) != 0)
		return -1;

	return 0;
}

#endif /* PRJCONF_NET_EN */
