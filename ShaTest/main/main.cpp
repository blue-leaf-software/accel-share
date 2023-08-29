
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
#include <vector>

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
      if (std::numeric_limits<T>::radix != 2) stream << value;
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
  inline std::ostream& operator <<(std::ostream& stream, const Hex<T>& value) {
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

class CDigestResult
{
public:
  uint8_t Digest[WC_SHA_DIGEST_SIZE];
  
  void Dump()
  {
    DumpDigest(Digest, sizeof(Digest));
  }
  
  bool Equals(const CDigestResult& rTest)
  {
    return 0 == memcmp(Digest, rTest.Digest, sizeof(Digest));
  }
};



void CompareSoftwareHardwareHash()
{
  string_view svData("To be or not to be, that is the question. Whether it is better to suffer the slings and arrows of outrageous fortune or, by opposing, end them");

  cout << "Software:\n";
  wc_Sha Sha1_Software;
  uint8_t abySoftwareHash[WC_SHA_DIGEST_SIZE];

  wc_InitSha(&Sha1_Software);
  wc_ShaUpdate(&Sha1_Software, reinterpret_cast<const unsigned char*>(svData.data()), svData.length());
  wc_ShaUpdate(&Sha1_Software, reinterpret_cast<const unsigned char*>(svData.data()), svData.length());
  wc_ShaFinal(&Sha1_Software, abySoftwareHash);
  wc_ShaFree(&Sha1_Software);


  esp_sha_enable_hw_accelerator();

  cout << "\n\nHardware:\n";
  wc_Sha Sha1_Hardware;
  uint8_t abyHardwareHash[WC_SHA_DIGEST_SIZE];

  wc_InitSha(&Sha1_Hardware);
  wc_ShaUpdate(&Sha1_Hardware, reinterpret_cast<const unsigned char*>(svData.data()), svData.length());
  wc_ShaUpdate(&Sha1_Hardware, reinterpret_cast<const unsigned char*>(svData.data()), svData.length());
  wc_ShaFinal(&Sha1_Hardware, abyHardwareHash);
  wc_ShaFree(&Sha1_Hardware);

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

  esp_sha_disable_hw_accelerator();
}

void InterleavedHashes(vector<string_view>& vData, vector<wc_Sha>& ShaContexts, vector<CDigestResult>& rResult)
{
  for (int iData = 0; iData < vData.size(); ++iData)
  {
    cout << "Data " << iData << " has " << vData[iData].length() << " bytes\n";
    ShaContexts.push_back(wc_Sha());
  }

  for (wc_Sha& sha : ShaContexts)
  {
    wc_InitSha(&sha);
  }
  
  for (int iData = 0; iData < vData.size(); ++iData)
  {
    for (int iSha = 0; iSha < ShaContexts.size(); ++iSha)
    {
      int iRecord = (iData + iSha) % vData.size();
      string_view svData = vData[iRecord];
      wc_Sha& rSha = ShaContexts[iSha];
      cout << "Update sha " << iSha << " with data " << iRecord << endl;
      wc_ShaUpdate(&rSha, reinterpret_cast<const unsigned char*>(svData.data()), svData.length());
    }
  }
  
  for (int iSha = 0; iSha < ShaContexts.size(); ++iSha)
  {
    cout << "Finalizing sha " << iSha << endl;
    CDigestResult Digest;
    wc_ShaFinal(&ShaContexts[iSha], Digest.Digest);
    rResult.push_back(Digest);
  }
}

void SequentialHashes(vector<string_view>& vData, vector<wc_Sha>& ShaContexts, vector<CDigestResult>& rResult)
{
  for (int iData = 0; iData < vData.size(); ++iData)
  {
    ShaContexts.push_back(wc_Sha());
  }

  for (int iSha = 0; iSha < ShaContexts.size(); ++iSha)
  {
    wc_InitSha(&ShaContexts[iSha]);
    for (int iData = 0; iData < vData.size(); ++iData)
    {
      int iRecord = (iData + iSha) % vData.size();
      string_view svData = vData[iRecord];
      wc_Sha& rSha = ShaContexts[iSha];
      cout << "Update sha " << iSha << " with data " << iRecord << endl;
      wc_ShaUpdate(&rSha, reinterpret_cast<const unsigned char*>(svData.data()), svData.length());
    }
    
    CDigestResult Digest;
    wc_ShaFinal(&ShaContexts[iSha], Digest.Digest);
    rResult.push_back(Digest);
  }
}

void CompareInterleavedHashes()
{
  vector<string_view> vData = 
  { "It was the best of times, it was the worst of times, it was the age of wisdom, it was the age of foolishness.",
    "Twas brillig and the slithy toves did gyre and gimble in the wabe; all mimsy were the borogroves, and the mome raths outgrabe.",
    "Beware the Jabberwock by son! The jaws the bite, the claws that catch! Beware the Jubjub bird, and sun the frumious Bandersnatch"
  };

  vector<wc_Sha> Sha_Software;
  vector<wc_Sha> Sha_Hardware;
  
  vector<CDigestResult> Result_Software;
  vector<CDigestResult> Result_Hardware;

  bool bInterleave = true; 

  cout << "**** Software calculation of interleaved hashs\n";
  if (bInterleave)
  {
    InterleavedHashes(vData, Sha_Software, Result_Software);
  }
  else
  {
    SequentialHashes(vData, Sha_Software, Result_Software);
  }

  esp_sha_enable_hw_accelerator();

  cout << "**** Hardware calculation of interleaved hashs\n";
  if (bInterleave)
  {
    InterleavedHashes(vData, Sha_Hardware, Result_Hardware);
  }
  else
  {
    SequentialHashes(vData, Sha_Hardware, Result_Hardware);
  }
  
  esp_sha_disable_hw_accelerator();
  
  assert(Result_Software.size() == Result_Hardware.size());

  for (int iResult = 0; iResult < Result_Software.size(); ++iResult)
  {
    cout << "Software result: ";
    Result_Software[iResult].Dump();

    cout << "Hardware result: ";
    Result_Hardware[iResult].Dump();

    if (Result_Software[iResult].Equals(Result_Hardware[iResult]))
    {
      cout << "Equal!\n";
    }
    else
    {
      cout << "Different :-(\n";
    }

  }
}


extern "C" void app_main(void)
{
  cout << "Sha interleaving test\n";
  
  if (WOLFSSL_SUCCESS != wolfSSL_Init())
  {
    cout << "Wolf initialization failed\n";
    abort();
  }

  CompareSoftwareHardwareHash();
  CompareInterleavedHashes();
  cout << "Done.\n";
  while (true)
  {
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
