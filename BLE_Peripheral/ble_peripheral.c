/*
 * @Descripttion: 
 * @version: 
 * @Author: Newt
 * @Date: 2022-08-04 11:09:36
 * @LastEditors: Newt
 * @LastEditTime: 2022-11-03 18:03:07
 */

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <FreeRTOS.h>
#include <task.h>

#include "bluetooth.h"
#include "conn.h"
#include "gatt.h"
#include "hci_core.h"
#include "uuid.h"
#include "ble_peripheral_tp_server.h"
#include "log.h"
#include "bluetooth.h"
#include "ble_cli_cmds.h"
#include "hci_driver.h"
#include "ble_lib_api.h"
#include <lf686_glb.h>
#include <lf686_hbn.h>

int ble_start_adv(void)
{
    struct bt_le_adv_param adv_param = {
        // options:3, connectable undirected, adv one time
        .options = 3,
        .interval_min = BT_GAP_ADV_FAST_INT_MIN_3,
        .interval_max = BT_GAP_ADV_FAST_INT_MAX_3,
    };

    char *adv_name = "CHECK12";
    char *per_info = "WDNMD";
    char svc_data[] = {0x01, 0x02, 0x03};
    struct bt_data adv_data[] = {
        BT_DATA(BT_DATA_NAME_COMPLETE, adv_name, strlen(adv_name)),
        BT_DATA(BT_DATA_MANUFACTURER_DATA, per_info, strlen(per_info)),
        BT_DATA(BT_DATA_SVC_DATA128, svc_data, strlen(svc_data)),
    };

    return bt_le_adv_start(&adv_param, adv_data, ARRAY_SIZE(adv_data), NULL, 0);
}

void bt_enable_cb(int err)
{
    ble_tp_init();
    ble_start_adv();
}

void ble_stack_start(void)
{
    MSG("[OS] ble_controller_init...\r\n");
    GLB_Set_EM_Sel(GLB_EM_8KB);
    ble_controller_init(configMAX_PRIORITIES - 1);

    // Initialize BLE Host stack
    MSG("[OS] hci_driver_init...\r\n");
    hci_driver_init();

    MSG("[OS] bt_enable...\r\n");
    bt_enable(bt_enable_cb);
}

void ble_init(void)
{
    ble_stack_start();
}
