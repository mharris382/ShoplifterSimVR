"""
validate_gameplay_tags.py
=========================
Drop this file anywhere inside your UE project tree (e.g. alongside your
.uproject, in Tools/, Scripts/, etc.).  Run it with no arguments:

    python validate_gameplay_tags.py

It will:
  1. Walk UP from its own location to find the project root
     (the directory that contains the .uproject file).
  2. Walk DOWN from the project root to discover every folder named "Source"
     — covering the main module, all plugins, and any nested structure.
  3. Locate Config/DefaultGameplayTags.ini automatically.
  4. Scan all discovered Source trees for hard-coded GameplayTag strings.
  5. Merge those with the REQUIRED_TAGS list below.
  6. Add any missing tags to the INI — existing tags are never modified.

Optional flags:
    --dry-run     Print what would change without writing anything
    --verbose     Print every discovered tag and which file it came from
    --ini PATH    Override the INI path (for non-standard project layouts)
"""

import argparse
import re
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# ❶  MANUAL TAG LIST
#    Add any tags that are referenced indirectly (e.g. constructed at runtime,
#    referenced from Blueprint-only graphs, or just policy tags your code
#    depends on but doesn't mention literally).
#
#    Format:  "Root.Group" : ["Leaf1", "Leaf2", ...]
#    Expands to: Root.Group.Leaf1, Root.Group.Leaf2, ...
#
#    An empty leaf list is valid for a bare root tag with no children.
# ---------------------------------------------------------------------------
REQUIRED_TAGS: dict[str, list[str]] = {
    "AI.State.Awareness" : ["None", "VeryLow", "Low", "Default", "High", "VeryHigh"],
    "AI.Attributes.WorkEthic" : ["Lazy", "Strict"],
    "AI.Attributes.FitnessLevel" : ["OutOfShape", "Average", "Fit"],
    "AI.Attributes.Traits.Special" : ["Gluttonous", "BladderIssues", "PhotographicMemory", "PhoneAddict", "Paranoid"],
    "AI.Faction": ["Criminal", "Civilian", "Mall", "Shop", "Unaffiliated"],
    
    #"Mall.State.AlertLevel": ["None", "Green", "Yellow", "Red"],
    #"Shop.State.AlertLevel": ["None", "Green", "Yellow", "Red"],
    
    "Crime.Tier" : ["Annoyance", "Misdemeanor", "MiddleTier", "GrandTheft"],
    "Crime.Type" : ["Theft", "PropertyDestruction", "Harrassement", "PublicDisturbance", "Assault", "Fraud"],
    "Crime.Type.Theft" : ["Petty", "Grand"],

    "Entity.Attributes.BodyType" : ["Overweight", "Skinny", "Average"],
    "Entity.Attributes.Age" : ["Adult", "Child"],
    
    # "Ability.Jump":   [],                          # bare tag, no children
    # "Ability.Combat": ["Melee", "Ranged"],         # → Ability.Combat.Melee, Ability.Combat.Ranged
    # "State":          ["Crouching", "Sprinting"],
    # "Event.Damage":   ["Fire", "Ice", "Physical"],
}


def expand_required_tags(groups: dict[str, list[str]]) -> set[str]:
    """Expand the grouped dict into a flat set of fully-qualified tag strings."""
    tags: set[str] = set()
    for root, leaves in groups.items():
        root = root.strip()
        if not leaves:
            tags.add(root)
        else:
            for leaf in leaves:
                tags.add(f"{root}.{leaf.strip()}")
    return tags

# ---------------------------------------------------------------------------
# ❷  SOURCE SCAN CONFIGURATION
#    Regex patterns that indicate a gameplay tag string literal follows.
#    Captures the quoted content after the keyword/macro.
#
#    Patterns cover:
#      FGameplayTag::RequestGameplayTag(FName("a.b.c"))
#      UGameplayTagsManager::Get().RequestGameplayTag("a.b.c")
#      GAMEPLAY_TAG(MyTag, "a.b.c")              <- custom macros
#      FGameplayTag Tag(FName("a.b.c"))
#      Tag = FGameplayTag::RequestGameplayTag(TEXT("a.b.c"))
#      AddTag(FGameplayTag::RequestGameplayTag(FName("a.b.c")))
#      GetTag(TEXT("a.b.c"))
#      MatchesTag / HasTag / HasTagExact etc. with literal strings
# ---------------------------------------------------------------------------
# Source file extensions to scan
SOURCE_EXTENSIONS = {".cpp", ".h", ".cs", ".ini", ".uplugin", ".uproject"}

