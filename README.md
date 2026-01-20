# DCMTK-HTJ2K

A DCMTK codec library for encoding and decoding DICOM images using the High-Throughput JPEG 2000 (HTJ2K) compression and decompression technique.

## Overview

DCMTK-HTJ2K provides HTJ2K codec support for [DCMTK](https://github.com/DCMTK/dcmtk), enabling seamless compression and decompression of DICOM images using the HTJ2K standard. The library integrates [OpenJPH](https://github.com/aous72/openjph) for HTJ2K operations with DCMTK's DICOM handling capabilities.

### Supported Transfer Syntaxes

- High Throughput JPEG 2000 Image Compression - Lossless Only (1.2.840.10008.1.2.4.201)
- High Throughput JPEG 2000 with RPCL Options Image Compression - Lossless Only (1.2.840.10008.1.2.4.202)
- High Throughput JPEG 2000 Image Compression (1.2.840.10008.1.2.4.203)

## Features

- **HTJ2K Encoding**: Compress DICOM images using HTJ2K lossless and lossy compression.
- **HTJ2K Decoding**: Decompress HTJ2K-encoded DICOM images.
- **DCMTK Integration**: Seamless integration with DCMTK codec framework.
- **Configurable Parameters**: Support for codeblock dimensions, fragment sizes, and encoding options.

## Dependencies

- **DCMTK** (3.6.9 or higher)
- **OpenJPH** (0.26.0 or higher)
- **CMake** (3.12.0 or higher)

## Building

### Quick Build

```bash
./build.sh [Release|Debug]
```

The build script accepts an optional build configuration parameter (`Release` or `Debug`). If not specified, it defaults to `Release`.

The provided build script automatically downloads, builds, and installs dependencies. It also builds and installs DCMTK-HTJ2K:
- Library files to `build/ReleaseOrDebug/lib/`.
- Header files to `build/ReleaseOrDebug/include/DCMTKHTJ2K/`.
- CMake configuration files to `build/ReleaseOrDebug/lib/cmake/DCMTKHTJ2K/`.

## Usage

### Registering Encoder

```cpp
#include "dcmtkhtj2k/djencode.h"

// Register HTJ2K encoder with default parameters
HtJ2kEncoderRegistration::registerCodecs();

// Or with custom parameters
HtJ2kEncoderRegistration::registerCodecs(
    OFTrue,             // jp2k_optionsEnabled
    5,                  // jp2k_decompositions (decompositions)
    64,                 // jp2k_cblkwidth (codeblock width)
    64,                 // jp2k_cblkheight (codeblock height)
    EJ2KPO_default,     // jp2k_progressionOrder (progression order)
    OFTrue,             // preferCookedEncoding
    0,                  // fragmentSize (0 = unlimited)
    OFTrue,             // createOffsetTable
    EJ2KUC_default,     // uidCreation
    OFFalse             // convertToSC
);
```

### Registering Decoder

```cpp
#include "dcmtkhtj2k/djdecode.h"

// Register HTJ2K decoder with default parameters
HtJ2kDecoderRegistration::registerCodecs();

// Or with custom parameters
HtJ2kDecoderRegistration::registerCodecs(
    EJ2KUC_default,     // uidCreation
    EJ2KPC_restore,     // planarConfig
    OFFalse             // ignoreOffsetTable
);
```

### Cleanup

```cpp
// Cleanup encoder
HtJ2kEncoderRegistration::cleanup();

// Cleanup decoder
HtJ2kDecoderRegistration::cleanup();
```

## Using in Your Project

CMake Integration
```bash
find_package(DCMTKHTJ2K REQUIRED)
target_link_libraries(your_target DCMTKHTJ2K)
```

## API Documentation

The library provides the following main classes:

- **`HtJ2kEncoderRegistration`**: Singleton for registering HTJ2K encoder.
- **`HtJ2kDecoderRegistration`**: Singleton for registering HTJ2K decoder.
- **`HtJ2kCodecParameter`**: Codec configuration parameters.
- **`HtJ2kEncoder`**: HTJ2K encoding implementation.
- **`HtJ2kDecoder`**: HTJ2K decoding implementation.

## License
DCMTK-HTJ2K is released under the MIT License.
