alias f := format
alias l := check
alias lint := check
alias r := run-workflows

run-workflows:
  @act -P ubuntu-22.04=ghcr.io/catthehacker/ubuntu:runner-22.04

check:
  @nix build ".#all-checks" -L
  @nix flake check

check-format:
  @nix build ".#all-formats" -L

format:
  @nix build ".#format-all"
  @result/bin/format-all

addlicense:
  @find . -type f \( -name "*.c" -o -name "*.h" \) -exec addlicense -f LICENSE -l mit {} \;

autogen:
  @cd {{invocation_directory()}} && autoreconf --install
  @cd {{invocation_directory()}} && automake --add-missing --copy >/dev/null 2>&1
