#include "dcmtkhtj2k/djcodece.h"

#include "dcmtk/config/osconfig.h"

// ofstd includes
#include <cmath>

#include "dcmtk/ofstd/ofbmanip.h"
#include "dcmtk/ofstd/offile.h" /* for class OFFile */
#include "dcmtk/ofstd/oflist.h"
#include "dcmtk/ofstd/ofstd.h"
#include "dcmtk/ofstd/ofstdinc.h"
#include "dcmtk/ofstd/ofstream.h"

// dcmdata includes
#include "dcmtk/dcmdata/dcdatset.h" /* for class DcmDataset */
#include "dcmtk/dcmdata/dcdeftag.h" /* for tag constants */
#include "dcmtk/dcmdata/dcovlay.h"  /* for class DcmOverlayData */
#include "dcmtk/dcmdata/dcpixseq.h" /* for class DcmPixelSequence */
#include "dcmtk/dcmdata/dcpxitem.h" /* for class DcmPixelItem */
#include "dcmtk/dcmdata/dcswap.h"   /* for swapIfNecessary */
#include "dcmtk/dcmdata/dcuid.h"    /* for dcmGenerateUniqueIdentifer()*/
#include "dcmtk/dcmdata/dcvrcs.h"   /* for class DcmCodeString */
#include "dcmtk/dcmdata/dcvrds.h"   /* for class DcmDecimalString */
#include "dcmtk/dcmdata/dcvrlt.h"   /* for class DcmLongText */
#include "dcmtk/dcmdata/dcvrst.h"   /* for class DcmShortText */
#include "dcmtk/dcmdata/dcvrus.h"   /* for class DcmUnsignedShort */

// dcmhtj2k includes
#include "dcmtkhtj2k/djcparam.h" /* for class DJP2KCodecParameter */
#include "dcmtkhtj2k/djrparam.h" /* for class D2RepresentationParameter */

// dcmimgle includes
#include "dcmtk/dcmimgle/dcmimage.h" /* for class DicomImage */

// HT-J2K library (OpenJPH) includes
#include "openjph/ojph_arch.h"
#include "openjph/ojph_codestream.h"
#include "openjph/ojph_file.h"
#include "openjph/ojph_mem.h"
#include "openjph/ojph_params.h"
BEGIN_EXTERN_C
#ifdef HAVE_FCNTL_H
#include <fcntl.h> /* for O_RDONLY */
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h> /* required for sys/stat.h */
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h> /* for stat, fstat */
#endif
END_EXTERN_C

E_TransferSyntax Ht2kLosslessEncoder::supportedTransferSyntax() const {
  return EXS_HighThroughputJPEG2000LosslessOnly;
}

E_TransferSyntax HtJ2kRPCLLosslessEncoder::supportedTransferSyntax() const {
  return EXS_HighThroughputJPEG2000withRPCLOptionsLosslessOnly;
}

E_TransferSyntax HtJ2kLossyEncoder::supportedTransferSyntax() const {
  return EXS_HighThroughputJPEG2000;
}

// --------------------------------------------------------------------------

HtJ2kEncoderBase::HtJ2kEncoderBase() : DcmCodec() {}

HtJ2kEncoderBase::~HtJ2kEncoderBase() {}

OFBool HtJ2kEncoderBase::canChangeCoding(
    E_TransferSyntax const oldRepType,
    E_TransferSyntax const newRepType) const {
  // this codec only handles conversion from uncompressed to HT-J2K.
  DcmXfer oldRep(oldRepType);
  return (oldRep.getStreamCompression() == ESC_none &&
          (newRepType == supportedTransferSyntax()));
}

OFCondition HtJ2kEncoderBase::decode(
    DcmRepresentationParameter const * /* fromRepParam */,
    DcmPixelSequence * /* pixSeq */,
    DcmPolymorphOBOW & /* uncompressedPixelData */,
    DcmCodecParameter const * /* cp */, DcmStack const & /* objStack */) const {
  // we are an encoder only
  return EC_IllegalCall;
}

OFCondition HtJ2kEncoderBase::decode(
    DcmRepresentationParameter const *fromRepParam, DcmPixelSequence *pixSeq,
    DcmPolymorphOBOW &uncompressedPixelData, DcmCodecParameter const *cp,
    DcmStack const &objStack, OFBool &removeOldRep) const {
  return EC_IllegalCall;
}

OFCondition HtJ2kEncoderBase::decodeFrame(
    DcmRepresentationParameter const * /* fromParam */,
    DcmPixelSequence * /* fromPixSeq */, DcmCodecParameter const * /* cp */,
    DcmItem * /* dataset */, Uint32 /* frameNo */, Uint32 & /* startFragment */,
    void * /* buffer */, Uint32 /* bufSize */,
    OFString & /* decompressedColorModel */) const {
  // we are an encoder only
  return EC_IllegalCall;
}

OFCondition HtJ2kEncoderBase::encode(
    E_TransferSyntax const /* fromRepType */,
    DcmRepresentationParameter const * /* fromRepParam */,
    DcmPixelSequence * /* fromPixSeq */,
    DcmRepresentationParameter const * /* toRepParam */,
    DcmPixelSequence *& /* toPixSeq */, DcmCodecParameter const * /* cp */,
    DcmStack & /* objStack */) const {
  // we don't support re-coding for now.
  return EC_IllegalCall;
}

