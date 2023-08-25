
#include <stdio.h>
#include <string_view>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "user_settings.h"
#include "ssl.h"
#include "sha.h"

#include <iostream>
#include <iomanip>
#include <limits>

using namespace std;

// based on: https://stackoverflow.com/questions/21416425/printing-zero-padded-hex-with-stdcout
template <typename T>
struct Hex
{
    // C++11:
    // static constexpr int Width = (std::numeric_limits<T>::digits + 1) / 4;
    // Otherwise:
    enum { Width = (std::numeric_limits<T>::digits + 1) / 4 };
    const T& value;
    const int width;

    Hex(const T& value, int width = Width)
    : value(value), width(width)
    {}

    void write(std::ostream& stream) const {
        if(std::numeric_limits<T>::radix != 2) stream << value;
        else {
            std::ios_base::fmtflags flags = stream.setf(
                std::ios_base::hex, std::ios_base::basefield);
            char fill = stream.fill('0');
            stream << std::setw(width) << value;
            stream.fill(fill);
            stream.setf(flags, std::ios_base::basefield);
        }
    }
};

template <typename T>
inline Hex<T> hex(const T& value, int width = Hex<T>::Width) {
    return Hex<T>(value, width);
}

template <typename T>
inline std::ostream& operator << (std::ostream& stream, const Hex<T>& value) {
    value.write(stream);
    return stream;
}

void DumpDigest(const uint8_t *pData, size_t szData)
{
  while (szData--)
  {
    cout << hex((int)*pData++, 2) << '-';
  }
  cout << endl;
}

extern "C" void app_main(void)
{
  cout << "Sha interleaving test\n";
  
  if (WOLFSSL_SUCCESS != wolfSSL_Init())
  {
    cout << "Wolf initialization failed\n";
    abort();
  }

  string_view svData("To be or not to be, that is the question. Whether it is better to suffer the slings and arrows of outrageous fortune or, by opposing, end them");
  
  cout << "Software:\n";
  wc_Sha Sha1_Software;
  uint8_t abySoftwareHash[WC_SHA_DIGEST_SIZE];

  wc_InitSha(&Sha1_Software);
  wc_ShaUpdate(&Sha1_Software, reinterpret_cast<const unsigned char*>(svData.data()), svData.length());
  wc_ShaUpdate(&Sha1_Software, reinterpret_cast<const unsigned char*>(svData.data()), svData.length());
  wc_ShaFinal(&Sha1_Software, abySoftwareHash);
  

  bool bEnableHardwareAcceleration = true; 
  if (bEnableHardwareAcceleration)
  {
    esp_sha_enable_hw_accelerator();
  }

  cout << "\n\nHardware:\n";
  wc_Sha Sha1_Hardware;
  uint8_t abyHardwareHash[WC_SHA_DIGEST_SIZE];
  
  wc_InitSha(&Sha1_Hardware);
  wc_ShaUpdate(&Sha1_Hardware, reinterpret_cast<const unsigned char*>(svData.data()), svData.length());
  wc_ShaUpdate(&Sha1_Hardware, reinterpret_cast<const unsigned char*>(svData.data()), svData.length());
  wc_ShaFinal(&Sha1_Hardware, abyHardwareHash);
  
  cout << "Software result: ";
  DumpDigest(abySoftwareHash, sizeof(abySoftwareHash));

  cout << "Hardware result: ";
  DumpDigest(abyHardwareHash, sizeof(abyHardwareHash));
  
  if (0 == memcmp(abySoftwareHash, abyHardwareHash, sizeof(abyHardwareHash)))
  {
    cout << "Equal!\n";
  }
  else
  {
    cout << "Different :-(\n";
  }
  
  if (bEnableHardwareAcceleration)
  {
    esp_sha_disable_hw_accelerator();
  }

  cout << "Done.\n";
  while (true)
  {
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
