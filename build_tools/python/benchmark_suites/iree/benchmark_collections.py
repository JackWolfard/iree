## Copyright 2022 The IREE Authors
#
# Licensed under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""Generates all benchmarks."""

from typing import List, Tuple

from e2e_test_artifacts import iree_artifacts
from e2e_test_framework.definitions import iree_definitions
from benchmark_suites.iree import (benchmark_tags, riscv_benchmarks,
                                   x86_64_benchmarks, adreno_benchmarks,
                                   armv8_a_benchmarks, cuda_benchmarks,
                                   mali_benchmarks, vulkan_nvidia_benchmarks,
                                   vmvx_benchmarks)

COMPILE_STATS_ID_SUFFIX = "-compile-stats"


def generate_benchmarks(
) -> Tuple[List[iree_definitions.ModuleGenerationConfig],
           List[iree_definitions.E2EModelRunConfig]]:
  benchmarks = [
      x86_64_benchmarks.Linux_x86_64_Benchmarks(),
      cuda_benchmarks.Linux_CUDA_Benchmarks(),
      riscv_benchmarks.Linux_RV64_Benchmarks(),
      riscv_benchmarks.Linux_RV32_Benchmarks(),
      armv8_a_benchmarks.Android_ARMv8_A_Benchmarks(),
      adreno_benchmarks.Android_Adreno_Benchmarks(),
      mali_benchmarks.Android_Mali_Benchmarks(),
      vulkan_nvidia_benchmarks.Linux_Vulkan_NVIDIA_Benchmarks(),
      vmvx_benchmarks.Android_VMVX_Benchmarks()
  ]
  all_gen_configs: List[iree_definitions.ModuleGenerationConfig] = []
  all_run_configs: List[iree_definitions.E2EModelRunConfig] = []
  for benchmark in benchmarks:
    module_generation_configs, run_configs = benchmark.generate()
    all_gen_configs += module_generation_configs
    all_run_configs += run_configs

  compile_stats_gen_configs = []
  # For now we simply track compilation statistics of all modules.
  for gen_config in all_gen_configs:
    compile_config = gen_config.compile_config
    # Use POSIX path, see the comment of iree_definitions.MODULE_DIR_VARIABLE.
    scheduling_stats_path = f"{iree_definitions.MODULE_DIR_VARIABLE}/{iree_artifacts.SCHEDULING_STATS_FILENAME}"
    compile_stats_config = iree_definitions.CompileConfig.build(
        id=compile_config.id + COMPILE_STATS_ID_SUFFIX,
        tags=compile_config.tags + [benchmark_tags.COMPILE_STATS],
        compile_targets=compile_config.compile_targets,
        extra_flags=compile_config.extra_flags + [
            # Enable zip polyglot to provide component sizes.
            "--iree-vm-emit-polyglot-zip=true",
            # Disable debug symbols to provide correct component sizes.
            "--iree-llvmcpu-debug-symbols=false",
            # Dump scheduling statistics
            "--iree-scheduling-dump-statistics-format=json",
            f"--iree-scheduling-dump-statistics-file={scheduling_stats_path}"
        ])
    compile_stats_gen_configs.append(
        iree_definitions.ModuleGenerationConfig.build(
            imported_model=gen_config.imported_model,
            compile_config=compile_stats_config,
            tags=gen_config.tags))
  all_gen_configs += compile_stats_gen_configs

  return (all_gen_configs, all_run_configs)
