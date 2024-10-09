import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_NAME
from esphome.components import esp32_ble_tracker

DEPENDENCIES = ['esp32_ble_tracker']
AUTO_LOAD = ['sensor']

goodnature_ble_ns = cg.esphome_ns.namespace('goodnature_ble')
GoodnatureBleListener = goodnature_ble_ns.class_('GoodnatureBleListener', esp32_ble_tracker.ESPBTDeviceListener, cg.Component)

CONF_DEVICES = 'devices'
CONF_SERIAL = 'serial'

DEVICE_SCHEMA = cv.Schema({
    cv.Required(CONF_NAME): cv.string,
    cv.Required(CONF_SERIAL): cv.string,
})

CONFIG_SCHEMA = esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA({
    cv.GenerateID(): cv.declare_id(GoodnatureBleListener),
    cv.Optional(CONF_DEVICES): cv.ensure_list(DEVICE_SCHEMA),
}).extend(esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await esp32_ble_tracker.register_ble_device(var, config)

    if CONF_DEVICES in config:
        for device in config[CONF_DEVICES]:
            cg.add(var.add_device(device[CONF_NAME], device[CONF_SERIAL]))
