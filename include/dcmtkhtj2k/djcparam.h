#ifndef DCMTKHTJ2K_DJCPARAM_H
#define DCMTKHTJ2K_DJCPARAM_H

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmdata/dccodec.h" /* for DcmCodecParameter */
#include "djutils.h"               /* for enums */

/** codec parameter for HT-J2K codecs
 */
class DCMTKHTJ2K_EXPORT HtJ2kCodecParameter : public DcmCodecParameter {
 public:
  /** constructor, for use with encoders.
   *  @param jp2k_optionsEnabled       enable/disable use of all HT-J2K
   * parameters
   *  @param jp2k_decompositions      HT-J2K decomposition levels parameter
   *  @param jp2k_cblkwidth            HT-J2K codeblock width parameter
   *  @param jp2k_cblkheight           HT-J2K codeblock height parameter
   *  @param jp2k_progressionOrder          progression order to be used in the
   * HT-J2K codestream
   *  @param preferCookedEncoding      true if the "cooked" lossless encoder
   * should be preferred over the "raw" one
   *  @param fragmentSize              maximum fragment size (in kbytes) for
   * compression, 0 for unlimited.
   *  @param createOffsetTable         create offset table during image
   * compression
   *  @param uidCreation               mode for SOP Instance UID creation
   *  @param convertToSC               flag indicating whether image should be
   * converted to Secondary Capture upon compression
   *  @param planarConfiguration       flag describing how planar configuration
   * of decompressed color images should be handled
   *  @param ignoreOffsetTable         flag indicating whether to ignore the
   * offset table when decompressing multiframe images
   */
  HtJ2kCodecParameter(
      OFBool jp2k_optionsEnabled, Uint16 jp2k_decompositions = 5,
      Uint16 jp2k_cblkwidth = 64, Uint16 jp2k_cblkheight = 64,
      J2K_ProgressionOrder jp2k_progressionOrder = EJ2KPO_default,
      OFBool preferCookedEncoding = OFTrue, Uint32 fragmentSize = 0,
      OFBool createOffsetTable = OFTrue,
      J2K_UIDCreation uidCreation = EJ2KUC_default,
      OFBool convertToSC = OFFalse,
      J2K_PlanarConfiguration planarConfiguration = EJ2KPC_restore,
      OFBool ignoreOffsetTable = OFFalse);

  /** constructor, for use with decoders. Initializes all encoder options to
   * defaults.
   *  @param uidCreation               mode for SOP Instance UID creation (used
   * both for encoding and decoding)
   *  @param planarConfiguration       flag describing how planar configuration
   * of decompressed color images should be handled
   *  @param ignoreOffsetTable         flag indicating whether to ignore the
   * offset table when decompressing multiframe images
   */
  HtJ2kCodecParameter(
      J2K_UIDCreation uidCreation = EJ2KUC_default,
      J2K_PlanarConfiguration planarConfiguration = EJ2KPC_restore,
      OFBool ignoreOffsetTable = OFFalse);

  /// copy constructor
  HtJ2kCodecParameter(HtJ2kCodecParameter const &arg);

  /// destructor
  virtual ~HtJ2kCodecParameter();

  /** this methods creates a copy of type DcmCodecParameter *
   *  it must be overweritten in every subclass.
   *  @return copy of this object
   */
  virtual DcmCodecParameter *clone() const;

  /** returns the class name as string.
   *  can be used as poor man's RTTI replacement.
   */
  virtual char const *className() const;

  /** returns secondary capture conversion flag
   *  @return secondary capture conversion flag
   */
  OFBool getConvertToSC() const { return convertToSC_; }

  /** returns create offset table flag
   *  @return create offset table flag
   */
  OFBool getCreateOffsetTable() const { return createOffsetTable_; }

  /** returns mode for SOP Instance UID creation
   *  @return mode for SOP Instance UID creation
   */
  J2K_UIDCreation getUIDCreation() const { return uidCreation_; }

  /** returns mode for handling planar configuration
   *  @return mode for handling planar configuration
   */
  J2K_PlanarConfiguration getPlanarConfiguration() const {
    return planarConfiguration_;
  }

  /** returns flag indicating whether or not the "cooked" lossless encoder
   *  should be preferred over the "raw" one
   *  @return raw/cooked lossless encoding flag
   */
  OFBool cookedEncodingPreferred() const { return preferCookedEncoding_; }

  /** returns maximum fragment size (in kbytes) for compression, 0 for
   * unlimited.
   *  @return maximum fragment size for compression
   */
  Uint32 getFragmentSize() const { return fragmentSize_; }

  /** returns HT-J2K parameter decompositions
   *  @return HT-J2K parameter decompositions
   */
  Uint16 get_decompositions() const { return jp2k_decompositions_; }

  /** returns HT-J2K parameter cblkwidth
   *  @return HT-J2K parameter cblkwidth
   */
  Uint16 get_cblkwidth() const { return jp2k_cblkwidth_; }

  /** returns HT-J2K parameter cblkheight
   *  @return HT-J2K parameter cblkheight
   */
  Uint16 get_cblkheight() const { return jp2k_cblkheight_; }

  /** returns HT-J2K parameter progression order
   *  @return HT-J2K parameter progression order
   */
  J2K_ProgressionOrder get_progressionOrder() const {
    return jp2k_progressionOrder_;
  }

  /** returns true if HT-J2K parameters are enabled, false otherwise
   *  @return true if HT-J2K parameters are enabled, false otherwise
   */
  OFBool getUseCustomOptions() const { return jp2k_optionsEnabled_; }

  /** returns true if the offset table should be ignored when decompressing
   * multiframe images
   *  @return true if the offset table should be ignored when decompressing
   * multiframe images
   */
  OFBool ignoreOffsetTable() const { return ignoreOffsetTable_; }

 private:
  /// private undefined copy assignment operator
  HtJ2kCodecParameter &operator=(HtJ2kCodecParameter const &);

  // ****************************************************
  // **** Parameters describing the encoding process ****

  /// enable/disable use of HT-J2K parameters
  OFBool jp2k_optionsEnabled_;

  /// HT-J2K decomposition levels parameter
  Uint16 jp2k_decompositions_;

  /// HT-J2K codeblock width parameter
  Uint16 jp2k_cblkwidth_;

  /// HT-J2K codeblock height parameter
  Uint16 jp2k_cblkheight_;

  /// HT-J2K progression order parameter
  J2K_ProgressionOrder jp2k_progressionOrder_;

  /// maximum fragment size (in kbytes) for compression, 0 for unlimited.
  Uint32 fragmentSize_;

  /// create offset table during image compression
  OFBool createOffsetTable_;

  /// Flag indicating if the "cooked" lossless encoder should be preferred over
  /// the "raw" one
  OFBool preferCookedEncoding_;

  /// mode for SOP Instance UID creation (used both for encoding and decoding)
  J2K_UIDCreation uidCreation_;

  /// flag indicating whether image should be converted to Secondary Capture
  /// upon compression
  OFBool convertToSC_;

  // ****************************************************
  // **** Parameters describing the decoding process ****

  /// flag describing how planar configuration of decompressed color images
  /// should be handled
  J2K_PlanarConfiguration planarConfiguration_;

  /// flag indicating if temporary files should be kept, false if they should be
  /// deleted after use
  OFBool ignoreOffsetTable_;
};

#endif
