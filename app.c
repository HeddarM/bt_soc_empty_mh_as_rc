/***************************************************************************//**
 * @file
 * @brief Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/

#include "temperature.h"

#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "app.h"
#include "app_log.h"
#include "sl_sensor_rht.h"
#include "gatt_db.h"

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;
/**************************************************************************//**
 * Declaration de variables
 *****************************************************************************/
int timer_step = 0;
static sl_sleeptimer_timer_handle_t timer_handle;

void timer_callback(sl_sleeptimer_timer_handle_t *handle_timer, void *data);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  app_log_info("%s\n",__FUNCTION__);
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
SL_WEAK void app_process_action(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application code here!                              //
  // This is called infinitely.                                              //
  // Do not call blocking functions from here!                               //
  /////////////////////////////////////////////////////////////////////////////
}

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      app_assert_status(sc);

      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        160, // min. adv. interval (milliseconds * 1.6)
        160, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events
      app_assert_status(sc);
      // Start advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_legacy_advertiser_connectable);
      app_assert_status(sc);
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      app_log_info("%s: connection_opened!\n",__FUNCTION__);
      sl_sensor_rht_init();   //Initialisation du capteur
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      app_log_info("%s: connection_closed!\n",__FUNCTION__);
      sl_sensor_rht_deinit();     // Désinitialisation du capteur
      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_legacy_advertiser_connectable);
      app_assert_status(sc);
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////
    // This event indicates that a lecture has been done
    case sl_bt_evt_gatt_server_user_read_request_id:

      if (evt->data.evt_gatt_server_user_read_request.characteristic == gattdb_temperature)
      {
          uint16_t sent_len = 0;

          app_log_info("evt_gatt_server_user_read_request.characteristic : gattdb_temperature \n");
          int16_t temp = lect_temp ();

          uint8_t tableau_temp[2];

          tableau_temp[0] = 1;
          tableau_temp[1] = 2;

          sc = sl_bt_gatt_server_send_user_read_response (evt->data.evt_gatt_server_user_read_request.connection,
                                                          gattdb_temperature,
                                                          0,
                                                          sizeof(temp),
                                                          tableau_temp,
                                                          &sent_len);
          app_assert_status(sc);
           if (sc != SL_STATUS_OK)
             {
               app_log_info("reponse non recue \n");
             }
           else
             {
               app_log_info("reponse recue avec sent len = %u \n", sent_len);
             }
      }
      break;

    case sl_bt_evt_gatt_server_characteristic_status_id:

      uint16_t characteristic = evt->data.evt_gatt_server_characteristic_status.characteristic;
      uint8_t status_flag = evt->data.evt_gatt_server_characteristic_status.status_flags;
      uint16_t client_config_flag = evt->data.evt_gatt_server_characteristic_status.client_config_flags;


      if (characteristic == gattdb_temperature &&
          status_flag == 1 &&
          client_config_flag == 1 )
        {
          sl_sleeptimer_start_periodic_timer_ms( &timer_handle,
                                                 1000,                       // période en ms = 1s
                                                 timer_callback,             // fonction appelée à toutes les 1s
                                                 NULL,                       // pas de données pour l'instant
                                                 0,                          //
                                                 0);

          app_log_info("notify detected with characteristic status : %u  et client config flag %u \n", status_flag, client_config_flag);
          lect_temp ();
        }


      break;


    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

void timer_callback(sl_sleeptimer_timer_handle_t *handle_timer, void *data)
{
  (void)handle_timer;
  (void)data;

  timer_step++;
  app_log_info("Timer step %d\n", timer_step);
}
