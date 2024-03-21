/************************************************************************
 *
 * File Name  : ble_service_dis.c
 * Description: This file contains the definitions and functions of BLE DIS
 *
 *
 ************************************************************************/
#include "ble_service_dis.h"
#include "ble_profile.h"

/* will be defined in bleAppSetting.h, if there is no definition then set to the default value. */
#ifndef BLE_COMPANY_ID_L
#define BLE_COMPANY_ID_L            0xFF
#endif

#ifndef BLE_COMPANY_ID_H
#define BLE_COMPANY_ID_H            0xFF
#endif

/** ble_svcs_dis_handler
 * @note This callback receives the DIS events.
 * Each of these events can be associated with parameters.
*/

void ble_svcs_dis_handler(ble_evt_att_param_t *p_param);


/**************************************************************************
 * DIS Service Value Definitions
 **************************************************************************/

/** DIS characteristic serial number value.
 * @note Return the "Read data" when central send "Read Request".
*/
const uint8_t dis_serial_number_string[]     = "1234";

/** DIS characteristic manufacturer name string value.
 * @note Return the "Read data" when central send "Read Request".
*/
const uint8_t dis_manufacturer_name_string[] = "BLE";


/** DIS characteristic firmware revision string value.
 * @note Return the "Read data" when central send "Read Request".
*/
const uint8_t dis_firmware_revision_string[] = "1.0";


/** DIS characteristic model number string value.
 * @note Return the "Read data" when central send "Read Request".
*/
const uint8_t dis_model_number_string[]      = "RT58x";


/** DIS characteristic hardware revision string value.
 * @note Return the "Read data" when central send "Read Request".
*/
const uint8_t dis_hardware_revision_string[] = "U01";


/** DIS characteristic software revision string value.
 * @note Return the "Read data" when central send "Read Request".
*/
const uint8_t dis_software_revision_string[] = "1.0";


/** DIS characteristic system ID string value.
 * @note Return the "Read data" when central send "Read Request".
*/
SystemId_Format dis_system_id =
{
    0x00000055AA55AA55,                                                   //Manufacturer, 5 bytes shall be used.
    0x00AA55AA,                                                           //Organizationally Unique ID, 3 bytes shall be used.
};

/** DIS characteristic PnP ID string value.
 * @note Return the "Read data" when central send "Read Request".
*/
PnPId_Format dis_pnp_id =
{
    BLE_GATT_DIS_PNPID_VID_SOURCE_BLUETOOTH_SIG,                          //Vendor ID Source, identifies the source of the Vendor ID field.
    ((uint16_t)BLE_COMPANY_ID_H << 8) | BLE_COMPANY_ID_L,                 //Vendor ID
    0x0000,                                                               //Product ID: Manufacturer managed identifier for this product
    0x0000,                                                               //Product Version: Manufacturer managed version for this product
};


// Set DIS Server Default Data.
string_format_t dis_serial_number =
{
    (uint8_t *)dis_serial_number_string,
    SIZE_STRING(dis_serial_number_string),
};

string_format_t dis_manufacturer_name =
{
    (uint8_t *)dis_manufacturer_name_string,
    SIZE_STRING(dis_manufacturer_name_string),
};

string_format_t dis_firmware_revision =
{
    (uint8_t *)dis_firmware_revision_string,
    SIZE_STRING(dis_firmware_revision_string),
};

string_format_t dis_model_number =
{
    (uint8_t *)dis_model_number_string,
    SIZE_STRING(dis_model_number_string),
};

string_format_t dis_hardware_revision =
{
    (uint8_t *)dis_hardware_revision_string,
    SIZE_STRING(dis_hardware_revision_string),
};

string_format_t dis_software_revision =
{
    (uint8_t *)dis_software_revision_string,
    SIZE_STRING(dis_software_revision_string),
};

ble_svcs_dis_data_t dis_data =
{
    &dis_serial_number,
    &dis_manufacturer_name,
    &dis_system_id,
    &dis_firmware_revision,
    &dis_model_number,
    &dis_hardware_revision,
    &dis_software_revision,
    &dis_pnp_id,
};

/**************************************************************************
 * DIS UUID Definitions
 **************************************************************************/

/** DIS UUID.
 * @note 16-bits UUID
 * @note UUID: 180A
*/

const uint16_t attr_uuid_dis_primary_service[] =
{
    GATT_SERVICES_DEVICE_INFORMATION,
};

/** DIS characteristic SERIAL_NUMBER_STRING UUID.
 * @note 16-bits UUID
 * @note UUID: 2A25
*/

const uint16_t attr_uuid_dis_charc_serial_number_string[] =
{
    GATT_SPEC_CHARC_SERIAL_NUMBER_STRING,
};

/** DIS characteristic MANUFACTURER_NAME_STRING UUID.
 * @note 16-bits UUID
 * @note UUID: 2A29
*/

const uint16_t attr_uuid_dis_charc_manufacturer_name_string[] =
{
    GATT_SPEC_CHARC_MANUFACTURER_NAME_STRING,
};

/** DIS characteristic SYSTEM_ID UUID.
 * @note 16-bits UUID
 * @note UUID: 2A23
*/

const uint16_t attr_uuid_dis_charc_system_id[] =
{
    GATT_SPEC_CHARC_SYSTEM_ID,
};

/** DIS characteristic FIRMWARE_REVISION_STRING UUID.
 * @note 16-bits UUID
 * @note UUID: 2A26
*/

const uint16_t attr_uuid_dis_charc_firmware_revision_string[] =
{
    GATT_SPEC_CHARC_FIRMWARE_REVISION_STRING,
};

