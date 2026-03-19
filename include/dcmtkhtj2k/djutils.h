#ifndef DCMTKHTJ2K_DJUTILS_H
#define DCMTKHTJ2K_DJUTILS_H

#include "dcmtk/config/osconfig.h"
#include "dcmtk/oflog/oflog.h"
#include "dcmtk/ofstd/ofcond.h" /* for class OFCondition */
#include "dldefine.h"

#define DCMTKHTJ2K_VERSION_STRING "OpenJPH, Version 0.26.3 (unmodified)"

// global definitions for logging mechanism provided by the oflog module

extern DCMTKHTJ2K_EXPORT OFLogger DCM_htJ2kLogger;

#define DCMTKHTJ2K_TRACE(msg) OFLOG_TRACE(DCM_htJ2kLogger, msg)
#define DCMTKHTJ2K_DEBUG(msg) OFLOG_DEBUG(DCM_htJ2kLogger, msg)
#define DCMTKHTJ2K_INFO(msg) OFLOG_INFO(DCM_htJ2kLogger, msg)
#define DCMTKHTJ2K_WARN(msg) OFLOG_WARN(DCM_htJ2kLogger, msg)
#define DCMTKHTJ2K_ERROR(msg) OFLOG_ERROR(DCM_htJ2kLogger, msg)
#define DCMTKHTJ2K_FATAL(msg) OFLOG_FATAL(DCM_htJ2kLogger, msg)

// include this file in doxygen documentation

/** @file djutils.h
 *  @brief enumerations, error constants and helper functions for the dcmhtj2k
 * module
 */

/** describes the condition under which a compressed or decompressed image
 *  receives a new SOP instance UID.
 */
enum HTJ2K_UIDCreation {
  /** Upon compression, assign new SOP instance UID if compression is lossy.
   *  Upon decompression never assign new SOP instance UID.
   */
  EHTJ2KUC_default,

  /// always assign new SOP instance UID on compression and decompression
  EHTJ2KUC_always,

  /// never assign new SOP instance UID
  EHTJ2KUC_never
};

/** describes how the decoder should handle planar configuration of
 *  decompressed color images.
 */
enum HTJ2K_PlanarConfiguration {
  /// restore planar configuration as indicated in data set
  EHTJ2KPC_restore,

  /** automatically determine whether color-by-plane is required from
   *  the SOP Class UID and decompressed photometric interpretation
   */
  EHTJ2KPC_auto,

  /// always create color-by-pixel planar configuration
  EHTJ2KPC_colorByPixel,

  /// always create color-by-plane planar configuration
  EHTJ2KPC_colorByPlane
};

/** describes how the encoder handles the image bit depth
 *  upon lossy compression.
 */
enum HTJ2K_CompressionBitDepth {
  /// keep original bit depth
  EHTJ2KBD_original,

  /** limit bit depth to a certain value, i.e. scale down
   *  if the original image bit depth is larger
   */
  EHTJ2KBD_limit,

  /** force bit depth to a certain value, i.e. scale up
   *  or scale down the original image to match the given
   *  bit depth.
   */
  EHTJ2KBD_force
};

/** describes the progression order used in the codestream
 */
enum HTJ2K_ProgressionOrder {
  /// use default progression order as defined in HT-J2K standard
  EHTJ2KPO_default,

  /// layer-resolution-component-position progression order
  EHTJ2KPO_LRCP,

  /// resolution-layer-component-position progression order
  EHTJ2KPO_RLCP,

  /// resolution-position-component-layer progression order
  EHTJ2KPO_RPCL,

  /// position-component-resolution-layer progression order
  EHTJ2KPO_PCRL,

  /// component-position-resolution-layer progression order
  EHTJ2KPO_CPRL
};

// CONDITION CONSTANTS

/// error condition constant: Too small buffer used for image data (internal
/// error)
extern DCMTKHTJ2K_EXPORT const OFConditionConst
    EC_HTJ2KUncompressedBufferTooSmall;

/// error condition constant: Too small buffer used for compressed image data
/// (internal error)
extern DCMTKHTJ2K_EXPORT const OFConditionConst
    EC_HTJ2KCompressedBufferTooSmall;

/// error condition constant: The image uses some features which the codec does
/// not support
extern DCMTKHTJ2K_EXPORT const OFConditionConst
    EC_HTJ2KCodecUnsupportedImageType;

/// error condition constant: The codec was fed with invalid parameters (e.g.
/// height = -1)
extern DCMTKHTJ2K_EXPORT const OFConditionConst EC_HTJ2KCodecInvalidParameters;

/// error condition constant: The codec was fed with unsupported parameters
/// (e.g. 32 bit per sample)
extern DCMTKHTJ2K_EXPORT const OFConditionConst EC_HTJ2KCodecUnsupportedValue;

/// error condition constant: The compressed image is invalid
extern DCMTKHTJ2K_EXPORT const OFConditionConst EC_HTJ2KInvalidCompressedData;

/// error condition constant: The images' color transformation is not supported
/// in this bit depth
extern DCMTKHTJ2K_EXPORT const OFConditionConst
    EC_HTJ2KUnsupportedBitDepthForTransform;

/// error condition constant: The images' color transformation is not supported
extern DCMTKHTJ2K_EXPORT const OFConditionConst
    EC_HTJ2KUnsupportedColorTransform;

/// error condition constant: Unsupported bit depth in HT-J2K transfer syntax
extern DCMTKHTJ2K_EXPORT const OFConditionConst EC_HTJ2KUnsupportedBitDepth;

/// error condition constant: Cannot compute number of fragments for HT-J2K
/// frame
extern DCMTKHTJ2K_EXPORT const OFConditionConst
    EC_HTJ2KCannotComputeNumberOfFragments;

/// error condition constant: Image data mismatch between DICOM header and
/// HT-J2K bitstream
extern DCMTKHTJ2K_EXPORT const OFConditionConst EC_HTJ2KImageDataMismatch;

/// error condition constant: Unsupported photometric interpretation for
/// near-lossless HT-J2K compression
extern DCMTKHTJ2K_EXPORT const OFConditionConst
    EC_HTJ2KUnsupportedPhotometricInterpretation;

/// error condition constant: Unsupported pixel representation for near-lossless
/// HT-J2K compression
extern DCMTKHTJ2K_EXPORT const OFConditionConst
    EC_HTJ2KUnsupportedPixelRepresentation;

/// error condition constant: Unsupported type of image for HT-J2K compression
extern DCMTKHTJ2K_EXPORT const OFConditionConst EC_HTJ2KUnsupportedImageType;

/// error condition constant: Trailing data after image
extern DCMTKHTJ2K_EXPORT const OFConditionConst EC_HTJ2KTooMuchCompressedData;

#endif