# Regex: capture bare tag strings (dot-separated identifiers inside quotes)
TAG_PATTERN = re.compile(
    r"""
    (?:
        RequestGameplayTag   |   # FGameplayTag::RequestGameplayTag(FName("…"))
        FGameplayTag\s*\(    |   # FGameplayTag Tag("…") constructor form
        GAMEPLAY_TAG\s*\(    |   # Custom GAMEPLAY_TAG macro
        AddTag\s*\(          |   # AddTag("…")
        GetTag\s*\(          |   # GetTag("…")
        HasTag\s*\(          |   # HasTag("…")
        HasTagExact\s*\(     |
        MatchesTag\s*\(      |
        MatchesTagExact\s*\( |
        GameplayTagFromString |
        TagFromString
    )
    [\s\S]{0,60}?            # allow TEXT(…) or FName(…) wrappers
    "([A-Za-z][A-Za-z0-9_.]*\.[A-Za-z0-9_.]+)"  # capture the tag string
    """,
    re.VERBOSE,
)

# Also catch bare string literals that look like tags next to common keywords
# (looser fallback – produces more noise but catches macro-wrapped usages)
LOOSE_TAG_PATTERN = re.compile(
    r'"([A-Za-z][A-Za-z0-9_]*(?:\.[A-Za-z0-9_]+){1,})"'
)

# Minimum dot count for a string to qualify as a tag (reduces false positives)
MIN_DOT_COUNT = 1

# ---------------------------------------------------------------------------
# ❸  EXCLUDED TAG ROOTS
#    If a tag's first segment matches any entry here it is silently ignored.
#    Useful for log categories, module names, test namespaces, or any other
#    dot-separated strings your codebase uses that aren't gameplay tags.
# ---------------------------------------------------------------------------
EXCLUDED_ROOTS: set[str] = {
    "Icons", "Intensity", 
    "ContentBrowser", 
    "ClassIcon",
      "ISMBatchTest",
        "ISMRuntimePCGInterop",
          "ISMRuntimeAnimation", "Tool", "User", "TimeOfDay", "Skill"
    # "ISMRuntime",   # suppress all ISMRuntime.* strings (e.g. test fixture names)
    # "LogMyPlugin",  # log category false-positives
}

# File-like suffixes — strings ending with these are paths/includes, not tags
FILE_SUFFIXES = (
    ".generated.h",   # must come before ".h" so the longer match wins
    ".h", ".cpp", ".cs", ".inl", ".ini",
    ".uplugin", ".uproject", ".uasset", ".umap",
    ".json", ".xml", ".txt", ".png", ".jpg", ".cfg",
)

# Macros whose string argument is NOT a gameplay tag (test names, log categories, etc.)
# The string immediately following these tokens will be ignored.
NON_TAG_MACROS = re.compile(
    r"""
    (?:
        IMPLEMENT_SIMPLE_AUTOMATION_TEST   |
        IMPLEMENT_COMPLEX_AUTOMATION_TEST  |
        IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST |
        DEFINE_LOG_CATEGORY_STATIC         |
        DEFINE_LOG_CATEGORY                |
        UE_LOG\s*\(                        |
        TEXT\s*\(\s*"[^"]*"\s*\)           # TEXT("…") used as a display string
    )
    [\s\S]{0,120}?
    "([^"]+)"
    """,
    re.VERBOSE,
)

# Filename stems that indicate a test file — tags found here get flagged
TEST_FILE_PATTERNS = re.compile(r"[Tt]est|[Ss]pec|[Ff]ixture", re.IGNORECASE)


def looks_like_file(tag: str) -> bool:
    """Return True if the candidate string looks like a filename, not a tag."""
    lower = tag.lower()
    return any(lower.endswith(s) for s in FILE_SUFFIXES)


def collect_non_tag_strings(text: str) -> set[str]:
    """Return the set of strings that appear as arguments to non-tag macros."""
    return {m.group(1) for m in NON_TAG_MACROS.finditer(text)}


def is_test_file(filepath: Path) -> bool:
    """Return True if the file is likely a test/spec file."""
    return bool(TEST_FILE_PATTERNS.search(filepath.stem))


# ---------------------------------------------------------------------------
# Project root discovery
# ---------------------------------------------------------------------------

def find_project_root(start: Path) -> Path:
    """
    Walk upward from `start` until we find a directory containing a .uproject
    file.  That directory is the project root.

    Raises RuntimeError if we reach the filesystem root without finding one.
    """
    current = start.resolve()
    while True:
        uprojects = list(current.glob("*.uproject"))
        if uprojects:
            return current
        parent = current.parent
        if parent == current:
            raise RuntimeError(
                f"Could not find a .uproject file searching upward from:\n  {start}\n"
                "Make sure this script lives somewhere inside your UE project tree."
            )
        current = parent


