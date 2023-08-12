#pragma once

#include <stdint.h>
#include <chrono>
#include <atomic>

/// <summary>
/// Fake a calculation hardware accelerator. This one
/// just accumulates input values. 
/// </summary>
class CAccelerator
{
public:
  static constexpr size_t CalculationSize = 5;

private:

  uint8_t m_auInput[CalculationSize];

  uint8_t m_auOutput[CalculationSize];

  std::atomic<std::chrono::steady_clock::time_point> m_CalculationEnds;

public:

  CAccelerator();

  /// <summary>
  /// Returns true iff there is a calculation in progress. 
  /// </summary>
  bool IsCalculating() const;

  void BeginCalculation(const uint8_t* pData, bool bFirst);

  void SetInput(const uint8_t* pResult);

  void GetOutput(uint8_t* pResult);

};


extern CAccelerator AccelerationEngine;
