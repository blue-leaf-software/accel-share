#include "ShaAcceleratorDriver.h"
#include <cassert>

using namespace std;
using namespace chrono;

CShaAcceleratorDriver::CShaAcceleratorDriver(bool bSupportsInterleaving)
  : m_bSupportsInterleaving(bSupportsInterleaving)
{
  m_uCalculationToken = 0;
  m_pCurrentContext = nullptr;
  m_LastCalculationStarted = std::chrono::steady_clock::time_point::min();
}

ShaAcceleratorResult CShaAcceleratorDriver::BeginCalculation(CShaContext& rContext, uint8_t* pDataToHash, milliseconds Timeout)
{
  const unique_lock<mutex> Lock(m_Lock);

  if (IsWorkingOn(rContext))
  {
    // If the engine is working on the context supplied we must 
    // wait for it to finish (can't fall-back to software, for example). 
    WaitForCalculationToFinish();
  }
  else if (!WaitForCalculationToFinish(Timeout))
  {
    return ShaAcceleratorResult::Timeout;
  }

  // hardware is free for us to use now!
  UpdateContextState();

  // assign a token to track the context on first use. 
  bool bFirst = ECalculationState::Initialize == rContext.Accelerator.m_State;
  if (bFirst)
  {
    rContext.Accelerator.m_uCalculationToken = NextToken();
  }

  // restore previous intermediate results, as appropriate. 
  if (nullptr != m_pCurrentContext && !IsWorkingOn(rContext))
  {
    if (ECalculationState::AwaitingResult == m_pCurrentContext->Accelerator.m_State)
    {
      return ShaAcceleratorResult::Busy;
    }

    assert(ECalculationState::ResultInHardware == m_pCurrentContext->Accelerator.m_State);
    StashIntermediateResult(*m_pCurrentContext);
    m_pCurrentContext = nullptr;
  }

  if (!bFirst)
  {
    // Every new calculation gets a different token so we can track
    // intermediate results. 
    rContext.Accelerator.m_uCalculationToken = NextToken();
  }

  // A previous calculation was set aside; put it back in the hardware. 
  if (ECalculationState::PartialResult == rContext.Accelerator.m_State)
  {
    RestoreIntermediateResult(rContext);
  }

  BeginCalculation(pDataToHash, bFirst);
  rContext.Accelerator.m_State = ECalculationState::AwaitingResult;
  m_LastCalculationStarted = steady_clock::now();

  // Better hope the context still exists for the entire duration we hold onto it. 
  // Could be bad if context is on the stack and goes out of scope without correct 
  // disposal! (i.e., calling CompleteCalculation or AbandonCalculation())
  m_pCurrentContext = &rContext;

  return ShaAcceleratorResult::Started;
}

bool CShaAcceleratorDriver::CompleteCalculation(CShaContext& rContext, std::chrono::milliseconds Timeout)
{
  const unique_lock<mutex> Lock(m_Lock);
  bool bComplete = false;

  switch (rContext.Accelerator.m_State)
  {
  case ECalculationState::Initialize:
    assert(false); // should start before finishing. 
    rContext.Accelerator.m_State = ECalculationState::Abandoned;
    bComplete = true;
    break;

  case ECalculationState::AwaitingResult:
    assert(IsWorkingOn(rContext));
    if (IsWorkingOn(rContext))
    {
      if (WaitForCalculationToFinish(Timeout))
      {
        StashIntermediateResult(rContext);
        rContext.Accelerator.m_State = ECalculationState::Complete;
        m_pCurrentContext = nullptr;
        bComplete = true;
      }
      else
      {
        bComplete = false;
      }
    }
    else
    {
      // should not be possible to be waiting for a result and not
      // be the current context. 
      assert(false);
    }
    break;

  case ECalculationState::PartialResult:
    rContext.Accelerator.m_State = ECalculationState::Complete;
    bComplete = true;
    break;

  case ECalculationState::Abandoned:
    bComplete = true;
    break;

  case ECalculationState::Complete:
    rContext.Accelerator.m_State = ECalculationState::Complete;
    bComplete = true;
    break;

  default:
    assert(false);
    rContext.Accelerator.m_State = ECalculationState::Abandoned;
    bComplete = true;
    break;
  }

  return bComplete;
}

