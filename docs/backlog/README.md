# Full-scope requirements index

`requirements-index.json` faithfully indexes **485 checkbox entries** from the unchanged
`docs/MASTER_TODO_100_PERCENT.md`. IDs are deterministic hashes of section/text/duplicate occurrence.
The source SHA-256 and line numbers make drift detectable. Keep the original terminology.

Inherited checkmarks describe the earlier assessment, not current C++ parity. Do not infer percent
complete from checkbox counts: entries differ in size and overlap. Native truth is STATUS + tests.
When implementing an item, cite its REQ-ID, source-project matrix row and issue, then add an explicit
native acceptance test/evidence. Do not silently drop an item because its source app is not mentioned
in the current chat. New DevFleet integration requirements live separately in DEVFLEET_INTERFACE_VISION.

Any AI provider can consume the JSON without external chat memory or vendor-specific tooling.
