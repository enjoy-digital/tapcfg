/**
 *  tapcfg - A cross-platform configuration utility for TAP driver
 *  Copyright (C) 2008  Juho Vähä-Herttua
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 */

/**
 * This file is heavily based on the tun.c source file of aiccu -
 * Automatic IPv6 Connectivity Client Utility, which is released
 * under three clause BSD-like license. You can download the source
 * code of aiccu from http://www.sixxs.net/tools/aiccu/ website
 */

struct tap_reg {
	char *guid;
	struct tap_reg *next;
};

struct panel_reg {
	char *name;
	char *guid;
	struct panel_reg *next;
};

/* Get a working tunnel adapter */
static struct tap_reg *
get_tap_reg()
{
	struct tap_reg *first = NULL;
	struct tap_reg *last = NULL;
	HKEY adapter_key;
	LONG status;
	int i;

	status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
	                      TAP_ADAPTER_KEY,
	                      0,
	                      KEY_READ,
	                      &adapter_key);
	if (status != ERROR_SUCCESS) {
		taplog_log(TAPLOG_ERR,
		           "Error opening registry key: %s\n",
		           TAP_ADAPTER_KEY);
		return NULL;
	}

	for (i=0; 1; i++) {
		char enum_name[256];
		char unit_string[256];
		HKEY unit_key;
		char component_id[256];
		DWORD data_type;
		DWORD len;

		len = sizeof(enum_name);
		status = RegEnumKeyEx(adapter_key, i, enum_name,
		                      &len, NULL, NULL, NULL, NULL);
		if (status == ERROR_NO_MORE_ITEMS) {
			break;
		} else if (status != ERROR_SUCCESS) {
			taplog_log(TAPLOG_ERR,
			           "Error enumerating registry subkeys of key: %s (t0)\n",
			           TAP_ADAPTER_KEY);
			break;
		}

		snprintf(unit_string, sizeof(unit_string),
		         "%s\\%s", TAP_ADAPTER_KEY, enum_name);
		status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, unit_string,
		                      0, KEY_READ, &unit_key);
		if (status != ERROR_SUCCESS) {
			taplog_log(TAPLOG_WARNING,
			           "Error opening registry key: %s (t1)\n",
			           unit_string);
			continue;
		}

		len = sizeof(component_id);
		status = RegQueryValueEx(unit_key, "ComponentId",
					 NULL, &data_type,
					 (LPBYTE) component_id,
					 &len);
		if (status != ERROR_SUCCESS || data_type != REG_SZ) {
			taplog_log(TAPLOG_WARNING,
				   "Error opening registry key: %s\\ComponentId (t2)\n",
				    unit_string);
		} else {
			char net_cfg_instance_id[256];

			len = sizeof(net_cfg_instance_id);
			status = RegQueryValueEx(unit_key, "NetCfgInstanceId",
						 NULL, &data_type,
						 (LPBYTE) net_cfg_instance_id,
						 &len);

			if (status == ERROR_SUCCESS && data_type == REG_SZ) {
				/* The component ID is usually tap0801 or tap0901
				 * depending on the version, for convenience we
				 * accept all tapXXXX components */
				if (strlen(component_id) == 7 &&
				    !strncmp(component_id, "tap", 3)) {
					struct tap_reg *reg;

					reg = calloc(1, sizeof(struct tap_reg));
					if (!reg) { 
						continue;
					}
					reg->guid = strdup(net_cfg_instance_id);

					/* Update the linked list */
					if (!first) first = reg;
					if (last) last->next = reg;
					last = reg;
				}
			}
		}

		RegCloseKey(unit_key);
	}

	RegCloseKey(adapter_key);

	return first;
}

static void
free_tap_reg(struct tap_reg *tap_reg)
{
	while (tap_reg) {
		struct tap_reg *next = tap_reg->next;

		free(tap_reg->guid);
		free(tap_reg);

		tap_reg = next;
	}
}

