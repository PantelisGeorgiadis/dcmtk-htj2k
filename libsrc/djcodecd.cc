#include "dcmtkhtj2k/djcodecd.h"

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmdata/dcdatset.h" /* for class DcmDataset */
#include "dcmtk/dcmdata/dcdeftag.h" /* for tag constants */
#include "dcmtk/dcmdata/dcpixseq.h" /* for class DcmPixelSequence */
#include "dcmtk/dcmdata/dcpxitem.h" /* for class DcmPixelItem */
#include "dcmtk/dcmdata/dcswap.h"   /* for swapIfNecessary() */
#include "dcmtk/dcmdata/dcuid.h"    /* for dcmGenerateUniqueIdentifer()*/
#include "dcmtk/dcmdata/dcvrpobw.h" /* for class DcmPolymorphOBOW */
#include "dcmtk/ofstd/ofcast.h"     /* for casts */
#include "dcmtk/ofstd/offile.h"     /* for class OFFile */
#include "dcmtk/ofstd/ofstd.h"      /* for class OFStandard */
#include "dcmtk/ofstd/ofstream.h"   /* for ofstream */
#include "dcmtkhtj2k/djcparam.h"    /* for class DJP2KCodecParameter */

// HT-J2K library (OpenJPH) includes
#include "openjph/ojph_arch.h"
#include "openjph/ojph_codestream.h"
#include "openjph/ojph_file.h"
#include "openjph/ojph_mem.h"
#include "openjph/ojph_params.h"

HtJ2kDecoderBase::HtJ2kDecoderBase() : DcmCodec() {}

HtJ2kDecoderBase::~HtJ2kDecoderBase() {}

OFBool HtJ2kDecoderBase::canChangeCoding(
    E_TransferSyntax const oldRepType,
    E_TransferSyntax const newRepType) const {
  // this codec only handles conversion from HT-J2K to uncompressed.

  DcmXfer newRep(newRepType);
  if (newRep.getStreamCompression() == ESC_none &&
      ((oldRepType == EXS_HighThroughputJPEG2000LosslessOnly) ||
       (oldRepType == EXS_HighThroughputJPEG2000) ||
       (oldRepType == EXS_HighThroughputJPEG2000withRPCLOptionsLosslessOnly)))
    return OFTrue;

  return OFFalse;
}

