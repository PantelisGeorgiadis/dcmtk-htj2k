/*
 * Basic tests for the DCMTKHTJ2K library
 */

#include <gtest/gtest.h>

#include <fstream>
#include <vector>

#include "dcmtk/dcmdata/dcdeftag.h"
#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmdata/dcuid.h"
#include "dcmtk/dcmimage/diregist.h"
#include "dcmtk/oflog/oflog.h"
#include "dcmtk/ofstd/oftempf.h"
#include "dcmtkhtj2k/djdecode.h"
#include "dcmtkhtj2k/djencode.h"

namespace {

struct TestLogConfig {
  TestLogConfig() { OFLog::configure(OFLogger::INFO_LOG_LEVEL); }
};

static TestLogConfig g_testLogConfig;

void PopulateDatasetWithRequiredAttributes(
    DcmDataset *dataset, Uint16 rows, Uint16 cols, Uint16 bitsAllocated,
    Uint16 samplesPerPixel, OFString const &photometricInterpretation,
    Uint16 pixelRepresentation) {
  // Required image attributes
  ASSERT_TRUE(dataset->putAndInsertUint16(DCM_Rows, rows).good());
  ASSERT_TRUE(dataset->putAndInsertUint16(DCM_Columns, cols).good());
  ASSERT_TRUE(
      dataset->putAndInsertUint16(DCM_SamplesPerPixel, samplesPerPixel).good());
  ASSERT_TRUE(dataset
                  ->putAndInsertString(DCM_PhotometricInterpretation,
                                       photometricInterpretation.c_str())
                  .good());
  ASSERT_TRUE(
      dataset->putAndInsertUint16(DCM_BitsAllocated, bitsAllocated).good());
  ASSERT_TRUE(
      dataset->putAndInsertUint16(DCM_BitsStored, bitsAllocated).good());
  ASSERT_TRUE(
      dataset->putAndInsertUint16(DCM_HighBit, bitsAllocated - 1).good());
  ASSERT_TRUE(
      dataset->putAndInsertUint16(DCM_PixelRepresentation, pixelRepresentation)
          .good());

  // Minimal required UIDs
  char studyUid[100];
  char seriesUid[100];
  char instanceUid[100];
  dcmGenerateUniqueIdentifier(studyUid, SITE_STUDY_UID_ROOT);
  dcmGenerateUniqueIdentifier(seriesUid, SITE_SERIES_UID_ROOT);
  dcmGenerateUniqueIdentifier(instanceUid, SITE_INSTANCE_UID_ROOT);

  ASSERT_TRUE(dataset
                  ->putAndInsertString(DCM_SOPClassUID,
                                       UID_SecondaryCaptureImageStorage)
                  .good());
  ASSERT_TRUE(
      dataset->putAndInsertString(DCM_StudyInstanceUID, studyUid).good());
  ASSERT_TRUE(
      dataset->putAndInsertString(DCM_SeriesInstanceUID, seriesUid).good());
  ASSERT_TRUE(
      dataset->putAndInsertString(DCM_SOPInstanceUID, instanceUid).good());
}

void VerifyProgressionOrder(OFTempFile const &tmpFile,
                            unsigned char const expectedProgressionOrder) {
  // Open temp file in a buffer
  std::ifstream input(tmpFile.getFilename(), std::ios::binary);
  // Scan for JPEG 2000 COD marker (0xFF52)
  bool foundCodMarker = false;
  char byte1, byte2;
  if (input.get(byte1)) {
    while (input.get(byte2)) {
      if (static_cast<unsigned char>(byte1) == 0xFF &&
          static_cast<unsigned char>(byte2) == 0x52) {
        foundCodMarker = true;
        break;
      }
      byte1 = byte2;
    }
  }
  ASSERT_TRUE(foundCodMarker);
  // Get COD marker length (2 bytes after COD marker)
  char codLength[2];
  // COD segment length
  ASSERT_TRUE(input.read(codLength, 2));
  // COD coding style default (1 byte after COD marker)
  char codingStyleDefault;
  ASSERT_TRUE(input.read(&codingStyleDefault, 1));
  // COD progression order (1 byte after coding style default)
  char progressionOrder;
  ASSERT_TRUE(input.read(&progressionOrder, 1));
  // Verify progression order
  ASSERT_TRUE(progressionOrder == expectedProgressionOrder);

  input.close();
}

TEST(BasicTest, FrameworkWorks) {
  EXPECT_EQ(1 + 1, 2);
  EXPECT_TRUE(true);
}

TEST(LibraryTest, BasicFunctionality) {
  // Register HTJ2K encoder with default parameters
  HtJ2kEncoderRegistration::registerCodecs();
  // Register HTJ2K decoder with default parameters
  HtJ2kDecoderRegistration::registerCodecs();

  // Cleanup encoder
  HtJ2kEncoderRegistration::cleanup();
  // Cleanup decoder
  HtJ2kDecoderRegistration::cleanup();

  SUCCEED();
}

TEST(CodecTest, BasicMonochrome8BitUnsignedCompressDecompressLossless) {
  const Uint16 rows = 256;
  const Uint16 cols = 256;
  const size_t pixelCount =
      static_cast<size_t>(rows) * static_cast<size_t>(cols);

  std::vector<Uint8> original(pixelCount);
  for (Uint16 r = 0; r < rows; ++r) {
    for (Uint16 c = 0; c < cols; ++c) {
      const size_t idx = static_cast<size_t>(r) * cols + c;
      original[idx] = static_cast<Uint8>(idx & 0xFF);
    }
  }

  DcmFileFormat fileformat;
  DcmDataset *dataset = fileformat.getDataset();

  // Populate dataset for an 8-bit monochrome image
  PopulateDatasetWithRequiredAttributes(dataset, rows, cols, 8, 1,
                                        "MONOCHROME2", 0);

  // Pixel data
  ASSERT_TRUE(
      dataset
          ->putAndInsertUint8Array(DCM_PixelData, original.data(),
                                   static_cast<unsigned long>(pixelCount))
          .good());

  // Register codecs
  HtJ2kEncoderRegistration::registerCodecs();
  HtJ2kDecoderRegistration::registerCodecs();

  const E_TransferSyntax htj2kLossless =
      EXS_HighThroughputJPEG2000withRPCLOptionsLosslessOnly;
  ASSERT_TRUE(dataset->chooseRepresentation(htj2kLossless, nullptr).good());
  ASSERT_TRUE(dataset->canWriteXfer(htj2kLossless));

  // Save to temp file
  OFTempFile tempFile;
  ASSERT_TRUE(tempFile.getStatus().good());
  ASSERT_TRUE(
      fileformat.saveFile(tempFile.getFilename(), htj2kLossless).good());

  // RPCL progression order [0x02]
  VerifyProgressionOrder(tempFile, 0x02);

  // Read back
  DcmFileFormat readFile;
  ASSERT_TRUE(readFile.loadFile(tempFile.getFilename()).good());

  DcmDataset *readDataset = readFile.getDataset();
  const E_TransferSyntax readXfer = readDataset->getOriginalXfer();
  ASSERT_EQ(readXfer, htj2kLossless);
  ASSERT_TRUE(
      readDataset->chooseRepresentation(EXS_LittleEndianExplicit, nullptr)
          .good());
  ASSERT_TRUE(readDataset->canWriteXfer(EXS_LittleEndianExplicit));

  Uint8 const *decoded = nullptr;
  unsigned long decodedCount = 0;
  ASSERT_TRUE(
      readDataset->findAndGetUint8Array(DCM_PixelData, decoded, &decodedCount)
          .good());
  ASSERT_EQ(decodedCount, static_cast<unsigned long>(pixelCount));

  for (size_t i = 0; i < pixelCount; ++i) {
    EXPECT_EQ(decoded[i], original[i]);
  }

  // Cleanup codecs
  HtJ2kEncoderRegistration::cleanup();
  HtJ2kDecoderRegistration::cleanup();
}

TEST(CodecTest, BasicMonochrome16BitUnsignedCompressDecompressLossless) {
  const Uint16 rows = 128;
  const Uint16 cols = 128;
  const size_t pixelCount =
      static_cast<size_t>(rows) * static_cast<size_t>(cols);

  std::vector<Uint16> original(pixelCount);
  for (Uint16 r = 0; r < rows; ++r) {
    for (Uint16 c = 0; c < cols; ++c) {
      const size_t idx = static_cast<size_t>(r) * cols + c;
      original[idx] = static_cast<Uint16>((r << 8) ^ c);
    }
  }

  DcmFileFormat fileformat;
  DcmDataset *dataset = fileformat.getDataset();

  // Populate dataset for an 16-bit unsigned monochrome image
  PopulateDatasetWithRequiredAttributes(dataset, rows, cols, 16, 1,
                                        "MONOCHROME2", 0);

  // Pixel data
  ASSERT_TRUE(
      dataset
          ->putAndInsertUint16Array(DCM_PixelData, original.data(),
                                    static_cast<unsigned long>(pixelCount))
          .good());

  // Register codecs
  HtJ2kEncoderRegistration::registerCodecs();
  HtJ2kDecoderRegistration::registerCodecs();

  const E_TransferSyntax htj2kLossless = EXS_HighThroughputJPEG2000LosslessOnly;
  ASSERT_TRUE(dataset->chooseRepresentation(htj2kLossless, nullptr).good());
  ASSERT_TRUE(dataset->canWriteXfer(htj2kLossless));

  // Save to temp file
  OFTempFile tempFile;
  ASSERT_TRUE(tempFile.getStatus().good());
  ASSERT_TRUE(
      fileformat.saveFile(tempFile.getFilename(), htj2kLossless).good());

  // Default LRCP progression order [0x00]
  VerifyProgressionOrder(tempFile, 0x00);

  // Read back
  DcmFileFormat readFile;
  ASSERT_TRUE(readFile.loadFile(tempFile.getFilename()).good());

  DcmDataset *readDataset = readFile.getDataset();
  const E_TransferSyntax readXfer = readDataset->getOriginalXfer();
  ASSERT_EQ(readXfer, htj2kLossless);
  ASSERT_TRUE(
      readDataset->chooseRepresentation(EXS_LittleEndianExplicit, nullptr)
          .good());
  ASSERT_TRUE(readDataset->canWriteXfer(EXS_LittleEndianExplicit));

  Uint16 const *decoded = nullptr;
  unsigned long decodedCount = 0;
  ASSERT_TRUE(
      readDataset->findAndGetUint16Array(DCM_PixelData, decoded, &decodedCount)
          .good());
  ASSERT_EQ(decodedCount, static_cast<unsigned long>(pixelCount));

  for (size_t i = 0; i < pixelCount; ++i) {
    EXPECT_EQ(decoded[i], original[i]);
  }

  // Cleanup codecs
  HtJ2kEncoderRegistration::cleanup();
  HtJ2kDecoderRegistration::cleanup();
}

TEST(CodecTest, BasicMonochrome16BitSignedCompressDecompressLossless) {
  const Uint16 rows = 128;
  const Uint16 cols = 128;
  const size_t pixelCount =
      static_cast<size_t>(rows) * static_cast<size_t>(cols);

  std::vector<Sint16> original(pixelCount);
  std::vector<Uint16> originalStored(pixelCount);
  for (Uint16 r = 0; r < rows; ++r) {
    for (Uint16 c = 0; c < cols; ++c) {
      const size_t idx = static_cast<size_t>(r) * cols + c;
      const Sint16 value = static_cast<Sint16>((static_cast<int>(r) - 64) * 4 +
                                               (static_cast<int>(c) - 64));
      original[idx] = value;
      originalStored[idx] = static_cast<Uint16>(value);
    }
  }

  DcmFileFormat fileformat;
  DcmDataset *dataset = fileformat.getDataset();

  // Populate dataset for an 16-bit signed monochrome image
  PopulateDatasetWithRequiredAttributes(dataset, rows, cols, 16, 1,
                                        "MONOCHROME2", 1);

  // Pixel data
  ASSERT_TRUE(
      dataset
          ->putAndInsertUint16Array(DCM_PixelData, originalStored.data(),
                                    static_cast<unsigned long>(pixelCount))
          .good());

  // Register codecs
  HtJ2kEncoderRegistration::registerCodecs();
  HtJ2kDecoderRegistration::registerCodecs();

  const E_TransferSyntax htj2kLossless = EXS_HighThroughputJPEG2000LosslessOnly;
  ASSERT_TRUE(dataset->chooseRepresentation(htj2kLossless, nullptr).good());
  ASSERT_TRUE(dataset->canWriteXfer(htj2kLossless));

  // Save to temp file
  OFTempFile tempFile;
  ASSERT_TRUE(tempFile.getStatus().good());
  ASSERT_TRUE(
      fileformat.saveFile(tempFile.getFilename(), htj2kLossless).good());

  // Read back
  DcmFileFormat readFile;
  ASSERT_TRUE(readFile.loadFile(tempFile.getFilename()).good());

  DcmDataset *readDataset = readFile.getDataset();
  const E_TransferSyntax readXfer = readDataset->getOriginalXfer();
  ASSERT_EQ(readXfer, htj2kLossless);
  ASSERT_TRUE(
      readDataset->chooseRepresentation(EXS_LittleEndianExplicit, nullptr)
          .good());
  ASSERT_TRUE(readDataset->canWriteXfer(EXS_LittleEndianExplicit));

  Uint16 const *decoded = nullptr;
  unsigned long decodedCount = 0;
  ASSERT_TRUE(
      readDataset->findAndGetUint16Array(DCM_PixelData, decoded, &decodedCount)
          .good());
  ASSERT_EQ(decodedCount, static_cast<unsigned long>(pixelCount));

  for (size_t i = 0; i < pixelCount; ++i) {
    const Sint16 decodedSigned = static_cast<Sint16>(decoded[i]);
    EXPECT_EQ(decodedSigned, original[i]);
  }

  // Cleanup codecs
  HtJ2kEncoderRegistration::cleanup();
  HtJ2kDecoderRegistration::cleanup();
}

TEST(CodecTest, BasicColorCompressDecompressLossless) {
  const Uint16 rows = 128;
  const Uint16 cols = 128;
  const Uint16 samplesPerPixel = 3;
  const size_t pixelCount =
      static_cast<size_t>(rows) * static_cast<size_t>(cols) * samplesPerPixel;

  std::vector<Uint8> original(pixelCount);
  for (Uint16 r = 0; r < rows; ++r) {
    for (Uint16 c = 0; c < cols; ++c) {
      const size_t base = (static_cast<size_t>(r) * cols + c) * samplesPerPixel;
      original[base + 0] = static_cast<Uint8>(r & 0xFF);
      original[base + 1] = static_cast<Uint8>(c & 0xFF);
      original[base + 2] = static_cast<Uint8>((r + c) & 0xFF);
    }
  }

  DcmFileFormat fileformat;
  DcmDataset *dataset = fileformat.getDataset();

  // Populate dataset for an 8-bit color image
  PopulateDatasetWithRequiredAttributes(dataset, rows, cols, 8, 3, "RGB", 0);
  ASSERT_TRUE(dataset->putAndInsertUint16(DCM_PlanarConfiguration, 0).good());

  // Pixel data
  ASSERT_TRUE(
      dataset
          ->putAndInsertUint8Array(DCM_PixelData, original.data(),
                                   static_cast<unsigned long>(pixelCount))
          .good());

  // Register codecs
  HtJ2kEncoderRegistration::registerCodecs();
  HtJ2kDecoderRegistration::registerCodecs();

  const E_TransferSyntax htj2kLossless = EXS_HighThroughputJPEG2000LosslessOnly;
  ASSERT_TRUE(dataset->chooseRepresentation(htj2kLossless, nullptr).good());
  ASSERT_TRUE(dataset->canWriteXfer(htj2kLossless));

  // Save to temp file
  OFTempFile tempFile;
  ASSERT_TRUE(tempFile.getStatus().good());
  ASSERT_TRUE(
      fileformat.saveFile(tempFile.getFilename(), htj2kLossless).good());

  // Read back
  DcmFileFormat readFile;
  ASSERT_TRUE(readFile.loadFile(tempFile.getFilename()).good());

  DcmDataset *readDataset = readFile.getDataset();
  const E_TransferSyntax readXfer = readDataset->getOriginalXfer();
  ASSERT_EQ(readXfer, htj2kLossless);
  ASSERT_TRUE(
      readDataset->chooseRepresentation(EXS_LittleEndianExplicit, nullptr)
          .good());
  ASSERT_TRUE(readDataset->canWriteXfer(EXS_LittleEndianExplicit));

  Uint8 const *decoded = nullptr;
  unsigned long decodedCount = 0;
  ASSERT_TRUE(
      readDataset->findAndGetUint8Array(DCM_PixelData, decoded, &decodedCount)
          .good());
  ASSERT_EQ(decodedCount, static_cast<unsigned long>(pixelCount));

  for (size_t i = 0; i < pixelCount; ++i) {
    EXPECT_EQ(decoded[i], original[i]);
  }

  // Cleanup codecs
  HtJ2kEncoderRegistration::cleanup();
  HtJ2kDecoderRegistration::cleanup();
}

}  // namespace