def find_source_dirs(project_root: Path) -> list[Path]:
    """
    Recursively find every directory named exactly 'Source' under the project
    root, skipping Binaries, Intermediate, Saved, and .git trees.

    This covers:
      <Project>/Source/
      <Project>/Plugins/<PluginA>/Source/
      <Project>/Plugins/<PluginB>/Source/
      … and any deeper nesting
    """
    SKIP_DIRS = {"Binaries", "Intermediate", "Saved", ".git", "DerivedDataCache", "Build"}
    found: list[Path] = []

    def _walk(directory: Path) -> None:
        try:
            entries = list(directory.iterdir())
        except PermissionError:
            return
        for entry in entries:
            if not entry.is_dir():
                continue
            if entry.name in SKIP_DIRS:
                continue
            if entry.name == "Source":
                found.append(entry)
                # Don't recurse further into a Source dir — rglob handles its contents
            else:
                _walk(entry)

    _walk(project_root)
    return found


# ---------------------------------------------------------------------------
# INI helpers
# ---------------------------------------------------------------------------

INI_ENTRY_RE = re.compile(
    r'^\+GameplayTagList=\(Tag="([^"]+)"',
    re.IGNORECASE,
)


def find_default_ini(project_root: Path) -> Path:
    """
    Locate DefaultGameplayTags.ini under Config/.
    Returns the path whether it exists yet or not (callers handle creation).
    """
    candidates = [
        project_root / "Config" / "DefaultGameplayTags.ini",
        project_root / "Config" / "Tags" / "DefaultGameplayTags.ini",
    ]
    for c in candidates:
        if c.exists():
            return c
    # Fallback: recursive search for any *GameplayTags*.ini in Config/
    config_dir = project_root / "Config"
    if config_dir.exists():
        for p in config_dir.rglob("*GameplayTags*.ini"):
            return p
    # Default target path even if it doesn't exist yet
    return project_root / "Config" / "DefaultGameplayTags.ini"


def parse_ini_tags(ini_path: Path) -> set[str]:
    """Return all tags already present in the INI file."""
    tags: set[str] = set()
    if not ini_path.exists():
        return tags
    for line in ini_path.read_text(encoding="utf-8-sig").splitlines():
        m = INI_ENTRY_RE.match(line.strip())
        if m:
            tags.add(m.group(1).strip())
    return tags


def format_ini_entry(tag: str) -> str:
    return f'+GameplayTagList=(Tag="{tag}",DevComment="")'


def write_missing_tags(ini_path: Path, missing: set[str]) -> None:
    """Append missing tags to the INI file, creating it if needed."""
    if ini_path.exists():
        original = ini_path.read_text(encoding="utf-8-sig")
    else:
        ini_path.parent.mkdir(parents=True, exist_ok=True)
        original = "[/Script/GameplayTags.GameplayTagsSettings]\n"

    lines = original.rstrip("\n").split("\n")

    section_header = "[/Script/GameplayTags.GameplayTagsSettings]"
    section_idx = next(
        (i for i, l in enumerate(lines) if l.strip() == section_header), None
    )

    new_entries = sorted(format_ini_entry(t) for t in missing)

    if section_idx is not None:
        insert_at = section_idx + 1
        for entry in new_entries:
            lines.insert(insert_at, entry)
            insert_at += 1
    else:
        lines.append("")
        lines.append(section_header)
        lines.extend(new_entries)

    ini_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


# ---------------------------------------------------------------------------
# Source scanning
# ---------------------------------------------------------------------------