OFCondition HtJ2kDecoderBase::decode(
    DcmRepresentationParameter const * /* fromRepParam */,
    DcmPixelSequence *pixSeq, DcmPolymorphOBOW &uncompressedPixelData,
    DcmCodecParameter const *cp, DcmStack const &objStack) const {
  // retrieve pointer to dataset from parameter stack
  DcmStack localStack(objStack);
  (void)localStack.pop();  // pop pixel data element from stack
  DcmObject *dobject =
      localStack.pop();  // this is the item in which the pixel data is located
  if ((!dobject) ||
      ((dobject->ident() != EVR_dataset) && (dobject->ident() != EVR_item)))
    return EC_InvalidTag;
  DcmItem *dataset = OFstatic_cast(DcmItem *, dobject);
  OFBool numberOfFramesPresent = OFFalse;

  // determine properties of uncompressed dataset
  Uint16 imageSamplesPerPixel = 0;
  if (dataset->findAndGetUint16(DCM_SamplesPerPixel, imageSamplesPerPixel)
          .bad())
    return EC_TagNotFound;
  // we only handle one or three samples per pixel
  if ((imageSamplesPerPixel != 3) && (imageSamplesPerPixel != 1))
    return EC_InvalidTag;

  Uint16 imageRows = 0;
  if (dataset->findAndGetUint16(DCM_Rows, imageRows).bad())
    return EC_TagNotFound;
  if (imageRows < 1) return EC_InvalidTag;

  Uint16 imageColumns = 0;
  if (dataset->findAndGetUint16(DCM_Columns, imageColumns).bad())
    return EC_TagNotFound;
  if (imageColumns < 1) return EC_InvalidTag;

  // number of frames is an optional attribute - we don't mind if it isn't
  // present.
  Sint32 imageFrames = 0;
  if (dataset->findAndGetSint32(DCM_NumberOfFrames, imageFrames).good())
    numberOfFramesPresent = OFTrue;

  if (imageFrames >= OFstatic_cast(Sint32, pixSeq->card()))
    imageFrames = pixSeq->card() -
                  1;  // limit number of frames to number of pixel items - 1
  if (imageFrames < 1)
    imageFrames = 1;  // default in case the number of frames attribute is
                      // absent or contains garbage

  Uint16 imageBitsStored = 0;
  if (dataset->findAndGetUint16(DCM_BitsStored, imageBitsStored).bad())
    return EC_TagNotFound;

  Uint16 imageBitsAllocated = 0;
  if (dataset->findAndGetUint16(DCM_BitsAllocated, imageBitsAllocated).bad())
    return EC_TagNotFound;

  Uint16 imageHighBit = 0;
  if (dataset->findAndGetUint16(DCM_HighBit, imageHighBit).bad())
    return EC_TagNotFound;

  // we only support up to 16 bits per sample
  if ((imageBitsStored < 1) || (imageBitsStored > 16))
    return EC_J2KUnsupportedBitDepth;

  // determine the number of bytes per sample (bits allocated) for the
  // de-compressed object.
  Uint16 bytesPerSample = 1;
  if (imageBitsStored > 8)
    bytesPerSample = 2;
  else if (imageBitsAllocated > 8)
    bytesPerSample = 2;

  // compute size of uncompressed frame, in bytes
  Uint32 frameSize =
      bytesPerSample * imageRows * imageColumns * imageSamplesPerPixel;

  // compute size of pixel data attribute, in bytes
  Uint32 totalSize = frameSize * imageFrames;
  if (totalSize & 1) totalSize++;  // align on 16-bit word boundary

  // assume we can cast the codec parameter to what we need
  HtJ2kCodecParameter const *djcp =
      OFreinterpret_cast(HtJ2kCodecParameter const *, cp);

  // determine planar configuration for uncompressed data
  OFString imageSopClass;
  OFString imagePhotometricInterpretation;
  dataset->findAndGetOFString(DCM_SOPClassUID, imageSopClass);
  dataset->findAndGetOFString(DCM_PhotometricInterpretation,
                              imagePhotometricInterpretation);

  // allocate space for uncompressed pixel data element
  Uint16 *pixeldata16 = NULL;
  OFCondition result = uncompressedPixelData.createUint16Array(
      totalSize / sizeof(Uint16), pixeldata16);
  if (result.bad()) return result;

  Uint8 *pixeldata8 = OFreinterpret_cast(Uint8 *, pixeldata16);
  Sint32 currentFrame = 0;
  Uint32 currentItem = 1;  // item 0 contains the offset table
  OFBool done = OFFalse;

  while (result.good() && !done) {
    DCMTKHTJ2K_DEBUG("HT-J2K decoder processes frame " << (currentFrame + 1));

    result = decodeFrame(pixSeq, djcp, dataset, currentFrame, currentItem,
                         pixeldata8, frameSize, imageFrames, imageColumns,
                         imageRows, imageSamplesPerPixel, bytesPerSample);

    if (result.good()) {
      // increment frame number, check if we're finished
      if (++currentFrame == imageFrames) done = OFTrue;
      pixeldata8 += frameSize;
    }
  }

  // Number of Frames might have changed in case the previous value was wrong
  if (result.good() && (numberOfFramesPresent || (imageFrames > 1))) {
    char numBuf[20];
    snprintf(numBuf, sizeof(numBuf), "%ld", OFstatic_cast(long, imageFrames));
    result =
        ((DcmItem *)dataset)->putAndInsertString(DCM_NumberOfFrames, numBuf);
  }

  if (result.good()) {
    // the following operations do not affect the Image Pixel Module
    // but other modules such as SOP Common.  We only perform these
    // changes if we're on the main level of the dataset,
    // which should always identify itself as dataset, not as item.
    if ((dataset->ident() == EVR_dataset) &&
        (djcp->getUIDCreation() == EJ2KUC_always)) {
      // create new SOP instance UID
      result = DcmCodec::newInstance((DcmItem *)dataset, NULL, NULL, NULL);
    }
  }

  return result;
}

OFCondition HtJ2kDecoderBase::decode(
    DcmRepresentationParameter const *fromRepParam, DcmPixelSequence *pixSeq,
    DcmPolymorphOBOW &uncompressedPixelData, DcmCodecParameter const *cp,
    DcmStack const &objStack, OFBool &removeOldRep) const {
  // removeOldRep is left as it is, pixel data in original DICOM dataset is not
  // modified
  return decode(fromRepParam, pixSeq, uncompressedPixelData, cp, objStack);
}

