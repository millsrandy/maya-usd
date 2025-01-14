//
// Copyright 2016 Pixar
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
#ifndef PXRUSDMAYA_PRIM_WRITER_REGISTRY_H
#define PXRUSDMAYA_PRIM_WRITER_REGISTRY_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/primWriter.h>
#include <mayaUsd/fileio/primWriterArgs.h>
#include <mayaUsd/fileio/primWriterContext.h>

#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>

#include <maya/MFnDependencyNode.h>

#include <functional>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdMayaPrimWriterRegistry
/// \brief Provides functionality to register and lookup USD writer plugins
/// for Maya nodes.
///
/// Use PXRUSDMAYA_DEFINE_WRITER(mayaTypeName, args, ctx) to define a new
/// writer function, or use
/// PXRUSDMAYA_REGISTER_WRITER(mayaTypeName, writerClass) to register a writer
/// class with the registry.
///
/// The plugin is expected to create a prim at <tt>ctx->GetAuthorPath()</tt>.
///
/// In order for the core system to discover the plugin, you need a
/// \c plugInfo.json that contains the Maya type name and the Maya plugin to
/// load:
/// \code
/// {
///     "UsdMaya": {
///         "PrimWriter": {
///             "mayaPlugin": "myMayaPlugin",
///             "providesTranslator": [
///                 "myMayaType"
///             ]
///         }
///     }
/// }
/// \endcode
///
/// The registry contains information for both Maya built-in node types
/// and for any user-defined plugin types. If UsdMaya does not ship with a
/// writer plugin for some Maya built-in type, you can register your own
/// plugin for that Maya built-in type.
struct UsdMayaPrimWriterRegistry
{
    /// Writer factory function, i.e. a function that creates a prim writer
    /// for the given Maya node/USD paths and context.
    typedef std::function<UsdMayaPrimWriterSharedPtr(
        const MFnDependencyNode&,
        const SdfPath&,
        UsdMayaWriteJobContext&)>
        WriterFactoryFn;

    /// Writer function, i.e. a function that writes a prim. This is the
    /// signature of the function defined by the PXRUSDMAYA_DEFINE_WRITER
    /// macro.
    typedef std::function<bool(const UsdMayaPrimWriterArgs&, UsdMayaPrimWriterContext*)> WriterFn;

    /// Predicate function, i.e. a function that can tell the level of support
    /// the writer function will provide for a given set of export options.
    using ContextPredicateFn = std::function<
        UsdMayaPrimWriter::ContextSupport(const UsdMayaJobExportArgs&, const MObject&)>;

    /// \brief Register \p fn as a factory function providing a
    /// UsdMayaPrimWriter subclass that can be used to write \p mayaType.
    /// Provide a supportability of the primWriter. Use "supported" to
    /// override the default primWriter
    ///
    /// If you can't provide a valid UsdMayaPrimWriter for the given arguments,
    /// return a null pointer from the factory function \p fn.
    ///
    /// Example for registering a writer factory in your custom plugin:
    /// \code{.cpp}
    /// class MyWriter : public UsdMayaPrimWriter {
    ///     static UsdMayaPrimWriterSharedPtr Create(
    ///             const MFnDependencyNode& depNodeFn,
    ///             const SdfPath& usdPath,
    ///             UsdMayaWriteJobContext& jobCtx);
    /// };
    /// TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaPrimWriterRegistry, MyWriter) {
    ///     UsdMayaPrimWriterRegistry::Register("myCustomMayaNode",
    ///             MyWriter::Create);
    /// }
    /// \endcode
    MAYAUSD_CORE_PUBLIC
    static void Register(
        const std::string& mayaType,
        ContextPredicateFn pred,
        WriterFactoryFn    fn,
        bool               fromPython = false);

    /// \brief Register \p fn as a factory function providing a
    /// UsdMayaPrimWriter subclass that can be used to write \p mayaType.
    /// If you can't provide a valid UsdMayaPrimWriter for the given arguments,
    /// return a null pointer from the factory function \p fn.
    ///
    /// Example for registering a writer factory in your custom plugin:
    /// \code{.cpp}
    /// class MyWriter : public UsdMayaPrimWriter {
    ///     static UsdMayaPrimWriterSharedPtr Create(
    ///             const MFnDependencyNode& depNodeFn,
    ///             const SdfPath& usdPath,
    ///             UsdMayaWriteJobContext& jobCtx);
    /// };
    /// TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaPrimWriterRegistry, MyWriter) {
    ///     UsdMayaPrimWriterRegistry::Register("myCustomMayaNode",
    ///             MyWriter::Create);
    /// }
    /// \endcode
    MAYAUSD_CORE_PUBLIC
    static void Register(const std::string& mayaType, WriterFactoryFn fn, bool fromPython = false);