/** DIS characteristic MODEL_NUMBER_STRING UUID.
 * @note 16-bits UUID
 * @note UUID: 2A24
*/

const uint16_t attr_uuid_dis_charc_model_number_string[] =
{
    GATT_SPEC_CHARC_MODEL_NUMBER_STRING,
};

/** DIS characteristic HARDWARE_REVISION_STRING UUID.
 * @note 16-bits UUID
 * @note UUID: 2A27
*/

const uint16_t attr_uuid_dis_charc_hardware_revision_string[] =
{
    GATT_SPEC_CHARC_HARDWARE_REVISION_STRING,
};

/** DIS characteristic SOFTWARE_REVISION_STRING UUID.
 * @note 16-bits UUID
 * @note UUID: 2A28
*/

const uint16_t attr_uuid_dis_charc_software_revision_string[] =
{
    GATT_SPEC_CHARC_SOFTWARE_REVISION_STRING,
};

/** DIS characteristic PNP_ID UUID.
 * @note 16-bits UUID
 * @note UUID: 2A50
*/

const uint16_t attr_uuid_dis_charc_pnp_id[] =
{
    GATT_SPEC_CHARC_PNP_ID,
};


/**************************************************************************
 * DIS Service Value Definitions
 **************************************************************************/


/**************************************************************************
 * DIS Service/ Characteristic Definitions
 **************************************************************************/

const ble_att_param_t att_dis_primary_service =
{
    (void *)attr_uuid_type_primary_service,
    (void *)attr_uuid_dis_primary_service,
    sizeof(attr_uuid_dis_primary_service),
    (
        //GATT_DECLARATIONS_PROPERTIES_BROADCAST |
        GATT_DECLARATIONS_PROPERTIES_READ |
        //GATT_DECLARATIONS_PROPERTIES_WRITE_WITHOUT_RESPONSE |
        //GATT_DECLARATIONS_PROPERTIES_WRITE |
        //GATT_DECLARATIONS_PROPERTIES_NOTIFY |
        //GATT_DECLARATIONS_PROPERTIES_INDICATE |
        //GATT_DECLARATIONS_PROPERTIES_AUTHENTICATED_SIGNED_WRITES |
        //GATT_DECLARATIONS_PROPERTIES_EXTENDED_PROPERTIES |
        0x00
    ),
    (
        ATT_TYPE_FORMAT_16UUID |            //otherwise, 128bit UUID
        //ATT_VALUE_BOND_ENABLE |
        //ATT_PERMISSION_ENC_READ |
        //ATT_PERMISSION_ENC_WRITE |
        //ATT_PERMISSION_AUTHE_READ |
        //ATT_PERMISSION_AUTHE_WRITE |
        //ATT_PERMISSION_AUTHO_READ |
        //ATT_PERMISSION_AUTHO_WRITE |
        0x00
    ),
    attr_null_access,                       //This function should be set to attr_null_access when att_len or p_uuid_value is a null value.
};

const ble_att_param_t att_dis_characteristic_serial_number_string =
{
    (void *)attr_uuid_type_characteristic,
    (void *)attr_uuid_dis_charc_serial_number_string,
    sizeof(attr_uuid_dis_charc_serial_number_string),
    (
        //GATT_DECLARATIONS_PROPERTIES_BROADCAST |
        GATT_DECLARATIONS_PROPERTIES_READ |
        //GATT_DECLARATIONS_PROPERTIES_WRITE_WITHOUT_RESPONSE |
        //GATT_DECLARATIONS_PROPERTIES_WRITE |
        //GATT_DECLARATIONS_PROPERTIES_NOTIFY |
        //GATT_DECLARATIONS_PROPERTIES_INDICATE |
        //GATT_DECLARATIONS_PROPERTIES_AUTHENTICATED_SIGNED_WRITES |
        //GATT_DECLARATIONS_PROPERTIES_EXTENDED_PROPERTIES |
        0x00
    ),
    (
        ATT_TYPE_FORMAT_16UUID |            //otherwise, 128bit UUID
        //ATT_VALUE_BOND_ENABLE |
        //ATT_PERMISSION_ENC_READ |
        //ATT_PERMISSION_ENC_WRITE |
        //ATT_PERMISSION_AUTHE_READ |
        //ATT_PERMISSION_AUTHE_WRITE |
        //ATT_PERMISSION_AUTHO_READ |
        //ATT_PERMISSION_AUTHO_WRITE |
        0x00
    ),
    attr_null_access,                       //This function should be set to attr_null_access when att_len or p_uuid_value is a null value.
};

const ble_att_param_t att_dis_serial_number_string =
{
    (void *)attr_uuid_dis_charc_serial_number_string,
    (void *)0,
    0,
    (
        //GATT_DECLARATIONS_PROPERTIES_BROADCAST |
        GATT_DECLARATIONS_PROPERTIES_READ |
        //GATT_DECLARATIONS_PROPERTIES_WRITE_WITHOUT_RESPONSE |
        //GATT_DECLARATIONS_PROPERTIES_WRITE |
        //GATT_DECLARATIONS_PROPERTIES_NOTIFY |
        //GATT_DECLARATIONS_PROPERTIES_INDICATE |
        //GATT_DECLARATIONS_PROPERTIES_AUTHENTICATED_SIGNED_WRITES |
        //GATT_DECLARATIONS_PROPERTIES_EXTENDED_PROPERTIES |
        0x00
    ),
    (
        ATT_TYPE_FORMAT_16UUID |            //otherwise, 128bit UUID
        //ATT_VALUE_BOND_ENABLE |
        //ATT_PERMISSION_ENC_READ |
        //ATT_PERMISSION_ENC_WRITE |
        //ATT_PERMISSION_AUTHE_READ |
        //ATT_PERMISSION_AUTHE_WRITE |
        //ATT_PERMISSION_AUTHO_READ |
        //ATT_PERMISSION_AUTHO_WRITE |
        0x00
    ),
    ble_svcs_dis_handler,       //registered callback function
};


