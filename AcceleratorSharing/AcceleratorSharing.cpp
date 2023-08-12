// AcceleratorSharing.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <cassert>

#include "ShaAcceleratorDriver.h"

using namespace std;

/// <summary>
/// Mock of hardware acceleration engine. 
/// </summary>
CAccelerator AccelerationEngine;

/// <summary>
/// Driver to serialize access to acceleration engine. 
/// </summary>
CShaAcceleratorDriver AcceleratorMediator(true);

/// <summary>
/// Mocks a similar API to Wolf's wc_Sha* functions. 
/// </summary>
#pragma region

void Init(CShaContext& rContext)
{
  rContext.Accelerator.Init();

  uint8_t uCanaryValue = 80;
  memset(rContext.ShaResult, uCanaryValue, rContext.BufferSize);
}

void Update(CShaContext& rContext, const uint8_t *pData, size_t szData)
{
  assert(szData <= rContext.BufferSize); // for simplicity. 

  uint8_t auDataToHash[rContext.BufferSize];
  memset(auDataToHash, 0, sizeof(auDataToHash));
  memcpy(auDataToHash, pData, szData);

  auto Result = AcceleratorMediator.BeginCalculation(rContext, auDataToHash, 1min);

  // todo: implement a software fallback. 
  assert(ShaAcceleratorResult::Started == Result);
}

void Update(CShaContext& rContext, const uint8_t uData)
{
  Update(rContext, &uData, sizeof(uData));
}

void Finalize(CShaContext& rContext, uint8_t *pResult)
{
  bool bDone = AcceleratorMediator.CompleteCalculation(rContext, 1min);
  // todo: deal with other results
  assert(bDone);

  memcpy(pResult, rContext.ShaResult, rContext.BufferSize);
}

uint8_t Finalize(CShaContext& rContext)
{
  uint8_t auHashResult[rContext.BufferSize];
  Finalize(rContext, auHashResult);
  return auHashResult[0];
}

void Free(CShaContext& rContext)
{
  AcceleratorMediator.AbandonCalculation(rContext);
}
#pragma endregion

/// <summary>
/// Few test cases. 
/// </summary>
int main()
{
  cout << "Acceleration engine mediation tester\n";
  cout << "====================================\n";

  // A calculation
  CShaContext c1;
  Init(c1);
  Update(c1, 7);
  Update(c1, 11);
  assert(7 + 11 == Finalize(c1));

  // Another calculation
  CShaContext c2;
  Init(c2);
  Update(c2, 3);
  Update(c2, 4);
  assert(3 + 4 == Finalize(c2));

  // Overlapping calculations
  Init(c1);
  Init(c2);

  Update(c1, 123);
  Update(c2, 6);

  Update(c1, 45);
  Update(c2, 11);

  uint8_t uResult1 = Finalize(c1);
  uint8_t uResult2 = Finalize(c2);
  assert(123 + 45 == uResult1);
  assert(6 + 11 == uResult2);

  cout << "Done.\n";
}