OFCondition HtJ2kEncoderBase::encode(
    E_TransferSyntax const fromRepType,
    DcmRepresentationParameter const *fromRepParam,
    DcmPixelSequence *fromPixSeq, DcmRepresentationParameter const *toRepParam,
    DcmPixelSequence *&toPixSeq, DcmCodecParameter const *cp,
    DcmStack &objStack, OFBool &removeOldRep) const {
  return EC_IllegalCall;
}

OFCondition HtJ2kEncoderBase::encode(
    Uint16 const *pixelData, Uint32 const length,
    DcmRepresentationParameter const *toRepParam, DcmPixelSequence *&pixSeq,
    DcmCodecParameter const *cp, DcmStack &objStack) const {
  OFCondition result = EC_Normal;
  HtJ2kRepresentationParameter defRep;

  // retrieve pointer to dataset from parameter stack
  DcmStack localStack(objStack);
  (void)localStack.pop();  // pop pixel data element from stack
  DcmObject *dobject =
      localStack.pop();  // this is the item in which the pixel data is located
  if ((!dobject) ||
      ((dobject->ident() != EVR_dataset) && (dobject->ident() != EVR_item)))
    return EC_InvalidTag;
  DcmItem *dataset = OFstatic_cast(DcmItem *, dobject);

  // assume we can cast the codec and representation parameters to what we need
  HtJ2kCodecParameter const *djcp =
      OFreinterpret_cast(HtJ2kCodecParameter const *, cp);
  HtJ2kRepresentationParameter const *djrp =
      OFreinterpret_cast(HtJ2kRepresentationParameter const *, toRepParam);
  double compressionRatio = 0.0;

  if (!djrp) djrp = &defRep;

  if (supportedTransferSyntax() == EXS_HighThroughputJPEG2000LosslessOnly ||
      supportedTransferSyntax() ==
          EXS_HighThroughputJPEG2000withRPCLOptionsLosslessOnly ||
      djrp->useLosslessProcess()) {
    if (djcp->cookedEncodingPreferred())
      result = RenderedEncode(pixelData, length, dataset, djrp, pixSeq, djcp,
                              compressionRatio);
    else
      result = losslessRawEncode(pixelData, length, dataset, djrp, pixSeq, djcp,
                                 compressionRatio);
  } else {
    // near-lossless mode always uses the "cooked" encoder since this one is
    // guaranteed not to "mix" overlays and pixel data in one cell subjected to
    // lossy compression.
    result = RenderedEncode(pixelData, length, dataset, djrp, pixSeq, djcp,
                            compressionRatio);
  }

  // the following operations do not affect the Image Pixel Module
  // but other modules such as SOP Common.  We only perform these
  // changes if we're on the main level of the dataset,
  // which should always identify itself as dataset, not as item.
  if (result.good() && dataset->ident() == EVR_dataset) {
    if (result.good()) {
      if (supportedTransferSyntax() == EXS_HighThroughputJPEG2000LosslessOnly ||
          supportedTransferSyntax() ==
              EXS_HighThroughputJPEG2000withRPCLOptionsLosslessOnly ||
          djrp->useLosslessProcess()) {
        // lossless process - create new UID if mode is EUC_always or if we're
        // converting to Secondary Capture
        if (djcp->getConvertToSC() || (djcp->getUIDCreation() == EJ2KUC_always))
          result = DcmCodec::newInstance(dataset, "DCM", "121320",
                                         "Uncompressed predecessor");
      } else {
        // lossy process - create new UID unless mode is EUC_never and we're not
        // converting to Secondary Capture
        if (djcp->getConvertToSC() || (djcp->getUIDCreation() != EJ2KUC_never))
          result = DcmCodec::newInstance(dataset, "DCM", "121320",
                                         "Uncompressed predecessor");

        // update image type
        if (result.good()) result = DcmCodec::updateImageType(dataset);

        // update derivation description
        if (result.good())
          result = updateDerivationDescription(dataset, djrp, compressionRatio);

        // update lossy compression ratio
        if (result.good())
          result = updateLossyCompressionRatio(dataset, compressionRatio);
      }
    }

    // convert to Secondary Capture if requested by user.
    // This method creates a new SOP class UID, so it should be executed
    // after the call to newInstance() which creates a Source Image Sequence.
    if (result.good() && djcp->getConvertToSC())
      result = DcmCodec::convertToSecondaryCapture(dataset);
  }

  return result;
}

OFCondition HtJ2kEncoderBase::encode(
    Uint16 const *pixelData, Uint32 const length,
    DcmRepresentationParameter const *toRepParam, DcmPixelSequence *&pixSeq,
    DcmCodecParameter const *cp, DcmStack &objStack,
    OFBool &removeOldRep) const {
  // removeOldRep is left as it is, pixel data in original DICOM dataset is not
  // modified
  return encode(pixelData, length, toRepParam, pixSeq, cp, objStack);
}

OFCondition HtJ2kEncoderBase::determineDecompressedColorModel(
    DcmRepresentationParameter const * /* fromParam */,
    DcmPixelSequence * /* fromPixSeq */, DcmCodecParameter const * /* cp */,
    DcmItem * /* dataset */, OFString & /* decompressedColorModel */) const {
  return EC_IllegalCall;
}

