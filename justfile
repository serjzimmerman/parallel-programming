alias f := format
alias l := check
alias lint := check
alias r := run-workflows

run-workflows:
    @act -P ubuntu-22.04=ghcr.io/catthehacker/ubuntu:runner-22.04

check:
    @nix flake check

check-format:
    @nix flake check

format:
    @nix fmt

addlicense:
    @find . -type f \( -name "*.c" -o -name "*.h" -o \) -exec addlicense -f LICENSE -l mit {} \;

autogen:
    @cd {{ invocation_directory() }} && autoreconf --install
    @cd {{ invocation_directory() }} && automake --add-missing --copy >/dev/null 2>&1
