/* mbed Microcontroller Library
 * Copyright (c) 2017 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "utest/utest.h"
#include "unity/unity.h"
#include "greentea-client/test_env.h"

#include "mbed.h"
#if TARGET_MTS_MDOT_F411RE
#include "SX1272_LoRaRadio.h"
#endif

#if defined(TARGET_K64F) || defined(TARGET_DISCO_L072CZ_LRWAN1)
#include "SX1276_LoRaRadio.h"
#endif
#include "LoRaWANInterface.h"

using namespace utest::v1;

void lora_event_handler(lora_events_t events);

#if TARGET_MTS_MDOT_F411RE
    SX1272_LoRaRadio Radio(LORA_MOSI, LORA_MISO, LORA_SCK, LORA_NSS, LORA_RESET,
                           LORA_DIO0, LORA_DIO1, LORA_DIO2, LORA_DIO3, LORA_DIO4,
                           LORA_DIO5, NC, NC, LORA_TXCTL, LORA_RXCTL, NC, NC);
#endif
#if TARGET_K64F
    SX1276_LoRaRadio Radio(D11, D12, D13, D10, A0,
                           D2, D3, D4, D5, D8,
                           D9, NC, NC, NC, NC, A4, NC, NC);
#endif

#if defined(TARGET_DISCO_L072CZ_LRWAN1)
    #define LORA_SPI_MOSI   PA_7
    #define LORA_SPI_MISO   PA_6
    #define LORA_SPI_SCLK   PB_3
    #define LORA_CS         PA_15
    #define LORA_RESET      PC_0
    #define LORA_DIO0       PB_4
    #define LORA_DIO1       PB_1
    #define LORA_DIO2       PB_0
    #define LORA_DIO3       PC_13
    #define LORA_DIO4       PA_5
    #define LORA_DIO5       PA_4
    #define LORA_ANT_RX     PA_1
    #define LORA_ANT_TX     PC_2
    #define LORA_ANT_BOOST  PC_1
    #define LORA_TCXO       PA_12   // 32 MHz

    SX1276_LoRaRadio Radio(LORA_SPI_MOSI, LORA_SPI_MISO, LORA_SPI_SCLK, LORA_CS, LORA_RESET,
                           LORA_DIO0, LORA_DIO1, LORA_DIO2, LORA_DIO3, LORA_DIO4, NC,
                           NC, NC, LORA_ANT_TX, LORA_ANT_RX,
                           NC, LORA_ANT_BOOST, LORA_TCXO);
#endif

class LoRaTestHelper
{
public:
    LoRaTestHelper() :cur_event(0), event_lock(false) {
        // Fill event buffer
        memset(event_buffer, 0xFF, sizeof(event_buffer));
    };
    ~LoRaTestHelper() {};

    bool find_event(uint8_t event_code);

    uint8_t event_buffer[10];
    uint8_t cur_event;
    bool event_lock;
};

bool LoRaTestHelper::find_event(uint8_t event_code)
{
    event_lock = true;

    for (uint8_t i = 0; i < 10; i++) {
        if (event_buffer[i] == event_code) {
            event_buffer[i] = 0xFF;
            event_lock = false;
            return true;
        }
    }

    event_lock = false;
    return false;
}

LoRaWANInterface lorawan(Radio);
LoRaTestHelper lora_helper;

void connection_without_params_otaa_with_json_incorrect()
{
    lora_mac_status_t ret;
    uint8_t counter = 0;

    ret = lorawan.initialize();
    if (ret != LORA_MAC_STATUS_OK) {
        TEST_ASSERT_MESSAGE(false, "Initialization failed");
        return;
    }

    ret = lorawan.connect();
    if (ret != LORA_MAC_STATUS_OK && ret != LORA_MAC_STATUS_CONNECT_IN_PROGRESS) {
        TEST_ASSERT_MESSAGE(false, "Connect failed");
        return;
    }

    while (1) {
        if (lora_helper.find_event(CONNECTED)) {
            TEST_ASSERT_MESSAGE(false, "Connection! There should not be a connection..");
            break;
        } else if (counter == 30) {
            // Test passed.
            break;
        }
        wait_ms(1000);
        counter += 1;
    }
}


utest::v1::status_t greentea_failure_handler(const Case *const source, const failure_t reason) {
    greentea_case_failure_abort_handler(source, reason);
    return STATUS_CONTINUE;
}

Case cases[] = {
    Case("Connect without parameters, and .json file data is incorrect.", connection_without_params_otaa_with_json_incorrect, greentea_failure_handler),
};

utest::v1::status_t greentea_test_setup(const size_t number_of_cases) {
    GREENTEA_SETUP(90, "default_auto");
    return greentea_test_setup_handler(number_of_cases);
}


int main() {
    lorawan.lora_event_callback(lora_event_handler);

    Specification specification(greentea_test_setup, cases, greentea_test_teardown_handler);
    Harness::run(specification);
}

void lora_event_handler(lora_events_t events)
{
    if (lora_helper.event_lock) {
        return;
    }

    lora_helper.event_buffer[lora_helper.cur_event % 10] = events;
    lora_helper.cur_event++;
}
