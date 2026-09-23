# Native third-party inventory

MTerm-authored source: MIT (root LICENSE). Native headers retain self-reported contribution IDs.

- **Qt 6.8.3 / MinGW 13.1.0**: initial local development baseline, not latest/security-approved.
  Native runtime directly uses Core, Gui, Widgets and Sql; Qt Test is for development.
  Qt libraries are dynamically linked. Consult the exact selected modules' upstream license
  files and Qt redistribution/source/relinking obligations before distributing binaries.
  Local packaging is explicitly unsigned development staging, not completed licensing review.
- **libvterm 0.3.3 fork**: pinned neovim/libvterm commit
  `934bc2fbf21800ac3458a499df8820ca5fb45fd3`, MIT, copyright Paul Evans.
  Original LICENSE, CODE-MAP, README, source and UPSTREAM.json are retained under
  native/third_party/libvterm. MTerm adapters live outside upstream source. The fork README
  notes its maintenance status; do not treat this pin as a promise of ongoing upstream support.
- **Compiler runtime**: deployed MinGW runtime DLLs need their own redistribution notices.
- **Electron reference**: existing pinned npm dependencies are development/reference-only and
  are not included in the native staging recipe. Its transitive license audit remains separate.

Do not re-label third-party code as AI-authored or MIT merely because MTerm itself is MIT.
Do not upload .tools, binary staging or user profiles as application source.
