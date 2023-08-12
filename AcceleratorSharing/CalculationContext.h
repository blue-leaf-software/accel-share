#pragma once

#include <stdint.h>

#include "CalculationState.h"
#include "Accelerator.h"

class CShaAcceleratorContext
{
  ECalculationState m_State;

  uint32_t m_uCalculationToken;

  friend class CShaAcceleratorDriver;

public:
  CShaAcceleratorContext();

  void Init();
};

struct CShaContext
{
  static constexpr const size_t BufferSize = CAccelerator::CalculationSize;

  uint8_t ShaResult[BufferSize];

  CShaAcceleratorContext Accelerator;
};