OFCondition HtJ2kEncoderBase::adjustOverlays(DcmItem *dataset,
                                             DicomImage &image) const {
  if (dataset == NULL) return EC_IllegalCall;

  unsigned int overlayCount = image.getOverlayCount();
  if (overlayCount > 0) {
    Uint16 group = 0;
    DcmStack stack;
    unsigned long bytesAllocated = 0;
    Uint8 *buffer = NULL;
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned long frames = 0;
    DcmElement *elem = NULL;
    OFCondition result = EC_Normal;

    // adjust overlays (prior to grayscale compression)
    for (unsigned int i = 0; i < overlayCount; i++) {
      // check if current overlay is embedded in pixel data
      group = OFstatic_cast(Uint16, image.getOverlayGroupNumber(i));
      stack.clear();
      if ((dataset->search(DcmTagKey(group, 0x3000), stack, ESM_fromHere,
                           OFFalse))
              .bad()) {
        // separate Overlay Data not found. Assume overlay is embedded.
        bytesAllocated =
            image.create6xxx3000OverlayData(buffer, i, width, height, frames);
        if (bytesAllocated > 0) {
          elem =
              new DcmOverlayData(DcmTagKey(group, 0x3000));  // DCM_OverlayData
          if (elem) {
            result = elem->putUint8Array(buffer, bytesAllocated);
            delete[] buffer;
            if (result.good()) {
              dataset->insert(elem, OFTrue /*replaceOld*/);
              // DCM_OverlayBitsAllocated
              result = dataset->putAndInsertUint16(DcmTagKey(group, 0x0100), 1);
              // DCM_OverlayBitPosition
              if (result.good())
                result =
                    dataset->putAndInsertUint16(DcmTagKey(group, 0x0102), 0);
            } else {
              delete elem;
              return result;
            }
          } else {
            delete[] buffer;
            return EC_MemoryExhausted;
          }
        } else
          return EC_IllegalCall;
      }
    }
  }
  return EC_Normal;
}

OFCondition HtJ2kEncoderBase::updateLossyCompressionRatio(DcmItem *dataset,
                                                          double ratio) const {
  if (dataset == NULL) return EC_IllegalCall;

  // set Lossy Image Compression to "01" (see DICOM part 3, C.7.6.1.1.5)
  OFCondition result =
      dataset->putAndInsertString(DCM_LossyImageCompression, "01");
  if (result.bad()) return result;

  // set Lossy Image Compression Ratio
  OFString s;
  char const *oldRatio = NULL;
  if ((dataset->findAndGetString(DCM_LossyImageCompressionRatio, oldRatio))
          .good() &&
      oldRatio) {
    s = oldRatio;
    s += "\\";
  }

  // append lossy compression ratio
  char buf[64];
  OFStandard::ftoa(buf, sizeof(buf), ratio, OFStandard::ftoa_uppercase, 0, 5);
  s += buf;

  result =
      dataset->putAndInsertString(DCM_LossyImageCompressionRatio, s.c_str());
  if (result.bad()) return result;

  // count VM of lossy image compression ratio
  size_t i;
  size_t s_vm = 0;
  size_t s_sz = s.size();
  for (i = 0; i < s_sz; ++i)
    if (s[i] == '\\') ++s_vm;

  // set Lossy Image Compression Method
  char const *oldMethod = NULL;
  OFString m;
  if ((dataset->findAndGetString(DCM_LossyImageCompressionMethod, oldMethod))
          .good() &&
      oldMethod) {
    m = oldMethod;
    m += "\\";
  }

  // count VM of lossy image compression method
  size_t m_vm = 0;
  size_t m_sz = m.size();
  for (i = 0; i < m_sz; ++i)
    if (m[i] == '\\') ++m_vm;

  // make sure that VM of Compression Method is not smaller than  VM of
  // Compression Ratio
  while (m_vm++ < s_vm) m += "\\";

  m += "ISO_14495_1";
  return dataset->putAndInsertString(DCM_LossyImageCompressionMethod,
                                     m.c_str());
}

OFCondition HtJ2kEncoderBase::updateDerivationDescription(
    DcmItem *dataset, HtJ2kRepresentationParameter const *djrp,
    double ratio) const {
  OFString derivationDescription;
  char buf[64];

  derivationDescription = "near lossless HT-J2K compression, factor ";
  OFStandard::ftoa(buf, sizeof(buf), ratio, OFStandard::ftoa_uppercase, 0, 5);
  derivationDescription += buf;

  // append old Derivation Description, if any
  char const *oldDerivation = NULL;
  if ((dataset->findAndGetString(DCM_DerivationDescription, oldDerivation))
          .good() &&
      oldDerivation) {
    derivationDescription += " [";
    derivationDescription += oldDerivation;
    derivationDescription += "]";
    if (derivationDescription.length() > 1024) {
      // ST is limited to 1024 characters, cut off tail
      derivationDescription.erase(1020);
      derivationDescription += "...]";
    }
  }

  OFCondition result = dataset->putAndInsertString(
      DCM_DerivationDescription, derivationDescription.c_str());
  if (result.good())
    result = DcmCodec::insertCodeSequence(dataset, DCM_DerivationCodeSequence,
                                          "DCM", "113040", "Lossy Compression");
  return result;
}

