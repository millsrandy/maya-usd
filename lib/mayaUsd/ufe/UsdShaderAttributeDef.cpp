//
// Copyright 2022 Autodesk
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

#include "UsdShaderAttributeDef.h"

#include "Global.h"
#include "Utils.h"

#include <mayaUsd/base/tokens.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/token.h>
#include <pxr/usd/ndr/declare.h>
#include <pxr/usd/sdr/shaderProperty.h>

#include <map>
#include <vector>

namespace MAYAUSD_NS_DEF {
namespace ufe {

PXR_NAMESPACE_USING_DIRECTIVE

UsdShaderAttributeDef::UsdShaderAttributeDef(const SdrShaderPropertyConstPtr& shaderAttributeDef)
    : Ufe::AttributeDef()
    , fShaderAttributeDef(shaderAttributeDef)
{
    if (!TF_VERIFY(fShaderAttributeDef)) {
        throw std::runtime_error("Invalid shader attribute definition");
    }
}

UsdShaderAttributeDef::~UsdShaderAttributeDef() { }

std::string UsdShaderAttributeDef::name() const
{
    TF_DEV_AXIOM(fShaderAttributeDef);
    return fShaderAttributeDef->GetName().GetString();
}

std::string UsdShaderAttributeDef::type() const
{
    TF_DEV_AXIOM(fShaderAttributeDef);
    return usdTypeToUfe(fShaderAttributeDef);
}

std::string UsdShaderAttributeDef::defaultValue() const
{
    TF_DEV_AXIOM(fShaderAttributeDef);
    std::ostringstream defaultValue;
    defaultValue << fShaderAttributeDef->GetDefaultValue();
    return defaultValue.str();
}

Ufe::AttributeDef::IOType UsdShaderAttributeDef::ioType() const
{
    TF_DEV_AXIOM(fShaderAttributeDef);
    return fShaderAttributeDef->IsOutput() ? Ufe::AttributeDef::OUTPUT_ATTR
                                           : Ufe::AttributeDef::INPUT_ATTR;
}

namespace {
typedef std::unordered_map<std::string, std::function<Ufe::Value(const PXR_NS::SdrShaderProperty&)>>
                         MetadataMap;
static const MetadataMap _metaMap = {
    // Conversion map between known USD metadata and its MaterialX equivalent:
    { MayaUsdMetadata->UIName.GetString(),
      [](const PXR_NS::SdrShaderProperty& p) {
          return !p.GetLabel().IsEmpty() ? p.GetLabel().GetString()
                                         : UsdMayaUtil::prettifyName(p.GetName().GetString());
      } },
    { "doc",
      [](const PXR_NS::SdrShaderProperty& p) {
          return !p.GetHelp().empty() ? p.GetHelp() : Ufe::Value();
      } },
    { MayaUsdMetadata->UIFolder.GetString(),
      [](const PXR_NS::SdrShaderProperty& p) {
          return !p.GetPage().IsEmpty() ? p.GetPage().GetString() : Ufe::Value();
      } },
    { "enum",
      [](const PXR_NS::SdrShaderProperty& p) {
          std::string r;
          for (auto&& opt : p.GetOptions()) {
              if (!r.empty()) {
                  r += ", ";
              }
              r += opt.first.GetString();
          }
          return !r.empty() ? r : Ufe::Value();
      } },
    { "enumvalues",
      [](const PXR_NS::SdrShaderProperty& p) {
          std::string r;
          for (auto&& opt : p.GetOptions()) {
              if (opt.second.IsEmpty()) {
                  continue;
              }
              if (!r.empty()) {
                  r += ", ";
              }
              r += opt.second.GetString();
          }
          return !r.empty() ? r : Ufe::Value();
      } },
    { MayaUsdMetadata->UISoftMin
          .GetString(), // Maya has 0-100 sliders. In rendering, sliders are 0-1.
      [](const PXR_NS::SdrShaderProperty& p) {
          // Will only be returned if the metadata does not exist.
          static const auto defaultSoftMin = std::unordered_map<std::string, Ufe::Value> {
              { Ufe::Attribute::kFloat, std::string { "0" } },
              { Ufe::Attribute::kFloat3, std::string { "0,0,0" } },
              { Ufe::Attribute::kColorFloat3, std::string { "0,0,0" } },
              { Ufe::Attribute::kDouble, std::string { "0" } },
#ifdef UFE_V4_FEATURES_AVAILABLE
              { Ufe::Attribute::kFloat2, std::string { "0,0" } },
              { Ufe::Attribute::kFloat4, std::string { "0,0,0,0" } },
              { Ufe::Attribute::kColorFloat4, std::string { "0,0,0,0" } },
#endif
          };
          // If there is a UIMin value, use it as the soft min.
          const NdrTokenMap& metadata = p.GetMetadata();
          auto               it = metadata.find(MayaUsdMetadata->UIMin);
          if (it != metadata.cend()) {
              return Ufe::Value(it->second);
          }
          auto itDefault = defaultSoftMin.find(usdTypeToUfe(&p));
          return itDefault != defaultSoftMin.end() ? itDefault->second : Ufe::Value();
      } },
    { MayaUsdMetadata->UISoftMax
          .GetString(), // Maya has 0-100 sliders. In rendering, sliders are 0-1.
      [](const PXR_NS::SdrShaderProperty& p) {
          // Will only be returned if the metadata does not exist.
          static const auto defaultSoftMax = std::unordered_map<std::string, Ufe::Value> {
              { Ufe::Attribute::kFloat, std::string { "1" } },
              { Ufe::Attribute::kFloat3, std::string { "1,1,1" } },
              { Ufe::Attribute::kColorFloat3, std::string { "1,1,1" } },
              { Ufe::Attribute::kDouble, std::string { "1" } },
#ifdef UFE_V4_FEATURES_AVAILABLE
              { Ufe::Attribute::kFloat2, std::string { "1,1" } },
              { Ufe::Attribute::kFloat4, std::string { "1,1,1,1" } },
              { Ufe::Attribute::kColorFloat4, std::string { "1,1,1,1" } },
#endif
          };
          // If there is a UIMax value, use it as the soft max.
          const NdrTokenMap& metadata = p.GetMetadata();
          auto               it = metadata.find(MayaUsdMetadata->UIMax);
          if (it != metadata.cend()) {
              return Ufe::Value(it->second);
          }
          auto itDefault = defaultSoftMax.find(usdTypeToUfe(&p));
          return itDefault != defaultSoftMax.end() ? itDefault->second : Ufe::Value();
      } },
    // If Ufe decides to use another completely different convention, it can be added here:
};
} // namespace

Ufe::Value UsdShaderAttributeDef::getMetadata(const std::string& key) const
{
    TF_DEV_AXIOM(fShaderAttributeDef);
    const NdrTokenMap& metadata = fShaderAttributeDef->GetMetadata();
    auto               it = metadata.find(TfToken(key));
    if (it != metadata.cend()) {
        return Ufe::Value(it->second);
    }

    const NdrTokenMap& hints = fShaderAttributeDef->GetHints();
    it = hints.find(TfToken(key));
    if (it != hints.cend()) {
        return Ufe::Value(it->second);
    }

    MetadataMap::const_iterator itMapper = _metaMap.find(key);
    if (itMapper != _metaMap.end()) {
        return itMapper->second(*fShaderAttributeDef);
    }

    return {};
}

bool UsdShaderAttributeDef::hasMetadata(const std::string& key) const
{
    TF_DEV_AXIOM(fShaderAttributeDef);
    const NdrTokenMap& metadata = fShaderAttributeDef->GetMetadata();
    auto               it = metadata.find(TfToken(key));
    if (it != metadata.cend()) {
        return true;
    }

    const NdrTokenMap& hints = fShaderAttributeDef->GetHints();
    it = hints.find(TfToken(key));
    if (it != hints.cend()) {
        return true;
    }

    MetadataMap::const_iterator itMapper = _metaMap.find(key);
    if (itMapper != _metaMap.end() && !itMapper->second(*fShaderAttributeDef).empty()) {
        return true;
    }

    return false;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
