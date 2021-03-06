/* SoftAP based Custom Provisioning Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/* This file is mostly a boiler-plate code that applications can use without much change */

#include <stdio.h>
#include <string.h>
#include <esp_err.h>
#include <esp_log.h>

#include <esp_wifi.h>
#include <tcpip_adapter.h>

#include <wifi_provisioning/wifi_config.h>
#include <custom_provisioning/custom_config.h>

#include "app_prov.h"

static const char* TAG = "app_prov_handler";

/****************** Handler for Custom Configuration *******************/
static esp_err_t custom_config_handler(const custom_config_t *config)
{
    ESP_LOGI(TAG, "Custom config received :\n\tInfo : %s\n\tVersion : %d",
             config->info, config->version);
    return ESP_OK;
}

custom_prov_config_handler_t custom_prov_handler = custom_config_handler;

/****************** Handlers for Wi-Fi Configuration *******************/
static esp_err_t get_status_handler(wifi_prov_config_get_data_t *resp_data)
{
    /* Initialise to zero */
    memset(resp_data, 0, sizeof(wifi_prov_config_get_data_t));

    if (app_prov_get_wifi_state(&resp_data->wifi_state) != ESP_OK) {
        ESP_LOGW(TAG, "Prov app not running");
        return ESP_FAIL;
    }

    if (resp_data->wifi_state == WIFI_PROV_STA_CONNECTED) {
        ESP_LOGI(TAG, "Connected state");

        /* IP Addr assigned to STA */
        tcpip_adapter_ip_info_t ip_info;
        tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info);
        char *ip_addr = ip4addr_ntoa(&ip_info.ip);
        strcpy(resp_data->conn_info.ip_addr, ip_addr);

        /* AP information to which STA is connected */
        wifi_ap_record_t ap_info;
        esp_wifi_sta_get_ap_info(&ap_info);
        memcpy(resp_data->conn_info.bssid, (char *)ap_info.bssid, sizeof(ap_info.bssid));
        memcpy(resp_data->conn_info.ssid,  (char *)ap_info.ssid,  sizeof(ap_info.ssid));
        resp_data->conn_info.channel   = ap_info.primary;
        resp_data->conn_info.auth_mode = ap_info.authmode;
    } else if (resp_data->wifi_state == WIFI_PROV_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Disconnected state");

        /* If disconnected, convey reason */
        app_prov_get_wifi_disconnect_reason(&resp_data->fail_reason);
    } else {
        ESP_LOGI(TAG, "Connecting state");
    }
    return ESP_OK;
}

static wifi_config_t *wifi_cfg;

static esp_err_t set_config_handler(const wifi_prov_config_set_data_t *req_data)
{
    if (wifi_cfg) {
        free(wifi_cfg);
        wifi_cfg = NULL;
    }

    wifi_cfg = (wifi_config_t *) calloc(1, sizeof(wifi_config_t));
    if (!wifi_cfg) {
        ESP_LOGE(TAG, "Unable to alloc wifi config");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "WiFi Credentials Received : \n\tssid %s \n\tpassword %s",
             req_data->ssid, req_data->password);
    memcpy((char *) wifi_cfg->sta.ssid, req_data->ssid,
           strnlen(req_data->ssid, sizeof(wifi_cfg->sta.ssid)));
    memcpy((char *) wifi_cfg->sta.password, req_data->password,
           strnlen(req_data->password, sizeof(wifi_cfg->sta.password)));
    return ESP_OK;
}

static esp_err_t apply_config_handler(void)
{
    if (!wifi_cfg) {
        ESP_LOGE(TAG, "WiFi config not set");
        return ESP_FAIL;
    }

    app_prov_configure_sta(wifi_cfg);
    ESP_LOGI(TAG, "WiFi Credentials Applied");

    free(wifi_cfg);
    wifi_cfg = NULL;
    return ESP_OK;
}

wifi_prov_config_handlers_t wifi_prov_handlers = {
    .get_status_handler   = get_status_handler,
    .set_config_handler   = set_config_handler,
    .apply_config_handler = apply_config_handler,
};
