#include "dcmtkhtj2k/djcparam.h"

#include "dcmtk/config/osconfig.h"
#include "dcmtk/ofstd/ofstd.h"

HtJ2kCodecParameter::HtJ2kCodecParameter(
    OFBool jp2k_optionsEnabled, Uint16 jp2k_decompositions,
    Uint16 jp2k_cblkwidth, Uint16 jp2k_cblkheight,
    HTJ2K_ProgressionOrder jp2k_progressionOrder, OFBool preferCookedEncoding,
    Uint32 fragmentSize, OFBool createOffsetTable,
    HTJ2K_UIDCreation uidCreation, OFBool convertToSC,
    HTJ2K_PlanarConfiguration planarConfiguration, OFBool ignoreOffsetTble)
    : DcmCodecParameter(),
      jp2k_optionsEnabled_(jp2k_optionsEnabled),
      jp2k_decompositions_(jp2k_decompositions),
      jp2k_cblkwidth_(jp2k_cblkwidth),
      jp2k_cblkheight_(jp2k_cblkheight),
      jp2k_progressionOrder_(jp2k_progressionOrder),
      fragmentSize_(fragmentSize),
      createOffsetTable_(createOffsetTable),
      preferCookedEncoding_(preferCookedEncoding),
      uidCreation_(uidCreation),
      convertToSC_(convertToSC),
      planarConfiguration_(planarConfiguration),
      ignoreOffsetTable_(ignoreOffsetTble) {}

HtJ2kCodecParameter::HtJ2kCodecParameter(
    HTJ2K_UIDCreation uidCreation,
    HTJ2K_PlanarConfiguration planarConfiguration, OFBool ignoreOffsetTble)
    : DcmCodecParameter(),
      jp2k_optionsEnabled_(OFFalse),
      jp2k_decompositions_(5),
      jp2k_cblkwidth_(64),
      jp2k_cblkheight_(64),
      jp2k_progressionOrder_(EHTJ2KPO_default),
      fragmentSize_(0),
      createOffsetTable_(OFTrue),
      preferCookedEncoding_(OFTrue),
      uidCreation_(uidCreation),
      convertToSC_(OFFalse),
      planarConfiguration_(planarConfiguration),
      ignoreOffsetTable_(ignoreOffsetTble) {}

HtJ2kCodecParameter::HtJ2kCodecParameter(HtJ2kCodecParameter const &arg)
    : DcmCodecParameter(arg),
      jp2k_optionsEnabled_(arg.jp2k_optionsEnabled_),
      jp2k_decompositions_(arg.jp2k_decompositions_),
      jp2k_cblkwidth_(arg.jp2k_cblkwidth_),
      jp2k_cblkheight_(arg.jp2k_cblkheight_),
      jp2k_progressionOrder_(arg.jp2k_progressionOrder_),
      fragmentSize_(arg.fragmentSize_),
      createOffsetTable_(arg.createOffsetTable_),
      preferCookedEncoding_(arg.preferCookedEncoding_),
      uidCreation_(arg.uidCreation_),
      convertToSC_(arg.convertToSC_),
      planarConfiguration_(arg.planarConfiguration_),
      ignoreOffsetTable_(arg.ignoreOffsetTable_) {}

HtJ2kCodecParameter::~HtJ2kCodecParameter() {}

DcmCodecParameter *HtJ2kCodecParameter::clone() const {
  return new HtJ2kCodecParameter(*this);
}

char const *HtJ2kCodecParameter::className() const {
  return "HtJ2kCodecParameter";
}
