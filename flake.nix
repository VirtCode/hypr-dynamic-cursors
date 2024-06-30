{
  description = "a plugin to make your hyprland cursor more realistic, also adds shake to find";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
    hyprland.url = "git+https://github.com/hyprwm/Hyprland?submodules=1";
  };

  outputs = {
    self,
    nixpkgs,
    ...
  } @ inputs: let
    forAllSystems = function:
      nixpkgs.lib.genAttrs [
        "x86_64-linux"
      ] (system: function nixpkgs.legacyPackages.${system});
  in {
    packages = forAllSystems (pkgs: {
      default = self.packages.${pkgs.system}.hypr-dynamic-cursors;
      hypr-dynamic-cursors = pkgs.callPackage ./nix/package.nix {inherit inputs pkgs;};
    });
  };
}
