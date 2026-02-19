#ifndef DCMTKHTJ2K_DJDECODE_H
#define DCMTKHTJ2K_DJDECODE_H

#include "dcmtk/config/osconfig.h"
#include "dcmtk/ofstd/oftypes.h" /* for OFBool */
#include "djutils.h"             /* for enums */

class HtJ2kCodecParameter;
class HtJ2kDecoder;

/** singleton class that registers decoders for all supported HT-J2K
 * processes.
 */
class DCMTKHTJ2K_EXPORT HtJ2kDecoderRegistration {
 public:
  /** registers decoder for all supported HT-J2K processes.
   *  If already registered, call is ignored unless cleanup() has
   *  been performed before.
   *  @param uidcreation flag indicating whether or not
   *    a new SOP Instance UID should be assigned upon decompression.
   *  @param planarconfig flag indicating how planar configuration
   *    of color images should be encoded upon decompression.
   *  @param ignoreOffsetTable flag indicating whether to ignore the offset
   * table when decompressing multiframe images
   */
  static void registerCodecs(
      J2K_UIDCreation uidcreation = EJ2KUC_default,
      J2K_PlanarConfiguration planarconfig = EJ2KPC_restore,
      OFBool ignoreOffsetTable = OFFalse);

  /** deregisters decoders.
   *  Attention: Must not be called while other threads might still use
   *  the registered codecs, e.g. because they are currently decoding
   *  DICOM data sets through dcmdata.
   */
  static void cleanup();

  /** get version information of the CharLS library.
   *  Typical output format: "CharLS, Revision 55020 (modified)"
   *  @return name and version number of the CharLS library
   */
  static OFString getLibraryVersionString();

 private:
  /// flag indicating whether the decoders are already registered.
  static OFBool registered_;

  /// pointer to codec parameter shared by all decoders
  static HtJ2kCodecParameter *cp_;

  /// pointer to decoder
  static HtJ2kDecoder *decoder_;
};

#endif
