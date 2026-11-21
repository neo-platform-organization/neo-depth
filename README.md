# neo depth

This repository contains neo's depth kernel: native platform providers,
resource services, preOS/CLI orchestration, and their tests. The surface VM defines the public language contract and validates both native
interfaces before startup. The graph/compiler implementation lives here.

The repositories are independent siblings:

```text
workdir/
  neo/        surface VM and build entry
  depth/      this repository (GitHub: neo-depth)
  neo-image/  live image (neo-os/image.neo)
```

Build from the surface repository using its single root Makefile. There is no
separate build file or workflow here. Outputs remain build/ARCH/TARGET/ in
the surface repository.

## Structure

- language/: graph engine, JIT implementation, Weft lowering, and language provider.
- plugins/: Windows, Linux, and standalone x86_64 providers and device adapters.
- provider.c: registers the selected provider through the surface-owned ABI.
- bindings.h: keeps private provider symbols separate from the public surface.
- common/: resource handles, event pumping, framebuffer transport, the REPL,
  hosted runners, launcher mechanisms, and acceptance tests.
- tools/: regression fixtures, source embedding, and QEMU boot checks.

The current plugin is linked at build time. ABI version 1 checks version,
structure size, required operations, and lifetime. It supports early standalone
boot without a host dynamic loader. It does not yet load arbitrary shared
libraries, hot-swap live providers, implement a general CPU scheduler, or supply
new GPU drivers. Those can be added here as real providers/services later.

Depth plugins are trusted native code. Moving code into a plugin does not
sandbox it. Language/package grants remain enforced at the surface and service
boundaries; platform workers retain their existing host containment.

The preOS is a depth facility. Application behavior belongs in neo-image.
Detailed launcher and language milestones remain in the surface documentation.
