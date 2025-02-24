//
// Copyright 2020 Autodesk
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#pragma once

// Make sure ufe.h is included, if it hasn't been already.
#include <ufe/ufe.h>

// UFE v1 has no UFE_V2 macro.
// UFE v2 is the minimum supported (so no UFE_V2 needed).

#if !defined(UFE_V3_FEATURES_AVAILABLE) && !defined(UFE_V3)
#define UFE_V3(...)
#endif

#if !defined(UFE_V4_FEATURES_AVAILABLE) && !defined(UFE_V4)
#define UFE_V4(...)
#endif