const ble_att_param_t att_dis_characteristic_manufacturer_name_string =
{
    (void *)attr_uuid_type_characteristic,
    (void *)attr_uuid_dis_charc_manufacturer_name_string,
    sizeof(attr_uuid_dis_charc_manufacturer_name_string),
    (
        //GATT_DECLARATIONS_PROPERTIES_BROADCAST |
        GATT_DECLARATIONS_PROPERTIES_READ |
        //GATT_DECLARATIONS_PROPERTIES_WRITE_WITHOUT_RESPONSE |
        //GATT_DECLARATIONS_PROPERTIES_WRITE |
        //GATT_DECLARATIONS_PROPERTIES_NOTIFY |
        //GATT_DECLARATIONS_PROPERTIES_INDICATE |
        //GATT_DECLARATIONS_PROPERTIES_AUTHENTICATED_SIGNED_WRITES |
        //GATT_DECLARATIONS_PROPERTIES_EXTENDED_PROPERTIES |
        0x00
    ),
    (
        ATT_TYPE_FORMAT_16UUID |            //otherwise, 128bit UUID
        //ATT_VALUE_BOND_ENABLE |
        //ATT_PERMISSION_ENC_READ |
        //ATT_PERMISSION_ENC_WRITE |
        //ATT_PERMISSION_AUTHE_READ |
        //ATT_PERMISSION_AUTHE_WRITE |
        //ATT_PERMISSION_AUTHO_READ |
        //ATT_PERMISSION_AUTHO_WRITE |
        0x00
    ),
    attr_null_access,                       //This function should be set to attr_null_access when att_len or p_uuid_value is a null value.
};

const ble_att_param_t att_dis_manufacturer_name_string =
{
    (void *)attr_uuid_dis_charc_manufacturer_name_string,
    (void *)0,
    0,
    (
        //GATT_DECLARATIONS_PROPERTIES_BROADCAST |
        GATT_DECLARATIONS_PROPERTIES_READ |
        //GATT_DECLARATIONS_PROPERTIES_WRITE_WITHOUT_RESPONSE |
        //GATT_DECLARATIONS_PROPERTIES_WRITE |
        //GATT_DECLARATIONS_PROPERTIES_NOTIFY |
        //GATT_DECLARATIONS_PROPERTIES_INDICATE |
        //GATT_DECLARATIONS_PROPERTIES_AUTHENTICATED_SIGNED_WRITES |
        //GATT_DECLARATIONS_PROPERTIES_EXTENDED_PROPERTIES |
        0x00
    ),
    (
        ATT_TYPE_FORMAT_16UUID |            //otherwise, 128bit UUID
        //ATT_VALUE_BOND_ENABLE |
        //ATT_PERMISSION_ENC_READ |
        //ATT_PERMISSION_ENC_WRITE |
        //ATT_PERMISSION_AUTHE_READ |
        //ATT_PERMISSION_AUTHE_WRITE |
        //ATT_PERMISSION_AUTHO_READ |
        //ATT_PERMISSION_AUTHO_WRITE |
        0x00
    ),
    ble_svcs_dis_handler,       //registered callback function
};


const ble_att_param_t att_dis_characteristic_system_id =
{
    (void *)attr_uuid_type_characteristic,
    (void *)attr_uuid_dis_charc_system_id,
    sizeof(attr_uuid_dis_charc_system_id),
    (
        //GATT_DECLARATIONS_PROPERTIES_BROADCAST |
        GATT_DECLARATIONS_PROPERTIES_READ |
        //GATT_DECLARATIONS_PROPERTIES_WRITE_WITHOUT_RESPONSE |
        //GATT_DECLARATIONS_PROPERTIES_WRITE |
        //GATT_DECLARATIONS_PROPERTIES_NOTIFY |
        //GATT_DECLARATIONS_PROPERTIES_INDICATE |
        //GATT_DECLARATIONS_PROPERTIES_AUTHENTICATED_SIGNED_WRITES |
        //GATT_DECLARATIONS_PROPERTIES_EXTENDED_PROPERTIES |
        0x00
    ),
    (
        ATT_TYPE_FORMAT_16UUID |            //otherwise, 128bit UUID
        //ATT_VALUE_BOND_ENABLE |
        //ATT_PERMISSION_ENC_READ |
        //ATT_PERMISSION_ENC_WRITE |
        //ATT_PERMISSION_AUTHE_READ |
        //ATT_PERMISSION_AUTHE_WRITE |
        //ATT_PERMISSION_AUTHO_READ |
        //ATT_PERMISSION_AUTHO_WRITE |
        0x00
    ),
    attr_null_access,                       //This function should be set to attr_null_access when att_len or p_uuid_value is a null value.
};