OFCondition HtJ2kEncoderBase::losslessRawEncode(
    Uint16 const *pixelData, Uint32 const length, DcmItem *dataset,
    HtJ2kRepresentationParameter const *djrp, DcmPixelSequence *&pixSeq,
    HtJ2kCodecParameter const *djcp, double &compressionRatio) const {
  compressionRatio = 0.0;  // initialize if something goes wrong

  // determine image properties
  Uint16 bitsAllocated = 0;
  Uint16 bitsStored = 0;
  Uint16 bytesAllocated = 0;
  Uint16 samplesPerPixel = 0;
  Uint16 planarConfiguration = 0;
  Uint16 columns = 0;
  Uint16 rows = 0;
  Sint32 numberOfFrames = 1;
  OFBool byteSwapped =
      OFFalse;  // true if we have byte-swapped the original pixel data
  OFString photometricInterpretation;
  Uint16 pixelRepresentation = 0;
  OFCondition result =
      dataset->findAndGetUint16(DCM_BitsAllocated, bitsAllocated);
  if (result.good())
    result = dataset->findAndGetUint16(DCM_BitsStored, bitsStored);
  if (result.good())
    result = dataset->findAndGetUint16(DCM_SamplesPerPixel, samplesPerPixel);
  if (result.good()) result = dataset->findAndGetUint16(DCM_Columns, columns);
  if (result.good()) result = dataset->findAndGetUint16(DCM_Rows, rows);
  if (result.good())
    dataset->findAndGetUint16(DCM_PixelRepresentation, pixelRepresentation);
  if (result.good())
    result = dataset->findAndGetOFString(DCM_PhotometricInterpretation,
                                         photometricInterpretation);
  if (result.good()) {
    result = dataset->findAndGetSint32(DCM_NumberOfFrames, numberOfFrames);
    if (result.bad() || numberOfFrames < 1) numberOfFrames = 1;
    result = EC_Normal;
  }
  if (result.good() && (samplesPerPixel > 1)) {
    result =
        dataset->findAndGetUint16(DCM_PlanarConfiguration, planarConfiguration);
  }

  if (result.good()) {
    // check if bitsAllocated is 8 or 16 or 32 - we don't handle anything else
    if (bitsAllocated == 8) {
      bytesAllocated = 1;
    } else if (bitsAllocated == 16) {
      bytesAllocated = 2;
    } else if (bitsAllocated == 32) {
      bytesAllocated = 4;
    } else {
      if (photometricInterpretation == "MONOCHROME1" ||
          photometricInterpretation == "MONOCHROME2" ||
          photometricInterpretation == "RGB" ||
          photometricInterpretation == "YBR_FULL") {
        // A bitsAllocated value that we don't handle, but a color model that
        // indicates that the cooked encoder could handle this case. Fall back
        // to cooked encoder.
        return RenderedEncode(pixelData, length, dataset, djrp, pixSeq, djcp,
                              compressionRatio);
      }

      // an image that is not supported by either the raw or the cooked encoder.
      result = EC_J2KUnsupportedImageType;
    }

    // make sure that all the descriptive attributes have sensible values
    if ((columns < 1) || (rows < 1) || (samplesPerPixel < 1))
      result = EC_J2KUnsupportedImageType;

    // make sure that we have at least as many bytes of pixel data as we expect
    if (bytesAllocated * samplesPerPixel * columns * rows *
            OFstatic_cast(unsigned long, numberOfFrames) >
        length)
      result = EC_J2KUncompressedBufferTooSmall;
  }

  DcmPixelSequence *pixelSequence = NULL;
  DcmPixelItem *offsetTable = NULL;

  // create initial pixel sequence
  if (result.good()) {
    pixelSequence = new DcmPixelSequence(DcmTag(DCM_PixelData, EVR_OB));
    if (pixelSequence == NULL)
      result = EC_MemoryExhausted;
    else {
      // create empty offset table
      offsetTable = new DcmPixelItem(DcmTag(DCM_Item, EVR_OB));
      if (offsetTable == NULL)
        result = EC_MemoryExhausted;
      else
        pixelSequence->insert(offsetTable);
    }
  }

  DcmOffsetList offsetList;
  unsigned long compressedSize = 0;
  unsigned long compressedFrameSize = 0;
  double uncompressedSize = 0.0;

  // render and compress each frame
  if (result.good()) {
    // byte swap pixel data to little endian if bits allocate is 8
    if ((gLocalByteOrder == EBO_BigEndian) && (bitsAllocated == 8)) {
      swapIfNecessary(EBO_LittleEndian, gLocalByteOrder,
                      OFstatic_cast(void *, OFconst_cast(Uint16 *, pixelData)),
                      length, sizeof(Uint16));
      byteSwapped = OFTrue;
    }

    unsigned long frameCount = OFstatic_cast(unsigned long, numberOfFrames);
    unsigned long frameSize = columns * rows * samplesPerPixel * bytesAllocated;
    Uint8 const *framePointer = OFreinterpret_cast(Uint8 const *, pixelData);

    // compute original image size in bytes, ignoring any padding bits.
    uncompressedSize =
        columns * rows * samplesPerPixel * bitsStored * frameCount / 8.0;

    for (unsigned long i = 0; (i < frameCount) && (result.good()); ++i) {
      // compress frame
      DCMTKHTJ2K_DEBUG("HT-J2K encoder processes frame " << (i + 1) << " of "
                                                         << frameCount);
      result = compressRawFrame(
          framePointer, bitsAllocated, columns, rows, samplesPerPixel,
          planarConfiguration, pixelRepresentation, photometricInterpretation,
          pixelSequence, offsetList, compressedFrameSize, djcp, djrp);

      compressedSize += compressedFrameSize;
      framePointer += frameSize;
    }
  }

  // store pixel sequence if everything went well.
  if (result.good())
    pixSeq = pixelSequence;
  else {
    delete pixelSequence;
    pixSeq = NULL;
  }

  // create offset table
  if ((result.good()) && (djcp->getCreateOffsetTable())) {
    result = offsetTable->createOffsetTable(offsetList);
  }

  if (compressedSize > 0) compressionRatio = uncompressedSize / compressedSize;

  // byte swap pixel data back to local endian if necessary
  if (byteSwapped) {
    swapIfNecessary(gLocalByteOrder, EBO_LittleEndian,
                    OFstatic_cast(void *, OFconst_cast(Uint16 *, pixelData)),
                    length, sizeof(Uint16));
  }

  return result;
}