    /// \brief Wraps \p fn in a WriterFactoryFn and registers the wrapped
    /// function as a prim writer provider.
    /// This is a helper method for the macro PXRUSDMAYA_DEFINE_WRITER;
    /// you probably want to use PXRUSDMAYA_DEFINE_WRITER directly instead.
    MAYAUSD_CORE_PUBLIC
    static void RegisterRaw(const std::string& mayaType, WriterFn fn);

    /// \brief Finds a writer if one exists for \p mayaTypeName.
    ///
    /// If there is no writer plugin for \p mayaTypeName, returns nullptr.
    MAYAUSD_CORE_PUBLIC
    static WriterFactoryFn Find(
        const std::string&          mayaTypeName,
        const UsdMayaJobExportArgs& exportArgs,
        const MObject&              exportObj);

    /// \brief Check for external primWriter for \p mayaTypeName.
    MAYAUSD_CORE_PUBLIC
    static void CheckForWriterPlugin(const std::string& mayaTypeName);

    /// \brief Registers a maya node type to *not* create a new prim.
    ///
    /// This is relevant for Maya nodes that may not result in a new prim in
    /// Usd.  For example, the Maya node may be exported as an applied API
    /// schema on an exported UsdPrim.
    ///
    /// This could be relevant when determining whether or not a transform can
    /// be collapsed.  For example:
    /// ```
    /// |Mesh             (transform)
    ///   |MeshShape      (mesh)
    ///   |MyNode         (transform)
    ///     |MyNodeShape  (typeThatShouldNotBeExported)
    /// ```
    /// The "Mesh" and "MeshShape" nodes are collapsible because "MyNode" should
    /// not result in a prim.
    MAYAUSD_CORE_PUBLIC
    static void RegisterPrimless(const std::string& mayaTypeName);

    /// \brief Returns true if \p mayaTypeName nodes should result in a prim in
    /// Usd.
    ///
    /// This typically returns true unless \p mayaTypeName was explicitly
    /// registered to not be exported.
    MAYAUSD_CORE_PUBLIC
    static bool IsPrimless(const std::string& mayaTypeName);
};

// Note, TF_REGISTRY_FUNCTION_WITH_TAG needs a type to register with so we
// create a dummy struct in the macro.

/// \brief Defines a writer function for the given Maya type; the function
/// should write a USD prim for the given Maya node. The return status indicates
/// whether the operation succeeded.
///
/// Example:
/// \code{.cpp}
/// PXRUSDMAYA_DEFINE_WRITER(myCustomMayaNode, args, context) {
///     context->GetUsdStage()->DefinePrim(context->GetAuthorPath());
///     return true;
/// }
/// \endcode
#define PXRUSDMAYA_DEFINE_WRITER(mayaTypeName, argsVarName, ctxVarName)                           \
    struct UsdMayaWriterDummy_##mayaTypeName                                                      \
    {                                                                                             \
    };                                                                                            \
    static bool UsdMaya_PrimWriter_##mayaTypeName(                                                \
        const UsdMayaPrimWriterArgs&, UsdMayaPrimWriterContext*);                                 \
    TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaPrimWriterRegistry, UsdMayaWriterDummy_##mayaTypeName)   \
    {                                                                                             \
        UsdMayaPrimWriterRegistry::RegisterRaw(#mayaTypeName, UsdMaya_PrimWriter_##mayaTypeName); \
    }                                                                                             \
    bool UsdMaya_PrimWriter_##mayaTypeName(                                                       \
        const UsdMayaPrimWriterArgs& argsVarName, UsdMayaPrimWriterContext* ctxVarName)

/// \brief Registers a pre-existing writer class for the given Maya type;
/// the writer class should be a subclass of UsdMayaPrimWriter with a three-place
/// constructor that takes <tt>(const MFnDependencyNode& depNodeFn,
/// const SdfPath& usdPath, UsdMayaWriteJobContext& jobCtx)</tt> as arguments.
///
/// Example:
/// \code{.cpp}
/// class MyWriter : public UsdMayaPrimWriter {
///     MyWriter(
///             const MFnDependencyNode& depNodeFn,
///             const SdfPath& usdPath,
///             UsdMayaWriteJobContext& jobCtx) {
///         // ...
///     }
/// };
/// PXRUSDMAYA_REGISTER_WRITER(myCustomMayaNode, MyWriter);
/// \endcode
#define PXRUSDMAYA_REGISTER_WRITER(mayaTypeName, writerClass)                              \
    TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaPrimWriterRegistry, mayaTypeName##_##writerClass) \
    {                                                                                      \
        UsdMayaPrimWriterRegistry::Register(                                               \
            #mayaTypeName,                                                                 \
            [](const MFnDependencyNode& depNodeFn,                                         \
               const SdfPath&           usdPath,                                           \
               UsdMayaWriteJobContext&  jobCtx) {                                           \
                return std::make_shared<writerClass>(depNodeFn, usdPath, jobCtx);          \
            });                                                                            \
    }

PXR_NAMESPACE_CLOSE_SCOPE

#endif
