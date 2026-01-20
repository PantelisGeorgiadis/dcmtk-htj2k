#ifndef DCMTKHTJ2KDEFINE_H
#define DCMTKHTJ2KDEFINE_H

#include "dcmtk/config/osconfig.h"
#include "dcmtk/ofstd/ofdefine.h"

#ifdef DCMTKHTJ2K_EXPORTS
#define DCMTKHTJ2K_EXPORT DCMTK_DECL_EXPORT
#else
#define DCMTKHTJ2K_EXPORT DCMTK_DECL_IMPORT
#endif

#endif
