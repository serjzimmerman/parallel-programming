{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    lint-nix.url = "github:xc-jp/lint.nix";
  };

  outputs = {
    self,
    nixpkgs,
    flake-utils,
    ...
  } @ inputs: let
    systems = ["x86_64-linux" "aarch64-linux"];
  in
    flake-utils.lib.eachSystem systems (system: let
      pkgs = import nixpkgs {inherit system;};

      clang-tools = pkgs.clang-tools_17;
      lints = (import ./nix/lints.nix {inherit inputs;}) pkgs ./.;
      nativeBuildInputs = with pkgs; [
        clang-tools
        act
        just
        mpi
        automake
        autoconf
        addlicense
        gdb
        gnuplot
        bc
      ];

      shell =
        (pkgs.mkShell.override {
          stdenv = pkgs.gcc49Stdenv;
        }) {
          inherit nativeBuildInputs;
        };
    in {
      devShells = {
        default = shell;
        devGcc = shell;
      };

      legacyPackages =
        lints
        // {};

      formatter = pkgs.alejandra;
    });
}