OFCondition HtJ2kDecoderBase::decodeFrame(
    DcmRepresentationParameter const * /* fromParam */,
    DcmPixelSequence *fromPixSeq, DcmCodecParameter const *cp, DcmItem *dataset,
    Uint32 frameNo, Uint32 &currentItem, void *buffer, Uint32 bufSize,
    OFString &decompressedColorModel) const {
  OFCondition result = EC_Normal;

  // assume we can cast the codec parameter to what we need
  HtJ2kCodecParameter const *djcp =
      OFreinterpret_cast(HtJ2kCodecParameter const *, cp);

  // determine properties of uncompressed dataset
  Uint16 imageSamplesPerPixel = 0;
  if (dataset->findAndGetUint16(DCM_SamplesPerPixel, imageSamplesPerPixel)
          .bad())
    return EC_TagNotFound;
  // we only handle one or three samples per pixel
  if ((imageSamplesPerPixel != 3) && (imageSamplesPerPixel != 1))
    return EC_InvalidTag;

  Uint16 imageRows = 0;
  if (dataset->findAndGetUint16(DCM_Rows, imageRows).bad())
    return EC_TagNotFound;
  if (imageRows < 1) return EC_InvalidTag;

  Uint16 imageColumns = 0;
  if (dataset->findAndGetUint16(DCM_Columns, imageColumns).bad())
    return EC_TagNotFound;
  if (imageColumns < 1) return EC_InvalidTag;

  Uint16 imageBitsStored = 0;
  if (dataset->findAndGetUint16(DCM_BitsStored, imageBitsStored).bad())
    return EC_TagNotFound;

  Uint16 imageBitsAllocated = 0;
  if (dataset->findAndGetUint16(DCM_BitsAllocated, imageBitsAllocated).bad())
    return EC_TagNotFound;

  // we only support up to 16 bits per sample
  if ((imageBitsStored < 1) || (imageBitsStored > 16))
    return EC_J2KUnsupportedBitDepth;

  // determine the number of bytes per sample (bits allocated) for the
  // de-compressed object.
  Uint16 bytesPerSample = 1;
  if (imageBitsStored > 8)
    bytesPerSample = 2;
  else if (imageBitsAllocated > 8)
    bytesPerSample = 2;

  // number of frames is an optional attribute - we don't mind if it isn't
  // present.
  Sint32 imageFrames = 0;
  dataset->findAndGetSint32(DCM_NumberOfFrames, imageFrames).good();

  if (imageFrames >= OFstatic_cast(Sint32, fromPixSeq->card()))
    imageFrames = fromPixSeq->card() -
                  1;  // limit number of frames to number of pixel items - 1
  if (imageFrames < 1)
    imageFrames = 1;  // default in case the number of frames attribute is
                      // absent or contains garbage

  // if the user has provided this information, we trust him.
  // If the user has passed a zero, try to find out ourselves.
  if (currentItem == 0) {
    result =
        determineStartFragment(frameNo, imageFrames, fromPixSeq, currentItem);
  }

  if (result.good()) {
    // We got all the data we need from the dataset, let's start decoding
    DCMTKHTJ2K_DEBUG("Starting to decode frame " << frameNo << " with fragment "
                                                 << currentItem);
    result = decodeFrame(fromPixSeq, djcp, dataset, frameNo, currentItem,
                         buffer, bufSize, imageFrames, imageColumns, imageRows,
                         imageSamplesPerPixel, bytesPerSample);
  }

  if (result.good()) {
    // retrieve color model from given dataset
    result = dataset->findAndGetOFString(DCM_PhotometricInterpretation,
                                         decompressedColorModel);
  }

  return result;
}

OFCondition copyUint32ToUint8(ojph::ui32 **comps_data, int num_comps,
                              Uint8 *imageFrame, Uint16 columns, Uint16 rows);

OFCondition copyUint32ToUint16(ojph::ui32 **comps_data, int num_comps,
                               Uint16 *imageFrame, Uint16 columns, Uint16 rows);

