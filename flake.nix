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
    eachSystem = nixpkgs.lib.genAttrs systems;
    pkgsFor = nixpkgs.legacyPackages;
  in {
    packages = eachSystem (system: {
      default = self.packages.${system}.hypr-dynamic-cursors;
      hypr-dynamic-cursors = let
        inherit (inputs.hyprland.packages.${system}) hyprland;
        inherit (pkgsFor.${system}) stdenvNoCC gcc13;

        name = "hypr-dynamic-cursors";
      in
        stdenvNoCC.mkDerivation {
          inherit name;
          src = ./.;

          inherit (hyprland) buildInputs;
          nativeBuildInputs = hyprland.nativeBuildInputs ++ [hyprland gcc13];

          dontUseCmakeConfigure = true;
          dontUseMesonConfigure = true;
          dontUseNinjaBuild = true;
          dontUseNinjaInstall = true;

          installPhase = ''
            runHook preInstall

            mkdir -p "$out/lib"
            cp -r out/* "$out/lib/lib${name}.so"

            runHook postInstall
          '';
        };
    });
  };
}
