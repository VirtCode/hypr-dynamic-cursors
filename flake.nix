{
  description = "A plugin to make your hyprland cursor more realistic, also adds shake to find";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
    hyprland.url = "github:hyprwm/Hyprland";
  };

  outputs = {
    self,
    nixpkgs,
    ...
  } @ inputs: let
    systems = ["x86_64-linux"];
    forAllSystems = nixpkgs.lib.genAttrs systems;
    packagesForEach = nixpkgs.legacyPackages;
  in {
    packages = forAllSystems (system: rec {
      default = hypr-dynamic-cursors;
      hypr-dynamic-cursors = packagesForEach.${system}.callPackage ./nix/package.nix {
        inherit inputs;
        pkgs = packagesForEach.${system};
      };
    });
  };
}