OFCondition copyRGBUint8ToRGBUint8(ojph::ui32 **comps_data, Uint8 *imageFrame,
                                   Uint16 columns, Uint16 rows);

OFCondition copyRGBUint8ToRGBUint8Planar(ojph::ui32 **comps_data,
                                         Uint8 *imageFrame, Uint16 columns,
                                         Uint16 rows);

OFCondition HtJ2kDecoderBase::decodeFrame(
    DcmPixelSequence *fromPixSeq, HtJ2kCodecParameter const *cp,
    DcmItem *dataset, Uint32 frameNo, Uint32 &currentItem, void *buffer,
    Uint32 bufSize, Sint32 imageFrames, Uint16 imageColumns, Uint16 imageRows,
    Uint16 imageSamplesPerPixel, Uint16 bytesPerSample) {
  DcmPixelItem *pixItem = NULL;
  Uint8 *htj2kData = NULL;
  Uint8 *htj2kFragmentData = NULL;
  Uint32 fragmentLength = 0;
  size_t compressedSize = 0;
  Uint32 fragmentsForThisFrame = 0;
  OFCondition result = EC_Normal;
  OFBool ignoreOffsetTable = cp->ignoreOffsetTable();

  // compute the number of HT-J2K fragments we need in order to decode the next
  // frame
  fragmentsForThisFrame = computeNumberOfFragments(
      imageFrames, frameNo, currentItem, ignoreOffsetTable, fromPixSeq);
  if (fragmentsForThisFrame == 0) result = EC_J2KCannotComputeNumberOfFragments;

  // determine planar configuration for uncompressed data
  OFString imageSopClass;
  OFString imagePhotometricInterpretation;
  dataset->findAndGetOFString(DCM_SOPClassUID, imageSopClass);
  dataset->findAndGetOFString(DCM_PhotometricInterpretation,
                              imagePhotometricInterpretation);
  Uint16 imagePlanarConfiguration =
      0;  // 0 is color-by-pixel, 1 is color-by-plane

  if (imageSamplesPerPixel > 1) {
    switch (cp->getPlanarConfiguration()) {
      case EJ2KPC_restore:
        // get planar configuration from dataset
        imagePlanarConfiguration = 2;  // invalid value
        dataset->findAndGetUint16(DCM_PlanarConfiguration,
                                  imagePlanarConfiguration);
        // determine auto default if not found or invalid
        if (imagePlanarConfiguration > 1)
          imagePlanarConfiguration = determinePlanarConfiguration(
              imageSopClass, imagePhotometricInterpretation);
        break;
      case EJ2KPC_auto:
        imagePlanarConfiguration = determinePlanarConfiguration(
            imageSopClass, imagePhotometricInterpretation);
        break;
      case EJ2KPC_colorByPixel:
        imagePlanarConfiguration = 0;
        break;
      case EJ2KPC_colorByPlane:
        imagePlanarConfiguration = 1;
        break;
    }
  }

  // get the size of all the fragments
  if (result.good()) {
    // Don't modify the original values for now
    Uint32 fragmentsForThisFrame2 = fragmentsForThisFrame;
    Uint32 currentItem2 = currentItem;

    while (result.good() && fragmentsForThisFrame2--) {
      result = fromPixSeq->getItem(pixItem, currentItem2++);
      if (result.good() && pixItem) {
        fragmentLength = pixItem->getLength();
        if (result.good()) compressedSize += fragmentLength;
      }
    } /* while */
  }

  // get the compressed data
  if (result.good()) {
    Uint32 offset = 0;
    htj2kData = new Uint8[compressedSize];

    while (result.good() && fragmentsForThisFrame--) {
      result = fromPixSeq->getItem(pixItem, currentItem++);
      if (result.good() && pixItem) {
        fragmentLength = pixItem->getLength();
        result = pixItem->getUint8Array(htj2kFragmentData);
        if (result.good() && htj2kFragmentData) {
          memcpy(&htj2kData[offset], htj2kFragmentData, fragmentLength);
          offset += fragmentLength;
        }
      }
    } /* while */
  }

  if (result.good()) {
    // see if the last byte is a padding, otherwise, it should be 0xd9
    if (htj2kData[compressedSize - 1] == 0) compressedSize--;

    // start of OpenJPH decoding
    try {
      ojph::codestream codestream;
      ojph::mem_infile mem_file;

      mem_file.open(htj2kData, compressedSize);
      codestream.enable_resilience();
      codestream.read_headers(&mem_file);

      ojph::param_siz siz = codestream.access_siz();
      int num_comps = siz.get_num_components();
      int width = siz.get_recon_width(0);
      int height = siz.get_recon_height(0);

      if (width != imageColumns)
        result = EC_J2KImageDataMismatch;
      else if (height != imageRows)
        result = EC_J2KImageDataMismatch;
      else if (num_comps != imageSamplesPerPixel)
        result = EC_J2KImageDataMismatch;

      if (result.good()) {
        codestream.create();

        // Allocate line buffer
        ojph::ui32 **comps_data = new ojph::ui32 *[num_comps];
        for (int c = 0; c < num_comps; c++) {
          comps_data[c] = new ojph::ui32[width * height];
        }

        // Decode all lines
        for (ojph::ui32 y = 0; y < (ojph::ui32)height; y++) {
          for (int c = 0; c < num_comps; c++) {
            ojph::ui32 comp_num;
            ojph::line_buf *line = codestream.pull(comp_num);
            ojph::si32 *sp = line->i32;
            ojph::ui32 *dp = &comps_data[c][y * width];
            for (ojph::ui32 x = 0; x < (ojph::ui32)width; x++) {
              *dp++ = (ojph::ui32)*sp++;
            }
          }
        }

        codestream.close();

        // Copy the image depending on planar configuration and bits
        if (num_comps == 1)  // Greyscale
        {
          if (bytesPerSample == 1)
            copyUint32ToUint8(comps_data, num_comps,
                              OFreinterpret_cast(Uint8 *, buffer), imageColumns,
                              imageRows);
          else
            copyUint32ToUint16(comps_data, num_comps,
                               OFreinterpret_cast(Uint16 *, buffer),
                               imageColumns, imageRows);
        } else if (num_comps == 3) {
          if (imagePlanarConfiguration == 0) {
            copyRGBUint8ToRGBUint8(comps_data,
                                   OFreinterpret_cast(Uint8 *, buffer),
                                   imageColumns, imageRows);
          } else if (imagePlanarConfiguration == 1) {
            copyRGBUint8ToRGBUint8Planar(comps_data,
                                         OFreinterpret_cast(Uint8 *, buffer),
                                         imageColumns, imageRows);
          }
        }

        // Clean up
        for (int c = 0; c < num_comps; c++) {
          delete[] comps_data[c];
        }
        delete[] comps_data;
      }
    } catch (std::exception &ex) {
      DCMTKHTJ2K_ERROR("HT-J2K decoder caught OpenJPH exception: "
                       << (ex.what() ? ex.what() : "Unknown reason"));
      result =
          makeOFCondition(1, OFM_dcmjp2k, OF_error,
                          ex.what() ? ex.what() : "Unknown OpenJPH exception");
    }

    delete[] htj2kData;
  }

  if (result.good()) {
    // decompression is complete, finally adjust byte order if necessary
    if (bytesPerSample == 1)  // we're writing bytes into words
    {
      result = swapIfNecessary(gLocalByteOrder, EBO_LittleEndian, buffer,
                               bufSize, sizeof(Uint16));
    }
  }

  return result;
}

