#include "dcmtkhtj2k/djencode.h"

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmdata/dccodec.h" /* for DcmCodecStruct */
#include "dcmtkhtj2k/djcodece.h"
#include "dcmtkhtj2k/djcparam.h"

// initialization of static members
OFBool HtJ2kEncoderRegistration::registered_ = OFFalse;
HtJ2kCodecParameter *HtJ2kEncoderRegistration::cp_ = NULL;
Ht2kLosslessEncoder *HtJ2kEncoderRegistration::losslessencoder_ = NULL;
HtJ2kRPCLLosslessEncoder *HtJ2kEncoderRegistration::rpcllosslessencoder_ = NULL;
HtJ2kLossyEncoder *HtJ2kEncoderRegistration::lossyencoder_ = NULL;

void HtJ2kEncoderRegistration::registerCodecs(
    OFBool jp2k_optionsEnabled, Uint16 jp2k_decompositions,
    Uint16 jp2k_cblkwidth, Uint16 jp2k_cblkheight,
    J2K_ProgressionOrder jp2k_progressionOrder, OFBool preferCookedEncoding,
    Uint32 fragmentSize, OFBool createOffsetTable, J2K_UIDCreation uidCreation,
    OFBool convertToSC) {
  if (!registered_) {
    cp_ = new HtJ2kCodecParameter(jp2k_optionsEnabled, jp2k_decompositions,
                                  jp2k_cblkwidth, jp2k_cblkheight,
                                  jp2k_progressionOrder, preferCookedEncoding,
                                  fragmentSize, createOffsetTable, uidCreation,
                                  convertToSC, EJ2KPC_restore, OFFalse);

    if (cp_) {
      losslessencoder_ = new Ht2kLosslessEncoder();
      if (losslessencoder_)
        DcmCodecList::registerCodec(losslessencoder_, NULL, cp_);
      rpcllosslessencoder_ = new HtJ2kRPCLLosslessEncoder();
      if (rpcllosslessencoder_)
        DcmCodecList::registerCodec(rpcllosslessencoder_, NULL, cp_);
      lossyencoder_ = new HtJ2kLossyEncoder();
      if (lossyencoder_) DcmCodecList::registerCodec(lossyencoder_, NULL, cp_);
      registered_ = OFTrue;
    }
  }
}

void HtJ2kEncoderRegistration::cleanup() {
  if (registered_) {
    DcmCodecList::deregisterCodec(losslessencoder_);
    DcmCodecList::deregisterCodec(rpcllosslessencoder_);
    DcmCodecList::deregisterCodec(lossyencoder_);
    delete losslessencoder_;
    delete lossyencoder_;
    delete cp_;
    registered_ = OFFalse;
#ifdef DEBUG
    // not needed but useful for debugging purposes
    losslessencoder_ = NULL;
    rpcllosslessencoder_ = NULL;
    lossyencoder_ = NULL;
    cp_ = NULL;
#endif
  }
}

OFString HtJ2kEncoderRegistration::getLibraryVersionString() {
  return DCMTKHTJ2K_VERSION_STRING;
}
