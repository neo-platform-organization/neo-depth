# neo depth

Depth contains support for particular operating systems and machines:
Windows/Linux adapters, standalone x86_64 boot and devices, and platform
registration. Generic language, JIT, services, and preOS implementation belong
in the sibling surface repository under neo-vm/language and neo-vm/common.

Depth is passive source material described by neo.plugin/1 manifests. The VM
builder owns selection through its input/plugin catalog and single Makefile.
platform.plugin.json depends on the surface neo.services library. It does not
contain a language or preOS implementation.

Native components are trusted code. Packaging is not a security sandbox.
cpu/x86_64.c implements the surface machine.h contract. cpu.plugin.json
registers neo.cpu.x86_64, required by the surface language component. It owns
native instruction encoding and relocations; language decisions stay in neo.

Repositories remain independent siblings: neo, depth, neo-image.
Build from neo with make configure and make test. There is no depth build script.
