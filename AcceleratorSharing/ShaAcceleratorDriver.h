#pragma once

#include <mutex>
#include <chrono>

#include "Accelerator.h"

#include "CalculationContext.h"


enum class ShaAcceleratorResult
{
  Started, 

  Busy,

  Timeout,
};

/// <summary>
/// Mockup of thread-safe sha accelerator driver. 
/// </summary>
class CShaAcceleratorDriver
{
  /// <summary>
  /// Controls if interleaving calculations is allowed. 
  /// </summary>
  const bool m_bSupportsInterleaving;

  /// <summary>
  /// For thread-safe access to mutable fields. 
  /// </summary>
  std::mutex m_Lock;

  /// <summary>
  /// Next token to identify a calculation
  /// </summary>
  uint32_t m_uCalculationToken;

  /// <summary>
  /// The calculation we are currently working on. Null if there isn't one. 
  /// </summary>
  CShaContext* m_pCurrentContext;

  /// <summary>
  /// Time that last calculation was started. 
  /// </summary>
  std::chrono::steady_clock::time_point m_LastCalculationStarted;

  /// <summary>
  /// Typical amount of time it takes to accelerate a has calculation to help decide
  /// whether we should wait for the hardware or not. 
  /// </summary>
  static constexpr const std::chrono::microseconds m_TypicalCalculationTime = std::chrono::microseconds(200);

public:
  CShaAcceleratorDriver(bool bSupportsInterleaving);

  ShaAcceleratorResult BeginCalculation(CShaContext& rContext, uint8_t *pDataToHash, std::chrono::milliseconds Timeout);

  bool CompleteCalculation(CShaContext& rContext, std::chrono::milliseconds Timeout);

  void AbandonCalculation(CShaContext& rContext);

private:
  /// <summary>
  /// Determine if the context supplied is the one that the "hardware" is working on
  /// </summary>
  bool IsWorkingOn(CShaContext& rContext);

  /// <summary>
  /// Wait (forever) for the current calculation to complete. E.g., if we are asked
  /// to start another calculation for the context we are working on. 
  /// </summary>
  void WaitForCalculationToFinish();

  /// <summary>
  /// Wait no more than Timeout for the calculation to complete. Might wait for no time
  /// (e.g., if there's not chance the calculation will complete in the given time) or
  /// up to maximum of Timeout. 
  /// </summary>
  bool WaitForCalculationToFinish(std::chrono::milliseconds Timeout);

  /// <summary>
  /// Save result in the hardware so we can continue later. 
  /// </summary>
  void StashIntermediateResult(CShaContext& rContext);

  /// <summary>
  /// Restore a previously saved intermediate result to continue calculation. 
  /// </summary>
  void RestoreIntermediateResult(CShaContext& rContext);

  /// <summary>
  /// Generate a new token used to identify calculations. Monotonically increasing, 
  /// never zero. 
  /// </summary>
  uint32_t NextToken();

  /// <summary>
  /// Update the state of the current context (e.g., check to see if the hardware finished
  /// doing its calculation). 
  /// </summary>
  void UpdateContextState();

private: // hardware layer
  /// <summary>
  /// Determine if the hardware engine is still busy working on
  /// the last calculation. 
  /// </summary>
  bool IsCalculating();

  /// <summary>
  /// Save the previous result in a buffer so we can resume it later. 
  /// </summary>
  void GetLastResult(uint8_t* pBuffer);

  /// <summary>
  /// Copy the partial hash result from a previous calculation into the hardware
  /// so we can continue a previous calculation. 
  /// </summary>
  void StorePreviousResult(const uint8_t* pBuffer);

  /// <summary>
  /// Start a new calculation using the "hardware accelerator"
  /// </summary>
  void BeginCalculation(const uint8_t* pData, bool bFirst);

};