OFCondition HtJ2kEncoderBase::compressRawFrame(
    Uint8 const *framePointer, Uint16 bitsAllocated, Uint16 width,
    Uint16 height, Uint16 samplesPerPixel, Uint16 planarConfiguration,
    OFBool pixelRepresentation, OFString const &photometricInterpretation,
    DcmPixelSequence *pixelSequence, DcmOffsetList &offsetList,
    unsigned long &compressedSize, HtJ2kCodecParameter const *djcp,
    HtJ2kRepresentationParameter const *djrp) const {
  OFCondition result = EC_Normal;
  Uint16 bytesAllocated = bitsAllocated / 8;
  Uint32 frameSize = width * height * bytesAllocated * samplesPerPixel;
  Uint32 fragmentSize = djcp->getFragmentSize();

  try {
    ojph::codestream codestream;
    ojph::mem_outfile destinationBuffer;

    auto const colorTransform = samplesPerPixel > 1 ? true : false;
    codestream.set_planar(colorTransform == false);
    codestream.set_tilepart_divisions(true, false);
    codestream.request_tlm_marker(true);

    auto siz = codestream.access_siz();
    siz.set_image_extent(ojph::point(width, height));
    siz.set_num_components(samplesPerPixel);
    for (auto c = 0u; c < samplesPerPixel; c++) {
      siz.set_component(c, ojph::point(1, 1), bitsAllocated,
                        pixelRepresentation == 1 ? true : false);
    }
    siz.set_image_offset(ojph::point(0, 0));
    siz.set_tile_size(ojph::size(0, 0));
    siz.set_tile_offset(ojph::point(0, 0));

    auto cod = codestream.access_cod();

    std::string progressionOrder = "LRCP";
    if (djcp->getUseCustomOptions()) {
      J2K_ProgressionOrder po = djcp->get_progressionOrder();
      if (po == J2K_ProgressionOrder::EJ2KPO_LRCP) {
        progressionOrder = "LRCP";
      } else if (po == J2K_ProgressionOrder::EJ2KPO_RLCP) {
        progressionOrder = "RLCP";
      } else if (po == J2K_ProgressionOrder::EJ2KPO_RPCL) {
        progressionOrder = "RPCL";
      } else if (po == J2K_ProgressionOrder::EJ2KPO_PCRL) {
        progressionOrder = "PCRL";
      } else if (po == J2K_ProgressionOrder::EJ2KPO_CPRL) {
        progressionOrder = "CPRL";
      }
    }
    if (supportedTransferSyntax() ==
        EXS_HighThroughputJPEG2000withRPCLOptionsLosslessOnly) {
      progressionOrder = "RPCL";
    }

    cod.set_progression_order(progressionOrder.c_str());
    cod.set_color_transform(colorTransform);
    if (djcp->getUseCustomOptions()) {
      cod.set_block_dims(djcp->get_cblkwidth(), djcp->get_cblkheight());
    }
    cod.set_precinct_size(0, nullptr);
    cod.set_reversible(djrp->useLosslessProcess());

    auto numberOfDecompositions = 0u;
    size_t tw = width;
    size_t th = height;
    while (tw > 64 && th > 64) {
      numberOfDecompositions++;
      tw = static_cast<size_t>(ceil(tw / 2));
      th = static_cast<size_t>(ceil(th / 2));
    }
    cod.set_num_decomposition(
        numberOfDecompositions > 6 ? 6 : numberOfDecompositions);
    if (djcp->getUseCustomOptions()) {
      cod.set_num_decomposition(djcp->get_decompositions());
    }

    destinationBuffer.open();

    ojph::comment_exchange com_ex;
    codestream.write_headers(&destinationBuffer, &com_ex, 0);

    ojph::ui32 next_comp;
    auto const bytesPerPixel = bitsAllocated / 8;
    auto *cur_line = codestream.exchange(nullptr, next_comp);
    auto const height = siz.get_image_extent().y - siz.get_image_offset().y;
    for (auto y = 0u; y < height; y++) {
      for (auto c = 0u; c < siz.get_num_components(); c++) {
        auto dp = cur_line->i32;
        if (bitsAllocated <= 8) {
          auto sp = (Uint8 *)&framePointer[(y * width * bytesPerPixel *
                                            siz.get_num_components()) +
                                           c];
          for (auto x = 0; x < width; x++) {
            *dp++ = *sp;
            sp += siz.get_num_components();
          }
        } else {
          if (pixelRepresentation == 1) {
            auto sp = (Sint16 *)&framePointer[y * width * bytesPerPixel];
            for (auto x = 0; x < width; x++) {
              *dp++ = *sp++;
            }
          } else {
            auto sp = (Uint16 *)&framePointer[y * width * bytesPerPixel];
            for (auto x = 0; x < width; x++) {
              *dp++ = *sp++;
            }
          }
        }
        cur_line = codestream.exchange(cur_line, next_comp);
      }
    }

    codestream.flush();

    // Get compressed data from mem_outfile
    Uint8 *compressed_data =
        const_cast<Uint8 *>((Uint8 const *)destinationBuffer.get_data());

    // Store compressed frame
    auto compressedLen = (unsigned long)destinationBuffer.tell();
    compressedSize = compressedLen;
    result = pixelSequence->storeCompressedFrame(offsetList, compressed_data,
                                                 compressedLen, fragmentSize);

    codestream.close();
  } catch (std::exception &ex) {
    DCMTKHTJ2K_ERROR("HT-J2K encoder caught OpenJPH exception: "
                     << (ex.what() ? ex.what() : "Unknown reason"));
    result =
        makeOFCondition(1, OFM_dcmjp2k, OF_error,
                        ex.what() ? ex.what() : "Unknown OpenJPH exception");
  }

  return result;
}

