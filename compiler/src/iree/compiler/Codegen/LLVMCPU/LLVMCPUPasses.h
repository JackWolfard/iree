// Copyright 2023 The IREE Authors
//
// Licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
//
// This file includes the LLVMCPU Passes.
//
//===----------------------------------------------------------------------===//

#ifndef IREE_COMPILER_CODEGEN_LLVMCPU_PASSES_H_
#define IREE_COMPILER_CODEGEN_LLVMCPU_PASSES_H_

#include "iree/compiler/Codegen/Dialect/LoweringConfig.h"
#include "mlir/Pass/Pass.h"

namespace mlir {
namespace iree_compiler {

/// Performs the final conversion to LLVM dialect.
std::unique_ptr<OperationPass<ModuleOp>> createConvertToLLVMPass(
    bool reassociateFpReordering = false);

/// Checks CPU backend specific IR constraints (like no stack allocations)
std::unique_ptr<OperationPass<ModuleOp>>
createLLVMCPUCheckIRBeforeLLVMConversionPass();

std::unique_ptr<OperationPass<func::FuncOp>>
createLLVMCPUEmitVectorizationRemarksPass();

/// Pass to lower the module an hal.executable.variant operation to external
/// dialect. Currently this pass lowers to LLVM dialect, but could be
/// generalized to lower to any "final" dialect like SPIR-V/NVVM, etc.
std::unique_ptr<OperationPass<IREE::HAL::ExecutableVariantOp>>
createLLVMCPULowerExecutableTargetPass();

/// Pass to lower a sequence of operations to a iree_codegen.ukernel.*
/// operation.
std::unique_ptr<OperationPass<>> createLLVMCPULowerToUKernelsPass();

/// Materialize the encoding of operations. The layout to use for the encoded
/// operations are LLVMCPU specific.
std::unique_ptr<OperationPass<func::FuncOp>>
createLLVMCPUMaterializeEncodingPass();

std::unique_ptr<OperationPass<func::FuncOp>>
createLLVMCPUMmt4dVectorLoweringPass();

/// Pass to perform peeling on non-distributed loops.
std::unique_ptr<OperationPass<func::FuncOp>> createLLVMCPUPeelPass();

/// Pass to perform SplitReduction transformations of `LinalgOp`s.
std::unique_ptr<OperationPass<func::FuncOp>> createLLVMCPUSplitReductionPass(
    bool enableReassociateFpReductions = false);

/// Synchronizes LLVM linkage with MLIR symbol visibility.
std::unique_ptr<OperationPass<ModuleOp>>
createLLVMCPUSynchronizeSymbolVisibilityPass();

/// Pass to pad operations on tensors in top-down order.
enum class LLVMCPUTensorPadOption { ParallelDims, ReductionDims };
std::unique_ptr<OperationPass<func::FuncOp>> createLLVMCPUTensorPadPass(
    LLVMCPUTensorPadOption option = LLVMCPUTensorPadOption::ParallelDims);

/// Pass to tile and fuse TilingInterface ops with given tilingLevel.
std::unique_ptr<OperationPass<func::FuncOp>> createLLVMCPUTileAndFusePass(
    int64_t tilingLevel = -1);

/// Pass to tile TilingInterface ops with given tilingLevel.
std::unique_ptr<OperationPass<func::FuncOp>> createLLVMCPUTilePass(
    int64_t tilingLevel = -1);

/// Replaces llvm.intr.fma with its unfused mul and add ops.
std::unique_ptr<OperationPass<func::FuncOp>> createLLVMCPUUnfuseFMAOpsPass();

// Pass to lower Vector ops before conversion to LLVM.
struct LLVMCPUVectorLoweringPassOptions {
  std::string splitVectorTransfersTo = "";
  bool lowerVectorTransposeToAVX2 = false;
};
std::unique_ptr<OperationPass<func::FuncOp>> createLLVMCPUVectorLoweringPass();
std::unique_ptr<OperationPass<func::FuncOp>> createLLVMCPUVectorLoweringPass(
    const LLVMCPUVectorLoweringPassOptions &options);

struct LLVMCPUVectorizationPassOptions {
  bool enableVectorMasking = false;
  bool vectorizePadding = false;
  bool vectorizeGatherAccesses = false;
};
std::unique_ptr<OperationPass<func::FuncOp>> createLLVMCPUVectorizationPass();
std::unique_ptr<OperationPass<func::FuncOp>> createLLVMCPUVectorizationPass(
    const LLVMCPUVectorizationPassOptions &options);

/// A pass that converts certain vector.contract ops to custom kernels.
std::unique_ptr<OperationPass<func::FuncOp>>
createVectorContractCustomKernelsPass();

// Verifies that only supported IR constructs are passed to the compiler (like
// no Linalg transform markers are set).
std::unique_ptr<OperationPass<ModuleOp>>
createVerifyLinalgTransformLegalityPass();

//------------------------------------------------------------------------------
// LLVMCPU Codegen specific patterns.
//------------------------------------------------------------------------------

void populateUnfusedFMAOpsPassPatterns(MLIRContext *context,
                                       RewritePatternSet &patterns);

/// Populates `patterns` to convert certain vector.contract ops to special
/// "kernels" written either in SIMD intrinsics or inline assembly.
void populateVectorContractCustomKernelsPatterns(
    IREE::HAL::ExecutableTargetAttr target, RewritePatternSet &patterns);

//----------------------------------------------------------------------------//
// LLVMCPU backend Pass Pipelines.
//----------------------------------------------------------------------------//
/// Populates the passes to lower linalg ops on buffers. Currenly this
/// pipeline is only used for dispatches that just copy data from input
/// interfaces to output interface.
void addCPUBufferOpsTileAndVectorizePipeline(OpPassManager &passManager,
                                             bool enableVectorMasking);

/// Populates the passes to lower ops through data tiling transformations.
void addCPUDataTilingPipeline(OpPassManager &passManager);

/// Populates the passes to lower to scalars operations for linalg based
/// code-generation. This pipeline does not vectorize, but instead just
/// converts to memrefs
void addCPUDefaultPassPipeline(OpPassManager &passManager);

void addConvTileAndDecomposeExpertPassPipeline(OpPassManager &passManager,
                                               bool enableVectorMasking);

void addDoubleTilingPadExpertPassPipeline(OpPassManager &passManager,
                                          bool enableVectorMasking);

/// Populates the passes needed to multi level tile, fuse and vectorize
/// lowering of linalg ops on tensors to vectors operations.
void addMmt4dTilingExpertPassPipeline(OpPassManager &passManager,
                                      bool enableMicrokernels);

void addMultiTilingExpertPassPipeline(OpPassManager &passManager,
                                      int64_t numLevels, bool enablePeeling,
                                      bool enableVectorMasking,
                                      bool lowerToAVX2);

void addTensorToVectorsPassPipeline(OpPassManager &passManager,
                                    bool lowerToVectors = true);

/// Transform dialect-based common.
void addTransformDialectPasses(OpPassManager &passManager);

/// Populates the passes to lower to tiled/distributed/bufferized ops,
/// suitable for library call dispatch and lowering to loops.
void addVMVXDefaultPassPipeline(OpPassManager &passManager,
                                bool enableMicrokernels);
// Populates the passes needed to do tiling, decomposing, and vectorizing the
// convolution ops.
LogicalResult verifyConvTileAndDecomposeExpertConfig(
    Operation *op, IREE::Codegen::LoweringConfigAttr loweringConfig,
    IREE::Codegen::TranslationInfoAttr translationInfo,
    ArrayRef<int64_t> workgroupSize = {});

/// Populates the passes needed to do two-level tile + vectorize of linalg ops.
LogicalResult verifyDoubleTilingExpertPassPipelineConfig(
    Operation *op, IREE::Codegen::LoweringConfigAttr loweringConfig,
    IREE::Codegen::TranslationInfoAttr translationInfo,
    ArrayRef<int64_t> workgroupSize = {});

/// Populates the passes needed to multi level tile and lowering of linalg ops
/// on tensors to vectors operations.
LogicalResult verifyTensorToVectorsPassPipelineConfig(
    Operation *op, IREE::Codegen::LoweringConfigAttr loweringConfig,
    IREE::Codegen::TranslationInfoAttr translationInfo,
    ArrayRef<int64_t> workgroupSize = {});

//----------------------------------------------------------------------------//
// LLVMCPU Pass Pipelines for lowering to LLVM dialect.
//----------------------------------------------------------------------------//

/// Populates passes needed to lower a XLA HLO op to LLVM dialect via the
/// structured ops path. The pass manager `pm` in here should operate on the
/// module within the IREE::HAL::ExecutableOp.
void buildLLVMCPUCodegenPassPipeline(OpPassManager &passManager);

//----------------------------------------------------------------------------//
// LLVMCPU Linking Passes and Pipelines
//----------------------------------------------------------------------------//

/// Assigns executable constant ordinals across all LLVMCPU variants.
std::unique_ptr<OperationPass<IREE::HAL::ExecutableVariantOp>>
createLLVMCPUAssignConstantOrdinalsPass();

/// Assigns executable import ordinals across all LLVMCPU variants.
std::unique_ptr<OperationPass<IREE::HAL::ExecutableVariantOp>>
createLLVMCPUAssignImportOrdinalsPass();

/// Links LLVMCPU HAL executables within the top-level program module.
std::unique_ptr<OperationPass<mlir::ModuleOp>>
createLLVMCPULinkExecutablesPass();

/// Populates passes needed to link HAL executables across LLVMCPU targets.
void buildLLVMCPULinkingPassPipeline(OpPassManager &passManager);

}  // namespace iree_compiler
}  // namespace mlir

#endif  // IREE_COMPILER_CODEGEN_LLVMCPU_PASSES_H_
