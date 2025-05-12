
#include "temperature.h"


#include "app_log.h"
#include "app_assert.h"
#include "sl_sensor_rht.h"

int32_t  temperature = 0;
uint32_t humidite = 0;


/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/


int16_t lect_temp(void)
{
    sl_status_t sc;


    int16_t temperature_ble = 0;

    app_log_info("%s: Lecture_completed!\n",__FUNCTION__);
    sc = sl_sensor_rht_get(&humidite, &temperature) ;
    app_assert_status(sc); //verification que la fonction a bien réussie

    temperature_ble = temperature / 10 ; //conversion de la temperature au format ble

    //Récupération température
    app_log_info("Temperature en milli degres = %ld \n",temperature);
    app_log_info("Temperature en format BLE = %d \n",temperature_ble);

    return temperature_ble; //renvoie la temperature en format milli degres, si on veut en BLE on divise par 10.
}