def scan_source_files(roots: list[Path], verbose: bool = False) -> dict[str, list[str]]:
    """
    Walk source roots and return {tag: [file_path, …]} for every tag found.
    Uses the strict pattern first, then the loose fallback on the same text.
    """
    found: dict[str, list[str]] = {}
    # Tags found exclusively in test files: {tag: set(filepaths)}
    test_file_tags: dict[str, set[str]] = {}

    def record(tag: str, path: str, from_test_file: bool = False) -> None:
        # Basic sanity checks
        if tag.count(".") < MIN_DOT_COUNT:
            return
        if ".." in tag or tag.startswith(".") or tag.endswith("."):
            return
        # Reject anything that looks like a #include path or filename
        if looks_like_file(tag):
            return
        # Reject tags whose root segment is on the exclusion list
        root_segment = tag.split(".")[0]
        if root_segment in EXCLUDED_ROOTS:
            return
        found.setdefault(tag, []).append(path)
        if from_test_file:
            test_file_tags.setdefault(tag, set()).add(path)

    for root in roots:
        if not root.exists():
            continue
        for filepath in root.rglob("*"):
            if filepath.suffix.lower() not in SOURCE_EXTENSIONS:
                continue
            if not filepath.is_file():
                continue
            try:
                file_text = filepath.read_text(encoding="utf-8-sig", errors="replace")
            except OSError:
                continue

            rel = str(filepath)
            from_test = is_test_file(filepath)

            # Build the set of strings used in non-tag macros (test names, log cats…)
            non_tag = collect_non_tag_strings(file_text)

            # Strict pattern
            for m in TAG_PATTERN.finditer(file_text):
                candidate = m.group(1)
                if candidate not in non_tag:
                    record(candidate, rel, from_test)

            # Loose fallback
            for m in LOOSE_TAG_PATTERN.finditer(file_text):
                candidate = m.group(1)
                if candidate.count(".") >= MIN_DOT_COUNT and candidate not in non_tag:
                    record(candidate, rel, from_test)

    # Warn about tags that were only ever seen inside test files
    test_only = {
        tag for tag in test_file_tags
        if set(found[tag]) == test_file_tags[tag]  # every occurrence was in a test file
    }
    if test_only:
        print(
            f"\n⚠️  {len(test_only)} tag(s) found only in test files — "
            "consider nesting them under a 'Test.*' root tag:"
        )
        for tag in sorted(test_only):
            paths = sorted(test_file_tags[tag])
            short = [Path(p).name for p in paths]
            print(f"     {tag}  (in: {', '.join(short)})")

    if verbose:
        for tag, paths in sorted(found.items()):
            unique = sorted(set(paths))
            for p in unique:
                print(f"  [scan] {tag}  ←  {p}")

    return found


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Validate and sync GameplayTags into the project INI.\n"
                    "Run with no arguments from anywhere inside your UE project."
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Report what would change without writing anything.",
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Print every discovered tag and which file it came from.",
    )
    parser.add_argument(
        "--ini",
        default=None,
        help="Override the path to the GameplayTags INI (for non-standard layouts).",
    )
    args = parser.parse_args()

    # --- Infer project root from this script's location ---
    script_path = Path(__file__).resolve()
    try:
        project_root = find_project_root(script_path.parent)
    except RuntimeError as e:
        print(f"ERROR: {e}", file=sys.stderr)
        return 1

    print(f"[info] Project root : {project_root}")

    # --- Locate INI ---
    if args.ini:
        ini_path = Path(args.ini).resolve()
        print(f"[info] INI (override): {ini_path}")
    else:
        ini_path = find_default_ini(project_root)
        status = "(exists)" if ini_path.exists() else "(will be created)"
        print(f"[info] INI            : {ini_path}  {status}")

    # --- Discover all Source directories ---
    source_dirs = find_source_dirs(project_root)
    if not source_dirs:
        print("WARNING: No 'Source' directories found under the project root.", file=sys.stderr)
    else:
        print(f"[info] Source dirs found ({len(source_dirs)}):")
        for sd in source_dirs:
            rel = sd.relative_to(project_root)
            print(f"         {rel}")

    # --- Scan source ---
    print("\n[step 1/3] Scanning source files for tag references …")
    scanned = scan_source_files(source_dirs, verbose=args.verbose)
    print(f"           Found {len(scanned)} unique tag candidates in source.")

    # --- Collect all required tags ---
    all_required: set[str] = expand_required_tags(REQUIRED_TAGS) | set(scanned.keys())
    print(f"           Manual list : {len(expand_required_tags(REQUIRED_TAGS))} tag(s) from {len(REQUIRED_TAGS)} group(s).")
    print(f"           Total (union): {len(all_required)} tag(s).")

    # --- Parse existing INI ---
    print("\n[step 2/3] Parsing existing INI …")
    existing_tags = parse_ini_tags(ini_path)
    print(f"           {len(existing_tags)} tag(s) already present.")

    # --- Diff ---
    missing = all_required - existing_tags
    if not missing:
        print("\n✅  All required tags are already present in the INI. Nothing to do.")
        return 0

    print(f"\n[step 3/3] {len(missing)} tag(s) missing from INI:")
    for tag in sorted(missing):
        sources = scanned.get(tag, ["<manual list>"])
        unique_sources = sorted(set(sources))
        short_sources = [
            str(Path(s).relative_to(project_root)) if Path(s).is_relative_to(project_root) else s
            for s in unique_sources
        ]
        print(f"  + {tag}")
        if args.verbose:
            for s in short_sources[:3]:
                print(f"      from: {s}")

    if args.dry_run:
        print("\n⚠️  DRY RUN – no changes written.")
        return 0

    write_missing_tags(ini_path, missing)
    print(f"\n✅  Wrote {len(missing)} missing tag(s) to: {ini_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())