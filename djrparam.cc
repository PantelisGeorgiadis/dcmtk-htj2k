#include "dcmtkhtj2k/djrparam.h"

#include "dcmtk/config/osconfig.h"
#include "dcmtk/ofstd/ofstd.h"

HtJ2kRepresentationParameter::HtJ2kRepresentationParameter(
    OFBool losslessProcess)
    : DcmRepresentationParameter(), losslessProcess_(losslessProcess) {}

HtJ2kRepresentationParameter::HtJ2kRepresentationParameter(
    HtJ2kRepresentationParameter const &arg)
    : DcmRepresentationParameter(arg), losslessProcess_(arg.losslessProcess_) {}

HtJ2kRepresentationParameter::~HtJ2kRepresentationParameter() {}

DcmRepresentationParameter *HtJ2kRepresentationParameter::clone() const {
  return new HtJ2kRepresentationParameter(*this);
}

char const *HtJ2kRepresentationParameter::className() const {
  return "HtJ2kRepresentationParameter";
}

OFBool HtJ2kRepresentationParameter::operator==(
    DcmRepresentationParameter const &arg) const {
  char const *argname = arg.className();
  if (argname) {
    OFString argstring(argname);
    if (argstring == className()) {
      HtJ2kRepresentationParameter const &argll =
          OFreinterpret_cast(HtJ2kRepresentationParameter const &, arg);
      if (losslessProcess_ && argll.losslessProcess_)
        return OFTrue;
      else if (losslessProcess_ != argll.losslessProcess_)
        return OFFalse;
      return OFTrue;
    }
  }
  return OFFalse;
}