OFCondition HtJ2kDecoderBase::encode(
    Uint16 const * /* pixelData */, Uint32 const /* length */,
    DcmRepresentationParameter const * /* toRepParam */,
    DcmPixelSequence *& /* pixSeq */, DcmCodecParameter const * /* cp */,
    DcmStack & /* objStack */) const {
  // we are a decoder only
  return EC_IllegalCall;
}

OFCondition HtJ2kDecoderBase::encode(
    Uint16 const *pixelData, Uint32 const length,
    DcmRepresentationParameter const *toRepParam, DcmPixelSequence *&pixSeq,
    DcmCodecParameter const *cp, DcmStack &objStack,
    OFBool &removeOldRep) const {
  return EC_IllegalCall;
}

OFCondition HtJ2kDecoderBase::encode(
    E_TransferSyntax const /* fromRepType */,
    DcmRepresentationParameter const * /* fromRepParam */,
    DcmPixelSequence * /* fromPixSeq */,
    DcmRepresentationParameter const * /* toRepParam */,
    DcmPixelSequence *& /* toPixSeq */, DcmCodecParameter const * /* cp */,
    DcmStack & /* objStack */) const {
  // we don't support re-coding for now.
  return EC_IllegalCall;
}

OFCondition HtJ2kDecoderBase::encode(
    E_TransferSyntax const fromRepType,
    DcmRepresentationParameter const *fromRepParam,
    DcmPixelSequence *fromPixSeq, DcmRepresentationParameter const *toRepParam,
    DcmPixelSequence *&toPixSeq, DcmCodecParameter const *cp,
    DcmStack &objStack, OFBool &removeOldRep) const {
  return EC_IllegalCall;
}

