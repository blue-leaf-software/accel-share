
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"

#include <iostream>

using namespace std;

extern "C" void app_main(void)
{
  cout << "Sha interleaving test\n";


  cout << "Done.\n";
  while (true)
  {
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
