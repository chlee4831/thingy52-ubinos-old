/*
 * ble_stack.c
 *
 *  Created on: 2020. 06. 11.
 *      Author: YJPark
 */

//#include "../itf/sw_config.h"
//#include "../itf/csos.h"
#include <ubinos.h>
#include <stdio.h>
#include <stdint.h>

#include "nrf.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "peer_manager.h"
#include "peer_manager_handler.h"
#include "ble_conn_params.h"
#include "ble_db_discovery.h"
#include "nrf_ble_scan.h"

#include "ble.h"
#include "ble_gap.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_db_discovery.h"
#include "ble_conn_params.h"

#include "ble_profile.h"
#include "ble_process.h"
#include "ble_stack.h"

#include "LAP_api.h"
#include "LAP_main.h"
#include "malloc.h"

typedef struct
{
    uint8_t evt;
    uint16_t conn_handle;
    uint8_t len;
    uint8_t msg[PAAR_MAXIMUM_PACKET_SIZE];
} ble_rx_msgt;

static msgq_pt ble_rx_msgq;

#define APP_ADV_DURATION                BLE_GAP_ADV_TIMEOUT_GENERAL_UNLIMITED   /**< The advertising time-out (in units of seconds). When set to 0, we will never time out. */
#define APP_ADV_INTERVAL                64                                      /**< The advertising interval (in units of 0.625 ms; this value corresponds to 40 ms). */

static uint8_t m_adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET; /**< Advertising handle used to identify an advertising set. */

/**@brief Struct that ble_gap_scan parameter. */
static ble_gap_scan_params_t m_scan_params =
{
        .active = SCAN_ACTIVE,
        .interval = SCAN_INTERVAL,
        .window = SCAN_WINDOW,
        .filter_policy = BLE_GAP_SCAN_FP_ACCEPT_ALL,
        .timeout = SCAN_TIMEOUT,
        .scan_phys = BLE_GAP_PHY_1MBPS, };

/**@brief Struct that ble_gap_connection parameter. */
static ble_gap_conn_params_t m_connection_param =
{
        .min_conn_interval = MIN_CONNECTION_INTERVAL,
        .max_conn_interval = MAX_CONNECTION_INTERVAL,
        .slave_latency = SLAVE_LATENCY,
        .conn_sup_timeout = SUPERVISION_TIMEOUT };

/**@brief Struct that contains pointers to the encoded advertising data. */
static ble_gap_adv_data_t m_adv_data =
{
        .adv_data =
        {
                .p_data = NULL,
                .len = 0 },
        .scan_rsp_data =
        {
                .p_data = NULL,
                .len = 0 } };

NRF_BLE_SCAN_DEF(m_scan); /**< Scanning Module instance. */
NRF_BLE_GATT_DEF(m_gatt); /**< GATT module instance. */

#define APP_BLE_OBSERVER_PRIO           3                                       /**< Application's BLE observer priority. You shouldn't need to modify this value. */
#define APP_BLE_CONN_CFG_TAG            1                                       /**< A tag identifying the SoftDevice BLE configuration. */

#define BLE_SEND_MSG_COMPLETE_DELAY 100

static ble_paar_t m_ble_paar;
static sem_pt _ble_evt_sem;

//Semphore : PAAR ble send msg complete check
static sem_pt _ble_notification_complete_sem;
static sem_pt _ble_indication_complete_sem;
static sem_pt _ble_write_request_complete_sem;
static sem_pt _ble_command_complete_sem;

//ble advertising initialization flag
static bool is_advertising_init = false;
//ble stack initialization flag
static bool is_ble_stack_initialized = false;

ble_rx_msgt temp_ble_cent_rx_buf;
ble_rx_msgt temp_ble_periph_buf;
ble_rx_msgt temp_ble_periph_adv_buf;

#define MAX_SIZE_TEMP_ADV_BUFFER	30
#define TEMP_ADV_MSG_INDEX			0

uint8_t temp_adv_buffer_index = 0;
uint8_t temp_adv_buffer_count = 0;

LAP_ble_adv_report temp_adv_buffer[MAX_SIZE_TEMP_ADV_BUFFER];

static void ble_stack_init(void);
static void gap_params_init(void);
static void services_init(void);
static void conn_params_init(void);
static void on_ble_evt(ble_evt_t *p_ble_evt, void *p_context);
static void PAAR_advertising_init(void);
static void PAAR_advertising_start(void);
static void PAAR_advertising_stop(void);