OFCondition HtJ2kEncoderBase::RenderedEncode(
    Uint16 const *pixelData, Uint32 const length, DcmItem *dataset,
    HtJ2kRepresentationParameter const *djrp, DcmPixelSequence *&pixSeq,
    HtJ2kCodecParameter const *djcp, double &compressionRatio) const {
  compressionRatio = 0.0;  // initialize if something goes wrong

  // determine a few image properties
  OFString photometricInterpretation;
  Uint16 bitsAllocated = 0;
  OFCondition result = dataset->findAndGetOFString(
      DCM_PhotometricInterpretation, photometricInterpretation);
  if (result.good())
    result = dataset->findAndGetUint16(DCM_BitsAllocated, bitsAllocated);
  if (result.bad()) return result;

  // The cooked encoder only handles the following photometic interpretations
  if (photometricInterpretation != "MONOCHROME1" &&
      photometricInterpretation != "MONOCHROME2" &&
      photometricInterpretation != "RGB" &&
      photometricInterpretation != "YBR_FULL") {
    // a photometric interpretation that we don't handle. Fall back to raw
    // encoder (unless in near-lossless mode)
    return losslessRawEncode(pixelData, length, dataset, djrp, pixSeq, djcp,
                             compressionRatio);
  }

  Uint16 pixelRepresentation = 0;
  result =
      dataset->findAndGetUint16(DCM_PixelRepresentation, pixelRepresentation);
  if (result.bad()) return result;

  DcmPixelSequence *pixelSequence = NULL;
  DcmPixelItem *offsetTable = NULL;

  // ignore modality transformation (rescale slope/intercept or LUT) stored in
  // the dataset
  unsigned long flags = CIF_IgnoreModalityTransformation;
  // don't convert YCbCr (Full and Full 4:2:2) color images to RGB
  flags |= CIF_KeepYCbCrColorModel;
  // Don't optimize memory usage, but keep using the same bitsAllocated.
  // Without this, the DICOM and the HT-J2K value for bitsAllocated could
  // differ and the decoder would error out.
  flags |= CIF_UseAbsolutePixelRange;

  DicomImage *dimage = new DicomImage(dataset, EXS_LittleEndianImplicit,
                                      flags);  // read all frames
  if (dimage == NULL) return EC_MemoryExhausted;
  if (dimage->getStatus() != EIS_Normal) {
    delete dimage;
    return EC_IllegalCall;
  }

  // create overlay data for embedded overlays
  result = adjustOverlays(dataset, *dimage);

  // determine number of bits per sample
  int bitsPerSample = dimage->getDepth();
  if (result.good() && (bitsPerSample > 16)) result = EC_J2KUnsupportedBitDepth;

  // create initial pixel sequence
  if (result.good()) {
    pixelSequence = new DcmPixelSequence(DcmTag(DCM_PixelData, EVR_OB));
    if (pixelSequence == NULL)
      result = EC_MemoryExhausted;
    else {
      // create empty offset table
      offsetTable = new DcmPixelItem(DcmTag(DCM_Item, EVR_OB));
      if (offsetTable == NULL)
        result = EC_MemoryExhausted;
      else
        pixelSequence->insert(offsetTable);
    }
  }

  DcmOffsetList offsetList;
  unsigned long compressedSize = 0;
  unsigned long compressedFrameSize = 0;
  double uncompressedSize = 0.0;

  // render and compress each frame
  if (result.good()) {
    unsigned long frameCount = dimage->getFrameCount();

    // compute original image size in bytes, ignoring any padding bits.
    Uint16 samplesPerPixel = 0;
    if ((dataset->findAndGetUint16(DCM_SamplesPerPixel, samplesPerPixel)).bad())
      samplesPerPixel = 1;
    uncompressedSize = dimage->getWidth() * dimage->getHeight() *
                       bitsPerSample * frameCount * samplesPerPixel / 8.0;

    for (unsigned long i = 0; (i < frameCount) && (result.good()); ++i) {
      // compress frame
      DCMTKHTJ2K_DEBUG("HT-J2K encoder processes frame " << (i + 1) << " of "
                                                         << frameCount);
      result = compressRenderedFrame(pixelSequence, dimage,
                                     photometricInterpretation, offsetList,
                                     compressedFrameSize, djcp, i, djrp);

      compressedSize += compressedFrameSize;
    }
  }

  // store pixel sequence if everything went well.
  if (result.good())
    pixSeq = pixelSequence;
  else {
    delete pixelSequence;
    pixSeq = NULL;
  }

  // create offset table
  if ((result.good()) && (djcp->getCreateOffsetTable())) {
    result = offsetTable->createOffsetTable(offsetList);
  }

  // adapt attributes in image pixel module
  if (result.good()) {
    // adjustments needed for both color and monochrome
    if (bitsPerSample > 8)
      result = dataset->putAndInsertUint16(DCM_BitsAllocated, 16);
    else
      result = dataset->putAndInsertUint16(DCM_BitsAllocated, 8);
    if (result.good())
      result = dataset->putAndInsertUint16(DCM_BitsStored, bitsPerSample);
    if (result.good())
      result = dataset->putAndInsertUint16(DCM_HighBit, bitsPerSample - 1);
  }

  if (compressedSize > 0) compressionRatio = uncompressedSize / compressedSize;
  delete dimage;
  return result;
}

