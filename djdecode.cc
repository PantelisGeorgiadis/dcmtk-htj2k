#include "dcmtkhtj2k/djdecode.h"

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmdata/dccodec.h" /* for DcmCodecStruct */
#include "dcmtkhtj2k/djcodecd.h"
#include "dcmtkhtj2k/djcparam.h"

// initialization of static members
OFBool HtJ2kDecoderRegistration::registered_ = OFFalse;
HtJ2kCodecParameter *HtJ2kDecoderRegistration::cp_ = NULL;
HtJ2kDecoder *HtJ2kDecoderRegistration::decoder_ = NULL;

void HtJ2kDecoderRegistration::registerCodecs(
    J2K_UIDCreation uidcreation, J2K_PlanarConfiguration planarconfig,
    OFBool ignoreOffsetTable) {
  if (!registered_) {
    cp_ = new HtJ2kCodecParameter(uidcreation, planarconfig, ignoreOffsetTable);
    if (cp_) {
      decoder_ = new HtJ2kDecoder();
      if (decoder_) DcmCodecList::registerCodec(decoder_, NULL, cp_);

      registered_ = OFTrue;
    }
  }
}

void HtJ2kDecoderRegistration::cleanup() {
  if (registered_) {
    DcmCodecList::deregisterCodec(decoder_);
    delete decoder_;
    delete cp_;
    registered_ = OFFalse;
#ifdef DEBUG
    // not needed but useful for debugging purposes
    decoder_ = NULL;
    cp_ = NULL;
#endif
  }
}

OFString HtJ2kDecoderRegistration::getLibraryVersionString() {
  return DCMTKHTJ2K_VERSION_STRING;
}