OFCondition HtJ2kDecoderBase::determineDecompressedColorModel(
    DcmRepresentationParameter const * /* fromParam */,
    DcmPixelSequence * /* fromPixSeq */, DcmCodecParameter const * /* cp */,
    DcmItem *dataset, OFString &decompressedColorModel) const {
  OFCondition result = EC_IllegalParameter;
  if (dataset != NULL) {
    // retrieve color model from given dataset
    result = dataset->findAndGetOFString(DCM_PhotometricInterpretation,
                                         decompressedColorModel);
  }
  return result;
}

Uint16 HtJ2kDecoderBase::determinePlanarConfiguration(
    OFString const &sopClassUID, OFString const &photometricInterpretation) {
  // Hardcopy Color Image always requires color-by-plane
  if (sopClassUID == UID_RETIRED_HardcopyColorImageStorage) return 1;

  // The 1996 Ultrasound Image IODs require color-by-plane if color model is
  // YBR_FULL.
  if (photometricInterpretation == "YBR_FULL") {
    if ((sopClassUID == UID_UltrasoundMultiframeImageStorage) ||
        (sopClassUID == UID_UltrasoundImageStorage))
      return 1;
  }

  // default for all other cases
  return 0;
}

Uint32 HtJ2kDecoderBase::computeNumberOfFragments(Sint32 numberOfFrames,
                                                  Uint32 currentFrame,
                                                  Uint32 startItem,
                                                  OFBool ignoreOffsetTable,
                                                  DcmPixelSequence *pixSeq) {
  unsigned long numItems = pixSeq->card();
  DcmPixelItem *pixItem = NULL;

  // We first check the simple cases, that is, a single-frame image,
  // the last frame of a multi-frame image and the standard case where we do
  // have a single fragment per frame.
  if ((numberOfFrames <= 1) ||
      (currentFrame + 1 == OFstatic_cast(Uint32, numberOfFrames))) {
    // single-frame image or last frame. All remaining fragments belong to this
    // frame
    return (numItems - startItem);
  }
  if (OFstatic_cast(Uint32, numberOfFrames + 1) == numItems) {
    // multi-frame image with one fragment per frame
    return 1;
  }

  OFCondition result = EC_Normal;
  if (!ignoreOffsetTable) {
    // We do have a multi-frame image with multiple fragments per frame, and we
    // are not working on the last frame. Let's check the offset table if
    // present.
    result = pixSeq->getItem(pixItem, 0);
    if (result.good() && pixItem) {
      Uint32 offsetTableLength = pixItem->getLength();
      if (offsetTableLength == (OFstatic_cast(Uint32, numberOfFrames) * 4)) {
        // offset table is non-empty and contains one entry per frame
        Uint8 *offsetData = NULL;
        result = pixItem->getUint8Array(offsetData);
        if (result.good() && offsetData) {
          // now we can access the offset table
          Uint32 *offsetData32 = OFreinterpret_cast(Uint32 *, offsetData);

          // extract the offset for the NEXT frame. This offset is guaranteed to
          // exist because the "last frame/single frame" case is handled above.
          Uint32 offset = offsetData32[currentFrame + 1];

          // convert to local endian byte order (always little endian in file)
          swapIfNecessary(gLocalByteOrder, EBO_LittleEndian, &offset,
                          sizeof(Uint32), sizeof(Uint32));

          // determine index of start fragment for next frame
          Uint32 byteCount = 0;
          Uint32 fragmentIndex = 1;
          while ((byteCount < offset) && (fragmentIndex < numItems)) {
            pixItem = NULL;
            result = pixSeq->getItem(pixItem, fragmentIndex++);
            if (result.good() && pixItem) {
              byteCount += pixItem->getLength() +
                           8;  // add 8 bytes for item tag and length
              if ((byteCount == offset) && (fragmentIndex > startItem)) {
                // bingo, we have found the offset for the next frame
                return fragmentIndex - startItem;
              }
            } else
              break; /* something went wrong, break out of while loop */
          }          /* while */
        }
      }
    }
  }

  // So we have a multi-frame image with multiple fragments per frame and the
  // offset table is empty or wrong. Our last chance is to peek into the
  // HT-J2K bitstream and identify the start of the next frame.
  Uint32 nextItem = startItem;
  Uint8 *fragmentData = NULL;
  while (++nextItem < numItems) {
    pixItem = NULL;
    result = pixSeq->getItem(pixItem, nextItem);
    if (result.good() && pixItem) {
      fragmentData = NULL;
      result = pixItem->getUint8Array(fragmentData);
      if (result.good() && fragmentData && (pixItem->getLength() > 3)) {
        if (isJ2KStartOfImage(fragmentData)) {
          // found a HT-J2K SOC marker. Assume that this is the start of the
          // next frame.
          return (nextItem - startItem);
        }
      } else
        break; /* something went wrong, break out of while loop */
    } else
      break; /* something went wrong, break out of while loop */
  }

  // We're bust. No way to determine the number of fragments per frame.
  return 0;
}

