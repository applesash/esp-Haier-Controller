#!/usr/bin/env bash
# Stage, commit, and push the current branch.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

if [[ -z "${GIT_SSH_COMMAND:-}" && -f "$HOME/.ssh/haier_controller_ed25519" ]]; then
  export GIT_SSH_COMMAND="ssh -i $HOME/.ssh/haier_controller_ed25519 -o IdentitiesOnly=yes -o ConnectTimeout=15 -p 443 -o HostName=ssh.github.com"
fi

MESSAGE="${1:-Save changes}"
BRANCH="$(git branch --show-current)"

if [[ -z "$BRANCH" ]]; then
  echo "Repository is not on a branch." >&2
  exit 1
fi

git add -A
if git diff --cached --quiet; then
  echo "Nothing to commit."
else
  git commit -m "$MESSAGE"
fi
git push origin "$BRANCH"