const ble_att_param_t att_dis_system_id =
{
    (void *)attr_uuid_dis_charc_system_id,
    (void *)0,
    0,
    (
        //GATT_DECLARATIONS_PROPERTIES_BROADCAST |
        GATT_DECLARATIONS_PROPERTIES_READ |
        //GATT_DECLARATIONS_PROPERTIES_WRITE_WITHOUT_RESPONSE |
        //GATT_DECLARATIONS_PROPERTIES_WRITE |
        //GATT_DECLARATIONS_PROPERTIES_NOTIFY |
        //GATT_DECLARATIONS_PROPERTIES_INDICATE |
        //GATT_DECLARATIONS_PROPERTIES_AUTHENTICATED_SIGNED_WRITES |
        //GATT_DECLARATIONS_PROPERTIES_EXTENDED_PROPERTIES |
        0x00
    ),
    (
        ATT_TYPE_FORMAT_16UUID |            //otherwise, 128bit UUID
        //ATT_VALUE_BOND_ENABLE |
        //ATT_PERMISSION_ENC_READ |
        //ATT_PERMISSION_ENC_WRITE |
        //ATT_PERMISSION_AUTHE_READ |
        //ATT_PERMISSION_AUTHE_WRITE |
        //ATT_PERMISSION_AUTHO_READ |
        //ATT_PERMISSION_AUTHO_WRITE |
        0x00
    ),
    ble_svcs_dis_handler,       //registered callback function
};

const ble_att_param_t att_dis_characteristic_firmware_revision_string =
{
    (void *)attr_uuid_type_characteristic,
    (void *)attr_uuid_dis_charc_firmware_revision_string,
    sizeof(attr_uuid_dis_charc_firmware_revision_string),
    (
        //GATT_DECLARATIONS_PROPERTIES_BROADCAST |
        GATT_DECLARATIONS_PROPERTIES_READ |
        //GATT_DECLARATIONS_PROPERTIES_WRITE_WITHOUT_RESPONSE |
        //GATT_DECLARATIONS_PROPERTIES_WRITE |
        //GATT_DECLARATIONS_PROPERTIES_NOTIFY |
        //GATT_DECLARATIONS_PROPERTIES_INDICATE |
        //GATT_DECLARATIONS_PROPERTIES_AUTHENTICATED_SIGNED_WRITES |
        //GATT_DECLARATIONS_PROPERTIES_EXTENDED_PROPERTIES |
        0x00
    ),
    (
        ATT_TYPE_FORMAT_16UUID |            //otherwise, 128bit UUID
        //ATT_VALUE_BOND_ENABLE |
        //ATT_PERMISSION_ENC_READ |
        //ATT_PERMISSION_ENC_WRITE |
        //ATT_PERMISSION_AUTHE_READ |
        //ATT_PERMISSION_AUTHE_WRITE |
        //ATT_PERMISSION_AUTHO_READ |
        //ATT_PERMISSION_AUTHO_WRITE |
        0x00
    ),
    attr_null_access,                       //This function should be set to attr_null_access when att_len or p_uuid_value is a null value.
};

const ble_att_param_t att_dis_firmware_revision_string =
{
    (void *)attr_uuid_dis_charc_firmware_revision_string,
    (void *)0,
    0,
    (
        //GATT_DECLARATIONS_PROPERTIES_BROADCAST |
        GATT_DECLARATIONS_PROPERTIES_READ |
        //GATT_DECLARATIONS_PROPERTIES_WRITE_WITHOUT_RESPONSE |
        //GATT_DECLARATIONS_PROPERTIES_WRITE |
        //GATT_DECLARATIONS_PROPERTIES_NOTIFY |
        //GATT_DECLARATIONS_PROPERTIES_INDICATE |
        //GATT_DECLARATIONS_PROPERTIES_AUTHENTICATED_SIGNED_WRITES |
        //GATT_DECLARATIONS_PROPERTIES_EXTENDED_PROPERTIES |
        0x00
    ),
    (
        ATT_TYPE_FORMAT_16UUID |            //otherwise, 128bit UUID
        //ATT_VALUE_BOND_ENABLE |
        //ATT_PERMISSION_ENC_READ |
        //ATT_PERMISSION_ENC_WRITE |
        //ATT_PERMISSION_AUTHE_READ |
        //ATT_PERMISSION_AUTHE_WRITE |
        //ATT_PERMISSION_AUTHO_READ |
        //ATT_PERMISSION_AUTHO_WRITE |
        0x00
    ),
    ble_svcs_dis_handler,       //registered callback function
};

const ble_att_param_t att_dis_characteristic_model_number_string =
{
    (void *)attr_uuid_type_characteristic,
    (void *)attr_uuid_dis_charc_model_number_string,
    sizeof(attr_uuid_dis_charc_model_number_string),
    (
        //GATT_DECLARATIONS_PROPERTIES_BROADCAST |
        GATT_DECLARATIONS_PROPERTIES_READ |
        //GATT_DECLARATIONS_PROPERTIES_WRITE_WITHOUT_RESPONSE |
        //GATT_DECLARATIONS_PROPERTIES_WRITE |
        //GATT_DECLARATIONS_PROPERTIES_NOTIFY |
        //GATT_DECLARATIONS_PROPERTIES_INDICATE |
        //GATT_DECLARATIONS_PROPERTIES_AUTHENTICATED_SIGNED_WRITES |
        //GATT_DECLARATIONS_PROPERTIES_EXTENDED_PROPERTIES |
        0x00
    ),
    (
        ATT_TYPE_FORMAT_16UUID |            //otherwise, 128bit UUID
        //ATT_VALUE_BOND_ENABLE |
        //ATT_PERMISSION_ENC_READ |
        //ATT_PERMISSION_ENC_WRITE |
        //ATT_PERMISSION_AUTHE_READ |
        //ATT_PERMISSION_AUTHE_WRITE |
        //ATT_PERMISSION_AUTHO_READ |
        //ATT_PERMISSION_AUTHO_WRITE |
        0x00
    ),
    attr_null_access,                       //This function should be set to attr_null_access when att_len or p_uuid_value is a null value.
};

