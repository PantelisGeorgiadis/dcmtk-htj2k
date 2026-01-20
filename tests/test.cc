/*
 * Basic tests for DCMTKHTJ2K library
 */

#include <gtest/gtest.h>

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

  // Required image attributes
  ASSERT_TRUE(dataset->putAndInsertUint16(DCM_Rows, rows).good());
  ASSERT_TRUE(dataset->putAndInsertUint16(DCM_Columns, cols).good());
  ASSERT_TRUE(dataset->putAndInsertUint16(DCM_SamplesPerPixel, 1).good());
  ASSERT_TRUE(
      dataset->putAndInsertString(DCM_PhotometricInterpretation, "MONOCHROME2")
          .good());
  ASSERT_TRUE(dataset->putAndInsertUint16(DCM_BitsAllocated, 8).good());
  ASSERT_TRUE(dataset->putAndInsertUint16(DCM_BitsStored, 8).good());
  ASSERT_TRUE(dataset->putAndInsertUint16(DCM_HighBit, 7).good());
  ASSERT_TRUE(dataset->putAndInsertUint16(DCM_PixelRepresentation, 0).good());

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

  // Required image attributes
  ASSERT_TRUE(dataset->putAndInsertUint16(DCM_Rows, rows).good());
  ASSERT_TRUE(dataset->putAndInsertUint16(DCM_Columns, cols).good());
  ASSERT_TRUE(dataset->putAndInsertUint16(DCM_SamplesPerPixel, 1).good());
  ASSERT_TRUE(
      dataset->putAndInsertString(DCM_PhotometricInterpretation, "MONOCHROME2")
          .good());
  ASSERT_TRUE(dataset->putAndInsertUint16(DCM_BitsAllocated, 16).good());
  ASSERT_TRUE(dataset->putAndInsertUint16(DCM_BitsStored, 16).good());
  ASSERT_TRUE(dataset->putAndInsertUint16(DCM_HighBit, 15).good());
  ASSERT_TRUE(dataset->putAndInsertUint16(DCM_PixelRepresentation, 0).good());

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

  // Required image attributes
  ASSERT_TRUE(dataset->putAndInsertUint16(DCM_Rows, rows).good());
  ASSERT_TRUE(dataset->putAndInsertUint16(DCM_Columns, cols).good());
  ASSERT_TRUE(dataset->putAndInsertUint16(DCM_SamplesPerPixel, 1).good());
  ASSERT_TRUE(
      dataset->putAndInsertString(DCM_PhotometricInterpretation, "MONOCHROME2")
          .good());
  ASSERT_TRUE(dataset->putAndInsertUint16(DCM_BitsAllocated, 16).good());
  ASSERT_TRUE(dataset->putAndInsertUint16(DCM_BitsStored, 16).good());
  ASSERT_TRUE(dataset->putAndInsertUint16(DCM_HighBit, 15).good());
  ASSERT_TRUE(dataset->putAndInsertUint16(DCM_PixelRepresentation, 1).good());

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

  // Required image attributes
  ASSERT_TRUE(dataset->putAndInsertUint16(DCM_Rows, rows).good());
  ASSERT_TRUE(dataset->putAndInsertUint16(DCM_Columns, cols).good());
  ASSERT_TRUE(
      dataset->putAndInsertUint16(DCM_SamplesPerPixel, samplesPerPixel).good());
  ASSERT_TRUE(
      dataset->putAndInsertString(DCM_PhotometricInterpretation, "RGB").good());
  ASSERT_TRUE(dataset->putAndInsertUint16(DCM_PlanarConfiguration, 0).good());
  ASSERT_TRUE(dataset->putAndInsertUint16(DCM_BitsAllocated, 8).good());
  ASSERT_TRUE(dataset->putAndInsertUint16(DCM_BitsStored, 8).good());
  ASSERT_TRUE(dataset->putAndInsertUint16(DCM_HighBit, 7).good());
  ASSERT_TRUE(dataset->putAndInsertUint16(DCM_PixelRepresentation, 0).good());

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