OFCondition HtJ2kEncoderBase::compressRenderedFrame(
    DcmPixelSequence *pixelSequence, DicomImage *dimage,
    OFString const &photometricInterpretation, DcmOffsetList &offsetList,
    unsigned long &compressedSize, HtJ2kCodecParameter const *djcp,
    Uint32 frame, HtJ2kRepresentationParameter const *djrp) const {
  if (dimage == NULL) return EC_IllegalCall;

  // access essential image parameters
  int width = dimage->getWidth();
  int height = dimage->getHeight();
  int depth = dimage->getDepth();
  if ((depth < 1) || (depth > 16)) return EC_J2KUnsupportedBitDepth;

  Uint32 fragmentSize = djcp->getFragmentSize();

  DiPixel const *dinter = dimage->getInterData();
  if (dinter == NULL) return EC_IllegalCall;

  // There should be no other possibilities
  int samplesPerPixel = dinter->getPlanes();
  if (samplesPerPixel != 1 && samplesPerPixel != 3) return EC_IllegalCall;

  // get pointer to internal raw representation of image data
  void const *draw = dinter->getData();
  if (draw == NULL) return EC_IllegalCall;

  OFCondition result = EC_Normal;

  void const *planes[3] = {NULL, NULL, NULL};
  if (samplesPerPixel == 3) {
    // for color images, dinter->getData() returns a pointer to an array
    // of pointers pointing to the real plane data
    void const *const *draw_array = OFstatic_cast(void const *const *, draw);
    planes[0] = draw_array[0];
    planes[1] = draw_array[1];
    planes[2] = draw_array[2];
  } else {
    // for monochrome images, dinter->getData() directly returns a pointer
    // to the single monochrome plane.
    planes[0] = draw;
  }

  // This is the buffer with the uncompressed pixel data
  Uint8 *buffer;
  size_t buffer_size;

  Uint32 framesize = dimage->getWidth() * dimage->getHeight();
  switch (dinter->getRepresentation()) {
    case EPR_Uint8:
    case EPR_Sint8: {
      // image representation is 8 bit signed or unsigned
      if (samplesPerPixel == 1) {
        Uint8 const *yv =
            OFreinterpret_cast(Uint8 const *, planes[0]) + framesize * frame;
        buffer_size = framesize;
        buffer = new Uint8[buffer_size];
        memcpy(buffer, yv, framesize);
      } else {
        Uint8 const *rv =
            OFreinterpret_cast(Uint8 const *, planes[0]) + framesize * frame;
        Uint8 const *gv =
            OFreinterpret_cast(Uint8 const *, planes[1]) + framesize * frame;
        Uint8 const *bv =
            OFreinterpret_cast(Uint8 const *, planes[2]) + framesize * frame;

        buffer_size = framesize * 3;
        buffer = new Uint8[buffer_size];

        size_t i = 0;
        for (int row = height; row; --row) {
          for (int col = width; col; --col) {
            buffer[i++] = *rv;
            buffer[i++] = *gv;
            buffer[i++] = *bv;

            rv++;
            gv++;
            bv++;
          }
        }
      }
    } break;
    case EPR_Uint16:
    case EPR_Sint16: {
      // image representation is 16 bit signed or unsigned
      if (samplesPerPixel == 1) {
        Uint16 const *yv =
            OFreinterpret_cast(Uint16 const *, planes[0]) + framesize * frame;
        buffer_size = framesize * sizeof(Uint16);
        buffer = new Uint8[buffer_size];
        memcpy(buffer, yv, buffer_size);
      } else {
        Uint16 const *rv =
            OFreinterpret_cast(Uint16 const *, planes[0]) + framesize * frame;
        Uint16 const *gv =
            OFreinterpret_cast(Uint16 const *, planes[1]) + framesize * frame;
        Uint16 const *bv =
            OFreinterpret_cast(Uint16 const *, planes[2]) + framesize * frame;

        buffer_size = framesize * 3;
        Uint16 *buffer16 = new Uint16[buffer_size];
        buffer = OFreinterpret_cast(Uint8 *, buffer16);

        // Convert to byte count
        buffer_size *= 2;

        size_t i = 0;
        for (int row = height; row; --row) {
          for (int col = width; col; --col) {
            buffer16[i++] = *rv;
            buffer16[i++] = *gv;
            buffer16[i++] = *bv;

            rv++;
            gv++;
            bv++;
          }
        }
      }
    } break;
    default:
      // we don't support images with > 16 bits/sample
      return EC_J2KUnsupportedBitDepth;
      break;
  }

  int bitsAllocated = 8;
  int pixelRepresentation = 0;
  switch (dinter->getRepresentation()) {
    case EPR_Uint8:
      bitsAllocated = 8;
      pixelRepresentation = 0;
      break;
    case EPR_Sint8:
      bitsAllocated = 8;
      pixelRepresentation = 1;
      break;
    case EPR_Uint16:
      bitsAllocated = 16;
      pixelRepresentation = 0;
      break;
    case EPR_Sint16:
      bitsAllocated = 16;
      pixelRepresentation = 1;
      break;
    case EPR_Uint32:
    case EPR_Sint32:
      // 32-bit images not supported
      break;
  }

  try {
    ojph::codestream codestream;
    ojph::mem_outfile destinationBuffer;

    auto const colorTransform = samplesPerPixel > 1 ? true : false;
    codestream.set_planar(colorTransform == false);
    codestream.set_tilepart_divisions(true, false);
    codestream.request_tlm_marker(true);

    auto siz = codestream.access_siz();
    siz.set_image_extent(ojph::point(width, height));
    siz.set_num_components(samplesPerPixel);
    for (auto c = 0; c < samplesPerPixel; c++) {
      siz.set_component(c, ojph::point(1, 1), bitsAllocated,
                        pixelRepresentation == 1 ? true : false);
    }
    siz.set_image_offset(ojph::point(0, 0));
    siz.set_tile_size(ojph::size(0, 0));
    siz.set_tile_offset(ojph::point(0, 0));

    auto cod = codestream.access_cod();

    std::string progressionOrder = "LRCP";
    if (djcp->getUseCustomOptions()) {
      J2K_ProgressionOrder po = djcp->get_progressionOrder();
      if (po == J2K_ProgressionOrder::EJ2KPO_LRCP) {
        progressionOrder = "LRCP";
      } else if (po == J2K_ProgressionOrder::EJ2KPO_RLCP) {
        progressionOrder = "RLCP";
      } else if (po == J2K_ProgressionOrder::EJ2KPO_RPCL) {
        progressionOrder = "RPCL";
      } else if (po == J2K_ProgressionOrder::EJ2KPO_PCRL) {
        progressionOrder = "PCRL";
      } else if (po == J2K_ProgressionOrder::EJ2KPO_CPRL) {
        progressionOrder = "CPRL";
      }
    }
    if (supportedTransferSyntax() ==
        EXS_HighThroughputJPEG2000withRPCLOptionsLosslessOnly) {
      progressionOrder = "RPCL";
    }

    cod.set_progression_order(progressionOrder.c_str());
    cod.set_color_transform(colorTransform);
    if (djcp->getUseCustomOptions()) {
      cod.set_block_dims(djcp->get_cblkwidth(), djcp->get_cblkheight());
    }
    cod.set_precinct_size(0, nullptr);
    cod.set_reversible(djrp->useLosslessProcess());

    auto numberOfDecompositions = 0u;
    size_t tw = width;
    size_t th = height;
    while (tw > 64 && th > 64) {
      numberOfDecompositions++;
      tw = static_cast<size_t>(ceil(tw / 2));
      th = static_cast<size_t>(ceil(th / 2));
    }
    cod.set_num_decomposition(
        numberOfDecompositions > 6 ? 6 : numberOfDecompositions);
    if (djcp->getUseCustomOptions()) {
      cod.set_num_decomposition(djcp->get_decompositions());
    }

    destinationBuffer.open();

    ojph::comment_exchange com_ex;
    codestream.write_headers(&destinationBuffer, &com_ex, 0);

    ojph::ui32 next_comp;
    auto const bytesPerPixel = bitsAllocated / 8;
    auto *cur_line = codestream.exchange(nullptr, next_comp);
    auto const height = siz.get_image_extent().y - siz.get_image_offset().y;
    for (auto y = 0u; y < height; y++) {
      for (unsigned c = 0; c < siz.get_num_components(); c++) {
        auto dp = cur_line->i32;
        if (bitsAllocated <= 8) {
          auto sp = (Uint8 *)&buffer[(y * width * bytesPerPixel *
                                      siz.get_num_components()) +
                                     c];
          for (auto x = 0; x < width; x++) {
            *dp++ = *sp;
            sp += siz.get_num_components();
          }
        } else {
          if (pixelRepresentation == 1) {
            auto sp = (Sint16 *)&buffer[y * width * bytesPerPixel];
            for (auto x = 0; x < width; x++) {
              *dp++ = *sp++;
            }
          } else {
            auto sp = (Uint16 *)&buffer[y * width * bytesPerPixel];
            for (auto x = 0; x < width; x++) {
              *dp++ = *sp++;
            }
          }
        }
        cur_line = codestream.exchange(cur_line, next_comp);
      }
    }

    codestream.flush();

    // Get compressed data from mem_outfile
    Uint8 *compressed_data =
        const_cast<Uint8 *>((Uint8 const *)destinationBuffer.get_data());

    // Store compressed frame
    auto compressedLen = (unsigned long)destinationBuffer.tell();
    compressedSize = compressedLen;
    result = pixelSequence->storeCompressedFrame(offsetList, compressed_data,
                                                 compressedLen, fragmentSize);

    codestream.close();
  } catch (std::exception &ex) {
    DCMTKHTJ2K_ERROR("HT-J2K encoder caught OpenJPH exception: "
                     << (ex.what() ? ex.what() : "Unknown reason"));
    result =
        makeOFCondition(1, OFM_dcmjp2k, OF_error,
                        ex.what() ? ex.what() : "Unknown OpenJPH exception");
  }

  delete[] buffer;

  return result;
}

OFCondition HtJ2kEncoderBase::convertToUninterleaved(
    Uint8 *target, Uint8 const *source, Uint16 components, Uint32 width,
    Uint32 height, Uint16 bitsAllocated) const {
  Uint8 bytesAllocated = bitsAllocated / 8;
  Uint32 planeSize = width * height * bytesAllocated;

  if (bitsAllocated % 8 != 0) return EC_IllegalCall;

  for (Uint32 pos = 0; pos < width * height; pos++) {
    for (int i = 0; i < components; i++) {
      memcpy(&target[i * planeSize + pos * bytesAllocated], source,
             bytesAllocated);
      source += bytesAllocated;
    }
  }
  return EC_Normal;
}