OFBool HtJ2kDecoderBase::isJ2KStartOfImage(Uint8 *fragmentData) {
  // A valid JPEG 2000 codestream starts with SOC (FF4F),
  // followed by SIZ (FF51).
  if ((*fragmentData++) != 0xFF) return OFFalse;
  if ((*fragmentData++) != 0x4F) return OFFalse;
  if ((*fragmentData++) != 0xFF) return OFFalse;
  if (*fragmentData == 0x51) {
    return OFTrue;
  }
  return OFFalse;
}

OFCondition HtJ2kDecoderBase::createPlanarConfiguration1Byte(Uint8 *imageFrame,
                                                             Uint16 columns,
                                                             Uint16 rows) {
  if (imageFrame == NULL) return EC_IllegalCall;

  unsigned long numPixels = columns * rows;
  if (numPixels == 0) return EC_IllegalCall;

  Uint8 *buf = new Uint8[3 * numPixels + 3];
  if (buf) {
    memcpy(buf, imageFrame, (size_t)(3 * numPixels));
    Uint8 *s = buf;                           // source
    Uint8 *r = imageFrame;                    // red plane
    Uint8 *g = imageFrame + numPixels;        // green plane
    Uint8 *b = imageFrame + (2 * numPixels);  // blue plane
    for (unsigned long i = numPixels; i; i--) {
      *r++ = *s++;
      *g++ = *s++;
      *b++ = *s++;
    }
    delete[] buf;
  } else
    return EC_MemoryExhausted;
  return EC_Normal;
}

OFCondition HtJ2kDecoderBase::createPlanarConfiguration1Word(Uint16 *imageFrame,
                                                             Uint16 columns,
                                                             Uint16 rows) {
  if (imageFrame == NULL) return EC_IllegalCall;

  unsigned long numPixels = columns * rows;
  if (numPixels == 0) return EC_IllegalCall;

  Uint16 *buf = new Uint16[3 * numPixels + 3];
  if (buf) {
    memcpy(buf, imageFrame, (size_t)(3 * numPixels * sizeof(Uint16)));
    Uint16 *s = buf;                           // source
    Uint16 *r = imageFrame;                    // red plane
    Uint16 *g = imageFrame + numPixels;        // green plane
    Uint16 *b = imageFrame + (2 * numPixels);  // blue plane
    for (unsigned long i = numPixels; i; i--) {
      *r++ = *s++;
      *g++ = *s++;
      *b++ = *s++;
    }
    delete[] buf;
  } else
    return EC_MemoryExhausted;
  return EC_Normal;
}