/* Collect GUID's and names of all the Connections that are available */
static struct panel_reg *
get_panel_reg()
{
	struct panel_reg *first = NULL;
	struct panel_reg *last = NULL;
	HKEY network_connections_key;
	LONG status;
	int i;

	status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
	                      TAP_REGISTRY_KEY,
	                      0,
	                      KEY_READ,
	                      &network_connections_key);
	if (status != ERROR_SUCCESS) {
		taplog_log(TAPLOG_ERR,
		           "Error opening registry key: %s (p0)\n",
		           TAP_REGISTRY_KEY);
		return NULL;
	}

	for (i=0; 1; i++) {
		char enum_name[256];
		char connection_string[256];
		HKEY connection_key;
		char name_data[256];
		DWORD name_type;
		DWORD len;

		len = sizeof(enum_name);
		status = RegEnumKeyEx(network_connections_key, i, enum_name, &len,
		                      NULL, NULL, NULL, NULL);
		if (status == ERROR_NO_MORE_ITEMS) {
			break;
		} else if (status != ERROR_SUCCESS) {
			taplog_log(TAPLOG_ERR,
			           "Error enumerating registry subkeys of key: %s (p1)\n",
			           TAP_REGISTRY_KEY);
			break;
		} else if (enum_name[0] != '{') {
			continue;
		}

		snprintf(connection_string, sizeof(connection_string),
		         "%s\\%s\\Connection", TAP_REGISTRY_KEY, enum_name);
		status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, connection_string,
		                      0, KEY_READ, &connection_key);
		if (status != ERROR_SUCCESS) {
			taplog_log(TAPLOG_WARNING,
			           "Error opening registry key: %s (p2)\n",
			           connection_string);
			continue;
		}

		len = sizeof(name_data);
		status = RegQueryValueEx(connection_key, "Name", NULL,
		                         &name_type, (LPBYTE) name_data,
		                         &len);
		if (status != ERROR_SUCCESS || name_type != REG_SZ) {
			taplog_log(TAPLOG_WARNING,
			           "Error opening registry key: %s\\%s\\Name (p3)\n",
		                   TAP_REGISTRY_KEY, (LPBYTE) connection_string);
		} else {
			struct panel_reg *reg;

			reg = calloc(1, sizeof(struct panel_reg));
			reg->name = strdup(name_data);
			reg->guid = strdup(enum_name);

			/* Update the linked list */
			if (!first) first = reg;
			if (last) last->next = reg;
			last = reg;
		}

		RegCloseKey(connection_key);
	}

	RegCloseKey(network_connections_key);

	return first;
}

static void
free_panel_reg(struct panel_reg *panel_reg)
{
	while (panel_reg) {
		struct panel_reg *next = panel_reg->next;

		free(panel_reg->guid);
		free(panel_reg->name);
		free(panel_reg);

		panel_reg = next;
	}
}

static int
tapcfg_fixup_adapters(tapcfg_t *tapcfg)
{
	struct tap_reg *tap_reg = get_tap_reg(), *tr;
	struct panel_reg *panel_reg = get_panel_reg(), *pr;
	struct panel_reg *adapter = NULL;
	unsigned int found=0, valid=0;
	int ret = 0;

	assert(tapcfg);

	taplog_log(TAPLOG_DEBUG, "Available TAP adapters [name, GUID]:\n");

	/* loop through each TAP adapter registry entry */
	for (tr=tap_reg; tr != NULL; tr=tr->next) {
		struct tap_reg *iterator;
		int links = 0, valid_adapter = 1;

		/* loop through each network connections entry in the control panel */
		for (pr=panel_reg; pr != NULL; pr=pr->next) {
			if (!strcmp(tr->guid, pr->guid)) {
				taplog_log(TAPLOG_DEBUG, "'%s' %s\n", pr->name, tr->guid);
				adapter = pr;
				links++;

				if (!strcasecmp(tapcfg->ifname, pr->name)) {
					found++;
				}
			}
		}

		if (!links) {
			taplog_log(TAPLOG_WARNING,
			           "*** Adapter with GUID %s doesn't have a link from the "
			           "control panel\n", tr->guid);
			valid_adapter = 0;
		} else if (links > 1) {
			taplog_log(TAPLOG_WARNING,
			           "*** Adapter with GUID %s has %u links from the Network "
			           "Connections control panel, it should only be 1\n",
			           tr->guid, links);
			valid_adapter = 0;
		}

		for (iterator=tap_reg; iterator != NULL; iterator=iterator->next) {
			if (tr != iterator && !strcmp(tr->guid, iterator->guid)) {
				taplog_log(TAPLOG_WARNING, "*** Duplicate Adapter GUID %s\n", tr->guid);
				valid_adapter = 0;
			}
		}

		if (valid_adapter) {
			valid++;
		}
	}

	if (found == 1 && valid == 1) {
		taplog_log(TAPLOG_DEBUG,
		           "Using configured interface %s\n",
		           tapcfg->ifname);
	} else if (found == 0 && valid == 1 && adapter) {
		taplog_log(TAPLOG_INFO,
		           "Using adapter '%s' instead of '%s' because it was the only one found\n",
		           adapter->name, tapcfg->ifname);
		strncpy(tapcfg->ifname, adapter->name, MAX_IFNAME);
	} else {
		taplog_log(TAPLOG_WARNING,
		           "Found %u adapters, %u of which were valid, don't know what to use\n",
		           found, valid);
		ret = -1;
	}

	free_tap_reg(tap_reg);
	free_panel_reg(panel_reg);

	return ret;
}
