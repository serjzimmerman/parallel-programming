{ pkgs, ... }:
let
  clang-tools = pkgs.clang-tools_17;
  nativeBuildInputs =
    {
      with_debug ? false,
    }:
    (with pkgs; [
      clang-tools
      act
      just
      (mpi.overrideAttrs (
        _final: prev: {
          configureFlags = prev.configureFlags ++ (lib.optionals with_debug [ "--enable-debug" ]);
        }
      ))
      automake
      autoconf
      addlicense
      gdb
      gnuplot
      bc
      (boost.override { useMpi = true; })
      bear
      range-v3
    ]);
  shell =
    {
      with_debug ? false,
    }:
    (pkgs.mkShell.override { stdenv = pkgs.llvmPackages_17.stdenv; }) {
      nativeBuildInputs = nativeBuildInputs { inherit with_debug; };
      BOOST_LIBDIR = "${pkgs.boost}/lib";
    };
in
{
  devShells = {
    default = shell { with_debug = false; };
    debug = shell { with_debug = true; };
  };
}
