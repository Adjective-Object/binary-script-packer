let
    pkgs = import <nixpkgs> {};
    dependencies = [
        
    ];

in 
with pkgs; {
    devEnv = stdenv.mkDerivation {
        name = "binscripter-dev";
        buildInputs = [ stdenv gnumake gcc ] ++ dependencies;
    };
}