#define HUB_SOSP_ID_LEN 	6
static uint8_t hub_sosp_id[HUB_SOSP_ID_LEN] =
{
        0x00, };

uint8_t* get_hub_sosp_id()
{
    return hub_sosp_id;
}

/**@brief Function for ble softdevice enable
 */
static void ble_stack_init(void)
{

    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, on_ble_evt, NULL);

}

/**@brief Function for setting ble stack initialization flag
 */
static void set_ble_stack_init_flag(bool set)
{
    is_ble_stack_initialized = set;
}

/**@brief Function for getting ble stack initialization flag
 */
bool get_ble_stack_init_flag()
{
    return is_ble_stack_initialized;
}

/**@brief Function for waiting ble stack initialization
 */
void ble_stack_init_wait()
{
    while (get_ble_stack_init_flag() == false)
    {
        task_sleep(100);
    }
}

/**@brief Function for initializing BLE gap
 */
static void gap_params_init(void)
{

    uint32_t err_code;
    ble_gap_conn_params_t gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode, (const uint8_t*) PAAR_DEVICE_NAME, strlen(PAAR_DEVICE_NAME));

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONNECTION_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONNECTION_INTERVAL;
    gap_conn_params.slave_latency = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout = SUPERVISION_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing PAAR profile service
 */
static void services_init(void)
{

    uint32_t err_code;
    ble_paar_init_t ble_paar_init;

    memset(&ble_paar_init, 0, sizeof(ble_paar_init));
    ble_paar_init.data_handler = NULL;

    err_code = ble_paar_service_init(&m_ble_paar, &ble_paar_init);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for connection parameter error handler
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    //unused function
}

/**@brief Function for initializing connection parameters
 */
