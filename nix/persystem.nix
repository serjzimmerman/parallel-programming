{ pkgs, ... }:
let
  clang-tools = pkgs.clang-tools_17;
  nativeBuildInputs = with pkgs; [
    clang-tools
    act
    just
    (mpi.overrideAttrs (
      _final: prev: { configureFlags = prev.configureFlags ++ [ "--enable-debug" ]; }
    ))
    automake
    autoconf
    addlicense
    gdb
    gnuplot
    bc
  ];
  shell = (pkgs.mkShell.override { stdenv = pkgs.gcc49Stdenv; }) { inherit nativeBuildInputs; };
in
{
  devShells.default = shell;
}
