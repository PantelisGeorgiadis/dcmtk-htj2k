
#include "dcmtkhtj2k/djutils.h"

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmdata/dcerror.h"

OFLogger DCM_ht2kLogger = OFLog::getLogger("DCMTKHTJ2K");

#define MAKE_DCMTKHTJ2K_ERROR(number, name, description) \
  makeOFConditionConst(EC_##name, OFM_dcmjp2k, number, OF_error, description)

MAKE_DCMTKHTJ2K_ERROR(
    1, J2KUncompressedBufferTooSmall,
    "Uncompressed pixel data too short for uncompressed image");
MAKE_DCMTKHTJ2K_ERROR(2, J2KCompressedBufferTooSmall,
                      "Allocated too small buffer for compressed image data");
MAKE_DCMTKHTJ2K_ERROR(3, J2KCodecUnsupportedImageType,
                      "Codec does not support this HT-J2K image");
MAKE_DCMTKHTJ2K_ERROR(4, J2KCodecInvalidParameters,
                      "Codec received invalid compression parameters");
MAKE_DCMTKHTJ2K_ERROR(5, J2KCodecUnsupportedValue,
                      "Codec received unsupported compression parameters");
MAKE_DCMTKHTJ2K_ERROR(6, J2KInvalidCompressedData,
                      "Invalid compressed image data");
MAKE_DCMTKHTJ2K_ERROR(7, J2KUnsupportedBitDepthForTransform,
                      "Codec does not support the image's color transformation "
                      "with this bit depth");
MAKE_DCMTKHTJ2K_ERROR(
    8, J2KUnsupportedColorTransform,
    "Codec does not support the image's color transformation");
MAKE_DCMTKHTJ2K_ERROR(9, J2KUnsupportedBitDepth,
                      "Unsupported bit depth in HT-J2K transfer syntax");
MAKE_DCMTKHTJ2K_ERROR(10, J2KCannotComputeNumberOfFragments,
                      "Cannot compute number of fragments for HT-J2K frame");
MAKE_DCMTKHTJ2K_ERROR(
    11, J2KImageDataMismatch,
    "Image data mismatch between DICOM header and HT-J2K bitstream");
MAKE_DCMTKHTJ2K_ERROR(12, J2KUnsupportedPhotometricInterpretation,
                      "Unsupported photometric interpretation for "
                      "near-lossless HT-J2K compression");
MAKE_DCMTKHTJ2K_ERROR(
    13, J2KUnsupportedPixelRepresentation,
    "Unsupported pixel representation for near-lossless HT-J2K compression");
MAKE_DCMTKHTJ2K_ERROR(14, J2KUnsupportedImageType,
                      "Unsupported type of image for HT-J2K compression");
MAKE_DCMTKHTJ2K_ERROR(15, J2KTooMuchCompressedData,
                      "Too much compressed data, trailing data after image");
