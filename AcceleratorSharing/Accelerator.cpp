#include "Accelerator.h"
#pragma once

#include <cassert>

using namespace std;
using namespace chrono;

CAccelerator::CAccelerator()
{
  m_CalculationEnds = steady_clock::time_point::min();
}

bool CAccelerator::IsCalculating() const
{
  return steady_clock::now() < m_CalculationEnds.load();
}

void CAccelerator::BeginCalculation(const uint8_t* pData, bool bFirst)
{
  // work in progress! shouldn't be starting a new calculation!
  assert(!IsCalculating());
  
  m_CalculationEnds = steady_clock::now() + 100ms;

  if (bFirst)
  {
    memset(m_auInput, 0, sizeof(m_auInput));
  }

  uint8_t* pSource = m_auInput;
  uint8_t* pDestination = m_auOutput;
  for (int iByte = 0; iByte < CalculationSize; ++iByte)
  {
    *pDestination++ = *pSource++ + *pData++;
  }

  memcpy(m_auInput, m_auOutput, CalculationSize);
}

void CAccelerator::SetInput(const uint8_t* pResult)
{
  // work in progress! shouldn't be setting values at this point.
  assert(!IsCalculating()); 

  memcpy(m_auInput, pResult, CalculationSize);
}

void CAccelerator::GetOutput(uint8_t* pResult)
{
  // work in progress! shouldn't be retrieving values at this point.
  assert(!IsCalculating());


  memcpy(pResult, m_auOutput, CalculationSize);
}
