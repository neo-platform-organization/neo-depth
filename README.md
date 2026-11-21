# neo depth

This is passive implementation material for the neo VM builder. It contains
source, headers, and standard neo.plugin/1 manifests. It does not decide which
parts the VM includes and has no independent build entry or plugin manager.

The independent repositories are siblings:

```text
workdir/
  neo/        surface VM, input/plugin catalog, and the one Makefile
  depth/      this repository (GitHub: neo-depth)
  neo-image/  live graph data and programs
```

From neo/, run make configure. First choose whether the VM should include any
depth components, then select compatible plugins from the input/plugin list.
No builds a bootstrap VM without these implementations. Yes does not include
everything automatically: dependency resolution follows the selected entries.

- platform.plugin.json describes target support and resource mechanisms.
- language.plugin.json describes the graph engine and compiler material.
- preos.plugin.json describes CLI/REPL and launcher mechanisms.
- plugins/ contains OS/architecture implementations, not a second manager.
- language/ contains the language implementation and its typed exports.
- common/ contains resource and preOS implementations and tests.
- tools/ contains acceptance and boot-test tools used for selected full builds.

The VM owns inclusion, validates the compiled interfaces, and controls startup.
Native material is trusted code after it is selected and compiled; a repository
split is not a security boundary. Current components are coarse. Individual
CPU schedulers or GPU drivers are not implemented merely by this packaging.

See neo/documentation/development/plugin-selection.md in the sibling surface
repository for the format, commands, validation, and current limitations.
