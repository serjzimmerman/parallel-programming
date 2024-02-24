{inputs, ...}: let
  inherit (inputs) lint-nix;
in
  pkgs: src:
    lint-nix.lib.lint-nix (let
      clang-tools = pkgs.clang-tools_17;

      cpp-extensions = [
        ".c"
        ".cpp"
        ".h"
        ".hpp"
        ".cc"
        ".cl"
      ];
    in {
      inherit pkgs src;

      linters = {
        typos = {
          ext = [".h" ".c" ".md" ".sh"];
          cmd = "${pkgs.typos}/bin/typos $filename";
        };

        ruff = {
          ext = ".py";
          cmd = "${pkgs.ruff}/bin/ruff $filename";
        };
      };

      formatters = {
        yamlfmt = {
          ext = [".yaml" ".yml" ".clang-format" ".clang-tidy"];
          cmd = "${pkgs.yamlfmt}/bin/yamlfmt $filename";
        };

        alejandra = {
          ext = ".nix";
          cmd = "${pkgs.alejandra}/bin/alejandra --quiet";
          stdin = true;
        };

        clang-format = {
          ext = cpp-extensions;
          cmd = "${clang-tools}/bin/clang-format";
          stdin = true;
        };

        black = {
          ext = ".py";
          cmd = "${pkgs.black}/bin/black $filename";
        };
      };
    })
