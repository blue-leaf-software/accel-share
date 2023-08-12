#pragma once

/// <summary>
/// Tracks the current state of an accelerated calculation. 
/// </summary>
enum class ECalculationState
{
  /// <summary>
  /// First calculation hasn't been done yet. 
  /// </summary>
  Initialize,

  /// <summary>
  /// Calculation loaded into accelerator; waiting 
  /// on the result. 
  /// </summary>
  AwaitingResult,

  /// <summary>
  /// Calculation is complete and the result is
  /// sitting in the hardware. 
  /// </summary>
  ResultInHardware,

  /// <summary>
  /// Result from the last calculation is cached
  /// in the context. This intermediate result
  /// must be loaded back into the hardware to
  /// continue. 
  /// </summary>
  PartialResult,

  /// <summary>
  /// Calculation has been abondoned. Result in
  /// context is not valid. 
  /// </summary>
  Abandoned,

  /// <summary>
  /// Calculation is completed. Final result in
  /// context is valid. 
  /// </summary>
  Complete,
};