const ble_att_param_t att_dis_model_number_string =
{
    (void *)attr_uuid_dis_charc_model_number_string,
    (void *)0,
    0,
    (
        //GATT_DECLARATIONS_PROPERTIES_BROADCAST |
        GATT_DECLARATIONS_PROPERTIES_READ |
        //GATT_DECLARATIONS_PROPERTIES_WRITE_WITHOUT_RESPONSE |
        //GATT_DECLARATIONS_PROPERTIES_WRITE |
        //GATT_DECLARATIONS_PROPERTIES_NOTIFY |
        //GATT_DECLARATIONS_PROPERTIES_INDICATE |
        //GATT_DECLARATIONS_PROPERTIES_AUTHENTICATED_SIGNED_WRITES |
        //GATT_DECLARATIONS_PROPERTIES_EXTENDED_PROPERTIES |
        0x00
    ),
    (
        ATT_TYPE_FORMAT_16UUID |            //otherwise, 128bit UUID
        //ATT_VALUE_BOND_ENABLE |
        //ATT_PERMISSION_ENC_READ |
        //ATT_PERMISSION_ENC_WRITE |
        //ATT_PERMISSION_AUTHE_READ |
        //ATT_PERMISSION_AUTHE_WRITE |
        //ATT_PERMISSION_AUTHO_READ |
        //ATT_PERMISSION_AUTHO_WRITE |
        0x00
    ),
    ble_svcs_dis_handler,       //registered callback function
};

const ble_att_param_t att_dis_characteristic_hardware_revision_string =
{
    (void *)attr_uuid_type_characteristic,
    (void *)attr_uuid_dis_charc_hardware_revision_string,
    sizeof(attr_uuid_dis_charc_hardware_revision_string),
    (
        //GATT_DECLARATIONS_PROPERTIES_BROADCAST |
        GATT_DECLARATIONS_PROPERTIES_READ |
        //GATT_DECLARATIONS_PROPERTIES_WRITE_WITHOUT_RESPONSE |
        //GATT_DECLARATIONS_PROPERTIES_WRITE |
        //GATT_DECLARATIONS_PROPERTIES_NOTIFY |
        //GATT_DECLARATIONS_PROPERTIES_INDICATE |
        //GATT_DECLARATIONS_PROPERTIES_AUTHENTICATED_SIGNED_WRITES |
        //GATT_DECLARATIONS_PROPERTIES_EXTENDED_PROPERTIES |
        0x00
    ),
    (
        ATT_TYPE_FORMAT_16UUID |            //otherwise, 128bit UUID
        //ATT_VALUE_BOND_ENABLE |
        //ATT_PERMISSION_ENC_READ |
        //ATT_PERMISSION_ENC_WRITE |
        //ATT_PERMISSION_AUTHE_READ |
        //ATT_PERMISSION_AUTHE_WRITE |
        //ATT_PERMISSION_AUTHO_READ |
        //ATT_PERMISSION_AUTHO_WRITE |
        0x00
    ),
    attr_null_access,                       //This function should be set to attr_null_access when att_len or p_uuid_value is a null value.
};

const ble_att_param_t att_dis_hardware_revision_string =
{
    (void *)attr_uuid_dis_charc_hardware_revision_string,
    (void *)0,
    0,
    (
        //GATT_DECLARATIONS_PROPERTIES_BROADCAST |
        GATT_DECLARATIONS_PROPERTIES_READ |
        //GATT_DECLARATIONS_PROPERTIES_WRITE_WITHOUT_RESPONSE |
        //GATT_DECLARATIONS_PROPERTIES_WRITE |
        //GATT_DECLARATIONS_PROPERTIES_NOTIFY |
        //GATT_DECLARATIONS_PROPERTIES_INDICATE |
        //GATT_DECLARATIONS_PROPERTIES_AUTHENTICATED_SIGNED_WRITES |
        //GATT_DECLARATIONS_PROPERTIES_EXTENDED_PROPERTIES |
        0x00
    ),
    (
        ATT_TYPE_FORMAT_16UUID |            //otherwise, 128bit UUID
        //ATT_VALUE_BOND_ENABLE |
        //ATT_PERMISSION_ENC_READ |
        //ATT_PERMISSION_ENC_WRITE |
        //ATT_PERMISSION_AUTHE_READ |
        //ATT_PERMISSION_AUTHE_WRITE |
        //ATT_PERMISSION_AUTHO_READ |
        //ATT_PERMISSION_AUTHO_WRITE |
        0x00
    ),
    ble_svcs_dis_handler,       //registered callback function
};

const ble_att_param_t att_dis_characteristic_software_revision_string =
{
    (void *)attr_uuid_type_characteristic,
    (void *)attr_uuid_dis_charc_software_revision_string,
    sizeof(attr_uuid_dis_charc_software_revision_string),
    (
        //GATT_DECLARATIONS_PROPERTIES_BROADCAST |
        GATT_DECLARATIONS_PROPERTIES_READ |
        //GATT_DECLARATIONS_PROPERTIES_WRITE_WITHOUT_RESPONSE |
        //GATT_DECLARATIONS_PROPERTIES_WRITE |
        //GATT_DECLARATIONS_PROPERTIES_NOTIFY |
        //GATT_DECLARATIONS_PROPERTIES_INDICATE |
        //GATT_DECLARATIONS_PROPERTIES_AUTHENTICATED_SIGNED_WRITES |
        //GATT_DECLARATIONS_PROPERTIES_EXTENDED_PROPERTIES |
        0x00
    ),
    (
        ATT_TYPE_FORMAT_16UUID |            //otherwise, 128bit UUID
        //ATT_VALUE_BOND_ENABLE |
        //ATT_PERMISSION_ENC_READ |
        //ATT_PERMISSION_ENC_WRITE |
        //ATT_PERMISSION_AUTHE_READ |
        //ATT_PERMISSION_AUTHE_WRITE |
        //ATT_PERMISSION_AUTHO_READ |
        //ATT_PERMISSION_AUTHO_WRITE |
        0x00
    ),
    attr_null_access,                       //This function should be set to attr_null_access when att_len or p_uuid_value is a null value.
};