OFCondition HtJ2kDecoderBase::createPlanarConfiguration0Byte(Uint8 *imageFrame,
                                                             Uint16 columns,
                                                             Uint16 rows) {
  if (imageFrame == NULL) return EC_IllegalCall;

  unsigned long numPixels = columns * rows;
  if (numPixels == 0) return EC_IllegalCall;

  Uint8 *buf = new Uint8[3 * numPixels + 3];
  if (buf) {
    memcpy(buf, imageFrame, (size_t)(3 * numPixels));
    Uint8 *t = imageFrame;             // target
    Uint8 *r = buf;                    // red plane
    Uint8 *g = buf + numPixels;        // green plane
    Uint8 *b = buf + (2 * numPixels);  // blue plane
    for (unsigned long i = numPixels; i; i--) {
      *t++ = *r++;
      *t++ = *g++;
      *t++ = *b++;
    }
    delete[] buf;
  } else
    return EC_MemoryExhausted;
  return EC_Normal;
}

OFCondition HtJ2kDecoderBase::createPlanarConfiguration0Word(Uint16 *imageFrame,
                                                             Uint16 columns,
                                                             Uint16 rows) {
  if (imageFrame == NULL) return EC_IllegalCall;

  unsigned long numPixels = columns * rows;
  if (numPixels == 0) return EC_IllegalCall;

  Uint16 *buf = new Uint16[3 * numPixels + 3];
  if (buf) {
    memcpy(buf, imageFrame, (size_t)(3 * numPixels * sizeof(Uint16)));
    Uint16 *t = imageFrame;             // target
    Uint16 *r = buf;                    // red plane
    Uint16 *g = buf + numPixels;        // green plane
    Uint16 *b = buf + (2 * numPixels);  // blue plane
    for (unsigned long i = numPixels; i; i--) {
      *t++ = *r++;
      *t++ = *g++;
      *t++ = *b++;
    }
    delete[] buf;
  } else
    return EC_MemoryExhausted;
  return EC_Normal;
}

OFCondition copyUint32ToUint8(ojph::ui32 **comps_data, int num_comps,
                              Uint8 *imageFrame, Uint16 columns, Uint16 rows) {
  if (imageFrame == NULL) return EC_IllegalCall;

  unsigned long numPixels = columns * rows;
  if (numPixels == 0) return EC_IllegalCall;

  Uint8 *t = imageFrame;          // target
  ojph::ui32 *g = comps_data[0];  // grey plane
  for (unsigned long i = numPixels; i; i--) {
    *t++ = (Uint8)(*g++);
  }

  return EC_Normal;
}

OFCondition copyUint32ToUint16(ojph::ui32 **comps_data, int num_comps,
                               Uint16 *imageFrame, Uint16 columns,
                               Uint16 rows) {
  if (imageFrame == NULL) return EC_IllegalCall;

  unsigned long numPixels = columns * rows;
  if (numPixels == 0) return EC_IllegalCall;

  Uint16 *t = imageFrame;         // target
  ojph::ui32 *g = comps_data[0];  // grey plane
  for (unsigned long i = numPixels; i; i--) {
    *t++ = (Uint16)(*g++);
  }

  return EC_Normal;
}

OFCondition copyRGBUint8ToRGBUint8(ojph::ui32 **comps_data, Uint8 *imageFrame,
                                   Uint16 columns, Uint16 rows) {
  if (imageFrame == NULL) return EC_IllegalCall;

  unsigned long numPixels = columns * rows;
  if (numPixels == 0) return EC_IllegalCall;

  Uint8 *t = imageFrame;          // target
  ojph::ui32 *r = comps_data[0];  // red plane
  ojph::ui32 *g = comps_data[1];  // green plane
  ojph::ui32 *b = comps_data[2];  // blue plane
  for (unsigned long i = numPixels; i; i--) {
    *t++ = (Uint8)(*r++);
    *t++ = (Uint8)(*g++);
    *t++ = (Uint8)(*b++);
  }

  return EC_Normal;
}

OFCondition copyRGBUint8ToRGBUint8Planar(ojph::ui32 **comps_data,
                                         Uint8 *imageFrame, Uint16 columns,
                                         Uint16 rows) {
  if (imageFrame == NULL) return EC_IllegalCall;

  unsigned long numPixels = columns * rows;
  if (numPixels == 0) return EC_IllegalCall;

  Uint8 *t = imageFrame;  // target
  for (unsigned long j = 0; j < 3; j++) {
    ojph::ui32 *r = comps_data[j];  // color plane
    for (unsigned long i = numPixels; i; i--) {
      *t++ = (Uint8)(*r++);
    }
  }
  return EC_Normal;
}
