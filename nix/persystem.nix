{ pkgs, ... }:
let
  llvmStdenv = pkgs.llvmPackages_18.libcxxStdenv;
  boost = (
    pkgs.boost185.override {
      useMpi = true;
      stdenv = llvmStdenv;
    }
  );
  nativeBuildInputs =
    {
      with_debug ? false,
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
      bear
      boost
      range-v3
      cmake
      (fmt.override { stdenv = llvmStdenv; })
      valgrind
    ]);
  shell =
    {
      with_debug ? false,
    }:
    (pkgs.mkShell.override { stdenv = llvmStdenv; }) {
      nativeBuildInputs = nativeBuildInputs { inherit with_debug; };
      BOOST_LIBDIR = "${boost}/lib";
    };
in
{
  devShells = {
    default = shell { with_debug = false; };
    debug = shell { with_debug = true; };
  };
}
