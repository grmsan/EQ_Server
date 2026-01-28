Multiclass Scan Tool

This tool scans the `extras/THJServer` tree for multiclass-related references and
produces CSV and JSON outputs listing file, relative path, function/context,
line number, matched snippet, inferred type, and confidence.

Run:

```bash
python tools/multiclass_scan.py --root extras/THJServer --out tools/output
```

Outputs:
- `tools/output/multiclass_references.csv`
- `tools/output/multiclass_references.json`

Notes:
- File scanning includes common extensions: `.cpp, .h, .md, .sql, .lua, .py`.
- The script is heuristic-based and will catch a broad set of matches. Use the
CSV/JSON for triage and manual review.
