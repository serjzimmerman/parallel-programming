{ pkgs, ... }:
let
  llvmStdenv = pkgs.llvmPackages_18.libcxxStdenv;
  mkBoost =
    stdenv:
    (pkgs.boost185.override {
      useMpi = true;
      inherit stdenv;
    });
  nativeBuildInputs =
    {
      with_debug ? false,
      stdenv ? pkgs.stdenv,
    }:
    (with pkgs; [
      clang-tools_18
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
      lldb_18
      gnuplot
      bc
      (mkBoost stdenv)
      range-v3
      cmake
      (fmt.override { inherit stdenv; })
      valgrind
      llvmPackages_18.openmp
    ]);
  mkShell =
    {
      with_debug ? false,
      stdenv ? pkgs.stdenv,
    }:
    (pkgs.mkShell.override { inherit stdenv; }) {
      nativeBuildInputs = nativeBuildInputs { inherit with_debug stdenv; };
      BOOST_LIBDIR = "${mkBoost stdenv}/lib";
    };
in
{
  devShells = {
    default = mkShell {
      with_debug = false;
      stdenv = llvmStdenv;
    };
    debug = mkShell {
      with_debug = true;
      stdenv = llvmStdenv;
    };
    gccDefault = mkShell { };
    clangLibStdcxx = mkShell { stdenv = pkgs.llvmPackages_18.stdenv; };
  };
}