static void conn_params_init(void)
{

    uint32_t err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail = true;
    cp_init.evt_handler = NULL;
    cp_init.error_handler = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for sending ble central data
 */
uint32_t ble_send_central_data(const uint16_t m_conn_handle, uint16_t tx_handler, uint8_t *pbuf, uint16_t length)
{
    uint32_t send_data_state_err = 0;

    const ble_gattc_write_params_t write_params =
    {
            .write_op = BLE_GATT_OP_WRITE_REQ,
            .flags = BLE_GATT_EXEC_WRITE_FLAG_PREPARED_WRITE,
            .handle = tx_handler,
            .offset = 0,
            .len = length,
            .p_value = pbuf };

    send_data_state_err = sd_ble_gattc_write(m_conn_handle, &write_params);

    return send_data_state_err;
}

/**@brief Function for sending PAAR ble message(peripheral, cccd/rx handle) *Blocked function
 */
int32_t PAAR_send_ble_msg_central(uint16_t send_conn_handle, uint16_t handle, uint8_t *msg, uint16_t length)
{
    ret_code_t ret;

    sem_clear(_ble_write_request_complete_sem);

    ret = ble_send_central_data(send_conn_handle, handle, msg, length);
    if (ret != NRF_SUCCESS)
        return -4;

    return sem_take_timed(_ble_write_request_complete_sem, BLE_SEND_MSG_COMPLETE_DELAY);
}

uint32_t PAAR_send_ble_test_msg_central(uint16_t send_conn_handle, uint16_t handle, uint8_t *msg, uint16_t length)
{
    ret_code_t ret;

    sem_clear(_ble_write_request_complete_sem);

    ret = ble_send_central_data(send_conn_handle, handle, msg, length);
    return ret;
}

/**@brief Function for sending PAAR ble message(peripheral, PAAR tx handle) *Blocked function
 */
int32_t PAAR_send_ble_msg_peripheral(uint8_t *msg, uint16_t length)
{
    ret_code_t ret;

    sem_clear(_ble_notification_complete_sem);

    ret = ble_paar_ble_send_msg(&m_ble_paar, msg, length);
    if (ret != NRF_SUCCESS)
        return -4;

    return sem_take_timed(_ble_notification_complete_sem, BLE_SEND_MSG_COMPLETE_DELAY);
}

/**@brief Function for starting PAAR BLE scan
 */
void PAAR_scan_start(void)
{

    ret_code_t ret;

    uint8_t temp_test_cnt = 0;

    ret = nrf_ble_scan_start(&m_scan);
    if (ret != NRF_SUCCESS)
    {
        printf("BLE erros : SCAN_fail! %d\r\n", ret);
        temp_test_cnt++;
    }
//    printf("Scan result : %d \r\n", ret);

    APP_ERROR_CHECK(ret);

}

/**@brief Function for stopping PAAR BLE scan
 */
void PAAR_scan_stop(void)
{
    sd_ble_gap_scan_stop();
}

/**@brief Function for restarting PAAR BLE advertising
 */
void PAAR_adv_restart()
{
    PAAR_advertising_init();

    PAAR_advertising_start();
}

/**@brief Function for starting PAAR BLE advertising
 */
void PAAR_adv_start()
{
    PAAR_advertising_start();
}

/**@brief Function for stopping PAAR BLE advertising
 */
void PAAR_adv_stop(void)
{
    PAAR_advertising_stop();
}

/**@brief Function for setting PAAR BLE advertising data
 */
void PAAR_set_adv_data(uint8_t *adv_data, uint16_t adv_data_len, uint8_t *scan_rsp_data, uint16_t scan_rsp_data_len)
{
    m_adv_data.adv_data.p_data = adv_data;
    m_adv_data.adv_data.len = adv_data_len;
    m_adv_data.scan_rsp_data.p_data = scan_rsp_data;
    m_adv_data.scan_rsp_data.len = scan_rsp_data_len;

    PAAR_advertising_init();
}

/**@brief Function for BLE gap connection
 */
void PAAR_ble_gap_connect(ble_gap_addr_t *peer_addr)
{
    uint32_t r;
    r = sd_ble_gap_connect(peer_addr, &m_scan_params, &m_connection_param, APP_BLE_CONN_CFG_TAG);
//    printf("connecting result : %d\r\n", r);
}

/**@brief Function for BLE gap disconnection
 */
uint32_t PAAR_ble_gap_disconnect(uint16_t conn_handle)
{
    return sd_ble_gap_disconnect(conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
}

/**@brief Function for getting current peripheral information
 */
ble_paar_t* get_peripheral_info(void)
{
    return &m_ble_paar;
}

/**@brief Function for BLE GAP/GATT event handler.
 */
static void on_ble_evt(ble_evt_t *p_ble_evt, void *p_context)
{

    switch (p_ble_evt->header.evt_id)
    {
    /**************************************** Common evt ************************************************************/
    case BLE_GAP_EVT_CONNECTED:
    {
        uint16_t conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
        ble_gap_evt_connected_t *p_conn = &(p_ble_evt->evt.gap_evt.params.connected);

        if (p_conn->role == BLE_GAP_ROLE_CENTRAL)			// �̵��ܸ�(Smart Tag �Ǵ� Band)�� �����
        {
            BLE_process_event_send(BLE_CENTRAL_EVT, BLE_CENTRAL_ST_CONNECTED, conn_handle, 0, 0, NULL);
        }
        else if (p_conn->role == BLE_GAP_ROLE_PERIPH)		// smartphone app�� �����
        {
            m_ble_paar.conn_handle = conn_handle;
            sd_ble_gatts_sys_attr_set(m_ble_paar.conn_handle, NULL, 0, 0);

            BLE_process_event_send(BLE_PERIPHERAL_EVT, BLE_PERIPHERAL_ST_CONNECTED, conn_handle, 0, 0, NULL);
        }
        break;
    }

    case BLE_GAP_EVT_DISCONNECTED:
    {
        uint16_t conn_handle = p_ble_evt->evt.gap_evt.conn_handle;

        if (conn_handle == m_ble_paar.conn_handle)
        {
            BLE_process_event_send(BLE_PERIPHERAL_EVT, BLE_PERIPHERAL_ST_DISCONNECTED, conn_handle, 0, 0, NULL);
            m_ble_paar.conn_handle = BLE_CONN_HANDLE_INVALID;
        }
        else
        {
            BLE_process_event_send(BLE_CENTRAL_EVT, BLE_CENTRAL_ST_DISCONNECTED, conn_handle, 0, 0, NULL);
        }

        break;
    }

    case BLE_GAP_EVT_TIMEOUT:
    {
        ble_gap_evt_timeout_t *p_timeout = &(p_ble_evt->evt.gap_evt.params.timeout);

        if (p_timeout->src == BLE_GAP_TIMEOUT_SRC_SCAN)
        {
            BLE_process_event_send(BLE_CENTRAL_EVT, BLE_CENTRAL_ST_SCAN_TIMEOUT, BLE_CONN_HANDLE_INVALID, 0, 0, NULL);
        }
        else if (p_timeout->src == BLE_GAP_TIMEOUT_SRC_CONN)
        {
            BLE_process_event_send(BLE_CENTRAL_EVT, BLE_CENTRAL_ST_CONNECTION_TIMEOUT, BLE_CONN_HANDLE_INVALID, 0, 0, NULL);
        }
        break;
    }

        /**************************************** Central evt ************************************************************/
    case BLE_GAP_EVT_ADV_REPORT:
    {
        ble_gap_evt_adv_report_t *pPkt = &(p_ble_evt->evt.gap_evt.params.adv_report);

//			process_ADV_Report(pPkt);

        temp_ble_periph_adv_buf.evt = BLE_STACK_PERIPHERAL_ADV_EVT;
        temp_ble_periph_adv_buf.conn_handle = BLE_CONN_HANDLE_INVALID;

        if (temp_adv_buffer_count < MAX_SIZE_TEMP_ADV_BUFFER)
        {

            memcpy(&temp_adv_buffer[temp_adv_buffer_index].rssi, &pPkt->rssi, sizeof(int8_t));

            memcpy(&temp_adv_buffer[temp_adv_buffer_index].peer_address, &pPkt->peer_addr, sizeof(ble_gap_addr_t));

            memcpy(&temp_adv_buffer[temp_adv_buffer_index].direct_address, &pPkt->direct_addr, sizeof(ble_gap_addr_t));

            memcpy(&temp_adv_buffer[temp_adv_buffer_index].data_len, &pPkt->data.len, sizeof(uint16_t));

            if (temp_adv_buffer[temp_adv_buffer_index].data_len != LAP_ADV_DATA_LEN)
            {
                return;
            }

            memcpy(&temp_adv_buffer[temp_adv_buffer_index].data, pPkt->data.p_data, LAP_ADV_DATA_LEN);

            temp_ble_periph_adv_buf.msg[TEMP_ADV_MSG_INDEX] = temp_adv_buffer_index;

            temp_adv_buffer_index++;
            if (temp_adv_buffer_index >= MAX_SIZE_TEMP_ADV_BUFFER)
                temp_adv_buffer_index = 0;

            int r;
            r = msgq_send(ble_rx_msgq, (uint8_t*) &temp_ble_periph_adv_buf);
            if (r == 0)
            {
                temp_adv_buffer_count++;
            }
            else
            {
                printf("msgq send error : ble rx msg\r\n");
            }
        }
        break;
    }

    case BLE_GATTC_EVT_HVX:
    {
        uint16_t conn_handle = p_ble_evt->evt.gattc_evt.conn_handle;
        ble_gattc_evt_hvx_t *pPkt = &(p_ble_evt->evt.gattc_evt.params.hvx);

        memset(&temp_ble_cent_rx_buf, 0, sizeof(ble_rx_msgt));

        temp_ble_cent_rx_buf.evt = BLE_STACK_CENTRAL_EVT;
        temp_ble_cent_rx_buf.conn_handle = conn_handle;

        if (pPkt->len >= PAAR_MAXIMUM_PACKET_SIZE)
        {
            temp_ble_cent_rx_buf.len = PAAR_MAXIMUM_PACKET_SIZE;
            memcpy(&temp_ble_cent_rx_buf.msg, &pPkt->data, PAAR_MAXIMUM_PACKET_SIZE);
        }
        else
        {
            temp_ble_cent_rx_buf.len = pPkt->len;
            memcpy(&temp_ble_cent_rx_buf.msg, pPkt->data, pPkt->len);
        }

        msgq_send(ble_rx_msgq, (uint8_t*) &temp_ble_cent_rx_buf);
        break;
    }

    case BLE_GATTC_EVT_WRITE_RSP:
    {
        sem_give(_ble_write_request_complete_sem);

        break;
    }

        /**************************************** Peripehral evt ************************************************************/
    case BLE_GATTS_EVT_WRITE:
    {
        ble_gatts_evt_write_t *p_wpkt = &p_ble_evt->evt.gatts_evt.params.write;
        uint16_t conn_handle = p_ble_evt->evt.gatts_evt.conn_handle;

        if (p_wpkt->handle == m_ble_paar.tx_handles.cccd_handle && (p_wpkt->len == 2))
        {

            if ((p_wpkt->data[0] & 0x01) == 0x01)
            {
                m_ble_paar.is_notification_enabled = true;
            }
            else
            {
                m_ble_paar.is_notification_enabled = false;
            }

            BLE_process_event_send(BLE_PERIPHERAL_EVT, BLE_PERIPHERAL_ST_CCCD_ENABLED, 0, 0, 0, NULL);

        }
        else if (p_wpkt->handle == m_ble_paar.rx_handles.value_handle)
        {

            memset(&temp_ble_periph_buf, 0, sizeof(ble_rx_msgt));

            temp_ble_periph_buf.evt = BLE_STACK_PERIPHERAL_EVT;
            temp_ble_periph_buf.conn_handle = conn_handle;

            if (p_wpkt->len >= PAAR_MAXIMUM_PACKET_SIZE)
            {
                temp_ble_periph_buf.len = PAAR_MAXIMUM_PACKET_SIZE;
                memcpy(&temp_ble_periph_buf.msg, p_wpkt->data, PAAR_MAXIMUM_PACKET_SIZE);
            }
            else
            {
                temp_ble_periph_buf.len = p_wpkt->len;
                memcpy(&temp_ble_periph_buf.msg, p_wpkt->data, p_wpkt->len);
            }

            msgq_send(ble_rx_msgq, (uint8_t*) &temp_ble_periph_buf);
        }
        else
        {
//				int test_falg;
//				test_falg++;
        }

        break;
    }

    case BLE_GATTS_EVT_SYS_ATTR_MISSING:
    {
        // No system attributes have been stored.
        sd_ble_gatts_sys_attr_set(m_ble_paar.conn_handle, NULL, 0, 0);
        break;
    }

    case BLE_GATTS_EVT_HVN_TX_COMPLETE:
        sem_give(_ble_notification_complete_sem);
        break;
    }
}

/**@brief Function for initializing the GATT module. */
static void gatt_init(void)
{
    ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, NULL);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling Scanning Module events.
 */
static void PAAR_scan_evt_handler(scan_evt_t const *p_scan_evt)
{
    ret_code_t err_code;

    switch (p_scan_evt->scan_evt_id)
    {
    case NRF_BLE_SCAN_EVT_CONNECTING_ERROR:
    {
        err_code = p_scan_evt->params.connecting_err.err_code;
        APP_ERROR_CHECK(err_code);
    }
        break;
    case NRF_BLE_SCAN_EVT_SCAN_TIMEOUT:
        break;
    default:
        break;
    }
}

/**@brief Function for initializing the scanning and setting the filters.
 */
static void PAAR_scan_init(void)
{
    ret_code_t err_code;

    nrf_ble_scan_init_t init_scan;

    memset(&init_scan, 0, sizeof(init_scan));

    init_scan.connect_if_match = true;
    init_scan.conn_cfg_tag = APP_BLE_CONN_CFG_TAG;

    init_scan.p_scan_param = &m_scan_params;

    err_code = nrf_ble_scan_init(&m_scan, &init_scan, PAAR_scan_evt_handler);
    APP_ERROR_CHECK(err_code);

}

/**@brief Function for starting advertising.
 */
static void PAAR_advertising_start(void)
{
    if (is_advertising_init == false)
        return;

    ret_code_t err_code;

    err_code = sd_ble_gap_adv_start(m_adv_handle, APP_BLE_CONN_CFG_TAG);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for stopping advertising.
 */
static void PAAR_advertising_stop(void)
{
    if (is_advertising_init == false)
        return;

    ret_code_t err_code;

    err_code = sd_ble_gap_adv_stop(m_adv_handle);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the Advertising functionality.
 *
 * @details Encodes the required advertising data and passes it to the stack.
 *          Also builds a structure to be passed to the stack when starting advertising.
 */
static void PAAR_advertising_init(void)
{
    ret_code_t err_code;
    ble_advdata_t advdata;

    // Build and set advertising data.
    memset(&advdata, 0, sizeof(advdata));

    advdata.include_appearance = true;
    advdata.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

    ble_gap_adv_params_t adv_params;

    // Set advertising parameters.
    memset(&adv_params, 0, sizeof(adv_params));

    adv_params.primary_phy = BLE_GAP_PHY_1MBPS;
    adv_params.duration = APP_ADV_DURATION;
    adv_params.properties.type = BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED;
    adv_params.p_peer_addr = NULL;
    adv_params.filter_policy = BLE_GAP_ADV_FP_ANY;
    adv_params.interval = APP_ADV_INTERVAL;

    err_code = sd_ble_gap_adv_set_configure(&m_adv_handle, &m_adv_data, &adv_params);
    APP_ERROR_CHECK(err_code);

    is_advertising_init = true;
}

/**@brief Function for ble stack task.
 */
void ble_stack_task(void *arg)
{

    //APP timer init
    app_timer_init();

    //ble stack init
    ble_stack_init();

    //gap parameter setup
    gap_params_init();

    //gatt init
    gatt_init();

    //paar service profile setup
    services_init();

    //scan init
    PAAR_scan_init();

    //ble connection parameter init
    conn_params_init();

    //setup tx power : 4dBm
    sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_CONN, 0, 4);

    //set flag : ble stack init complete
    set_ble_stack_init_flag(true);

//	memcpy(hub_sosp_id, &(NRF_FICR->DEVICEADDR[0]), HUB_SOSP_ID_LEN);

    int r;
    ble_rx_msgt ble_rx_msg_buffer;

    //processing ble rx messages
    for (;;)
    {
        r = msgq_receive(ble_rx_msgq, (unsigned char*) &ble_rx_msg_buffer);
        if (0 != r)
        {
            logme("fail at msgq_receive\r\n");
        }
        else
        {
            switch (ble_rx_msg_buffer.evt)
            {
            //central rx message
            case BLE_STACK_CENTRAL_EVT:
            {
                uint8_t *temp_ble_rx_packet = NULL;
                temp_ble_rx_packet = (uint8_t*) malloc(ble_rx_msg_buffer.len);
                if (temp_ble_rx_packet == NULL)
                {
                    break;
                }
                else
                {
//                    printf("malloc BLE_STACK_CENTRAL_EVT\r\n");
                }

                memcpy(temp_ble_rx_packet, &ble_rx_msg_buffer.msg, ble_rx_msg_buffer.len);
                BLE_process_event_send(BLE_CENTRAL_EVT, BLE_CENTRAL_ST_DATA_RECEIVED, ble_rx_msg_buffer.conn_handle, 0, ble_rx_msg_buffer.len,
                        temp_ble_rx_packet);
            }
                break;
                //peripheral rx message
            case BLE_STACK_PERIPHERAL_EVT:
            {
                uint8_t *temp_ble_rx_packet = NULL;
                temp_ble_rx_packet = (uint8_t*) malloc(ble_rx_msg_buffer.len);
                if (temp_ble_rx_packet == NULL)
                {
                    break;
                }
                else
                {
//                    printf("malloc BLE_STACK_PERIPHERAL_EVT\r\n");
                }

                memcpy(temp_ble_rx_packet, &ble_rx_msg_buffer.msg, ble_rx_msg_buffer.len);
                BLE_process_event_send(BLE_PERIPHERAL_EVT, BLE_PERIPHERAL_ST_DATA_RECEIVED, ble_rx_msg_buffer.conn_handle, 0, ble_rx_msg_buffer.len,
                        temp_ble_rx_packet);
            }
                break;
            case BLE_STACK_PERIPHERAL_ADV_EVT:
            {
                process_ADV_Report(&temp_adv_buffer[ble_rx_msg_buffer.msg[TEMP_ADV_MSG_INDEX]]);
                memset(&temp_adv_buffer[ble_rx_msg_buffer.msg[TEMP_ADV_MSG_INDEX]], 0, sizeof(LAP_ble_adv_report));
                temp_adv_buffer_count--;
            }
                break;
            }
        }
    }
}

/**@brief Function for initializing ble stack task.
 */
void BLE_stack_task_init(void)
{

    int r;

    r = semb_create(&_ble_evt_sem);
    if (0 != r)
    {
        printf("fail to sem_create\n\r");
    }

    r = semb_create(&_ble_notification_complete_sem);
    if (0 != r)
    {
        printf("fail to sem_create\n\r");
    }

    r = semb_create(&_ble_indication_complete_sem);
    if (0 != r)
    {
        printf("fail to sem_create\n\r");
    }

    r = semb_create(&_ble_write_request_complete_sem);
    if (0 != r)
    {
        printf("fail to sem_create\n\r");
    }

    r = semb_create(&_ble_command_complete_sem);
    if (0 != r)
    {
        printf("fail to sem_create\n\r");
    }

    r = msgq_create(&ble_rx_msgq, sizeof(ble_rx_msgt), 30);
    if (0 != r)
    {
        printf("fail at msgq create\r\n");
    }

    r = task_create(NULL, ble_stack_task, NULL, task_gethighestpriority(), 1024, NULL);
    if (r != 0)
    {
        printf("== ble_task failed\n\r");
    }
    else
    {
        printf("== ble_task created\n\r");
    }
}
