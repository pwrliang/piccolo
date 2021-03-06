set(CMAKE_CXX_FLAGS "-fPIC -march=core2 -Wall -Wno-sign-compare -Wno-unused-function -fno-omit-frame-pointer -ggdb1")

add_library(gperfbase
            base/atomicops-internals-x86.cc
            base/dynamic_annotations.cc
            base/linuxthreads.cc
            base/logging.cc
            base/low_level_alloc.cc
            base/spinlock.cc
            base/sysinfo.cc
            base/vdso_support.cc
            common.cc
            internal_logging.cc
            stacktrace.cc
            stacktrace_with_context.cc)

add_library(tcmalloc
            central_freelist.cc
            heap-checker-bcad.cc
            heap-checker.cc
            heap-profile-table.cc
            heap-profiler.cc
            malloc_extension.cc
            malloc_hook.cc
            maybe_threads.cc
            memfs_malloc.cc
            memory_region_map.cc
            page_heap.cc
            raw_printer.cc
            sampler.cc
            span.cc
            stack_trace_table.cc
            static_vars.cc
            symbolize.cc
            system-alloc.cc
            tcmalloc.cc
            thread_cache.cc)

add_library(profiler
            profile-handler.cc
            profiledata.cc
            profiler.cc)