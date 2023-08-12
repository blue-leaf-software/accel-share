#include "CalculationContext.h"

CShaAcceleratorContext::CShaAcceleratorContext()
{
  Init();
}

void CShaAcceleratorContext::Init()
{
  m_State = ECalculationState::Initialize;
  m_uCalculationToken = 0;
}
