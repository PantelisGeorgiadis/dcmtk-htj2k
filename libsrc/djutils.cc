
#include "dcmtkhtj2k/djutils.h"

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmdata/dcerror.h"

OFLogger DCM_ht2kLogger = OFLog::getLogger("DCMTKHTJ2K");

#define MAKE_DCMTKHTJ2K_ERROR(number, name, description) \
  makeOFConditionConst(EC_##name, OFM_dcmjp2k, number, OF_error, description)

MAKE_DCMTKHTJ2K_ERROR(
    1, HTJ2KUncompressedBufferTooSmall,
    "Uncompressed pixel data too short for uncompressed image");
MAKE_DCMTKHTJ2K_ERROR(2, HTJ2KCompressedBufferTooSmall,
                      "Allocated too small buffer for compressed image data");
MAKE_DCMTKHTJ2K_ERROR(3, HTJ2KCodecUnsupportedImageType,
                      "Codec does not support this HT-J2K image");
MAKE_DCMTKHTJ2K_ERROR(4, HTJ2KCodecInvalidParameters,
                      "Codec received invalid compression parameters");
MAKE_DCMTKHTJ2K_ERROR(5, HTJ2KCodecUnsupportedValue,
                      "Codec received unsupported compression parameters");
MAKE_DCMTKHTJ2K_ERROR(6, HTJ2KInvalidCompressedData,
                      "Invalid compressed image data");
MAKE_DCMTKHTJ2K_ERROR(7, HTJ2KUnsupportedBitDepthForTransform,
                      "Codec does not support the image's color transformation "
                      "with this bit depth");
MAKE_DCMTKHTJ2K_ERROR(
    8, HTJ2KUnsupportedColorTransform,
    "Codec does not support the image's color transformation");
MAKE_DCMTKHTJ2K_ERROR(9, HTJ2KUnsupportedBitDepth,
                      "Unsupported bit depth in HT-J2K transfer syntax");
MAKE_DCMTKHTJ2K_ERROR(10, HTJ2KCannotComputeNumberOfFragments,
                      "Cannot compute number of fragments for HT-J2K frame");
MAKE_DCMTKHTJ2K_ERROR(
    11, HTJ2KImageDataMismatch,
    "Image data mismatch between DICOM header and HT-J2K bitstream");
MAKE_DCMTKHTJ2K_ERROR(12, HTJ2KUnsupportedPhotometricInterpretation,
                      "Unsupported photometric interpretation for "
                      "near-lossless HT-J2K compression");
MAKE_DCMTKHTJ2K_ERROR(
    13, HTJ2KUnsupportedPixelRepresentation,
    "Unsupported pixel representation for near-lossless HT-J2K compression");
MAKE_DCMTKHTJ2K_ERROR(14, HTJ2KUnsupportedImageType,
                      "Unsupported type of image for HT-J2K compression");
MAKE_DCMTKHTJ2K_ERROR(15, HTJ2KTooMuchCompressedData,
                      "Too much compressed data, trailing data after image");