void CShaAcceleratorDriver::AbandonCalculation(CShaContext& rContext)
{
  const unique_lock<mutex> Lock(m_Lock);
  if (nullptr != m_pCurrentContext && m_pCurrentContext->Accelerator.m_uCalculationToken == rContext.Accelerator.m_uCalculationToken)
  {
    m_pCurrentContext = nullptr;
  }
  rContext.Accelerator.m_State = ECalculationState::Abandoned;
}

void CShaAcceleratorDriver::StashIntermediateResult(CShaContext& rContext)
{
  GetLastResult(rContext.ShaResult);
  rContext.Accelerator.m_State = ECalculationState::PartialResult;
}

void CShaAcceleratorDriver::RestoreIntermediateResult(CShaContext& rContext)
{
  StorePreviousResult(rContext.ShaResult);
  rContext.Accelerator.m_State = ECalculationState::ResultInHardware;
}

bool CShaAcceleratorDriver::IsWorkingOn(CShaContext& rContext)
{
  // We track a token instead of the actual object because the object might
  // be copied to a different memory location. 
  return nullptr != m_pCurrentContext
    && m_pCurrentContext->Accelerator.m_uCalculationToken == rContext.Accelerator.m_uCalculationToken
    && ECalculationState::AwaitingResult == rContext.Accelerator.m_State;
}

void CShaAcceleratorDriver::WaitForCalculationToFinish()
{
  while (IsCalculating())
  {
    // busy wait. it shouldn't take long for 
    // the calculation to finish in the hardware. 
  }
}

bool CShaAcceleratorDriver::WaitForCalculationToFinish(std::chrono::milliseconds Timeout)
{
  if (IsCalculating())
  {
    auto ExpectedDone = m_LastCalculationStarted + m_TypicalCalculationTime;
    auto Now = steady_clock::now();
    if (ExpectedDone > Now + Timeout)
    {
      return false;
    }

    auto End = Now + Timeout;
    while (IsCalculating())
    {
      // busy wait? it probably won't take long for 
      // the calculation to finish. 
      if (steady_clock::now() > End)
      {
        return false;
      }
    }
  }

  return true;
}

uint32_t CShaAcceleratorDriver::NextToken()
{
  if (++m_uCalculationToken == 0)
  {
    // never 0. 
    ++m_uCalculationToken;
  }

  return m_uCalculationToken;
}

void CShaAcceleratorDriver::UpdateContextState()
{
  if (nullptr != m_pCurrentContext)
  {
    switch (m_pCurrentContext->Accelerator.m_State)
    {
    case ECalculationState::AwaitingResult:
      if (!AccelerationEngine.IsCalculating())
      {
        m_pCurrentContext->Accelerator.m_State = ECalculationState::ResultInHardware;
      }
      break;

    case ECalculationState::Initialize:
    case ECalculationState::ResultInHardware:
    case ECalculationState::PartialResult:
    case ECalculationState::Abandoned:
    case ECalculationState::Complete:
    default:
      break;
    }
  }
}

bool CShaAcceleratorDriver::IsCalculating()
{
  // read & return hardware status. 
  return AccelerationEngine.IsCalculating();
}

void CShaAcceleratorDriver::GetLastResult(uint8_t* pBuffer)
{
  AccelerationEngine.GetOutput(pBuffer);
}

void CShaAcceleratorDriver::StorePreviousResult(const uint8_t* pBuffer)
{
  AccelerationEngine.SetInput(pBuffer);
}

void CShaAcceleratorDriver::BeginCalculation(const uint8_t* pData, bool bFirst)
{
  AccelerationEngine.BeginCalculation(pData, bFirst);
}

