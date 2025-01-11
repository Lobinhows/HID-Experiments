#include <stdio.h>
#include <stdlib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"

#include "tinyusb.h"
#include "class/hid/hid_device.h"
#include "driver/gpio.h"



/// --------- USB and HID Reports ----------

const uint8_t hid_report_descriptor[] = {
    0x05, 0x01,       // USAGE_PAGE (Generic Desktop)
    0x09, 0x04,       // USAGE (Joystick)
    0xA1, 0x01,       // COLLECTION (Application)
    0x09, 0x01,       //   USAGE (Pointer)

    // Eixo do freio de mão
    0xA1, 0x00,       //   COLLECTION (Physical)
    0x09, 0x30,       //     USAGE (X)
    0x15, 0x00,       //     LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x0F, //     LOGICAL_MAXIMUM (255)
    0x75, 0x10,       //     REPORT_SIZE (16 bits)
    0x95, 0x01,       //     REPORT_COUNT (1)s
    0x81, 0x02,       //     INPUT (Data,Var,Abs)
    0xC0,             //   END_COLLECTION

    0xC0              // END_COLLECTION
};

const char* string_descriptor[5] = {
    (const char[]){0x09,0x04}, // Idioma: Inglês Americano (0x0409)
    "Teste Lobo",            // Fabricante
    "Handbrake HID",      // Produto
    "00000001"                 // Número de Série
};

static const uint8_t config_descriptor[] = {
    0x09,           // bLength: Tamanho do descritor (9 bytes)
    0x02,           // bDescriptorType: Configuration
    0x22, 0x00,     // wTotalLength: Tamanho total (34 bytes neste caso)
    0x01,           // bNumInterfaces: Número de interfaces (1 interface HID)
    0x01,           // bConfigurationValue: ID da configuração
    0x00,           // iConfiguration: String index (sem string associada)
    0x80,           // bmAttributes: Alimentação própria, sem suporte a wake-up
    0x32,           // bMaxPower: 100mA (0x32 = 50 * 2mA)

    // Interface Descriptor
    0x09,           // bLength: Tamanho do descritor de interface (9 bytes)
    0x04,           // bDescriptorType: Interface
    0x00,           // bInterfaceNumber: Índice da interface (0)
    0x00,           // bAlternateSetting: Sem configurações alternativas
    0x01,           // bNumEndpoints: 1 endpoint (além do endpoint 0 padrão)
    0x03,           // bInterfaceClass: HID (0x03)
    0x00,           // bInterfaceSubClass: Sem subclasse específica
    0x00,           // bInterfaceProtocol: Sem protocolo específico
    0x00,           // iInterface: String index (sem string associada)

    // HID Descriptor
    0x09,           // bLength: Tamanho do descritor HID (9 bytes)
    0x21,           // bDescriptorType: HID
    0x11, 0x01,     // bcdHID: Versão HID 1.11
    0x00,           // bCountryCode: Sem especificação de país
    0x01,           // bNumDescriptors: Número de descritores (1 - Report)
    0x22,           // bDescriptorType: Report Descriptor
    0x19, 0x00,     // wDescriptorLength: Tamanho do Report Descriptor (27 bytes)

    // Endpoint Descriptor (Interrupt IN)
    0x07,           // bLength: Tamanho do descritor de endpoint (7 bytes)
    0x05,           // bDescriptorType: Endpoint
    0x81,           // bEndpointAddress: Endpoint 1, IN (0x80 | 1)
    0x03,           // bmAttributes: Transferência Interrupt
    0x10, 0x00,     // wMaxPacketSize: Tamanho máximo do pacote (16 bytes)
    0x0A            // bInterval: Intervalo de polling (10ms)
};

/// --------- END - USB and HID Reports ---------   

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    // We use only one interface and one HID report descriptor, so we can ignore parameter 'instance'
    return hid_report_descriptor;
};

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
};

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
    (void) instance;
    (void) report_id;
    (void) report_type;
    uint16_t meme = sizeof(buffer);
    (void) buffer;

    (void) reqlen;

    return meme;
};




void app_main(void)
{

    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_cfg ={
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE
    };
    adc_oneshot_new_unit(&init_cfg,&adc_handle);

    adc_oneshot_chan_cfg_t chan_cfg ={
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12
    };

    adc_oneshot_config_channel(adc_handle,ADC_CHANNEL_4,&chan_cfg);


    const tinyusb_config_t tusb_config = {
        .device_descriptor = NULL,
        .string_descriptor = string_descriptor,
        .string_descriptor_count = sizeof(string_descriptor)/sizeof(string_descriptor[0]),
        .external_phy = false,
        .configuration_descriptor = config_descriptor
    };

    tinyusb_driver_install(&tusb_config);


    while(1){
        int data;
        adc_oneshot_read(adc_handle,4,&data);
        printf("data = %d\n",data);
        uint8_t send_data[2]={
            data & 0xFF, ((data>>8) & 0xFF)
        };

        if(tud_mounted()){
            tud_hid_report(HID_ITF_PROTOCOL_NONE,send_data,sizeof(send_data));
        };
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}