const ble_att_param_t att_dis_software_revision_string =
{
    (void *)attr_uuid_dis_charc_software_revision_string,
    (void *)0,
    0,
    (
        //GATT_DECLARATIONS_PROPERTIES_BROADCAST |
        GATT_DECLARATIONS_PROPERTIES_READ |
        //GATT_DECLARATIONS_PROPERTIES_WRITE_WITHOUT_RESPONSE |
        //GATT_DECLARATIONS_PROPERTIES_WRITE |
        //GATT_DECLARATIONS_PROPERTIES_NOTIFY |
        //GATT_DECLARATIONS_PROPERTIES_INDICATE |
        //GATT_DECLARATIONS_PROPERTIES_AUTHENTICATED_SIGNED_WRITES |
        //GATT_DECLARATIONS_PROPERTIES_EXTENDED_PROPERTIES |
        0x00
    ),
    (
        ATT_TYPE_FORMAT_16UUID |            //otherwise, 128bit UUID
        //ATT_VALUE_BOND_ENABLE |
        //ATT_PERMISSION_ENC_READ |
        //ATT_PERMISSION_ENC_WRITE |
        //ATT_PERMISSION_AUTHE_READ |
        //ATT_PERMISSION_AUTHE_WRITE |
        //ATT_PERMISSION_AUTHO_READ |
        //ATT_PERMISSION_AUTHO_WRITE |
        0x00
    ),
    ble_svcs_dis_handler,       //registered callback function
};

const ble_att_param_t att_dis_characteristic_pnp_id =
{
    (void *)attr_uuid_type_characteristic,
    (void *)attr_uuid_dis_charc_pnp_id,
    sizeof(attr_uuid_dis_charc_pnp_id),
    (
        //GATT_DECLARATIONS_PROPERTIES_BROADCAST |
        GATT_DECLARATIONS_PROPERTIES_READ |
        //GATT_DECLARATIONS_PROPERTIES_WRITE_WITHOUT_RESPONSE |
        //GATT_DECLARATIONS_PROPERTIES_WRITE |
        //GATT_DECLARATIONS_PROPERTIES_NOTIFY |
        //GATT_DECLARATIONS_PROPERTIES_INDICATE |
        //GATT_DECLARATIONS_PROPERTIES_AUTHENTICATED_SIGNED_WRITES |
        //GATT_DECLARATIONS_PROPERTIES_EXTENDED_PROPERTIES |
        0x00
    ),
    (
        ATT_TYPE_FORMAT_16UUID |            //otherwise, 128bit UUID
        //ATT_VALUE_BOND_ENABLE |
        //ATT_PERMISSION_ENC_READ |
        //ATT_PERMISSION_ENC_WRITE |
        //ATT_PERMISSION_AUTHE_READ |
        //ATT_PERMISSION_AUTHE_WRITE |
        //ATT_PERMISSION_AUTHO_READ |
        //ATT_PERMISSION_AUTHO_WRITE |
        0x00
    ),
    attr_null_access,                       //This function should be set to attr_null_access when att_len or p_uuid_value is a null value.
};

const ble_att_param_t att_dis_pnp_id =
{
    (void *)attr_uuid_dis_charc_pnp_id,
    (void *)0,
    0,
    (
        //GATT_DECLARATIONS_PROPERTIES_BROADCAST |
        GATT_DECLARATIONS_PROPERTIES_READ |
        //GATT_DECLARATIONS_PROPERTIES_WRITE_WITHOUT_RESPONSE |
        //GATT_DECLARATIONS_PROPERTIES_WRITE |
        //GATT_DECLARATIONS_PROPERTIES_NOTIFY |
        //GATT_DECLARATIONS_PROPERTIES_INDICATE |
        //GATT_DECLARATIONS_PROPERTIES_AUTHENTICATED_SIGNED_WRITES |
        //GATT_DECLARATIONS_PROPERTIES_EXTENDED_PROPERTIES |
        0x00
    ),
    (
        ATT_TYPE_FORMAT_16UUID |            //otherwise, 128bit UUID
        //ATT_VALUE_BOND_ENABLE |
        //ATT_PERMISSION_ENC_READ |
        //ATT_PERMISSION_ENC_WRITE |
        //ATT_PERMISSION_AUTHE_READ |
        //ATT_PERMISSION_AUTHE_WRITE |
        //ATT_PERMISSION_AUTHO_READ |
        //ATT_PERMISSION_AUTHO_WRITE |
        0x00
    ),
    ble_svcs_dis_handler,       //registered callback function
};


/**************************************************************************
 * BLE Service << DIS >> Local Variable
 **************************************************************************/
#ifndef MAX_NUM_CONN_DIS
// check MAX_NUM_CONN_DIS if defined or set to default 1.
#define MAX_NUM_CONN_DIS       1
#endif


// Length of system id for decoded data to send read response to client.
#define BLE_DIS_SYSTEM_ID_LEN            8

// Length of PnP id for decoded data to send read response to client.
#define BLE_DIS_PNP_ID_LEN               7


// Service basic information
ble_svcs_common_info_t          dis_basic_info[MAX_NUM_CONN_DIS];

// DIS information
ble_svcs_dis_info_t             *dis_info[MAX_NUM_CONN_DIS];

