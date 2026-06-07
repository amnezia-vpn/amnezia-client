#!/usr/bin/env python3
"""Plan and record upstream release-freeze automation state."""

from __future__ import annotations

import argparse
import json
import re
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


RELEASE_TAG_RE = re.compile(r"^\d+(?:\.\d+){3}$")
DEFAULT_STATE = {
    "schema": 1,
    "upstreamRepo": "amnezia-vpn/amnezia-client",
    "targetBranch": "feat/server-managed-split-tunnel",
    "baselineTag": "",
    "frozen": False,
    "frozenTag": "",
    "lastSyncedUpstreamDev": "",
}


def version_key(tag: str) -> tuple[int, ...]:
    if not RELEASE_TAG_RE.match(tag):
        raise ValueError(f"not an x.y.z.w release tag: {tag}")
    return tuple(int(part) for part in tag.split("."))


def is_newer(candidate: str, baseline: str) -> bool:
    if not candidate or not baseline or candidate == baseline:
        return False
    return version_key(candidate) > version_key(baseline)


def require_release_tag(tag: str, label: str) -> str:
    if not RELEASE_TAG_RE.match(tag):
        raise SystemExit(f"{label} must be an x.y.z.w release tag, got {tag!r}")
    return tag


def read_state(path: Path) -> dict[str, Any]:
    if not path.exists():
        return dict(DEFAULT_STATE)
    with path.open("r", encoding="utf-8") as stream:
        state = json.load(stream)
    if state.get("schema") != 1:
        raise SystemExit(f"Unsupported release freeze state schema in {path}: {state.get('schema')!r}")
    merged = dict(DEFAULT_STATE)
    merged.update(state)
    return merged


def write_state(path: Path, state: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="\n") as stream:
        json.dump(state, stream, indent=2, sort_keys=True)
        stream.write("\n")


def plan(args: argparse.Namespace) -> int:
    state = read_state(args.state_file)
    baseline_tag = args.baseline_tag or state.get("baselineTag") or args.latest_tag
    latest_tag = require_release_tag(args.latest_tag, "--latest-tag")
    if baseline_tag:
        require_release_tag(baseline_tag, "--baseline-tag")
    force_freeze_tag = require_release_tag(args.force_freeze_tag, "--force-freeze-tag") if args.force_freeze_tag else ""
    if baseline_tag and not force_freeze_tag and version_key(baseline_tag) > version_key(latest_tag):
        raise SystemExit(
            f"--baseline-tag ({baseline_tag}) is newer than --latest-tag ({latest_tag}); "
            "reset the baseline to the current latest release before waiting for the next release"
        )
    candidate_tag = force_freeze_tag or latest_tag

    frozen_tag = state.get("frozenTag", "") if state.get("frozen") else ""
    comparison_baseline = frozen_tag or baseline_tag

    if force_freeze_tag:
        action = "freeze"
        release_tag = force_freeze_tag
    elif state.get("frozen") and not is_newer(candidate_tag, comparison_baseline):
        action = "already-frozen"
        release_tag = frozen_tag
    elif is_newer(candidate_tag, comparison_baseline):
        action = "freeze"
        release_tag = candidate_tag
    else:
        action = "wait"
        release_tag = ""

    result = {
        "action": action,
        "baselineTag": baseline_tag,
        "latestTag": args.latest_tag,
        "releaseTag": release_tag,
        "frozen": bool(state.get("frozen")),
        "targetBranch": args.target_branch,
        "upstreamRepo": args.upstream_repo,
    }
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0


def record(args: argparse.Namespace) -> int:
    state = read_state(args.state_file)
    now = datetime.now(timezone.utc).isoformat(timespec="seconds")

    state["schema"] = 1
    state["upstreamRepo"] = args.upstream_repo
    state["targetBranch"] = args.target_branch
    state["baselineTag"] = args.baseline_tag or state.get("baselineTag") or args.latest_tag
    state["latestObservedTag"] = args.latest_tag
    state["updatedAt"] = now

    if args.action == "freeze":
        if not args.release_tag:
            raise SystemExit("--release-tag is required with --action freeze")
        state["frozen"] = True
        state["frozenTag"] = args.release_tag
        state["baselineTag"] = args.release_tag
        state["frozenAt"] = now
        state["frozenReleaseSha"] = args.release_sha
        state["lastSyncedUpstreamDev"] = args.upstream_dev_sha
    elif args.action == "wait":
        state["frozen"] = False
        state["frozenTag"] = ""
    else:
        raise SystemExit(f"Unsupported action: {args.action}")

    write_state(args.state_file, state)
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest="command", required=True)

    common = argparse.ArgumentParser(add_help=False)
    common.add_argument("--state-file", type=Path, default=Path(".github/upstream-release-freeze.json"))
    common.add_argument("--upstream-repo", default="amnezia-vpn/amnezia-client")
    common.add_argument("--target-branch", default="feat/server-managed-split-tunnel")
    common.add_argument("--latest-tag", required=True)
    common.add_argument("--baseline-tag", default="")

    plan_parser = subparsers.add_parser("plan", parents=[common])
    plan_parser.add_argument("--force-freeze-tag", default="")
    plan_parser.set_defaults(func=plan)

    record_parser = subparsers.add_parser("record", parents=[common])
    record_parser.add_argument("--action", choices=["wait", "freeze"], required=True)
    record_parser.add_argument("--release-tag", default="")
    record_parser.add_argument("--release-sha", default="")
    record_parser.add_argument("--upstream-dev-sha", default="")
    record_parser.set_defaults(func=record)

    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