// DIS callback function
ble_svcs_evt_dis_handler_t      dis_callback[MAX_NUM_CONN_DIS];

// DIS registered total count
uint8_t                         dis_count = 0;

// DIS decoded buffer
uint8_t                 disDecodedBuffer[8];

/**************************************************************************
 * BLE Service << DIS >> Public Function
 **************************************************************************/

/** DIS Initialization
*/
ble_err_t ble_svcs_dis_init(uint8_t host_id, ble_gatt_role_t role, ble_svcs_dis_info_t *p_info, ble_svcs_evt_dis_handler_t callback)
{
    ble_err_t status;
    uint8_t config_index;

    if (p_info == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // init service client basic information and get "config_index" & "dis_count"
    status = ble_svcs_common_init(host_id, role, MAX_NUM_CONN_DIS, dis_basic_info, &config_index, &dis_count);
    if (status != BLE_ERR_OK)
    {
        return status;
    }

    // Set service role
    p_info->role = role;

    // Set DIS data
    dis_info[config_index] = p_info;

    // Register DIS callback function
    dis_callback[config_index] = callback;

    // Get handles at initialization if role is set to BLE_GATT_ROLE_SERVER
    if ((role & BLE_GATT_ROLE_SERVER) != 0)
    {
        status = ble_svcs_dis_handles_get(host_id, BLE_GATT_ROLE_SERVER, dis_info[config_index]);
        if (status != BLE_ERR_OK)
        {
            return status;
        }
    }

    return BLE_ERR_OK;
}


/** Get DIS Handle Numbers
*/
ble_err_t ble_svcs_dis_handles_get(uint8_t host_id, ble_gatt_role_t role, ble_svcs_dis_info_t *p_info)
{
    ble_err_t status;
    ble_gatt_handle_table_param_t ble_gatt_handle_table_param;

    status = BLE_ERR_OK;
    do
    {
        ble_gatt_handle_table_param.host_id = host_id;
        ble_gatt_handle_table_param.gatt_role = p_info->role;
        ble_gatt_handle_table_param.p_element = (ble_att_param_t *)&att_dis_primary_service;

        if (role == BLE_GATT_ROLE_SERVER)
        {
            ble_gatt_handle_table_param.p_handle_num_addr = (void *)&p_info->server_info.handles;
        }
        else if (role == BLE_GATT_ROLE_CLIENT)
        {
            ble_gatt_handle_table_param.p_handle_num_addr = (void *)&p_info->client_info.handles;
        }
        else
        {
            info_color(LOG_RED, "Error role setting.\n");
            status = BLE_ERR_INVALID_PARAMETER;
            break;
        }
        status = ble_svcs_handles_mapping_get(&ble_gatt_handle_table_param);
    } while (0);

    return status;
}



/**************************************************************************
 * BLE Service << DIS >> General Callback Function
 **************************************************************************/
// post the event to the callback function
static void dis_evt_post(ble_evt_att_param_t *p_param, ble_svcs_evt_dis_handler_t *p_callback)
{
    // check callback is null or not
    if (*p_callback == NULL)
    {
        return;
    }
    // post event to user
    (*p_callback)(p_param);
}


// handle DIS client GATT event
static void handle_dis_client(uint8_t index, ble_evt_att_param_t *p_param)
{
    switch (p_param->opcode)
    {
    case OPCODE_ATT_READ_RESPONSE:
        if (p_param->handle_num == dis_info[index]->client_info.handles.hdl_serial_number_string)
        {
            // received read response from server
            p_param->event = BLESERVICE_DIS_SERIAL_NUMBER_STRING_READ_RSP_EVENT;
            dis_evt_post(p_param, &dis_callback[index]);
        }
        else if (p_param->handle_num == dis_info[index]->client_info.handles.hdl_manufacturer_name_string)
        {
            // received read response from server
            p_param->event = BLESERVICE_DIS_MANUFACTURER_NAME_STRING_READ_RSP_EVENT;
            dis_evt_post(p_param, &dis_callback[index]);
        }
        else if (p_param->handle_num == dis_info[index]->client_info.handles.hdl_system_id)
        {
            // received read response from server
            p_param->event = BLESERVICE_DIS_SYSTEM_ID_READ_RSP_EVENT;
            dis_evt_post(p_param, &dis_callback[index]);
        }
        else if (p_param->handle_num == dis_info[index]->client_info.handles.hdl_firmware_revision_string)
        {
            // received read response from server
            p_param->event = BLESERVICE_DIS_FIRMWARE_REVISION_STRING_READ_RSP_EVENT;
            dis_evt_post(p_param, &dis_callback[index]);
        }
        else if (p_param->handle_num == dis_info[index]->client_info.handles.hdl_model_number_string)
        {
            // received read response from server
            p_param->event = BLESERVICE_DIS_MODEL_NUMBER_STRING_READ_RSP_EVENT;
            dis_evt_post(p_param, &dis_callback[index]);
        }
        else if (p_param->handle_num == dis_info[index]->client_info.handles.hdl_hardware_revision_string)
        {
            // received read response from server
            p_param->event = BLESERVICE_DIS_HARDWARE_REVISION_STRING_READ_RSP_EVENT;
            dis_evt_post(p_param, &dis_callback[index]);
        }
        else if (p_param->handle_num == dis_info[index]->client_info.handles.hdl_software_revision_string)
        {
            // received read response from server
            p_param->event = BLESERVICE_DIS_SOFTWARE_REVISION_STRING_READ_RSP_EVENT;
            dis_evt_post(p_param, &dis_callback[index]);
        }
        else if (p_param->handle_num == dis_info[index]->client_info.handles.hdl_pnp_id)
        {
            // received read response from server
            p_param->event = BLESERVICE_DIS_PNP_ID_READ_RSP_EVENT;
            dis_evt_post(p_param, &dis_callback[index]);
        }
        break;
    case OPCODE_ATT_WRITE_RESPONSE:
    case OPCODE_ATT_RESTORE_BOND_DATA_COMMAND:
        break;
    case OPCODE_ATT_HANDLE_VALUE_NOTIFICATION:
        break;
    case OPCODE_ATT_HANDLE_VALUE_INDICATION:
        break;

    default:
        break;
    }
}


// handle DIS server GATT event
static void handle_dis_server(uint8_t index, ble_evt_att_param_t *p_param)
{
    switch (p_param->opcode)
    {
    case OPCODE_ATT_READ_REQUEST:
    case OPCODE_ATT_READ_BY_TYPE_REQUEST:
        if (p_param->handle_num == dis_info[index]->server_info.handles.hdl_serial_number_string)
        {
            ble_svcs_auto_handle_read_req(p_param, dis_data.serial_number_str->p_data, dis_data.serial_number_str->length);
        }
        else if (p_param->handle_num == dis_info[index]->server_info.handles.hdl_manufacturer_name_string)
        {
            ble_svcs_auto_handle_read_req(p_param, dis_data.manufacturer_name_str->p_data, dis_data.manufacturer_name_str->length);
        }
        else if (p_param->handle_num == dis_info[index]->server_info.handles.hdl_system_id)
        {
            disDecodedBuffer[0] = (dis_data.system_id->manufacturer_id & 0x00000000FF);
            disDecodedBuffer[1] = (dis_data.system_id->manufacturer_id & 0x000000FF00) >> 8;
            disDecodedBuffer[2] = (dis_data.system_id->manufacturer_id & 0x0000FF0000) >> 16;
            disDecodedBuffer[3] = (dis_data.system_id->manufacturer_id & 0x00FF000000) >> 24;
            disDecodedBuffer[4] = (dis_data.system_id->manufacturer_id & 0xFF00000000) >> 32;

            disDecodedBuffer[5] = (dis_data.system_id->organizationally_unique_id & 0x0000FF);
            disDecodedBuffer[6] = (dis_data.system_id->organizationally_unique_id & 0x00FF00) >> 8;
            disDecodedBuffer[7] = (dis_data.system_id->organizationally_unique_id & 0xFF0000) >> 16;

            ble_svcs_auto_handle_read_req(p_param, disDecodedBuffer, BLE_DIS_SYSTEM_ID_LEN);
        }
        else if (p_param->handle_num == dis_info[index]->server_info.handles.hdl_firmware_revision_string)
        {
            ble_svcs_auto_handle_read_req(p_param, dis_data.firmware_rev_str->p_data, dis_data.firmware_rev_str->length);
        }
        else if (p_param->handle_num == dis_info[index]->server_info.handles.hdl_model_number_string)
        {
            ble_svcs_auto_handle_read_req(p_param, dis_data.model_number_str->p_data, dis_data.model_number_str->length);
        }
        else if (p_param->handle_num == dis_info[index]->server_info.handles.hdl_hardware_revision_string)
        {
            ble_svcs_auto_handle_read_req(p_param, dis_data.hardware_rev_str->p_data, dis_data.hardware_rev_str->length);
        }
        else if (p_param->handle_num == dis_info[index]->server_info.handles.hdl_software_revision_string)
        {
            ble_svcs_auto_handle_read_req(p_param, dis_data.software_rev_str->p_data, dis_data.software_rev_str->length);
        }
        else if (p_param->handle_num == dis_info[index]->server_info.handles.hdl_pnp_id)
        {
            disDecodedBuffer[0] = dis_data.pnp_id->vendor_id_source;

            disDecodedBuffer[1] = (uint8_t) ((dis_data.pnp_id->vendor_id & 0x00FF) >> 0);
            disDecodedBuffer[2] = (uint8_t) ((dis_data.pnp_id->vendor_id & 0xFF00) >> 8);

            disDecodedBuffer[3] = (uint8_t) ((dis_data.pnp_id->product_id & 0x00FF) >> 0);
            disDecodedBuffer[4] = (uint8_t) ((dis_data.pnp_id->product_id & 0xFF00) >> 8);

            disDecodedBuffer[5] = (uint8_t) ((dis_data.pnp_id->product_version & 0x00FF) >> 0);
            disDecodedBuffer[6] = (uint8_t) ((dis_data.pnp_id->product_version & 0xFF00) >> 8);

            ble_svcs_auto_handle_read_req(p_param, disDecodedBuffer, BLE_DIS_PNP_ID_LEN);
        }

        break;

    default:
        break;
    }
}



/** ble_svcs_dis_handler
 * @note This callback receives the DIS events.
 * Each of these events can be associated with parameters.
*/

void ble_svcs_dis_handler(ble_evt_att_param_t *p_param)
{
    uint8_t index;

    if (ble_svcs_common_info_index_query(p_param->host_id, p_param->gatt_role, MAX_NUM_CONN_DIS, dis_basic_info, &index) != BLE_ERR_OK)
    {
        // Host id has not registered so there is no callback function -> do nothing
        return;
    }

    if (p_param->gatt_role == BLE_GATT_ROLE_CLIENT)
    {
        // handle DIS client GATT event
        handle_dis_client(index, p_param);
    }

    if (p_param->gatt_role == BLE_GATT_ROLE_SERVER)
    {
        // handle DIS server GATT event
        handle_dis_server(index, p_param);
    }
}


